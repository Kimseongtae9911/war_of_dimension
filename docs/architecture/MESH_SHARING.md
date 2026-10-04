# 메시 공유 구현

## 현재 동작

`CGameObject::LoadFrameHierarchyFromFile`은 각 프레임의 이름·transform·material을 그대로 읽고, `<Mesh>:` 레코드는 `CStandardMesh::LoadSharedGeometryFromFile`로 로딩한다. 이름만 다른 동일 geometry는 같은 CPU 배열과 D3D12 정점·인덱스 버퍼를 사용한다. 기존 `.bin` 에셋과 exporter 형식은 변경하지 않았다.

근거: `Client/WarOfDimension/Object.cpp`, `Mesh.cpp`, `MeshContent.cpp`. [인터랙티브 구조도](../diagrams/mesh-sharing/mesh-sharing.architecture.html), [JSON 원본과 검증 기록](../diagrams/mesh-sharing/README.md).

## 판별 기준과 소유권

캐시 키는 `(ID3D12Device 주소, SHA-256, geometry 직렬화 바이트 수)`다. 해시는 레코드의 정점 수, bounds, positions, UV, normal, tangent, color, subset/index 등에서 계산하며 메시 이름만 제외한다. 프레임 이름·transform·material은 원래 geometry 레코드 밖에 있다. 바이트가 다른 데이터는 별도 geometry가 되며 부동소수점 오차나 다른 직렬화 순서를 근사적으로 합치지 않는다.

`ReadMeshContentRecord`는 64KiB 단위로 내용을 해시하고 레코드 끝에 파일 위치를 남긴다. 캐시 hit는 그 위치에서 후속 프레임 파싱을 계속한다. miss는 시작 위치로 돌아가 기존 GPU 로더를 한 번 실행하고 끝 위치가 일치하는지 확인한다. 잘린 입력·음수 개수·정점 속성 개수 불일치·알 수 없는 태그를 거부한다. 최초 생성 시 해시와 실제 데이터 로딩으로 두 번 읽는 비용이 있다.

캐시는 `weak_ptr`만 보관한다. 정적 프레임은 `SetSharedMesh`의 공유 핸들로 geometry를 유지한다. 기존 intrusive 참조와 함께 동작하도록 `CMesh` 참조 수를 atomic으로 변경했다. 마지막 객체·핸들이 사라지면 geometry를 해제하며 만료된 캐시 항목은 주기적으로 정리한다. `SetMesh`를 통해 공유 핸들을 버리지 말고 캐시 geometry에는 `SetSharedMesh`를 사용한다.

캐시 탐색과 최초 생성은 mutex로 보호해 동시 요청이 같은 geometry를 중복 생성하지 않는다. 이 잠금은 렌더링이나 GPU 실행 동기화를 대신하지 않는다. 호출자는 업로드 command list를 실행하고 fence 완료를 확인한 후 렌더링에 사용하거나 `ReleaseUploadBuffers`를 호출해야 한다. 현재 모델 초기화의 실행 순서를 유지한다. 같은 device의 서로 다른 queue/list에서 사용하는 새 로더를 추가할 때도 업로드 완료를 동기화해야 한다. 공유 배열·버퍼는 생성 이후 읽기 전용으로 취급한다.

## 개별 객체에 남는 상태

| 종류 | 공유 범위 | 각 객체가 유지하는 상태 |
|---|---|---|
| 파일에서 읽은 정적 메시 | 동일 `CStandardMesh`, 정점·속성·subset/index 버퍼 | 프레임 이름, transform, material |
| 스킨드 메시 | base geometry의 정점·속성·subset/index 버퍼 | `CSkinnedMesh` wrapper, 메시 이름, bone index/weight 버퍼, bind-pose 상수 버퍼, 본 프레임 연결, animation controller 상태 |
| UI·파티클·절차적 메시 | 이 캐시에 등록하지 않음 | 기존 가변 UV·클릭 상태·시뮬레이션 데이터 |

스킨드 wrapper의 base 포인터는 공유 geometry를 가리키는 비소유 참조다. wrapper 내부 공유 핸들이 소유자를 유지하며 소멸 시 base 배열과 버퍼를 이중 해제하지 않는다. 본 관련 자원은 wrapper가 개별 해제한다. 공유 upload buffer의 반복 해제는 안전하게 처리한다. 이번 변경은 기존 애니메이션 계층 전체를 인스턴스별로 복제하는 작업을 포함하지 않는다.

## 검증 결과

2026-10-04, x64 Debug·Release에서 통합 솔루션 빌드, 로컬 서버 listener·서버 간 연결·클라이언트 창 시작을 확인했다. 기존 narrowing·signedness 등 경고는 남아 있다. `Test-AgentEnvironment.ps1`의 222개 C++ 소스 경로·Serena 3개 핵심 파일 심볼 조회·Archify doctor도 통과했다.

`MeshSharingTests.cpp`의 전용 진입점은 실제 D3D12 WARP·하드웨어 자원으로 다음 12개 검사를 통과했다: 이름만 다른 geometry, GPU 버퍼 동일성, cache hit 후 파일 위치, 변경된 position/UV/index/bounds 구분, device 격리, 동시 최초 로딩, transform 독립, 본 연결·bind-pose 독립, 잘린/음수 입력, upload 반복 해제, 마지막 소유자 해제, 만료 캐시 재생성. 종료 시 live geometry는 0개다.

실제 모델의 base geometry를 WARP에서 로딩한 결과는 다음과 같다. 중복 레코드는 같은 공유 포인터를 사용하는지, GPU 생성 수가 고유 내용 수와 일치하는지 검사하고 command list 실행·fence 완료·자원 해제까지 확인했다.

| 에셋 | 메시 레코드 | 고유 GPU geometry 생성 | 기존 geometry 재사용 | 이름이 다른 중복 |
|---|---:|---:|---:|---:|
| `Model/LobbyScene_No.bin` | 291 | 38 | 253 | 0 |
| `Model/Plane1.bin` | 5,185 | 78 | 5,107 | 0 |
| `Model/ModularModel.bin` | 724 | 698 | 26 | 26 |

이름이 다른 실제 중복 예시는 `Chr_HandRight_Female_02`와 `Chr_HandRight_Male_02`다. 서로 다른 프레임과 본 상태를 유지하면서 base geometry를 재사용한다.

```powershell
./scripts/Build.ps1 -Configuration Debug
./scripts/Test-MeshSharing.ps1 -Configuration Debug -AuditAssets
./scripts/Build.ps1 -Configuration Release
./scripts/Test-MeshSharing.ps1 -Configuration Release -AuditAssets
```

에셋 검사는 base geometry 생성 경로를 대상으로 한다. 모든 텍스처·본·장면을 렌더링한 전체 게임 플레이 검사나 FPS·실측 VRAM 측정은 수행하지 않았다. JSON의 `serializedBytes`는 파일 데이터 크기이며 VRAM 사용량을 뜻하지 않는다. 실행 스크립트의 결과만으로 로그인·전투 전체를 검증했다고 간주하지 않는다.
