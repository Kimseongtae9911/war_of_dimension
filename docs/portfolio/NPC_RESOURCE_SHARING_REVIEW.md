# 미니언·몬스터 자원 공통화 검토

## 검토 결과와 우선순위

미니언과 동일 종류의 몬스터는 **이미 모델을 한 번 로드하고 여러 인스턴스에서 공유**한다. 반복 수만큼 메시·텍스처·clip 행렬을 복제하는 구조가 아니다. 추가 메모리 절감은 GPU 복사에 사용한 임시 upload 버퍼 정리, 모델 사이 동일 DDS의 공유, 큰 비압축 텍스처의 크기·형식 개선에서 기대할 수 있다.

아래 비용 표는 2026-10-05 커밋 `38bb9ee7`의 공용 파티클 풀 적용 후 **검토 당시 기준선**이다. Release RTX 4070 SUPER `pooled` 3회 측정의 checkpoint를 재집계했다. 후속 2번 DDS 공유는 [적용 내용과 실제 GPU 검증](#후속-적용-모델-dds-공유)에 기록했다. 이후 **1번 조기 upload 해제·3번 에셋 압축·4번 상수 arena도 구현**했으며 [별도 최종 문서의 전후 3회 실측·9종 스크린샷](NPC_MEMORY_OPTIMIZATION.md)을 따른다. 아래 예상치·당시 미적용 설명과 이전 해시는 역사 자료로 보존한다.

| 우선순위 | 후보 | 확인한 근거 | 예상 효과와 범위 |
|---|---|---|---|
| 1 | NPC upload 버퍼를 GPU 복사 완료 후 해제 | 인게임 `ReleaseUploadBuffers`가 미니언·몬스터·타인 보스의 모델 계층을 순회하지 않음 | 일반 몬스터 텍스처 upload payload **400MiB**, 미니언 **12.25MiB**, Ogre **37MiB**가 정리 후보. CPU/Private 실측 감소량은 별도 검증 필요 |
| 2 | 모델 사이 DDS 자원 공통화 | Chest와 Beholder가 같은 `AlbedoPBR.dds`·`MSPBR.dds`를 각각 생성 | 두 장 합계 **8MiB의 중복 DEFAULT 텍스처 payload**와 대응 upload 후보 제거. 일반 몬스터 텍스처 400→392MiB |
| 3 | Minotaur 텍스처 최적화 | 몸통 4096² 3장·무기 2048² 3장, 모두 비압축 32bpp·1 mip | 동일 해상도 BC7 단일 mip의 산술 예시는 **240→60MiB**, **180MiB payload 절감**. 화질·채널·색 공간 검증 필요 |
| 4 | 미니언의 상수 버퍼를 하나의 upload arena에 별도 offset으로 보관 | 프레임·인스턴스마다 256byte 데이터를 별도 committed resource로 생성 | 64KiB 할당 기준 모델은 **18.75→0.125MiB**. 인스턴스별 데이터·GPU 수명은 유지하며 실제 장치 계상량은 재측정 필요 |
| 5 | 몬스터 종류의 생성 코드·애니메이션 설정을 데이터로 공통화 | 7개 파생 생성자가 모델 부착·controller·track 설정을 반복 | 유지보수 개선 중심. 이미 공유되는 모델 데이터의 큰 중복 절감으로 설명하지 않음 |

이 후보들은 서로 겹친다. 텍스처 공유 후 upload 정리 대상은 400→392MiB이고, 압축하면 대응 upload도 작아진다. **예상치를 더해 Private commit 감소량으로 제시하지 않는다.** GPU DEFAULT 자원·UPLOAD 자원·Working Set·Private commit은 서로 다른 지표다.

## 현재 비용 분해

전체 미니언·몬스터·Ogre 구간의 Private commit 순증가 평균 949.03MiB를 나누면 다음과 같다. [현재 메모리 구성](PARTICLE_BUFFER_POOL.md#최적화-후-클라이언트-메모리-구성-비율)과 같은 실행 근거다.

| 생성 구간 | Private 순증가 | Working Set 순증가 | DXGI LOCAL 순증가 |
|---|---:|---:|---:|
| 미니언 12개 | 28.55MiB | 15.64MiB | 12.31MiB |
| 일반 몬스터 7종·9개 | 836.86MiB | 429.82MiB | 403.11MiB |
| Ogre 보스 1개 | 83.63MiB | 44.51MiB | 38.55MiB |
| **합계** | **949.03MiB** | **489.97MiB** | **453.98MiB** |

각 행은 로딩 checkpoint 사이 순증가량이다. 최종 시점에 해당 자원이 단독 소유하는 heap을 측정한 값은 아니다. 일반 몬스터가 가장 큰 구간이며 미니언 반복 생성은 이 구간의 주원인이 아니다.

아래는 각 모델의 `model_begin`→`model_loaded` 순증가다. 이후 controller·객체 상수 생성 비용까지 포함한 위 표의 구간 값과 구분한다. 텍스처는 DDS 헤더·픽셀 payload를 직접 파싱했고 행렬은 `.bin`의 keyframe 수 × animated frame 수 × 64byte로 계산했다.

| 모델 | 인스턴스 | 로더 Private 순증가 | 텍스처 payload | clip 행렬 |
|---|---:|---:|---:|---:|
| FreeLich 미니언 | 12 | 25.83MiB | 12.25MiB | 0.53MiB |
| Unique Red | 1 | 38.49MiB | 16MiB | 4.37MiB |
| Rare Green | 1 | 34.78MiB | 16MiB | 1.63MiB |
| Golem | 1 | 129.45MiB | 64MiB | 0.40MiB |
| Bear | 1 | 105.36MiB | 48MiB | 5.91MiB |
| Minotaur | 1 | **488.69MiB** | **240MiB** | 2.53MiB |
| Chest | 2 | 18.02MiB | 8MiB | 0.43MiB |
| Beholder | 2 | 17.86MiB | 8MiB | 0.93MiB |
| Ogre | 1 | 83.15MiB | 37MiB | 3.82MiB |

일반 몬스터 7종의 텍스처 payload는 400MiB, clip 행렬은 **16.20MiB**다. Minotaur 로더의 Private 순증가가 일반 몬스터 전체 생성 구간의 약 58.4%에 해당한다. DEFAULT 텍스처와 대응 upload가 함께 존재하는 코드·헤더 크기 및 로더 순증가를 대조하면 비압축 텍스처와 upload 유지가 큰 비용이라는 판단을 뒷받침한다. 개별 native resource 계측 없이 전체 증가량을 두 항목에 정확히 귀속하지는 않는다.

## 이미 공통화된 자원

- `CIngameScene::BuildObjects`는 `FreeLichPBR.bin`을 한 번 로드하고 `MAX_MINION=12`개 `CMinion`에 같은 `CLoadedModelInfo`를 전달한다.
- 일반 몬스터는 7개 파일을 한 번씩 로드한다. 9개 인스턴스 중 Chest의 5·7번, Beholder의 6·8번은 각각 같은 모델을 사용한다.
- `SetChild(modelRoot, true)`로 모델 계층·메시·material을 유지하고, `CAnimationController`는 같은 `CAnimationSets`에 `AddRef`한다. clip keyframe 행렬은 모델별 한 벌이다.
- 인스턴스마다 track 시간·활성 상태·blend 및 skinning 결과 상수 버퍼를 만든다. 이 결과는 각 NPC가 다른 시점에 이동·공격·죽으므로 독립적으로 유지해야 한다.
- 파일에서 읽는 base geometry는 기존 내용 캐시를 사용한다. 미니언·7종 몬스터·Ogre의 총 26개 geometry 레코드를 SHA-256으로 대조했을 때 26개가 모두 달랐다. 이 표본의 모델 사이 추가 geometry 캐시 절감 후보는 0개다.

근거: [장면 생성](../../Client/WarOfDimension/Scene.cpp), [모델·controller·재질](../../Client/WarOfDimension/Object.cpp), [스킨드 메시](../../Client/WarOfDimension/Mesh.cpp), [기존 메시 공유 구현](../architecture/MESH_SHARING.md).

## 변경 전 모델 사이 텍스처 공유의 한계

변경 전 `CMaterial::LoadTextureFromFile`은 `Model/Textures/<name>.dds`로 경로를 만들고, `@` 토큰의 중복 검색을 모델의 부모·자식 계층 안에서만 수행했다. 서로 다른 모델 파일에서 같은 정상 토큰을 로드하면 새로운 `CTexture`와 DEFAULT/UPLOAD 자원이 만들어졌다.

Chest와 Beholder는 둘 다 `AlbedoPBR`·`MSPBR`를 정상 토큰으로 포함한다. 각 장은 1024² × 32bpp = 4MiB이므로 같은 두 파일을 두 번 로드한 중복이 **8MiB**다. 동일 모델 인스턴스 두 개 때문에 4회 생성되는 것은 아니다. NPC에서 사용하는 서로 다른 25개 DDS 파일의 전체 바이트 해시도 비교했으며, 이름이 다른 파일의 완전 동일 중복은 없었다.

검토에서 제안한 공유 대상은 읽기 전용 texture resource다. CTexture가 material root parameter와 SRV handle도 보관하므로 resource 소유권과 binding을 분리한다. 후속 구현은 기존 DDS 로딩 옵션(maxsize=0, flags=NONE, loader=DEFAULT, 파일의 전체 mip/array)을 고정하고 device·정규 경로·resource 용도를 키로 사용한다. sRGB 강제 등 옵션을 추가하면 키에도 반영해야 한다. 색이 다른 `RedHP`·`GreenHP`를 이름만 비슷하다고 합치지 않는다.

## upload 수명과 상수 버퍼

**1번의 누락은 미니언·몬스터만의 문제인가?** 별도 NPC 배열이 직접 순회되지 않는 것은 맞지만, 나머지가 모두 해제되는 것은 아니다. `m_ppOtherClient`와 `m_skillObjects` 등 별도 배열도 이 함수의 직접 방문 대상에 없다. 다른 객체와 공유하거나 별도 종료 경로에서 반환할 수 있으므로, 직접 순회 누락과 실제 자원 잔류를 구분해야 한다. 하늘·shader·일반 객체·계층 객체·billboard에는 기존 해제 호출이 있다. 미니언·몬스터는 이 배열에 포함되지 않아 자신의 계층을 통한 조기 해제가 빠졌다.

`CreateTextureResourceFromDDSFile`은 DEFAULT texture와 upload buffer를 만들고 `UpdateSubresources`로 복사를 기록한다. `CTexture::ReleaseUploadBuffers`와 `CGameObject::ReleaseUploadBuffers`의 계층 순회는 이미 있지만, `CIngameScene::ReleaseUploadBuffers`는 하늘·일반 shader·일반 객체·맵 배열·billboard만 처리한다. NPC 배열과 타인 보스 모델을 처리하지 않는다. billboard의 NPC 포인터는 표시 위치 참조이며 upload 해제는 별도로 생성한 HP UI 객체를 대상으로 한다. `CLoadedModelInfo`를 삭제해도 캐시 포인터 배열만 삭제하고 texture upload를 반환하지 않는다.

이 NPC DDS들은 단일 mip이고 행 크기가 256byte에 맞아 texture upload footprint의 payload가 픽셀 payload와 같다. 일반 몬스터 400MiB가 해제 후보인 이유다. geometry·bone index/weight의 upload는 이 숫자에 포함하지 않았다. [Microsoft fence 기반 자원 관리](https://learn.microsoft.com/en-us/windows/win32/direct3d12/fence-based-resource-management)에 따라 실제 GPU 복사 완료를 확인한 뒤 모든 소유 계층을 방문하고, 같은 공유 자원에 대한 반복 해제도 안전해야 한다.

미니언의 `AllCreateShaderVariables`는 24개 프레임과 NPC wrapper에 객체 CB를 만들므로 12개 기준 300개의 256byte 버퍼다. 현재 helper가 설정하는 64KiB placement alignment를 기준으로 예약량 모델은 18.75MiB이고, 별도 offset으로 300개를 보관한 arena를 같은 단위로 정렬하면 0.125MiB다. **이 차이 18.625MiB는 예약량 계산이며 Private commit 실측 절감량이 아니다.** frame 수에 따라 작은 CB를 많이 만드는 다른 몬스터도 같은 개선 후보다. skinning pose는 별도 영역으로 보관한다.

공유 material에서 `CMaterial::CreateShaderVariables`를 반복 호출하면 기존 `m_pd3dcbMaterial`을 반환하지 않고 새 포인터로 덮어쓴다. 미니언 한 material의 12회 호출 중 앞선 11개는 해제 경로에서 접근할 수 없게 되는 문제다. 생성 1회·멱등 처리 및 소유권 정리가 필요하다. 64KiB 기준 0.6875MiB의 정적 예약량 후보이며 장치의 실제 resident/Private 비용과 구분한다.

## 애니메이션·생성 코드 공통화의 경계

현재 `CAnimationSets`에는 읽기 전용 clip 행렬과 변경되는 `m_ppAnimatedBoneFrameCaches`가 함께 있다. 동일 모델 인스턴스가 그 계층을 재사용하고 `AdvanceTime`이 pose/transform을 갱신한다. 지금 렌더 루프는 NPC별 Animate→Update→Render를 순차 수행하지만, 모든 NPC의 애니메이션을 먼저 계산하거나 worker에서 병렬 처리하면 다른 인스턴스가 같은 pose를 덮어쓸 수 있다. `SetChild`도 공유 계층의 parent 포인터를 바꾼다. 독립 pose·frame transform·bone binding과 공유 clip/geometry/texture를 명확히 분리해야 한다.

따라서 새로운 구조의 공유 모델에는 불변 geometry·skin index/weight·bind pose·material·clip을 두고, 각 NPC에는 pose 배열·track·blend·world transform·dissolve·GPU 결과 영역을 두는 방향을 제안한다. 공유 skin 데이터도 frame 포인터와 분리해야 한다. 현재 구조를 그대로 더 강하게 공유하는 것은 권장하지 않는다. 이 분리는 수명/독립성 개선이 주효과이며 기존의 한 벌 clip을 12벌에서 1벌로 줄이는 메모리 성과로 계산하지 않는다.

7종 몬스터의 반복 생성자 설정은 `NpcDefinition` 같은 데이터의 모델 경로·track→clip 매핑·loop/once·run/attack/death·scale·sound로 정리할 수 있다. 기존 clip 번호 계약을 유지해야 하며 네트워크 run/attack/death뿐 아니라 blend 이전 track·직접 호출·미니언 HIT/ATTACK2 경로까지 확인하고 실제 사용 clip만 보관하는 별도 개선을 검토한다. 전체 clip 행렬이 16.20MiB이므로 예상 상한은 그보다 작으며 텍스처보다 우선순위가 낮다.

## 에셋 크기 개선의 조건

**3번은 메시나 애니메이션을 합치는 작업이 아니라 텍스처의 저장 형식을 줄이는 작업이다.** 현재 RGBA32는 픽셀마다 4byte를 사용한다. BC7은 4×4 픽셀을 16byte 블록으로 저장하므로 동일 해상도에서 픽셀 payload가 1/4이 된다. GPU는 압축된 자원을 보관하고 샘플링할 때 블록을 해석한다. 디스크 파일만 압축하고 실행 시 전체 RGBA로 펼치는 방식과 다르다. 손실 압축이므로 근접 표면·normal·alpha·재질 채널의 품질 확인이 필요하다.

Minotaur 몸통의 4096² 32bpp 3장은 192MiB, 무기의 2048² 32bpp 3장은 48MiB다. 전부 mip 없는 비압축 DDS다. 같은 해상도의 BC7은 [공식 형식의 4×4 texel당 16byte](https://learn.microsoft.com/en-us/windows/win32/direct3d11/bc7-format)를 기준으로 240→60MiB, 180MiB payload 감소가 된다. 몸통 2048²·무기 1024²로 낮추어 비압축을 유지해도 산술상 60MiB다. 두 방식을 동시에 적용하면 별도 계산이 필요하다.

일반 몬스터 경로 공유 후 392MiB를 모두 같은 해상도 BC7 단일 mip로 바꾸는 예시는 98MiB, 294MiB 감소다. 이는 에셋 변경을 적용하거나 화질을 검증한 결과가 아니다. mip chain을 새로 넣으면 해당 크기도 증가한다. albedo/alpha·metallic/smoothness 채널·normal RGB·sRGB/linear 및 먼 거리 aliasing을 직접 검증해야 한다. BC5 normal로 바꾸려면 현재 RGB 샘플링과 Z 복원 방식을 함께 확인해야 한다. 기존 에셋은 보존하고 새 대용량 파일은 Git LFS를 적용한다.

**해상도를 줄이는 선택은 별도다.** 가로·세로를 각각 절반으로 줄이면 픽셀 수가 1/4이 되지만 표면 세부 표현도 줄어든다. BC7은 해상도를 유지하고 색 표현을 근사한다. 240→60MiB는 동일 해상도·단일 mip라는 가정의 텍스처 payload 계산이며, 프로세스 Private commit이 180MiB 줄었다는 실측 결과가 아니다.

## 4번 설명: 상수 버퍼의 할당을 통합

각 NPC의 위치·회전·크기 행렬, object ID, dissolve 값은 서로 다르므로 값을 공유할 수 없다. `OBJECT_INFO`는 72byte이고 CBV 주소 정렬 때문에 256byte 간격으로 보관한다. 현재는 작은 데이터 하나마다 별도 committed UPLOAD resource를 만들어 64KiB 할당 단위의 낭비가 생긴다.

제안한 arena는 하나의 큰 UPLOAD buffer 안에서 각 데이터에 **서로 다른 offset**을 부여한다. 예를 들어 `미니언 A / 프레임 0`은 base+0, `미니언 A / 프레임 1`은 base+256, `미니언 B / 프레임 0`은 base+6400을 사용한다. GPU에는 해당 offset의 주소를 bind하므로 서로 다른 transform이 유지된다. 12×25=300개 영역의 유효 저장 공간은 76,800byte이며, 64KiB 단위로 올림한 하나의 할당은 128KiB다. 현재 300×64KiB=18.75MiB와 비교하면 18.625MiB의 **예약량 모델** 차이다.

CPU가 다음 프레임 값을 쓰는 동안 GPU가 이전 프레임 값을 읽을 수 있다. 따라서 frame별 영역이나 ring을 두고 fence 완료를 확인한 영역만 재사용해야 한다. 부족하면 새 arena를 할당하되 이전 자원은 GPU 완료까지 유지한다. 파티클 풀과 마찬가지로 메모리 할당 통합과 동시 실행 중 데이터 공유는 구분한다. 이 변경과 material CB 반복 생성 문제 해결은 아직 구현하지 않았다.

## 후속 적용: 모델 DDS 공유

`CMaterial::LoadTextureFromFile`의 일반 토큰과 `@` 토큰을 같은 파일 경로 조회로 통합했다. 모델 사이에서도 `(device, 정규 절대 경로, resource 용도)`가 같으면 `SharedDdsTexture`가 DEFAULT texture와 UPLOAD buffer를 한 벌만 생성한다. Windows 경로의 대소문자와 `..`를 정규화하며, 다른 device·용도·경로는 분리한다. 캐시는 `weak_ptr`여서 마지막 모델 또는 복사 wrapper가 사라지면 자원을 유지하지 않는다. 파일 변경 중 hot reload나 임의의 다른 queue에서 업로드 중인 자원 사용은 지원 범위가 아니다.

각 material의 `CTexture` wrapper, root parameter와 GPU handle 저장 공간은 독립적이다. **읽기 전용 SRV descriptor는 같은 Scene 힙에서 재사용**하여 기존 100~200개 SRV 힙을 중복 descriptor로 채우지 않는다. 힙을 새로 만들면 SRV 캐시는 비우고 새 힙에 descriptor를 만든다. UI·파티클의 기존 직접 DDS 로더는 이 캐시에 등록하지 않는다. 서로 다른 이름의 같은 픽셀을 내용 해시로 합치는 기능은 적용하지 않았다.

공용 upload는 `ReleaseUploadBuffers`로 반복 해제할 수 있으며 호출자는 GPU copy fence 완료를 보장한다. 업로드가 남은 동안 다른 command list가 공유를 요청하면 오류로 거부한다. 기존 모델 초기화의 단일 command list와 GPU 대기 순서를 유지한다. 이번에는 NPC 조기 해제 순회를 추가하지 않았으므로 **공유 후 남는 392MiB의 일반 몬스터 texture upload 조기 회수는 후속 작업**이다. 종료 시 남은 upload는 마지막 공유 소유자가 회수한다. 일반 CTexture의 남은 upload도 소멸 시 반환하고, 복사 시 resource/upload 소유권과 binding 배열을 안전하게 유지하도록 보완했다.

실제 Chest·Beholder 모델을 WARP에서 로드해 texture 참조 4개→고유 GPU resource 2개를 확인했다. `AlbedoPBR.dds`·`MSPBR.dds` 각각 1024² RGBA32, 단일 mip 4MiB다. 이 표본의 기존 16MiB DEFAULT/16MiB UPLOAD가 각각 8MiB로 줄어든다. DEFAULT는 `GetResourceAllocationInfo`, UPLOAD는 copy footprint buffer의 Width로 확인했다. **DEFAULT 8MiB와 UPLOAD 8MiB는 서로 다른 자원 절감이며 Private commit 16MiB 절감으로 단정하지 않는다.**

Debug·Release의 [전용 검사](../../scripts/Test-DdsSharing.ps1)는 12개 범주를 통과했다. 경로 정규화, 다른 경로·용도·device 분리, 미완료 다른 list 거부, 실패 DDS의 캐시 미등록, GPU 픽셀 readback 일치, fence 완료 후 공유, 반복 upload 해제, weak cache 만료/재생성, wrapper binding·복사 수명, 실제 두 모델의 자원/할당량 및 Scene 힙 교체, 마지막 소유자 해제를 검사한다. 종료 live DDS=0, GPU error=0이며 기존 모델의 UPLOAD 초기 상태 경고 `CREATERESOURCE_STATE_IGNORED`(1328) 16개는 숨기지 않았다.

Release 메시 공유 12개 검사와 에셋 3개 audit, 영웅 선택 파츠 1,576,969개 실제 변환 행렬 비교도 통과했다. 실제 Shader의 고정 효과 재생 1회는 Title→INGAME 및 풀 종료 검사를 통과했다. 이 기능 확인 실행의 Private commit 2,656.47MiB·Working Set 1,229.85MiB·DXGI LOCAL 1,518.34MiB는 **1회 관측**이다. 이전 3회 평균과의 차이를 DDS 공유 단독 성과로 사용하지 않는다. 모델 DDS 전체에 적용되므로 Chest/Beholder 외 모델의 공유와 종료 소유권 보완도 영향을 줄 수 있다. 실전 멀티플레이 전투·압축 화질·NPC 조기 upload 회수·arena는 미검증이다.

소스·에셋·바이너리 해시 및 검사 결과는 [DDS 적용 근거](evidence/dds-sharing-20261005.json), 구현 소유권과 재생성·검증 기록은 [Archify 구조도](../diagrams/dds-sharing/README.md)에 둔다. 원래의 NPC 검토 JSON과 공용 풀 3회 실측은 변경 전 기준선으로 보존한다.

```powershell
./scripts/Build.ps1 -Configuration Debug -Module Client
./scripts/Test-DdsSharing.ps1 -Configuration Debug
./scripts/Build.ps1 -Configuration Release -Module Client
./scripts/Test-DdsSharing.ps1 -Configuration Release
./scripts/Test-MeshSharing.ps1 -Configuration Release -AuditAssets
./scripts/Test-HeroSelection.ps1 -Configuration Release
./scripts/Measure-ClientMemory.ps1 -Configuration Release -Runs 1 -Scenario ParticleReuse -Modes pooled -OutputDirectory artifacts/logs/dds-sharing-final
./scripts/Test-AgentEnvironment.ps1
```

## 근거와 검증

[정적·실측 근거 JSON](evidence/npc-sharing-review-20261005.json)에 모델/소스/DDS SHA-256, 각 모델 frame·skin·clip·texture 비용, 최근 3회 구간값과 조건별 예상치를 보존했다. `.bin`은 기존 [Measure-ClientAssets.py](../../scripts/Measure-ClientAssets.py)로 끝까지 파싱했고, DDS magic/header·단일 mip·전체 파일 크기와 payload를 대조했다. geometry 26개·DDS 25개 해시, 합계·예상 byte 계산과 문서 링크를 검증했다.

최초 검토 시 compilation database 229개·Serena `Object.cpp` 194개 심볼 및 기존 메시 공유 구조도 showcase 9/9·오류/경고 0을 확인했다. 후속 DDS 적용은 231개 소스·에이전트 환경 검사와 새로운 DDS 공유 구조도 showcase 9/9·오류/경고 0을 통과했다. 실제 pose 독립성·NPC 조기 upload 해제·arena·에셋 압축과 DDS 공유의 반복 A/B는 후속 작업이다. 이번 검토·후속 구현은 커밋·push하지 않았다.
