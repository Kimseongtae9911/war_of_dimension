# 하늘 큐브맵과 인게임 UI·dissolve 후속 최적화 검토

2026-10-06, [UI BC7 적용](UI_TEXTURE_COMPRESSION.md) 이후의 코드와 DDS를 조사했다. **하늘은 같은 해상도의 BC7 적용으로 DDS payload 96→24MiB, 72MiB 절감 후보다. dissolve는 이미 BC1 압축과 mip chain을 사용해 0.67MiB이며 R8로 바꾸면 오히려 커진다.** 이 문서는 미적용 검토이며 실행 코드·에셋·shadow 해상도·위치 G-buffer 형식을 변경하지 않았다.

[DDS 헤더·후보별 byte 계산·기존 3회 구간 측정·소스 해시](evidence/skybox-dissolve-review-20261006.json), [읽기 전용 조사 스크립트](../../scripts/Measure-SkyboxDissolve.py)가 근거다. MiB는 1,048,576byte다. 후보의 DDS texel payload, 기존 실제 GPU allocation, 프로세스 Private Bytes를 구분하고 합산하지 않는다.

## `ingame_ui_dissolve_ready`의 의미

[GameFramework.cpp](../../Client/WarOfDimension/GameFramework.cpp)의 인게임 생성 분기는 `CTextureShader::BuildObjects`로 UI를 만든 후 `Model/Textures/dissolve.dds` 하나를 만들고 이 checkpoint를 남긴다. 따라서 이 구간의 메모리 증가량은 **UI와 dissolve 생성 합계**다. dissolve 한 장의 크기나 UI dissolve 효과만의 비용이 아니다.

실제 dissolve 소비자는 [DeferredRender.hlsl](../../Client/WarOfDimension/DeferredRender.hlsl)의 `PSTexturedAnimationObjMultipleRTs`다. 캐릭터·미니언·몬스터의 사라지는 상태에 공통 `t26` 텍스처를 사용한다. `cColor.r`에 `smoothstep`을 적용하고, 객체별 `gnObjectState`로 alpha를 줄여 `clip`한다. UI의 [UI.hlsl](../../Client/WarOfDimension/UI.hlsl)은 이 dissolve 텍스처를 읽지 않는다. 여러 객체가 동시에 사라져도 각자의 상태 상수만 다르고 마스크는 한 장이다.

같은 Release EXE로 UI 에셋만 바꾼 이전 각 3회 측정에서 checkpoint 직전 대비 순증가의 중앙값은 다음과 같다. 현재 새로 게임을 실행해 얻은 수치는 아니다.

| 구간 순증가 | UI 압축 전 | UI 압축 후 |
|---|---:|---:|
| `skybox_ready` Private Bytes | 192.38MiB | 192.71MiB |
| `skybox_ready` DXGI LOCAL usage | 96.0039MiB | 96.0039MiB |
| `ingame_ui_dissolve_ready` Private Bytes | 297.48MiB | 88.19MiB |
| `ingame_ui_dissolve_ready` DXGI LOCAL usage | 148.5117MiB | 42.8242MiB |

하늘은 이 시점에 DEFAULT texture와 임시 UPLOAD가 함께 존재한다. 따라서 Private 순증가 약 193MiB를 최종 하늘 GPU texture 크기로 부르지 않는다. [CIngameScene::ReleaseUploadBuffers](../../Client/WarOfDimension/Scene.cpp)→하늘 객체→material→texture는 복사 fence 완료 후 하늘 UPLOAD를 반환한다. 하늘 압축의 최종 절감에서 이미 반환하는 원본 UPLOAD 96MiB를 잔존량에 다시 더하지 않는다.

## 하늘: BC7을 먼저 비교할 가치가 있다

[CSkyBox](../../Client/WarOfDimension/Object.cpp)는 `SkyBox/Space.dds` 한 개를 `RESOURCE_TEXTURE_CUBE`로 읽는다. 원본 헤더와 전체 payload를 검사한 결과 2048×2048, 6면, B8G8R8A8_UNORM, 단일 mip이며 25,165,824texel의 alpha가 모두 255다. 원본 mip count 0 표기는 loader가 단일 mip으로 해석한다. 96MiB는 6면 합계이고 여섯 개 중복 큐브맵 비용이 아니다.

| 미적용 후보 | 예상 DDS payload | 현재 대비 절감 | 판단 |
|---|---:|---:|---|
| 2048, BC7, 단일 mip | **24.00MiB** | **72.00MiB, 75%** | 우선 추천. 해상도·면·기존 색 공간 유지 |
| 2048, BC7, 전체 12 mip | 약 32.00MiB | 약 64.00MiB | aliasing 개선을 함께 검토할 경우. 단일 mip보다 약 8MiB 추가 |
| 2048, BC1, 단일 mip | 12.00MiB | 84.00MiB | BC7보다 12MiB 추가 절감, 별·그라데이션·밝기 판정 품질 위험 증가 |
| 1024, BC7, 단일 mip | 6.00MiB | 90.00MiB | 작은 별·은하 디테일 손실 가능, 2048 BC7 검증 후 판단 |

