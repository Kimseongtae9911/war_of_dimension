# 파일 정리 기록

2026-10-04 실행 기준선과 VS2026 이전을 확인한 뒤 정리했다. 원본 `C:\GitFolder\war_of_dimension` 및 GitLab 이력을 복구 근거로 보존한다.

## 제거한 범위

| 범위 | 근거 |
|---|---|
| `.idea/`의 개인 IDE 파일, 빈 `Server/blockchainTest/private_key.pem` | 프로젝트·빌드·런타임에 사용하지 않는 로컬 설정과 빈 생성 대상 |
| `Client/WarOfDimension/WarOfDimension.exe`, `Server/Game_Server.exe`, `Server/Lobby_Server.exe` | 원본에 포함된 기존 실행 파일. 현재 소스로 Debug/Release 실행 파일을 artifacts에 재생성 |
| `Client/WarOfDimension/Model/Textures/Textures.zip` | 파일 6개가 현재 텍스처 폴더 또는 상위 Model 폴더에 존재하고 SHA-256이 모두 일치. FreeLichPBR.bin은 실제 런타임 Model 폴더에 보존 |
| `Client/WarOfDimension/Image/GUI/Customize/liftUp.zip` | 해제된 파일과 SHA-256 일치 |
| `Server/Game_Server/Resource.zip` | 실행 코드가 압축 파일을 읽지 않음. 파일 22개가 리소스 폴더에 존재하며 15개 동일, 7개는 기존 최신 tracked 리소스와 다름. 누락 HeightMesh는 복원한 뒤 보존하고, 기존 최신 리소스를 과거 백업으로 덮어쓰지 않음 |
| `Client/WarOfDimension/fmodex.dll`, `fmodex64.dll`, `fmodexL.dll`, `fmodex_vc.lib`, `fmod_memoryinfo.h` | 이전 FMOD Ex 세대의 미사용 파일. 전체 소스·프로젝트 참조 검색에 사용처가 없고, 실제 클라이언트는 fmod/fmodL에 링크·로딩 |
| `Server/Game_Server/Lib/libboost_*-vc143-*.lib` 6개 | 실제 소스는 Boost.PFR의 header-only reflection만 사용. 해당 바이너리 이름의 프로젝트·빌드 참조가 없으며 현재 링크에서 사용하지 않음 |
| `Server/Game_Server/packages.config`, TBB import·복원 검사와 packages 캐시 | 원본 빌드 재현 단계에서 복원했으나 전체 소스에 TBB API·include·HAS_TBB 사용이 없고, v145 링크·런타임에 TBB 의존성 없음. 실제 사용 중인 Microsoft PPL concurrent container와는 별개 |
| 최초 기준선 빌드가 모듈 폴더에 만든 `x64/`, 중첩 `Game_Server/`, `Lobby_Server/` 산출물 | tracked 소스가 없고 artifacts 경로로 모두 재생성. `.obj` 모델 리소스는 이 범위에 포함하지 않음 |

정리 대상의 원본 경로는 NewWod 내부로 제한한다. 일괄 재귀 삭제가 자동 승인 검토에서 `blocked by policy`로 거부되어, 바이너리·개인 설정·산출물은 저장소 밖 `C:\GitFolder\NewWodCleanupBackup\20261004`로 옮겨 복구 가능하게 보관한다. 정리된 tracked 파일은 초기 커밋 후보에서도 제외한다. 최초 TBB 복원용 스크립트 대신 설치 도구·필수 리소스를 점검하는 `Check-Prerequisites.ps1`을 사용한다. 현재 빌드는 오래된 TBB NuGet 패키지를 다운로드할 필요가 없다.

## 보존한 범위

- 실행 모델·텍스처·UI 이미지·음원·CSV·NavMesh·HeightMesh, FMOD 최신 세대 DLL·import library·헤더.
- Boost·RapidJSON 헤더. 현재 dependency closure만으로 vendor 배포를 임의 축소하지 않는다. 에이전트 index에서는 vendor 폴더를 제외한다.
- `Client/Deferred`, `Client/WOD`, `Client/TestClient`, `Server/Stress_Test`, `Server/blockchainTest`. 핵심 실행과 분리했지만 테스트·참고 가치가 있어 사용 여부 판단 없이 삭제하지 않는다.
- `Script/` 모델·NavMesh 내보내기 코드, `Data/` Excel 원본·생성 도구, 기획 자료.
- 위 실험 모듈은 메타데이터만 VS2026으로 맞췄으며 실행 검증 범위가 아니다.

