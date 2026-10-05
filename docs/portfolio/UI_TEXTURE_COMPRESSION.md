# UI BC7 압축과 사전 로딩 메모리 최적화

2026-10-05, 인게임 UI를 기존 생성 시점에 미리 로딩하면서 DDS를 BC7으로 압축했다. **같은 Release 실행 파일로 에셋만 바꿔 전후 각 3회 측정한 Private Bytes 중앙값은 1,964.52→1,756.43MiB로 208.09MiB(10.59%) 감소했다.** GPU 텍스처·복사 버퍼와 프로세스 지표는 각각 기록하고 합산하지 않는다.

[전체 측정·품질·해시 근거](evidence/ui-bc7-20261005/measurements.json), [6회 측정 CSV](evidence/ui-bc7-20261005/memory.csv), [플레이어/보스 전후 GPU 화면 32쌍](evidence/ui-bc7-20261005/gallery.html), [로딩·UV 흐름도](../diagrams/ui-bc7/README.md)를 함께 보존한다. 이전 [예상량](evidence/ui-bc7-estimate-20261005.json)은 적용 전 계산으로 유지한다.

## 문제와 적용 범위

인게임 `CTextureShader::BuildObjects`는 숨긴 승리/패배 화면과 스킬·게이지·미니맵·상점·아이템·피격/가속 효과도 모두 생성한다. 이 분기의 고유 DDS는 역할별 스킬 atlas를 합쳐 21개이며, 한 역할에서 실제 생성하는 대상은 **20종**이다. 모두 비압축 32bit RGBA 계열 단일 mip여서 같은 해상도에서도 데이터가 컸다.

21개 DDS를 `BC7_UNORM`으로 변경했다. 픽셀 크기·색 공간·원본 atlas 배치·기존 UI 크기와 효과 계산을 유지한다. 새 mip를 생성하거나 해상도를 줄이지 않았다. 같은 파일을 사용하는 Ready 스킬 아이콘도 압축 에셋을 읽지만, 본문의 메모리는 인게임 20종만 집계한다. Billboard·Title/Lobby 전용 UI·dissolve·skybox·음원·파티클 버퍼는 이번 변경 범위가 아니다. 사용자 지시대로 **8192 shadow map과 R32G32B32A32_FLOAT 위치 G-buffer를 유지**했다.

## 압축 상태로 GPU에 사전 로딩