BC7은 4×4에 16byte, BC1은 4×4에 8byte다. [Microsoft 블록 압축](https://learn.microsoft.com/en-us/windows/desktop/direct3d11/texture-block-compression-in-direct3d-11), [BC7](https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/bc7-format), [DirectXTex texconv](https://github.com/microsoft/DirectXTex/wiki/texconv)을 기준으로 계산했다. 이것은 **후보 payload 계산**이며 변환 결과의 실제 allocation·Private Bytes 감소·화질을 실측하지 않았다.

2048은 이미 4의 배수여서 UI에서 사용한 패딩이나 UIB7 메타데이터가 필요하지 않다. Cube resource·SRV·6면 순서·색 공간을 유지하고 압축 블록으로 로딩하는 후보다. 원본이 8bit LDR이므로 HDR용 BC6H를 먼저 선택할 근거는 없다.

[Skybox.hlsl](../../Client/WarOfDimension/Skybox.hlsl)은 같은 큐브맵을 기본 방향과 시간에 따라 회전한 두 방향으로 총 세 번 샘플링한다. 밝기 0.3/0.7 판정·반짝임·두 은하 레이어 혼합을 사용한다. 손실 압축으로 별의 밝기가 경계를 넘으면 반짝임이나 혼합 선택이 달라질 수 있다. 원본 DDS 수치 비교뿐 아니라 **6면·면 경계·여러 카메라 방향·고정 시간 여러 지점·시간 진행 영상**에서 점멸과 별 디테일을 비교해야 한다. sample 수를 줄이는 것은 메모리가 아닌 별도 효과 품질/GPU 시간 변경이다.

Lobby·Ready·Ingame은 각각 하늘 생성 경로가 있지만, 전환 시 이전 Scene이 반환되는 현재 구조다. 같은 장면의 동시 하늘 중복을 발견하지 않았다. 기존 [SharedDdsTexture](../../Client/WarOfDimension/SharedDdsTexture.cpp)는 cube 용도를 허용하지만 하늘은 현재 직접 로딩 경로를 사용한다. 이를 weak cache로 바꾸어도 이전 Scene의 마지막 소유자가 먼저 사라지는 전환에서는 다음 로딩의 cache hit를 보장하지 않는다. 영구 strong cache는 재로딩 시간을 줄이는 대신 Title에서도 하늘을 계속 보유하므로 추가 메모리 절감으로 분류하지 않는다.

## dissolve: 추가 형식 압축의 메모리 이득이 작다

현재 `Model/Textures/dissolve.dds`는 **1024×1024 DXT1/BC1_UNORM, 11 mip**이다. 모든 mip payload는 699,064byte = **0.666679MiB**다. 4×4보다 작은 마지막 mip도 최소 한 블록으로 계산했다.

| 미적용 후보 | 예상 DDS payload | 현재 대비 변화 | 판단 |
|---|---:|---:|---|
| 현재 BC1, 1024, 11 mip | 0.666679MiB | 기준 | 이미 압축된 공통 마스크 |
| BC4, 1024, 11 mip | 0.666679MiB | **절감 0** | R 채널 용도에는 맞지만 동일한 8byte/블록 |
| R8, 1024, 11 mip | 1.333333MiB | **0.666654MiB 증가** | 현재보다 약 2배, 메모리 후보로 추천하지 않음 |
| BC1, 512, 10 mip | 0.166679MiB | **0.50MiB 절감** | 소멸 경계 디테일 변화 대비 효과가 작아 낮은 우선순위 |

R 채널만 읽는다는 사실만으로 R8가 항상 작아지지는 않는다. 현재 BC1은 평균 4bit/texel이고 R8은 8bit/texel이다. BC4 또한 단일 채널 4bit/texel이므로 메모리 절감 목적의 형식 교체 근거가 없다. [공식 DXGI 형식](https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format)과 블록 압축 표가 근거다.

dissolve는 현재 Framework가 한 번 생성하여 공통 root table에 바인딩한다. 객체별로 DDS를 만들거나 마스크를 복제하지 않는다. 객체마다 필요한 dissolve 진행 상태는 현재 상수로 유지해야 한다. R8 후보에서 원본 R값을 보존한다면 미리 계산된 마스크를 유지할 수 있지만, BC4 재압축·해상도 축소는 `smoothstep`과 `clip`의 경계를 바꿀 수 있다. 형식 후보는 아직 변환하지 않았고 실제 소멸 효과의 전후 영상도 미검증이다.

## UI와 dissolve의 임시 복사 버퍼 회수

UI BC7 측정에서 `GetResourceAllocationInfo`로 확인한 **UI 20종의 남은 UPLOAD 실제 할당량은 플레이어 36.8750MiB, 보스 35.8125MiB**다. 역할별 대체 atlas이므로 두 값을 한 실행의 절감으로 더하지 않는다. 이것은 하늘·dissolve를 포함하지 않는 UI 고유 texture 집계다.

현재 [GameFramework](../../Client/WarOfDimension/GameFramework.cpp)는 copy 명령 제출→`WaitForGpuComplete`→Scene/Player의 upload 반환만 호출한다. 별도 Framework 소유 `m_pTextureShader`·`m_pDissolveTexture`는 순회 대상에 없고, [CTextureShader::ReleaseUploadBuffers](../../Client/WarOfDimension/Shader.cpp)도 부모의 빈 함수를 호출한다. 따라서 **fence 완료 후 UI 객체들이 실제로 가진 texture의 upload와 dissolve upload를 순회해 반환하는 후보**가 남아 있다. 최종 DEFAULT texture와 SRV·UI 크기·dissolve 동작은 유지하는 작업이다.

dissolve upload는 이번에 개별 resource allocation을 계측하지 않았으므로 정확한 MiB 성과를 제시하지 않는다. mip별 row pitch/placement·heap 정렬이 있어 0.666679MiB payload와 동일하다고 가정하지 않는다. UI의 36.8750MiB도 이후 전체 Private 감소량을 실측하기 전에는 그대로 프로세스 절감으로 부르지 않는다.

후속 구현 시 중복 UI 객체·texture 복사본의 소유권을 확인하고 모든 upload 참조를 반환해야 한다. GPU fence 이전 반환·DEFAULT/SRV 반환·파티클 버퍼나 매 프레임 UPLOAD 상수의 반환을 섞지 않는다. 현재 [CTexture::ReleaseUploadBuffers](../../Client/WarOfDimension/Object.cpp)는 내부 배열을 nullptr로 만들어 같은 wrapper에서 반복 호출할 수 있지만, 별도 복사본의 AddRef는 각각 남을 수 있다. 기존 UI capture는 남은 UPLOAD 20개를 포함한 40 resource 종료 검사를 수행하므로 조기 회수 적용 시 fence 직후 검사와 종료 검사를 나누어 검증해야 한다.

정상 종료와 장면 전환은 이미 texture 반환 경로가 있다. 이번 후보는 기존 영구 누수를 확정한 것이 아니라 **씬에서 사용하지 않는 복사 staging의 체류 시간**을 줄이는 것이다. 반복 진입·오류 경로는 별도로 검증한다.

## 메모리와 별도로 검토할 GPU 비용

- dissolve shader는 `gnObjectState > 0` 검사 전에 텍스처를 읽고 `smoothstep`을 수행한다. 소멸하지 않는 draw에서 이를 생략하는 후보가 있지만 컴파일러가 이미 이동했는지 compiled shader와 GPU 시간을 확인해야 한다. 실제 최적화 성과나 추가 메모리 절감으로 주장하지 않는다. 상태는 draw 상수이며 분기·mip 선택·사라지는 경계를 유지해야 한다.
- Speed UI는 `GaussianBlur(..., 0.7)`에서 반경 7의 15×15, 코드상 **225 sample/픽셀**을 수행한다. 기존 효과를 재현한 에셋 사전 blur 또는 separable pass가 GPU 시간 후보다. 사전 blur는 4칸 atlas와 alpha·필터·화면 크기 변화 검증이 필요하고, 별도 pass는 render target과 동기화 비용이 추가된다. 메모리 최적화와 구분한다.

## 적용 우선순위와 검증

1. **하늘 2048 BC7 단일 mip**: 72MiB payload 후보, cube 면/방향·별/반짝임·색 공간 보존을 먼저 검증한다. 원본 upload는 이미 해제하므로 잔존 upload 절감을 중복 계산하지 않는다.
2. **UI+dissolve upload 조기 반환**: UI 실제 잔존 36.88/35.81MiB와 dissolve staging을 fence 후 반환한다. UI·마스크의 DEFAULT/SRV는 유지한다. 정상 종료·재진입·실패와 GPU debug layer 검사를 포함한다.
3. **dissolve의 일반 draw sample 생략·Speed blur 비용**: GPU 시간 측정이 필요하다. dissolve 512 축소나 BC1 교체는 작은 메모리 이득·품질 영향 때문에 뒤로 둔다.

```powershell
python ./scripts/Measure-SkyboxDissolve.py --output artifacts/logs/skybox-dissolve-recheck.json
./scripts/Test-AgentEnvironment.ps1 -StaticOnly
serena project index-file Client/WarOfDimension/GameFramework.cpp .
serena project index-file Client/WarOfDimension/Object.cpp .
```

DDS 6면/mip 크기·전체 하늘 alpha·후보 계산과 기존 6실행의 구간 합계를 검사했다. compilation database 235 source 및 에이전트 정적 검사, Serena Framework 36/Object 195 심볼 색인 PASS. 기존 DDS/UI 수명 구조도를 대조했고 현재 구조/흐름은 변경하지 않아 frozen JSON·HTML은 재생성하지 않았다. 에셋 변환·새 게임 측정·후보 화질·GPU 시간은 미검증이다. 직전 UI Debug/Release 빌드·렌더링/종료 검증과 이번 읽기 전용 검토를 구분한다.