## 정리 후 확인

핵심 3개 모듈 Debug/Release x64 재빌드와 두 configuration의 로컬 시작 점검을 통과했다. `.obj` 리소스 보존, 프로젝트 source 참조 존재, LFS pointer·실제 파일 해시, 초기 커밋 후보에서 삭제된 파일 제외 여부도 확인했다. 세부 실행 결과는 [작업 목록](../../tasks/todo.md)에 기록했다.

## ServerCore 추출에 따른 중복 소스 제거 (2026-10-07)

`Server/Game_Server`와 `Server/Lobby_Server`에서 아래 `.cpp`를 각각 제거했다.
총 12개이며 대응 헤더와 서버별 어댑터는 유지한다.

| 각 서버의 제거 파일 | 대체 구현 |
|---|---|
| `SockAddr.cpp` | `ServerCore/Net.h`·`src/Net.cpp` |
| `Job.cpp`, `JobQueue.cpp` | `ServerCore/Concurrency.h`·서버별 budget/스케줄러 |
| `LogUtil.cpp` | `ServerCore/Diagnostics.h` |
| `OverlapEx.cpp` | `ServerCore/Net.h`·서버별 `OverlapEx.h` |
| `TCPSocket.cpp` | `ServerCore/Session.h`·서버별 `TCPSocket.h` |

두 프로젝트의 compile/filter 항목을 함께 제거했다.
공통 라이브러리 참조로 대체하며 Debug/Release 링크·네트워크 회귀를 검사했다.
삭제 파일의 이름을 참조하는 프로젝트 항목이 남지 않았는지 확인한다.

복구 근거는 이전 commit `86f83a94f76e7218c4cd2f34d992f9d2d02174f1`의 같은 경로다.
이 소스 추출에서 런타임 에셋·외부 라이브러리·원본 저장소는 삭제하거나 수정하지 않았다.
[구현·검증 기록](../architecture/SERVER_CORE.md)을 참조한다.


## 잔여 SocketUtil 구현 제거 (2026-10-10)

Game_Server·Lobby_Server의 `SocketUtil.cpp` 2개를 제거하고 프로젝트/filter 항목도 함께 제거했다.
두 파일의 동일한 PrintError는 `ServerCore/src/Net.cpp`의 `TransportHost::PrintError`로 대체했다.
기존 Startup·Cleanup·Runtime·GetLastError도 Core가 제공하며 각 `SocketUtil.h`는 별칭만 남긴다.
게임 서버의 listener용 세션 풀은 `Resource::m_acceptSessionPool`로 옮겨 사용처를 모두 갱신했다.

복구 근거는 HEAD `c80506cbbe2bd797300194d911521b1f176cc589`의 같은 경로와
작업 전 staged 이름 변경을 포함한 Git 제외 `artifacts/server-core-extraction-baseline/*-SocketUtil.cpp`다.
기존 staged 변경을 취소하거나 덮어쓰지 않았고 런타임 에셋·원본 저장소는 수정하지 않았다.
이번 결과는 [구현 문서](../architecture/SERVER_CORE.md)와 작업 목록에 기록한다.


## Resource 구현 공통화 (2026-10-10)

Game_Server·Lobby_Server의 Resource.cpp 2개를 제거하고 두 프로젝트/filter의 compile 항목도 제거했다.
I/O 객체 풀·획득 함수와 세션/소켓 재사용 풀 구현은 ServerCore/include/ServerCore/Resource.h로 대체했다.
서버별 Resource.h와 기존 타입 이름은 별칭으로 유지하고 사용처의 Resource:: 접근은 보존했다.
static 풀은 구성 템플릿의 inline static으로 정의한다. native 소켓 소유와 종료 배출 순서는 유지했다.

복구 근거는 HEAD의 같은 경로 및 이번 작업 시작 시 저장한 Git 제외
artifacts/resource-refactor/baseline/{Game_Server,Lobby_Server}-Resource.{h,cpp}다.
실행 에셋·외부 라이브러리·원본 저장소는 삭제하거나 수정하지 않았다.
구현과 검증 결과는 [ServerCore 구현 문서](../architecture/SERVER_CORE.md)와 작업 목록에 기록한다.

## 별칭 전용 네트워크 헤더 제거 (2026-10-10)

