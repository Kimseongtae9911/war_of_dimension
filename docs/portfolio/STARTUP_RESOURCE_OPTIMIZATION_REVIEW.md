# 장면별 자원·음원과 공통 렌더링 최적화 검토

2026-10-05, NPC 최적화 이후 코드와 에셋을 검토했다. **이 문서는 적용 전 검토와 당시 비용을 보존한다.** 이후 UI BC7 사전 로딩을 [구현·실측](UI_TEXTURE_COMPRESSION.md)했다. 음원/skybox/upload 조기 회수 등 다른 후보는 미적용이며 shadow map과 위치 FP32는 유지한다. 최신 [실측](NPC_MEMORY_OPTIMIZATION.md)의 기동·Title·공통 자원 632.22MiB는 `scene_setup_ready`까지의 누적 Private Bytes다. Title의 최종 잔존량이나 추가 절감량을 의미하지 않는다. PCM·DDS payload·논리 render target 용량을 이 수치에 더하거나 빼서 새 전체 메모리를 예측하지 않는다.

## Title 종료와 공통 자원의 구분

타이틀 배경·로그인/회원가입 버튼·Title player/camera는 Title을 떠나면 필요하지 않다. 현재 `ChangeSceneReleaseObject`는 GPU 완료를 기다리고 player/scene을 반환하며, `CTextureShader::DestroyInstance`는 UI 객체를 해제한다. Title UI 3개는 객체→재질→텍스처 마지막 참조 해제로 연결된다. `LoadTexture`는 참조 0으로 만들고 `SetTexture`가 소유 참조를 추가하므로 이 3개 텍스처에 숨은 영구 소유 참조는 발견하지 않았다. `CTexture` 소멸자는 직접 로딩 DEFAULT와 남은 upload도 반환한다. 코드 경로 확인이며 실제 반복 전환의 live resource 0을 검사한 결과는 아니다.

| Title 전용 DDS | texel payload |
|---|---:|
| 1024×1024 RGBA32 배경 | 4MiB |
| 328×83 RGBA32 SignIn 버튼 | 0.103851MiB |
| 328×83 RGBA32 SignUp 버튼 | 0.103851MiB |
| **합계, 객체·CB·heap 정렬·upload 제외** | **4.207703MiB** |

swapchain·depth·Deferred render target·Direct2D/DirectWrite·blur·shadow map은 Framework 공통 자원으로 인게임에서도 사용한다. `CTextureShader`는 `DestroyInstance`라는 이름과 달리 객체 컨테이너만 해제하고 인스턴스/셰이더를 다음 장면에서 재사용한다. Title 자원 해제로 632MiB 전체를 줄일 수 있다는 해석은 맞지 않는다. [장면 전환](../../Client/WarOfDimension/GameFramework.cpp), [UI 생성/반환](../../Client/WarOfDimension/Shader.cpp), [참조 소유권](../../Client/WarOfDimension/Object.cpp)이 근거다.

## 음원 직접 조사와 장면별 로딩 가능성

