# War Of Dimension 리팩토링 검토

분석일: 2026-10-04. 기준: GitLab `hhs4/war_of_dimension`의 `main`, 커밋 `78ac46327904995c120fdaa66a6310e5c88de32a`.

현재 단계는 로컬 이전 준비와 코드 분석이다. 사용자의 요청에 따라 GitHub 저장소 생성과 업로드는 보류했다. 게임 동작을 바꾸는 리팩토링은 아직 적용하지 않았다.

후속 개발 환경 정비에서 TBB 복원, 누락 HeightMesh 복원, VS2026 빌드·실행과 에이전트 설정을 진행했다. 아래 빌드 실패와 파일 해시 대조는 초기 분석 시점의 기록이며, 현재 결과는 [개발 환경](development/SETUP.md)과 [작업 목록](../tasks/todo.md)을 참조한다. 패킷·생명주기 등 코드 리팩토링 제안은 후속 작업으로 유지한다.

## 프로젝트 구성

| 모듈 | 역할과 근거 | 모듈 최상위 C++ 소스·헤더 수 |
|---|---|---:|
| `Client/WarOfDimension` | DirectX 12 렌더링, UI, 네트워크, FMOD 오디오 | 112 |
| `Server/Game_Server` | Winsock/IOCP, 게임 상태, NPC, 스킬, 매칭 | 290 |
| `Server/Lobby_Server` | 접속·로비, ODBC 데이터베이스, 거래·블록체인 | 63 |

`Client/WOD`, `Client/Deferred`, `Client/TestClient`, `Server/Stress_Test`, `Server/blockchainTest`도 존재한다. 운영 모듈과 실험 모듈의 경계는 실행·사용 여부를 확인한 뒤 정리하는 편이 좋다. 위 파일 수는 각 폴더 바로 아래의 `.cpp`와 `.h`만 센 값이며, 하위 외부 라이브러리는 제외했다.

## 권장 순서

| 순서 | 작업 | 이유 | 완료 기준 |
|---|---|---|---|
| 1 | 빌드·실행 기준선 재현 | 현재 복사본의 Game Server 빌드가 NuGet 패키지 누락으로 중단된다. 이후 변경의 회귀를 판단할 기준이 필요하다. | 새 checkout에서 의존성 복원, 핵심 모듈 Debug/Release x64 빌드, 실행 디렉터리와 데이터·DB 준비 방법 문서화 |
| 2 | TCP 패킷 누적·검증 로직 정리 | 분할 수신 잔여 데이터 처리의 구체적인 결함과 패킷 크기 검증 부재가 보인다. | 분할 헤더·분할 본문·다중 패킷·잘못된 길이에 대한 테스트와 수정, 기존 wire format 유지 |
| 3 | 서버 생명주기·소유권 정리 | Game Server 진입점이 초기화 결과를 무시하고, 명시적으로 할당한 서버 객체를 해제하지 않는다. | 초기화 실패 시 Run 진입 방지, 스코프 기반 소유권, 부분 초기화 실패와 정상 종료의 자원 정리 검증 |
| 4 | 공통 프로토콜·네트워크 기반 분리 | 클라이언트와 Lobby Server가 Game Server 내부 헤더를 상대 경로로 참조한다. 동일한 주소 래퍼도 중복된다. | Shared 모듈에서 프로토콜과 공통 주소 타입 제공, 기존 패킷 크기·필드 배치 검증, 전체 모듈 빌드 |
| 5 | 설정·데이터 경로 외부화 | 서버 IP, 포트, 데이터 내보내기 경로가 코드 또는 개발자 절대 경로에 묶여 있다. | 로컬 설정으로 실행·데이터 생성 가능, 필수 설정 누락 시 명확한 오류 |
| 6 | 대형 클래스의 책임 분리 | 렌더링·게임 상태·UI·패킷 처리의 수정 범위가 넓다. | 역할별로 작은 변경을 나누고 대표 게임 흐름의 회귀 확인 |

## 구체적인 발견

### 1. 빌드 기준선

`Server/Game_Server/packages.config`는 `tbb`와 `tbb.redist` 버전 `4.2.3.1`을 요구한다. `Game_Server.vcxproj:456` 이하에서 해당 패키지의 targets를 가져오며, 누락 시 빌드를 중단한다.

실행한 확인:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' `
    Server\Game_Server\Game_Server.vcxproj /t:Build `
    /p:Configuration=Debug /p:Platform=x64 /nologo /v:minimal
```

결과: `packages\tbb.redist.4.2.3.1\build\native\tbb.redist.targets` 누락으로 실패했다. C++ 컴파일·링크와 런타임 동작은 검증되지 않았다. 핵심 프로젝트의 toolset은 `v143`, 언어 표준은 x64에서 C++20이다. 현재 설치된 Visual Studio 환경과의 호환 여부, FMOD·ODBC·외부 헤더 경로, 실행 시 리소스 위치를 함께 확인해야 한다. 빌드 문제를 숨기기 위해 패키지 검사나 버전을 임의로 변경하지 않았다.

### 2. 패킷 수신 안정성