Microsoft DirectXTex `may2026`의 SHA-256 고정 `texconv`로 개발 시 압축한다. 옵션은 `-m 1 -f BC7_UNORM -bc x -gpu 0`이다. 원본 SHA와 DDS 헤더·형식·단일 mip·payload를 검사한 후에만 적용하며, 에셋 21개를 Git LFS로 등록했다. [변환 스크립트](../../scripts/Convert-UiTextures.py), [공식 texconv 옵션](https://github.com/microsoft/DirectXTex/wiki/texconv)이 근거다.

기존 장면 로딩에서 BC7 DDS를 읽어 임시 UPLOAD를 거쳐 BC7 DEFAULT texture에 복사한다. GPU에는 압축 블록으로 상주하며 샘플링할 때 하드웨어가 해석한다. 전체 이미지를 RGBA로 풀어 상주시키지 않는다. [Microsoft BC7 설명](https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/bc7-format), [블록 압축](https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/block-compression)을 따른다. 최초 스킬 발동·피격·상점 열림·결과 표시 시 파일 로딩을 시작하는 구조로 바꾸지 않았다. 정확한 cold/warm 로딩 시간은 이번에 측정하지 않았다.

기존 UI의 임시 upload 조기 회수 누락은 별도 후보로 남겨 두었다. 이번 측정에서도 UI upload는 남아 있고, 압축 때문에 크기가 줄었다. DEFAULT/UPLOAD 40개는 GPU 완료 후 정상 종료 시 마지막 참조가 반환됨을 별도로 검사했다. 생성 시점·장면 전환 수명과 동시성 경계를 임의로 바꾸지 않는다.

## 4픽셀 정렬과 UV/atlas 보존

BC7의 최상위 크기는 4픽셀 배수여야 한다. 21개 중 11개가 원래 이 조건을 만족하지 않았다. 단순 resize 대신 **내용 픽셀은 그대로 두고 양쪽에 1픽셀 wrap gutter를 넣은 뒤 오른쪽/아래를 4픽셀 배수까지 패딩**했다. 패딩 픽셀은 원본 반대편 경계에서 가져온다. 기존 wrap sampler의 음수 UV와 bilinear 경계도 원본 내용에 맞춰 유지하기 위한 처리다. 스크립트가 패딩 전후의 내용 영역 texel이 완전히 같은지 검사한다.

DDS `reserved1`의 byte 32부터 `UIB7`, 버전 1, 원본 width/height, 내용 offsetX/offsetY의 6개 uint32를 저장한다. 표준 DirectX DDS loader는 이 예약 필드를 무시한다. 프로젝트 helper는 이미 읽은 파일 bytes에서 내용 영역을 검사하고 `CTexture`→`CUIObject`→독립 mesh 상수에 scale/offset을 전달한다. 별도 파일 I/O나 내용 이미지의 CPU 디코딩은 추가하지 않는다.

`UI.hlsl`은 기존 atlas·progress 계산이 끝난 논리 UV에만 `frac(uv) × scale + offset`을 적용한다. 보정 없는 텍스처는 기존 좌표를 그대로 사용한다. Speed의 GaussianBlur도 각 샘플 좌표에 같은 보정을 적용하고, 다른 blur 호출의 기본값은 identity로 유지한다. 단일 mip 계약을 검사하여 `frac`의 경계가 새 mip 선택에 영향을 주는 경로를 만들지 않는다. UI CB의 float4는 C++/HLSL 모두 byte 32에 위치하고 `static_assert`로 확인했다. CB 예약은 기존과 같이 256byte 정렬된 한 영역이다.

관련 구현: [내용 영역 계약](../../Client/WarOfDimension/UiTextureLayout.h), [DDS helper](../../Client/WarOfDimension/stdafx.cpp), [texture/객체 전달](../../Client/WarOfDimension/Object.cpp), [mesh 상수](../../Client/WarOfDimension/Mesh.h), [UI 샘플링](../../Client/WarOfDimension/UI.hlsl), [공통 좌표/blur](../../Client/WarOfDimension/Common.hlsl). 표시 영역·버전·형식 오류와 upload 생성 실패 시 오류를 전달하며 준비된 texture는 RAII로 반환한다. 일반 DDS에는 identity를 적용한다.

## 예상 payload와 실제 GPU 할당량

`GetResourceAllocationInfo`로 실제 생성한 고유 texture와 남은 upload를 집계했다. 파일/DDS texel payload와 GPU heap·row pitch 정렬은 다르다.

| 지표 | 변경 전 | 변경 후 | 감소 |
|---|---:|---:|---:|
| 플레이어 20종 DDS payload | 142.29MiB | 35.68MiB | 106.61MiB |
| 플레이어 DEFAULT 실제 할당량 | 147.6250MiB | 41.9375MiB | **105.6875MiB** |
| 플레이어 남은 UPLOAD 실제 할당량 | 143.4375MiB | 36.8750MiB | 106.5625MiB |
| 보스 20종 DDS payload | 138.02MiB | 34.61MiB | 103.41MiB |
| 보스 DEFAULT 실제 할당량 | 143.4375MiB | 40.5625MiB | **102.8750MiB** |
| 보스 남은 UPLOAD 실제 할당량 | 139.1250MiB | 35.8125MiB | 103.3125MiB |

역할별 payload는 약 75% 감소했다. 이전 예상 35.62/34.55MiB와 달리 bilinear wrap gutter를 추가한 실제 결과는 35.68/34.61MiB다. 작은 texture의 최소 할당 단위와 정렬 때문에 DEFAULT 실제 할당량은 payload보다 크다. 두 역할의 스킬 atlas를 한 실행의 메모리에 더하지 않는다. 업로드와 압축의 비용은 겹치므로 추후 upload 조기 회수 성과도 별도로 실측한다.

## 전체 클라이언트 전후 실측

Windows x64 Release, RTX 4070 SUPER, 선택 외형·직업 `[0,1,2,4]`·스킬 fixture와 공용 파티클 합성 재생을 유지했다. **전후 동일 EXE SHA-256 `e8c0949a0c22a9cb0824bc770592b7dd31329891051f1fb4b0bb891d54c3f462`**다. 원본 에셋으로 3회 후 BC7으로 3회 순차 실행했고, 마지막 `ingame_ready`를 비교했다. 실제 네트워크 전투 최대 사용량이나 cold/warm 로딩 시간의 비교가 아니다.

| 지표, 각 3회 중앙값 | 변경 전 | 변경 후 | 감소 |
|---|---:|---:|---:|
| Process Private Bytes | 1,964.52MiB | **1,756.43MiB** | **208.09MiB, 10.59%** |
| Process Working Set | 897.17MiB | 790.44MiB | 106.73MiB |
| DXGI LOCAL usage | 1,190.4063MiB | 1,084.7188MiB | 105.6875MiB |
| DXGI NON_LOCAL usage | 244.22MiB | 137.03MiB | 107.19MiB |

Private 범위는 전 1,964.29~1,968.03MiB, 후 1,753.79~1,759.82MiB다. 양쪽 모두 파티클 19쌍·재사용 30회, 확정 영웅 행렬과 선택 스킬 fixture가 같음을 검사했다. LOCAL 감소량은 UI DEFAULT 실제 할당 감소량과 일치한다. 전체 프로세스 지표는 CPU allocator·driver·upload의 영향도 받으므로 **Private·WS·LOCAL·NON_LOCAL을 합산하거나 한 지표를 다른 지표의 소유량으로 해석하지 않는다.** 보스 전체 메모리는 반복 3회 측정하지 않았으며 역할별 texture 할당 검사와 구분한다.

## 화질·실제 화면 비교

실제 인게임에서 생성한 `CTextureShader`의 객체·재질·PSO를 고정 시간 1초에 렌더링했다. 플레이어/보스 각 16장씩 전후 **64개 GPU backbuffer 원본 PNG, 32쌍**을 보존했다. HUD·상점·승리/패배·피격 효과·Speed 4프레임·작은 icon/button·아이템 11칸·플레이어 48/보스 20 스킬 칸·게이지 0/25/50/75/100%를 비교한다. 효과·아이콘의 어두운/밝은 배경도 포함한다. 맵·DirectWrite 동적 텍스트·실제 네트워크 플레이를 제외한 UI 전용 비교다.

| 원본 상점 UI | BC7 상점 UI |
|---|---|
| ![원본 상점](evidence/ui-bc7-20261005/before/player/shop.png) | ![BC7 상점](evidence/ui-bc7-20261005/after/player/shop.png) |

실제 화면을 직접 대조했고 큰 글자/파츠 누락이나 atlas·progress 경계의 배치 변화는 보이지 않았다. **손실 압축이므로 픽셀과 투명도가 완전히 같지는 않다.** Pillow 12.3.0·NumPy 2.3.5로 내용 영역 RGBA와 두 배경 합성 오차, 실제 화면 RGB/foreground 오차를 기록했다. DDS 최소 채널 PSNR은 37.08dB, 실제 화면 foreground 최소는 41.34dB다. 최대 화면 오차는 255단계 중 87이며 Victory의 밝기 조건/Twinkle처럼 shader가 결과를 변형하므로 DDS와 화면 오차를 구분한다. 최대 오차의 국소 영역과 아이템/글자/투명 경계도 확대해서 확인했다. PSNR 하나로 모든 움직임·해상도에서 같은 품질을 보장하지 않는다.

## 실제 검증과 한계

- Client Debug/Release 빌드 PASS, compilation database 235개 source.
- 두 역할의 기존 사전 생성 고유 texture 20종, 실제 DEFAULT/UPLOAD allocation과 texture 복사 참조/UV 보존 검사 PASS.
- 잘못된 버전·빈 내용 크기·offset overflow·다중 mip·잘못된 형식·array·gutter 누락의 내용 영역 거부 9개 native 검사 PASS.
- 전후 Release 두 역할 및 적용 후 Debug 두 역할의 16화면 렌더링과 GPU 완료 후 `OnDestroy` PASS. UI DEFAULT/UPLOAD 40개 검사 참조까지 반환 후 resource 잔류 0.
- 8192 shadow map과 FP32 위치 RT의 실제 resource 형식을 검사하고 변경 전 source/hash와 대조했다.
- Release 메시 공유 12범주/실제 모델 3종, DDS 공유 Debug/Release 검사 PASS. 로컬 서버 연결·클라이언트 창 기동 PASS.
- 에이전트 환경 235개 source와 Serena UI capture 9개 symbol 색인, Archify showcase 9/9·4해상도 containment 및 작은 light/큰 dark 화면 직접 점검 PASS.

Debug UI 초기화의 기존 `CREATERESOURCE_STATE_IGNORED` ID 1328 경고는 플레이어 1,511개·보스 1,435개다. 비교 grid 객체 생성 이후 같은 경고가 플레이어 78개·보스 50개 남으며 종료 진단에도 누적되어 기록된다. 오류·device removed는 0이다. DDS 회귀의 기존 ID 1328 경고는 구성별 16개이며 기존 compiler 경고도 유지한다. 경고를 숨긴 무경고 전체 실행으로 주장하지 않는다.

정상 종료의 이번 UI 자원 반환만 검사했으며 클라이언트 전체 누수 0을 뜻하지 않는다. 반복 매치 재진입·장시간 네트워크 전투·모든 해상도/DPI·모든 시간의 Twinkle·실제 OOM·운영 DB/블록체인은 미검증이다. 원본 저장소와 사용자의 SLNX 수정은 보존했고 커밋·push는 하지 않았다.

## 재현

```powershell
./scripts/Build.ps1 -Configuration Release -Module Client
./scripts/Build.ps1 -Configuration Debug -Module Client
./scripts/Capture-UiTextures.ps1 -Configuration Release -Role player -OutputDirectory artifacts/logs/ui-recheck/player
./scripts/Capture-UiTextures.ps1 -Configuration Release -Role boss -OutputDirectory artifacts/logs/ui-recheck/boss
./scripts/Capture-UiTextures.ps1 -Configuration Debug -Role player -OutputDirectory artifacts/logs/ui-recheck-debug/player
./scripts/Measure-ClientMemory.ps1 -Configuration Release -Scenario ParticleReuse -Modes pooled -Runs 3 -OutputDirectory artifacts/logs/ui-recheck-memory
./scripts/Test-MeshSharing.ps1 -Configuration Release -AuditAssets
./scripts/Test-DdsSharing.ps1 -Configuration Debug
./scripts/Test-DdsSharing.ps1 -Configuration Release
./scripts/Test-Local.ps1 -Configuration Release
python ./scripts/Build-UiTextureEvidence.py
```

변환은 원본 RGBA 에셋을 가진 별도 `--source`를 `Convert-UiTextures.py`에 전달한다. 현재 압축 파일을 입력하면 원본 해시 검사로 거부한다. 고정 도구는 `artifacts/tools/directxtex-may2026/texconv.exe`에 준비하고 `--apply` 없이 출력과 품질을 검토할 수 있다. 적용 전 원본 DDS/EXE/source는 `artifacts/logs/ui-bc7-before`에, 실제 전후 실행 자료는 `ui-memory-before-final|after`와 `ui-capture-before-final|after|debug`에 보존했다. 로컬 원본·실행 파일·로그는 Git에서 제외한다. 공개 근거에는 byte 계산·원본/압축 DDS·현재 source·EXE·PNG SHA와 실제 checkpoint를 담았다.