Game_Server·Lobby_Server의 SockAddr.h·SocketUtil.h 총 4개를 제거했다.
사용처 include와 두 프로젝트/filters의 ClInclude 등록도 함께 제거했다.
각 서버 pch.h의 `using wod::core::SockAddr;`·`using wod::core::NetworkRuntime;`으로 대체한다.
옛 SocketUtil 이름은 유지하지 않고 호출부를 NetworkRuntime::Start/Stop/Get으로 변경했다.
Core의 NetworkRuntime 선언은 Net.h에 있으며 소켓·IOCP 소유와 종료 순서는 동일하다.

삭제 직전 헤더는 Git 제외 artifacts/runtime-cleanup/baseline/Server/{Game_Server,Lobby_Server}/에 보존했다.
Git HEAD의 같은 경로도 복구 근거다. 외부 코드·실행 에셋·원본 저장소는 수정하지 않았다.
앞선 SocketUtil.cpp 정리 항목의 TransportHost·호환 helper는 당시 상태를 설명하는 기록이며,
이번 후속 정리에서는 새 이름으로 통일하고 사용처 없는 helper를 제거했다.

## 작업·로그·자원 별칭 헤더 통합 (2026-10-10)

Game_Server·Lobby_Server의 Job.h·JobQueue.h·LogUtil.h·OverlapEx.h·Resource.h 총 10개를 제거했다.
두 서버 자체 헤더 전체의 using 선언을 조사했으며, 별칭만 있는 파일의 선언은 pch.h로 모았다.
로비 JobQueue.h의 PacketJobQueue는 추가 동작 없는 빈 파생 클래스여서 Core JobScheduler의 별칭으로 대체했다.
Global.h의 class 전방 선언과 프로젝트/filters 등록·소스 include를 함께 정리했다.
Job/JobQueue budget·작업 처리 경로·context 매핑·풀 구성과 소켓/세션 소유권은 유지한다.

삭제 직전 파일은 Git 제외 artifacts/alias-cleanup/baseline/Server/{Game_Server,Lobby_Server}/에 보존했다.
Git HEAD의 같은 경로도 복구 근거다. 이전 정리 항목의 Resource.h 등은 당시 상태를 설명한다.
MathUtil·CsvLoader·CNetworkMgr 등의 함수/클래스와 함께 있는 별칭, TCPSocket 정책 어댑터와
기존 Protocol 경로의 forwarding header는 유지했다. 외부 코드·실행 에셋·원본 저장소는 수정하지 않았다.

## GameObject 공통 기반 추출 (2026-10-10)

Game_Server/GameObject.h의 CGameObject·CMoveObject 기반과 로비 Transform/Physic의 겹치는 상태를
ServerCore/include/ServerCore/GameObject.h로 옮겼다. 이번에는 파일을 삭제하지 않았다.
게임 GameObject.h/cpp에는 타입/충돌 정책·타워/넥서스 콘텐츠가 남고 로비에는 이동/시야 규칙이 남는다.
삭제된 기존 클래스 본문·중복 멤버/접근 함수는 Core 템플릿과 상속으로 대체한다.
GameObject.cpp의 DirectX 충돌 변환 본문은 정책 함수로 옮기고 문장 토큰 동일성을 확인한다.
작업 직전 파일은 Git 제외 artifacts/game-object-extraction/baseline에 보존했다.
기존 Git HEAD도 복구 근거이며 외부 코드·에셋·원본 저장소는 수정하지 않았다.

## 세션 처리 기반 명명 정리 (2026-10-10)

ServerCore/include/ServerCore/Client.h를 SessionHandler.h로 옮기고 BasicClient를 SessionHandler로 변경했다.
클라이언트 상태 없이 세션의 수신 프레임 검증·전달과 연결 종료를 조율하는 역할을 이름에 반영했다.
두 서버 CClient·기존 검사·프로젝트/filters·현재 구현 문서의 참조를 갱신했다.
옛 이름의 별칭/forwarding header는 남기지 않았으며 처리 순서·가상 확장점·소유권은 유지했다.

이전 Client.h는 이번 대화에서 추가한 미커밋 파일이므로 복구 근거는 Git 제외
artifacts/session-handler-rename/baseline/Server/ServerCore/include/ServerCore/Client.h다.
동일 baseline에 변경 직전 사용처도 보존했다. 외부 코드·에셋·원본 저장소는 수정하지 않았다.