기동 시 12개 지정 디렉터리의 음원을 `createSound(..., FMOD_DEFAULT, ...)`로 생성한다. 장면 전환의 `Stop_All`은 재생을 중단하지만 `m_mapSound`를 유지한다. sound 반환은 현재 전체 `SoundManager::Release`에서 한다. 기본 sample 로딩은 압축 파일도 PCM으로 펼치며, BGM은 stream으로 작은 읽기/디코드 버퍼를 유지할 수 있다. [FMOD 공식 설명](https://www.fmod.com/docs/2.03/api/loading-and-playing-sounds-in-the-core-api.html)을 참고하고, 프로젝트 **2.00.01 DLL/헤더**에서 실제 생성 동작을 확인했다. 참고 문서 버전과 실행 버전은 구분한다.

[근거 JSON](evidence/startup-resource-review-20261005.json)은 파일 142개를 독립적으로 생성/반환한 PCM bytes, BGM sample/stream의 FMOD allocator 순증가, 파일/DLL/소스 SHA-256 및 DDS 헤더/payload를 담는다. `NOSOUND`, 재생 없음, 32채널의 별도 조사 프로세스다. 생성 실패 0개, 시스템 종료 후 FMOD allocator 잔여 0byte였다. PCM 합계는 파일별 데이터 합이며 실제 클라이언트 map 보유량·Private Bytes와 구분한다.

| 음원 | PCM payload | stream 생성 시 FMOD 증가, 재생 전 |
|---|---:|---:|
| Title BGM | 36.91MiB | 274.62KiB |
| Lobby BGM | 10.85MiB | 102.53KiB |
| Ready BGM | 27.69MiB | 259.74KiB |
| Ingame BGM | 11.03MiB | 126.18KiB |
| **BGM 4곡** | **86.48MiB** | 네 곡 동시 상주 불필요 |
| 나머지 138개 음원 | 36.39MiB | 이 조사에서는 sample 생성 |
| **142개 파일** | **122.86MiB** | 기동 음원 구간 Private 증가 125.85MiB와 다른 지표 |

인게임에서 Title/Lobby/Ready BGM을 반환하면 **75.44MiB의 PCM 데이터**를 제외할 수 있다. 효과음을 전부 유지해도 인게임 BGM 포함 파일별 PCM 합계는 47.42MiB다. BGM을 stream으로 바꾸면 전체 86.48MiB PCM 상주를 피할 수 있다. 이 효과는 서로 중첩되어 합산하지 않는다. 실제 재생 중 메모리·I/O·loop 이음새·전환 지연은 미검증이다.

장면별 로딩은 가능하다. 공통 버튼/기본 이벤트는 작은 공통 집합으로 유지하고 장면 BGM을 교체한다. 인게임 로딩 화면에서는 **네 참가자의 확정 직업·선택 스킬 합집합**, 기본 공격·이동·피격·사망, NPC/boss/tower 및 후속 효과를 담은 manifest를 준비한다. 내 스킬만 고르면 다른 참가자의 소리가 빠진다. 초기에는 효과음 36.39MiB 전체를 유지하며 BGM부터 변경하고, 선택 스킬별 효과음은 직접 문자열 호출과 애니메이션 이벤트까지 대조한 뒤 적용하는 순서가 적합하다.

FMOD System은 공통으로 유지한다. 이전 채널 stop→이전 scene sound release→새 scene 사전 로드/준비 완료→BGM 재생 순서를 명확히 하고, 로딩 스레드를 추가하면 map의 재생/반환과 접근을 직렬화한다. 긴 BGM부터 streaming을 적용하고 짧은 다중 효과음은 같은 sample을 여러 channel에서 재사용한다. 실패·로비 복귀·반복 전환·장시간 재생 검증이 필요하다.

`Sound/Effect/Jump.wav`와 `Sound/Jump.wav`는 basename이 같다. 현재 `m_mapSound.emplace` 결과를 확인하지 않아 두 번째 sound의 소유 포인터를 잃는 경로가 있다. 경로별 키/명시적 alias와 중복 등록 실패 시 반환이 필요하며 기존 호출이 기대하는 파일도 확인한다. [현재 로더/재생/종료](../../Client/WarOfDimension/SoundManager.cpp)가 근거다.

## 나머지 후보와 우선순위

| 순서 | 후보 | 검토 당시 비용·가능한 개선 | 확인해야 할 조건 |
|---|---|---|---|
| 1 | UI·dissolve 임시 upload 회수 | copy 완료 후 Framework 순회에 UI shader/dissolve가 없다. UI `ReleaseUploadBuffers`도 부모의 빈 구현으로 끝난다 | UI 전체 컨테이너의 material/mesh 방문. DEFAULT·상수·가변 UI mesh 유지. 잔존 actual allocation 계측 필요 |
| 2 | 장면별 BGM·streaming | BGM PCM 86.48MiB 중 비인게임 75.44MiB | stop/release 순서, 루프·복귀·로딩 중 끊김·실패 검증 |
| 3 | skybox BC7 | 2048²×6면 RGBA32 단일 mip의 `Space.dds`, **96→24MiB**, payload 72MiB 절감 후보 | 같은 해상도의 손실 압축. 여섯 면 경계·밴딩·색 공간 검증. skybox upload는 이미 회수함 |
| 4 | UI 압축·지연 로딩 | Victory/Defeat 각 **25.43MiB**, Speed **41.66MiB**, Blood **8.06MiB**. 승패 화면 둘 다 생성 후 숨김 | BC7/크기 검토, 상점/결과의 필요한 시점 로딩. 텍스트/alpha·첫 표시 지연 검증. 7680px Speed는 atlas 확인 없이 줄이지 않음 |
| 5 | shadow map 유지 확정 | 사용자 지시로 **8192²·32bit, 256MiB 유지** | 그림자 품질 우선. 해상도 축소는 적용 대상에서 제외 |
| 6 | 위치 G-buffer 유지 확정 | 사용자 지시로 **R32G32B32A32_FLOAT, 31.64MiB 유지** | 위치 정밀도·조명·그림자 품질 우선. FP16 형식 변경은 적용 대상에서 제외 |
| 7 | Title의 3D pass·생성 지연 | 공통 G-buffer 6장 texel 합계 **71.19MiB**, shadow map을 UI 화면에도 생성/사용 | Title 기동 peak·시간 개선. Lobby/Ready는 3D 캐릭터를 사용하며 인게임에는 필요 |
| 8 | blur·디버그 지연 생성 | 1920×1080 RGBA8 blur **7.91MiB**, debug PSO 4종 기동 생성 | `isBlurRender=true`가 기본이며 blur를 실제 사용. 지연 생성만으로 인게임 절감을 주장하지 않음 |

UI upload 회수와 texture 압축은 같은 복사본 비용에 영향을 주어 독립 절감량처럼 더하지 않는다. 기존 공유 DDS upload 0byte는 **공유 모델 DDS 범위**이며 직접 로딩 UI/dissolve까지 포함한 전체 UPLOAD 0byte가 아니다. 위 큰 UI 네 장의 payload 합계는 약 100.57MiB이지만 upload actual allocation은 row pitch/mip/heap 정렬을 계측해야 한다. Private Bytes 감소량과 같다고 보장하지 않는다.

화면 변화가 작은 **UI upload 회수·BGM 장면별 로딩/streaming**을 먼저 권한다. 다음은 skybox/대형 UI 압축이다. Shadow map 해상도와 위치 RT의 FP32 형식은 사용자 지시로 유지하며 5·6번은 적용 대상에서 제외한다. GPU 논리 용량·DDS payload를 전체 Private 절감량으로 합산하지 않는다.

## 후속 설명: 위치 RT의 16bit float 부작용

사용자가 6번도 적용 대상에서 제외했다. 아래는 제외 판단의 근거인 과거 대안 분석이며 현재 적용 계획이 아니다.

현재 `DeferredRender.hlsl`은 geometry의 world position을 RT에 기록하고 `gtxtInputTextures[2]`에서 다시 읽어 `DeferredLighting`과 `gmtxShadowTransform`에 사용한다. 32→16bit 저장은 월드 좌표를 반올림한다. HLSL에서 읽는 변수를 계속 float4로 두어도 이미 잃은 정밀도는 복구되지 않는다. [Microsoft DXGI 형식 설명](https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format)의 half 형식은 지수 5bit·가수 10bit다.

| 좌표 한 성분의 절댓값 구간 | 일반 FP16 인접 값 간격 | 최근접 반올림 시 오차 상한 |
|---|---:|---:|
| 128 이상 256 미만 | 0.125 | 0.0625 |
| 512 이상 1024 미만 | 0.5 | 0.25 |
| 1024 이상 2048 미만 | 1 | 0.5 |

단위는 게임 world 좌표이며 미터로 환산하지 않았다. 원점에서 멀어질수록 같은 표면에 저장되는 위치 값의 간격이 커진다. 이 표는 half 표현 간격의 계산이며 게임 화면에서 관측한 오류 크기가 아니다.

가능한 부작용은 점/spot 광원의 감쇠·반사광 경계의 계단 현상, 움직일 때 조명/그림자 경계 깜빡임, shadow comparison의 셀프 섀도잉/경계 변화다. 현재 shader는 위치를 shadow 좌표로 직접 변환하므로 **shadow map을 8192로 유지해도 위치 RT의 정밀도 축소가 그림자 품질에 영향을 줄 수 있다.** 조명과 shadow 평가의 시각적 위험이며, 서버 판정·충돌·실제 객체 transform/geometry를 변경하는 것은 아니다. 이후 같은 위치 RT를 쓰는 효과가 추가되면 그 효과도 검증한다.

논리 texel 절감은 15.82MiB다. 단순 형식 교체보다 camera-relative/view-space 좌표를 보관하거나 기존 depth로 위치를 복원하는 대안이 있으나 shader 계약과 오차 검증이 추가된다. 적용한다면 PSO/RTV/SRV 형식을 일치시키고 가까운/먼 월드 좌표, 근접 조명, 얇은 geometry, 정지/이동 camera, skinning과 8192 shadow의 전후 화면을 비교해야 한다. 현재는 미적용이다.

## 후속 설명: UI 동적 로딩의 지연과 적용 대상

정확한 end-to-end 로딩 시간은 아직 측정하지 않았다. 현재 승리/패배 각각 약 25.43MiB, Speed 41.66MiB가 비압축 DDS다. 최초 표시 시 로딩하면 파일 읽기→GPU 자원/임시 upload 준비→copy 제출→GPU 완료→descriptor/UI 공개가 필요하다. DDS는 기존 GPU용 데이터이므로 PNG/JPEG 디코딩 비용과 구분한다. 디스크/파일 캐시, GPU queue 부하와 allocator 상태에 따라 지연이 다르며 60fps의 한 프레임 예산은 약 16.7ms다. 총 지연이 짧아도 render thread를 막으면 프레임 끊김을 만들 수 있다.

승리/패배 화면은 결과 수신 시 실제 필요한 한 장만 준비하는 후보로 적합하다. 종료 전 사전 로딩이 가능하면 미리 시작하고, 서버에서 결과가 확정되기 전 임의로 승패를 예측하지 않는다. 결과 직후 시작한다면 준비 동안 가벼운 결과 UI를 표시할 수 있다. 현재 `Win/Defeat`는 사전 생성 객체의 DrawOn만 호출하므로 비동기 변경 시 준비/실패 상태와 표시 시점을 추가해야 한다.

반대로 Speed/Blood는 스킬 발동/피격 즉시 필요한 효과라 최초 이벤트에서 로딩을 시작하면 누락 또는 늦은 표시가 생길 수 있다. 이들은 압축/크기 검증 후 인게임 로딩 중 사전 준비하는 편이 적합하다. 상점 UI는 열리기 전 요청하거나 최초 열림의 준비 표시를 포함한다.

비동기 파일 I/O만으로 GPU 단계의 지연이 자동으로 사라지지 않는다. 기존 전역 scene descriptor 할당과 command list를 임의의 background thread에서 동시에 수정하지 않고, GPU 준비를 예약해 fence 완료 후 render thread에서 게시하는 수명 계약이 필요하다. 준비/복사 중 장면 종료 시 취소와 마지막 자원 해제를 포함한다. 전후 성과는 cold/warm cache 각각의 읽기·GPU 완료·첫 표시 시간과 최대 frame time, 메모리를 함께 측정한다. 이번에는 설계 설명만 추가했고 런타임 흐름을 바꾸지 않아 기존 구조도는 재생성하지 않았다.

## 후속 계산: UI를 BC7으로 압축해 사전 로딩

아래는 적용 전 예상이다. 후속 사용자 요청으로 실제 압축과 wrap gutter/UV 보정을 적용했다. 현재 수치와 품질·검증은 [UI 최적화 결과](UI_TEXTURE_COMPRESSION.md)를 따른다.

사용자는 동적 로딩 대신 압축 후 사전 로딩의 절감량과 저장 상태를 질문했다. **에셋 변환·런타임 적용은 아직 요청되지 않았고 실행하지 않았다.** 현재 `CTextureShader::BuildObjects`의 INGAME 분기에서 참조하는 21개 고유 DDS를 조사했다. 역할별 스킬 atlas가 다르므로 실제 한 역할에서 생성하는 대상은 20종이며, 두 역할의 atlas를 동시에 더하지 않는다. 이 범위는 모두 32bit RGBA 계열 단일 mip이다.

| 범위 | 현재 비압축 texel payload | BC7 예상 payload | 예상 절감 |
|---|---:|---:|---:|
| 대형 UI 4장: 승리·패배·Speed·Blood | 100.57MiB | 25.17MiB | 75.40MiB |
| 플레이어 인게임 UI 20종 | 142.29MiB | 35.62MiB | 106.67MiB |
| 보스 인게임 UI 20종 | 138.02MiB | 34.55MiB | 103.47MiB |

위 20종 모두 BC7 화질 검증을 통과한다는 가정에서 약 **75%**를 줄인다. 대형 네 장은 20종 합계의 부분집합이므로 별도 절감으로 합산하지 않는다. 미니맵·HP/MP·상점·아이템·역할별 스킬 atlas도 20종에 포함한다. Billboard UI, Title/Lobby/Ready UI, dissolve, UI mesh/CB, 공통 render target은 이 계산에 포함하지 않았다.

RGBA32는 픽셀당 4byte이고 BC7은 4×4픽셀 블록당 16byte다. 계산은 `ceil(width/4) × ceil(height/4) × 16`이며 실제 DDS 헤더의 현재 크기와 mip 수를 기준으로 했다. 여러 원본 치수가 4의 배수가 아니므로 변환 시 4픽셀 배수 canvas로 패딩하고 내용의 픽셀 크기와 UV/atlas 경계를 보존해야 한다. 같은 이미지 내용을 그대로 배치하는 계산이며 크기 축소·새 mip 생성의 절감이나 비용을 넣지 않았다. [BC7 형식](https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/bc7-format), [블록 압축의 치수·표시 조건](https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/block-compression)이 근거다.

**압축한 상태로 로딩한다는 의미가 맞다.** 개발 시 DDS를 BC7으로 변환해 저장하고, 인게임 로딩 단계에서 해당 BC7 블록을 읽어 upload 버퍼를 거쳐 BC7 DEFAULT texture로 복사한다. GPU 메모리에는 압축 블록으로 상주하며 텍스처를 샘플링할 때 GPU 하드웨어가 필요한 블록을 해석한다. 이미지 전체를 비압축 RGBA로 풀어 상주시키는 방식이 아니다. 현재 `CreateTextureResourceFromDDSFile`은 DDS loader가 만든 형식의 resource와 subresource를 그대로 업로드하며 `DDSTextureLoader12.cpp`는 BC7 형식과 블록 크기를 지원한다. 기존 NPC BC7 로딩도 같은 DDS 경로를 사용한다. 사전 로딩하므로 게임 중 첫 표시 시 파일 로딩을 시작하지 않는다. 읽기/전송량 감소는 기대할 수 있지만 로딩 시간은 측정하지 않았다.

BC7은 손실 압축이다. 글자·작은 아이콘·alpha 경계의 번짐이나 테두리를 원본과 비교하고 문제가 있는 항목은 비압축으로 유지해야 하므로 실제 채택 절감량은 위 값보다 작을 수 있다. GPU heap/row pitch 정렬·임시 upload 잔존량·프로세스 Private Bytes는 별도 지표다. upload 데이터도 줄지만 GPU 복사 완료 후 반환해야 하며 같은 절감을 중복 합산하지 않는다. **전체 클라이언트 메모리가 위 payload만큼 감소한다고 실측 없이 단정하지 않는다.**

[UI BC7 예상량 근거 JSON](evidence/ui-bc7-estimate-20261005.json)에 원본 21 DDS/생성 코드 SHA-256, 치수·mip·형식, 패딩 크기, byte 단위 계산과 역할별 합계를 보존했다. 에셋·코드·실행 파일을 변경하지 않은 헤더 기반 계산이며 압축 결과의 실제 allocation·화질·Private Bytes 측정은 후속 작업이다.

## 재현과 검증 범위

```powershell
python ./scripts/Measure-StartupResources.py --output artifacts/logs/startup-resource-recheck.json
./scripts/Test-AgentEnvironment.ps1
```

[조사 스크립트](../../scripts/Measure-StartupResources.py)는 포함된 FMOD 2.00.01 x64 DLL이 필요하며 에셋을 수정하지 않는다. DDS header mip count 0은 추가 mip 없는 파일 표기로 단일 mip으로 해석했다. GPU 숫자는 명시한 texel/DDS payload 계산이며 이번에 `GetResourceAllocationInfo`를 실행한 값이 아니다. 기존 [3회 전체 측정](evidence/npc-memory-20261005/measurements.json)과 이번 개별 음원 probe를 구분한다.

에이전트 환경 234개 compilation source, Serena GameFramework 36/Shader 208/SoundManager 14개 심볼 색인 PASS. [DDS 구조도](../diagrams/dds-sharing/README.md)와 [NPC 수명도](../diagrams/npc-memory/README.md)를 대조했으며 모델 DDS/arena 범위를 UI·음원까지 확대하지 않았다. 구현 구조/흐름 변경이 없어 frozen JSON/HTML은 재생성하지 않았다. Archify doctor PASS. 파일/소스 해시·합계·문서 링크 검산 완료. 게임 빌드·매치 실행은 반복하지 않았으며 Title 반복 전환 live resource, 실제 audio playback/loop, 압축·그림자·위치 RT 품질은 후속 검증이다.