- `Server/Game_Server/CNetworkMgr.cpp:251`의 조건이 `remaindata < 0`이다. 정상적으로 미완성 패킷이 남았을 때는 양수이므로, 잔여 데이터를 버퍼 앞으로 옮기지 않는다. 완성 패킷 다음에 미완성 패킷이 함께 도착한 경우 다음 수신에서 잔여 데이터가 잘못 이어질 수 있다. Lobby Server의 같은 역할 코드는 `CNetworkMgr.cpp:212`에서 `remaindata > 0`을 사용한다.
- `Game_Server/CNetworkMgr.cpp:240`, `Lobby_Server/CNetworkMgr.cpp:201`은 남은 데이터가 양수이면 바로 `BASE_PACKET`으로 해석한다. 기본 헤더 길이, 최소 패킷 길이, 타입별 길이 검증이 이 루프에 없다.
- `Game_Server/protocol.h:289`의 `BASE_PACKET::size`는 `unsigned char`이다. 길이 0을 허용하면 루프에서 포인터와 남은 길이가 줄지 않는다. 타입별 처리 전에 잘못된 길이를 차단해야 한다.
- 클라이언트 세션 수신도 `Game_Server/CClient.cpp:262`, `Lobby_Server/CClient.cpp:46`에 별도 구현되어 있다. 수신 버퍼 누적과 프레임 검증을 재사용 가능한 작은 모듈로 추출하고, 게임별 dispatch는 분리하는 방향이 적절하다.

위 내용은 코드에서 확인한 결함·위험이며 실행으로 재현한 결과는 아니다. 먼저 현재 구현을 재현하는 테스트를 만들고, wire format을 유지하며 수정할 것을 제안한다.

### 3. 서버 생명주기

`Server/Game_Server/main.cpp:6`에서 `new CServer()`를 사용하고 `Initialize()`의 bool 결과를 확인하지 않은 채 `Run()`으로 진행한다. 마지막에 `Release()`는 호출하지만 서버 객체에 대한 `delete`는 없다. Lobby Server의 `main.cpp`는 스택 객체와 초기화 실패 분기를 이미 사용한다.

`Game_Server/CServer.cpp:8`의 초기화는 리소스 로딩과 singleton 초기화를 순차적으로 수행하며 중간 실패 시 즉시 반환한다. `Release():136`도 중간 실패 시 반환한다. 어떤 자원까지 준비되었는지와 각 해제 함수의 반복 호출 안전성을 확인한 뒤 RAII와 역순 정리를 도입해야 한다.

### 4. 공통 모듈 경계

- `Client/WarOfDimension/stdafx.h:44` → `../../Server/Game_Server/protocol.h`.
- `Server/Lobby_Server/pch.h:28` → `../Game_Server/protocol.h`; 같은 파일 40~41행은 `Interface.h`, `MathUtil.h`를 참조한다.
- 두 서버의 `SockAddr.cpp`, `SockAddr.h`는 파일 해시가 동일하다. `Job.cpp`, `JobQueue.cpp`도 동일하지만 관련 헤더는 달라서 구현을 바로 합치는 것은 적절하지 않다.
- `TCPSocket`, `SocketUtil`, 세션·패킷 처리의 파일명은 같아도 코드가 다르다. 공통 기능과 게임·로비 전용 처리를 구분한 뒤 추출해야 한다.

프로토콜은 가장 먼저 경계를 잡기 좋은 대상이다. 패킷 상수, 크기, packing을 그대로 유지하고 양쪽 서버·클라이언트가 같은 헤더를 사용하도록 만든다.

### 5. 설정·데이터·대형 클래스

`Lobby_Server/pch.h:59`에는 Game Server 주소가 하드코딩되어 있고, 포트는 `Game_Server/protocol.h:24`에 정의되어 있다. `Data/paths.txt`는 특정 개발자의 `D:/Desktop/war_of_dimension` 경로를 사용한다. 로컬 개발 설정, 런타임 설정, 데이터 생성 경로를 구분하고 상대 경로 또는 명시적인 설정으로 제공해야 한다.

클라이언트의 `Shader.cpp`는 약 210KB, `Object.cpp`는 약 145KB, `GameFramework.cpp`와 `Scene.cpp`는 각각 약 91KB, `NetworkManager.cpp`는 약 79KB이다. 크기만으로 설계 결함을 단정할 수는 없지만 변경 책임이 모이는 지점이다. 네트워크 수신·dispatch, 장면 전환, UI 반응부터 경계를 확인하고 분리하는 것이 좋다. 서버에서는 `CGameMgr`, `CPacketMgr`, `GameUtil`의 책임을 같은 방식으로 검토한다.

## 첫 작업 제안

첫 변경은 **의존성 복원과 빌드 절차 문서화**로 제한한다. 기준선이 확보되면 **패킷 프레이밍 테스트와 잔여 데이터 처리 수정**을 다음 변경으로 진행한다. 그 이후 서버 생명주기와 Shared 프로토콜을 각각 독립된 변경으로 다룬다. 전체 폴더 이동이나 파일명 변경을 동시에 진행하지 않는다.

## 현재 검증 범위

코드·프로젝트·패키지 선언 분석, 원본 최신 커밋 대조, Git 대용량 객체 목록 확인, 로컬 파일 이전과 LFS 구성 확인을 수행했다. Game Server 빌드는 위 패키지 문제로 실패했다. 게임 실행, DB 연동, 멀티플레이·렌더링·성능 측정은 수행하지 않았다.
