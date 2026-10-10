# ServerCore 공통 기반 구현

서버 리팩토링 1단계와 서버에 남은 공통 기반의 추가 추출을 적용했다.
로비와 게임 서버가 `ServerCore` 정적 라이브러리를 함께 사용한다.
두 서버는 별도 프로세스이며, 소스와 구현을 공유한다.

[구현 구조도](../diagrams/server-core/README.md),
[소스·검증 근거](evidence/server-core-implementation-20261007.json)를 함께 관리한다.
[이전 계획](SERVER_CORE_PLAN.md)은 구현 전 조사 기록이다.

## 결과물과 범위

```text
Server/ServerCore/
├─ ServerCore.vcxproj                 공통 정적 라이브러리
├─ ServerCore.vcxproj.filters         소스·헤더·개발 설정 표시
├─ include/ServerCore/
│  ├─ Net.h                          NetworkRuntime·소켓·IOCP·프레임·종료 신호
│  ├─ Session.h                      세션·listener·풀 주입·세대 검사
│  ├─ SessionHandler.h               공통 수신·disconnect 및 가상 확장점
│  ├─ GameObject.h                   객체 기반·공통 위치/방향/속도 상태·충돌 정책 주입
│  ├─ Resource.h                     공통 I/O 자원·세션/소켓 재사용 구성
│  ├─ Concurrency.h                  Job·작업 큐·스케줄러·객체 풀·스레드 그룹
│  └─ Diagnostics.h                  로그·프로세스 오류 진단
├─ src/Net.cpp                      공통 네트워크 구현
└─ tests/                            별도 ServerCore.Tests 프로젝트·filters
Shared/Protocol/
├─ protocol.h                       기존 wire 선언
├─ Validation.h                     방향별 크기·필드 검사
└─ tests/abi-x64-msvc.json           분리 전 ABI 기준
scripts/
├─ Test-ServerCore.ps1               단위·ABI 검사
├─ Test-ServerCoreIntegration.ps1    정상·초기화 실패 회귀
└─ Test-ServerCoreNetwork.py         네트워크 회귀용 검사
```

`NewWod.slnx`와 서버 filter에 Core 및 테스트 프로젝트를 추가했다.
기존 세 실행 프로젝트의 경로를 보존했다.
Core는 x64 Debug/Release, C++20, v145로 빌드한다.
런타임은 서버와 동일한 `/MDd`·`/MD`를 사용한다.

공용 헤더 7개를 `ClInclude`에 등록했다.
Core와 테스트의 `.filters`에서 소스·헤더·개발 설정을 구분한다.
테스트 소스 5개는 `ServerCore.Tests` 프로젝트에서 표시한다.

기존 Core 구현·헤더·테스트에 함수 간격·다중 행 `if`·분기/`while` 뒤 빈 줄·마지막 `return` 분리를 적용했다.
연결된 `if`·`else if`·`else`의 본문은 한 문장이어도 중괄호로 감쌌다.
루트 `.clang-format`과 `AGENTS.md`를 포맷 기준으로 사용한다.
멤버는 `m_`, 함수 인자는 `_` 접두어로 구분한다.
기존 동작·패킷 계약·소유권은 유지했다. 이전 소스 해시는 1단계 구현 검증 당시의 기록이다.

| 영역 | 공통 구현 | 서버에 남은 책임 |
|---|---|---|
| 네트워크 | Winsock·소켓·IOCP·AcceptEx·DisconnectEx·송수신 | 접속 역할·사용자·매치·패킷 처리 |
| 프레임 | 분할/연속 수신·길이 검사·소유한 패킷 복사 | 패킷별 콘텐츠 처리 |
| 작업 | Job·실행 직렬화·budget·JobTarget·JobScheduler | 로비 사용자 정리 callback·게임 매치 큐 |
| 풀 | I/O 객체 소유·확장·임대/반환 검사·세션/소켓 재사용 | 세션 타입·풀 구성 선택·스킬 도메인 풀 |
| 객체 | GameObject·MoveObject·위치/방향/속도 상태 | 벡터/DirectX 충돌 정책·타워/넥서스·로비 이동/시야 규칙 |
| 수명 | 스레드 그룹·취소/완료 배출·종료 신호 | 종료 순서·콘텐츠 자원 정리 |
| Protocol | 공통 선언·방향별 형식 검사 | 인증·거래·게임 규칙 |

서버의 `TCPSocket`은 정책만 지정하는 얇은 어댑터다.
`SockAddr`·`NetworkRuntime`·작업·로그·I/O context·Resource 구성의 using 선언은 두 서버 `pch.h`에 모았다.
별칭 전용 헤더는 총 14개를 제거했다.
중복 `.cpp` 16개를 제거했다. [정리 근거](../development/CLEANUP.md)를 참조한다.

로비 작업 큐는 최대 5개, 게임 큐는 실행 시작 시 크기만큼 처리한다.
기존 PPL `shared_ptr` 우선순위 비교를 유지한다.
FIFO나 예약 시간순 실행을 새로 보장하지 않는다.
로비 content worker는 조건 변수로 대기하고 남은 작업을 다시 예약한다.

## 서버에 남은 공통 기반의 추가 추출 (2026-10-10)

파일이 각 프로젝트에 보이는 것과 구현이 중복되는 것은 구분한다.
공통 동작은 ServerCore에 한 번만 정의한다.
별칭 전용 `SockAddr.h`·`SocketUtil.h`·`Job.h`·`JobQueue.h`·`LogUtil.h`·`OverlapEx.h`·`Resource.h`는 후속 정리에서 제거했다.
`JobQueue.cpp`·`TCPSocket.cpp`·`SockAddr.cpp`는 1단계에서 이미 제거했다.

| 대상 | 현재 공통 구현 | 서버에 남은 내용 |
|---|---|---|
| `SockAddr.h`·`SocketUtil.h/cpp` | `Net.h`의 `SockAddr`·`NetworkRuntime` | `pch.h`의 using 선언, 별칭 헤더 4개·`.cpp` 2개 제거 |
| `TCPSocket.h` | `PooledSession`·`PooledListener`·`BasicListener::Accept(shared_ptr)` | context·Resource·LOBBY_PORT 정책과 기존 클래스 이름 |
| `OverlapEx.h` | `TaggedIoContext<Operation>`의 native 연산 매핑·cookie·세대 정보 | `pch.h`에서 서버별 `OP_TYPE`을 지정하는 별칭, 헤더 2개 제거 |
| `CClient.h/cpp` | `SessionHandler<Session>::Receive/Disconnect` | 패킷 검증·전달·서버별 상태 변경 |
| `Resource.h/cpp` | `IoResources`·`SessionPool`·`SocketResource`·`SocketPool`·자원 구성 템플릿 | `pch.h`의 자원 구성 별칭, 헤더 2개·`.cpp` 2개 제거 |
| `Job.h`·`JobQueue.h` | `IJob`·`Job`·`JobQueue`·`JobTarget`·`JobScheduler` | `pch.h`의 별칭·budget 선택, 헤더 4개 제거 |
| `LogUtil.h` | `Diagnostics.h`의 `LogPrinter` | `pch.h`의 using 선언, 헤더 2개 제거 |

`SessionHandler`는 템플릿으로 세션 타입을 주입하고 가상 함수로 콘텐츠 동작을 확장한다.
클라이언트/사용자 상태를 소유하지 않고 세션의 수신·연결 종료를 조율하므로 기존 `BasicClient`에서 이름을 변경했다.
`BasicSession`은 소켓 I/O·프레임 버퍼·세대 상태를 관리하고, `SessionHandler`는 해당 세션을 이용해 콘텐츠 처리로 연결한다.
각 서버의 `CClient`는 이 처리 확장점과 사용자·매치 등 도메인 상태를 담당한다.
공통 수신은 세대 스냅샷 → 프레임 분리 → 가상 검증 → 소유한 프레임 전달 → Recv 재등록 순서다.
프레임 또는 전달 실패는 disconnect 경로로 간다.
`GetTransportSession`, `ValidateFrame`, `DispatchFrame`은 필수 확장점이다.
`OnReceiveComplete`와 `OnDisconnectRequested`는 선택 확장점이다.
Core는 `Protocol`, `CUserMgr`, `CMatchMgr`, `CPacketMgr`, DirectX에 의존하지 않는다.

로비 `CClient`는 검증한 패킷을 자신의 작업 큐에 넣고 수신 완료 시 스케줄러에 등록한다.
`JobTarget`이 Five budget 큐와 disconnect/enqueued atomic 상태를 소유한다.
처리 정책은 `JobQueue(JobBudget)` 생성자 인자로 반드시 지정한다.
로비 CClient/Zone은 Five, 게임 CMatch는 Snapshot을 지정하며 별도 정책 래퍼 타입은 사용하지 않는다.
현재 JobQueue의 성능 개선은 [작업 목록](../../tasks/todo.md)의 후속 작업으로,
개선 전 기준 측정 → 개선 → 같은 조건의 개선 후 측정 순서로 진행할 예정이다. 아직 성능 측정값은 없다.
스케줄러는 중복 등록 억제·조건 변수 대기·남은 작업 재등록·Stop 깨우기를 담당한다.
연결 종료 시 큐를 비우고 등록 상태를 해제한 뒤 `OnJobQueueDisconnected`를 호출한다.
실제 사용자 정리는 로비의 `CUserMgr::ClientReset`이 수행한다.
대상 포인터는 비소유이며 producer/worker join까지 `CUserMgr`가 대상을 유지하는 기존 수명을 따른다.

게임 `CClient`는 로그인·RTT·테스트 패킷을 즉시 처리하고 나머지는 해당 매치 큐에 전달한다.
두 서버의 예약 패킷은 기존 `WithGeneration` 검사를 유지한다.
게임 상태를 ST_FREE로 바꾸는 동작은 `OnDisconnectRequested`에 남는다.
기존 게임 `SocketUtil::m_socketpool`은 listener에 공급할 세션 풀이라
도메인 자원의 소유자인 `Resource::m_acceptSessionPool`로 이동했다.
이 풀은 재사용 대기용 `Resource::m_sessionPool`과 역할이 달라 별도로 유지한다.

`TaggedIoContext`는 enum 숫자를 바꾸지 않고 `OP_ACCEPT/RECV/SEND/DISCONNECT` 이름으로 매핑한다.
Reset은 accept cookie를 보존하고 세대 정보의 유효 표시를 해제한다.
추가된 로비의 세대 저장 필드는 내부 context만 바꾸며 wire 구조체에는 영향이 없다.

최초 추가 추출에서 같은 이름의 자체 `.h/.cpp` 28쌍을 비교하고 CNetworkMgr·CPacketSender·CServer 사용처도 확인했다.
CPacketSender의 실제 패킷 작성과 CNetworkMgr의 접속 역할·타이머 이벤트는 서버별 계약이라 유지한다.
`main.cpp`의 예외 처리와 CServer의 worker 시작·종료 조합은 후속 공통화 후보다.
이번에는 지정한 통신·클라이언트·작업 기반과 인접한 I/O context에 범위를 한정했다.

신규 `ExtractionTests.cpp`는 분할/연속 수신·소유한 패킷·세대 차단·검증/전달 거부,
disconnect 제출 거부의 풀 반환·enum 매핑·Five budget 이후 재등록·중복 실행 억제,
disconnect 큐 폐기·대기 worker 종료를 검사한다.
이번 검증 결과는 [추가 추출 근거](evidence/server-core-extraction-20261010.json)에 기록한다.
아래 기존 1단계 검증 수치는 당시 결과로 유지한다.

### Resource 추가 공통화·참조 선언 형식

두 서버 Resource의 I/O 객체 획득과 static 풀 정의를 `IoResources<Context>`로 공통화했다.
`SessionResources<Context, Session>`은 게임의 accept용·재사용 대기용 세션 풀을 별도로 제공한다.
`SocketResources<Context>`는 로비의 native SOCKET 재사용 풀을 제공한다.
`Resource.cpp` 2개를 제거했고 자원 구성·기존 풀 이름의 별칭은 후속 정리에서 각 서버 `pch.h`로 모았다.

context 타입별 I/O 풀은 하나이며 구성 타입을 통해 접근해도 동일한 풀을 사용한다.
서로 다른 context 타입과 별도 서버 프로세스의 풀은 독립이다.
세션 풀은 shared_ptr을 소유하고 등록 시간을 갱신하며 기존 shared_ptr 비교를 유지한다.
소켓 풀은 등록 시간이 오래된 항목부터 선택한다. 소켓 값의 소유권은 IOCP에 있으므로 풀은 close하지 않는다.
기존 종료의 IOCP 완료 배출 → I/O 풀 Clear → 재사용 풀 비우기 순서는 유지한다.

참조 선언은 `SocketResource& _other`, `Func&& _func`, `auto& value`처럼 타입에 붙인다.
루트 `.clang-format`의 `ReferenceAlignment: Left`와 공통 지침에 규칙을 기록했다.
서버 자체 코드의 해당 공백만 바꾸었고 람다 캡처·주소/비트 연산·외부 코드는 별도로 보존했다.
JobQueue의 성능 개선과 개선 전·후 측정은 작업 목록 54번의 미착수 후속 작업이다.

Resource 공통화 이후 Debug/Release 서버 빌드·각 Core 126항목·ABI 110종/106상수를 통과했다.
추가 자원 검사 12항목은 풀 공유/격리·임대 중 Clear 거부·재사용·세션 소유·소켓 순서를 확인한다.
두 구성의 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종을 다시 실행했고,
각 시나리오 종료에서 pending/sockets/leased=0을 확인했다.
에이전트 환경 정적 검사도 통과했으며 현재 compilation database는 224개 소스다.
기존 경고와 DB 없는 LOCAL_TEST 검증 범위는 유지한다.

### NetworkRuntime 명명·API·별칭 정리

프로세스 공용 Winsock·IOCP의 수명을 관리하는 타입은 `NetworkRuntime`이다.
선언을 `Session.h`에서 `Net.h`로 옮기고 API는 `Start()`·`Stop()`·`Get()`으로 통일했다.
기존 `TransportHost`·`SocketUtil` 이름과 단순 전달 `Startup()`·`Cleanup()`·`Runtime()`은 제거했다.
사용처 없는 `GetLastError()`·`PrintError()` 호환 helper도 제거했다. 기존 예외·진단 경로는 유지한다.

두 서버 `pch.h`는 `PrecompiledHeader=NotUsing`인 공통 include 헤더다.
`<ServerCore/Net.h>`를 포함하고 서버 도메인 헤더보다 앞에서 `SockAddr`·`NetworkRuntime`을 using 선언한다.
별칭만 제공하던 `SockAddr.h`·`SocketUtil.h` 총 4개와 프로젝트/filters 등록·include를 제거했다.
Core 헤더는 서버 pch.h에 의존하지 않으며 세션은 Core의 `NetworkRuntime::Get()`에 접근한다.
풀 주입·기본 포트·accept ID 기록을 추가하는 어댑터는 유지한다.

현재 구조도의 모듈 의존성·I/O 호출·자원 수명은 동일하다.
구조도에 옛 클래스/API 이름이 없어 Archify JSON/HTML은 재생성하지 않았다.
이번 명명 정리의 실제 검증 결과는 [작업 목록](../../tasks/todo.md)의 56번에 기록한다.
evidence JSON의 소스 해시와 결과는 이전 작업 당시 snapshot으로 보존한다.

### 작업·로그·자원 별칭 헤더 통합

두 서버 자체 헤더를 조사해 별칭만 제공하는 Job.h·JobQueue.h·LogUtil.h·OverlapEx.h·Resource.h 총 10개를 추가 제거했다.
선언은 기존 namespace를 유지해 pch.h에 모으고 각 파일의 include·프로젝트/filters 등록을 제거했다.
OP_TYPE을 정의하는 enum.h → Core 별칭과 Resource 구성 → Global.h와 도메인 헤더 순서를 유지한다.
게임 Resource 구성에 필요한 Session은 별칭 앞에서 전방 선언한다.
함수/클래스 구현과 함께 있는 CsvLoader·MathUtil·CNetworkMgr 등의 내부 별칭은 해당 헤더에 유지한다.

로비의 PacketJobQueue는 클라이언트별 JobTarget 큐를 선택하는 JobScheduler의 별칭이다.
별도 worker가 큐마다 최대 5개 작업을 처리하고, 남은 작업 또는 disconnect를 다시 예약한다.
추가 구현이 없던 빈 파생 클래스를 제거했고 Global.h의 class 전방 선언도 별칭에 맞췄다.
게임은 IOCP 타이머의 매치 갱신 경로에서 CMatch::Update가 Snapshot 큐를 처리하므로 해당 스케줄러를 사용하지 않는다.
JobQueue와 JobScheduler의 역할은 다르며 이름 정리로 두 서버의 실행 모델을 통합하지 않는다.

구조도의 모듈·호출·수명은 동일하므로 Archify JSON/HTML은 재생성하지 않았다.
이번 검증은 작업 목록 57번에 기록하며 이전 evidence JSON은 수정하지 않는다.

### GameObject·로비 컴포넌트의 공통 기반

Core의 GameObject.h에 ObjectPosition·ObjectOrientation·ObjectMotion·GameObject·MoveObject를 추가했다.
ObjectPosition은 위치와 scalar/vec3/vec2 설정을, ObjectOrientation은 방향·시선·up/right를,
ObjectMotion은 속도·최대 속도·마찰 상태를 소유한다. 기존 동시성 경계는 바꾸지 않는다.

게임 CGameObject·CMoveObject는 GameObjectGeometry를 지정한 Core 템플릿의 별칭이다.
Geometry는 vec3/vec2·BoundingOrientedBox·XMFLOAT4X4 타입과 충돌 변환 함수를 제공한다.
공통 객체 기반의 ID·위치·가상 Update/Move/UpdateBoundingBox·매치/노드 상태를 Core로 옮겼다.
ID 기본값은 이전 미초기화 상태 대신 -1로 명시한다.
DirectX 변환은 Game_Server/GameObject.cpp에 남으며 기존 행렬 항목 갱신과 box Transform을 유지한다.
타워/넥서스의 공격·HP·대상 선택·패킷·승패 처리는 GameObject.h/cpp에 그대로 남는다.

로비 CTransform은 ObjectPosition과 ObjectOrientation을, CPhysic은 ObjectMotion을 상속한다.
생성/Reset 위치·높이 대입·시야 거리·이동 가속/감속은 로비 구현을 유지한다.
로비 SetLook은 시선만 대입하고 게임 MoveObject::SetLook은 up×look을 정규화해 right도 갱신한다.
vec2 SetPos는 높이를 보존한다. 로비 GetVelocity의 mutable 참조와 게임의 const 참조도 유지한다.
Core 자체는 DirectX·서버 pch·Protocol·게임 singleton을 포함하지 않는다.

GameObjectTests.cpp의 14항목은 상태 독립성·위치 높이 보존·서버별 SetLook 의미·충돌 정책 전달,
가상 Move/Update/충돌 갱신 및 기반 포인터 삭제를 검사한다.
충돌 정책 검사는 독립 대체 타입으로 위임 계약을 확인하며 실제 전투 전체 검증은 아니다.
실제 DirectX 어댑터의 변환 문장은 이전 구현과 토큰 비교하고 서버 빌드·통합 회귀로 연결을 확인한다.
현재 검증 결과는 [작업 목록](../../tasks/todo.md)의 58번에 기록하며 이전 evidence JSON은 당시 snapshot으로 유지한다.

## 패킷 계약

기존 선언을 `Shared/Protocol/protocol.h`로 옮겼다.
`Server/Game_Server/protocol.h`는 기존 도구를 위한 forwarding header다.
클라이언트와 두 서버의 컴파일 경로는 공통 헤더를 참조한다.

- 프레임 길이는 첫 byte이며 유효 범위는 2~255byte다.
- 내부 수신 버퍼는 256byte다. 분할 수신과 여러 프레임 수신을 처리한다.
- 길이 0/1은 header byte만 도착해도 거부한다.
- 방향별 허용 패킷 종류와 정확한 구조체 크기를 확인한다.
- 문자열 종료, 슬롯·직업·스킬·경로 인덱스, 회전값의 유한성을 검사한다.
- 지연 실행할 패킷은 수신 버퍼 대신 복사한 byte 배열을 소유한다.

분리 전 x64 MSVC 기준의 **구조체 110종·상수 106개**를 고정했다.
Debug/Release에서 크기·정렬·멤버 offset/크기와 상수를 대조한다.
packing과 wire 필드 배치는 유지했다.
이 Protocol에는 `WCHAR`·시간 표현 등이 있으나 DirectX 필드는 없다.

형식 검사는 전체 콘텐츠 검증을 대신하지 않는다.
스킬 소유·가격·인증·운영 DB 규칙은 각 서버의 책임이다.

## I/O와 자원 수명

| 자원 | 소유자 | 반환 시점 |
|---|---|---|
| Winsock·IOCP | `NetworkRuntime`·`IocpService` | 모든 worker와 완료를 정리한 뒤 |
| native socket | `IocpService` | 취소·close 후 잔여 완료 배출 |
| 수신/accept context | Session·listener | native 완료 후 소유 객체 종료 |
| 송신/앱 이벤트 context | `ObjectPool<OverlapEx>` | 마지막 완료 후 풀 반환 |
| 송신 byte | 송신 context | 부분 송신을 모두 완료한 뒤 |
| 작업 패킷 | Job 캡처 | 실행 또는 큐 정리 후 |
| worker | `ThreadGroup` | 생산자 종료 → IOCP 종료/join |

같은 소켓의 송신은 큐에서 순서대로 제출한다.
부분 송신은 offset을 늘려 나머지를 다시 제출한다.
즉시 오류도 추적 가능한 완료로 돌려줘 풀 반환 경로를 통일한다.

완료 callback은 연결별 mutex로 직렬화한다.
IOCP 상태 mutex를 해제한 뒤 callback mutex를 기다린다.
`Completion::Clear()`는 guard를 먼저 풀고 mutex 소유권을 반환한다.

Disconnect는 다른 native I/O를 취소한다.
미제출 송신을 실패 완료로 반환하고, native 완료가 배출된 뒤 재사용 callback을 전달한다.
일반 DisconnectEx 완료는 상대 연결 상태에 영향을 받는다.
서버 전체 종료는 소켓을 닫아 잔여 완료를 배출한다.

풀은 pending 객체 반환·중복 반환·임대 중 Clear를 거부한다.
세션을 초기화하거나 끊을 때 세대를 변경한다.
이전 접속에서 예약한 패킷 작업과 사용자 상태 타이머는 세대가 다르면 실행하지 않는다.
NPC·매치·스킬 타이머 전체를 새 수명 모델로 바꾼 것은 아니다.

## 종료와 초기화 실패

`Stop-Local.ps1`은 소유한 PID·실행 경로·시작 시간을 확인한다.
서버의 `Local\Wod.Server.Stop.<pid>` event를 열어 종료를 요청한다.
15초 이내 정상 종료 및 종료 코드 0을 확인한다.
시간 초과 시 해당 프로세스를 정리하고 검사 실패로 보고한다.

종료 순서는 생산자 중지/join → native I/O 취소/close → IOCP worker join →
잔여 완료·풀 검사 → 콘텐츠 자원 해제 → Winsock 해제다.
부분 worker 생성과 초기화 실패도 같은 정리 경로를 사용한다.
worker 예외는 기록하고 서버 종료를 요청하며 실패 종료 코드로 전달한다.
Debug CRT 오류를 stderr에 남겨 숨긴 프로세스의 오류 대기 창을 방지한다.

완료 검증은 `pending=0`, `sockets=0`, `leased=0`을 확인한다.
이 값은 Core I/O와 풀의 잔여 수이며 프로세스 전체 메모리 측정값은 아니다.

## 회귀 검사에서 함께 수정한 기존 결함

| 결함 | 수정 |
|---|---|
| 싱글턴 destructor에서 자기 `unique_ptr` 재해제 | 기본 destructor 사용 |
| 보스 슬롯에서 3개 영웅 시작 위치 배열 접근 | 영웅과 보스 배치 분리 |
| 로딩 대기 중 갱신 예약 누락 | 완료 대기 동안 갱신 재예약 |
| 몬스터 초기화가 12개 위치를 가정 | 실제 위치/방향 개수 내 선택·부족 시 실패 |
| 생성 전 미니언의 잘못된 navigation index | 비활성 NPC 갱신 제외·index 검사 |
| 재사용 NPC 이벤트 context의 대상 값 누락 | 대상 ID를 명시적으로 설정 |
| 매치 시작 패킷을 사용자 패킷으로 다시 처리 | switch 분기 종료·매치/참가자 범위 검사 |
| 로비 외형 파일 누락 시 무한 읽기 | 파일 열기·각 입력 실패 검사 |

몬스터 CSV의 `ExtraPos/Look` 파싱 완료는 6단계에 남는다.
이번에는 현재 읽은 위치만 사용하며 새로운 배치 데이터를 만들지 않았다.

## 검증과 재현

```powershell
./scripts/Build.ps1 -Module Servers -Configuration Debug
./scripts/Test-ServerCore.ps1 -Configuration Debug
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Debug -Python <python.exe>
# Release도 같은 순서로 실행
./scripts/Test-Local.ps1 -Configuration Debug -ServersOnly
./scripts/Test-AgentEnvironment.ps1
```

통합 검사는 8910/8911 포트를 사용하므로 구성별로 순차 실행한다.
검사 결과와 로그는 Git에서 제외하는 `artifacts/logs`에 기록한다.

검증 결과는 [소스·검증 근거](evidence/server-core-implementation-20261007.json)에 확정한다.

| 검사 | Debug | Release | 확인 범위 |
|---|---|---|---|
| 서버·Core·테스트 빌드 | PASS | PASS | x64 v145 링크 |
| Core 검사 | 93항목 PASS | 93항목 PASS | 프레임·송신·풀·동시성·세대·스레드 회수 |
| 패킷 ABI | PASS | PASS | 구조체 110종·상수 106개 |
| 네트워크 회귀 | 12종 PASS | 12종 PASS | 잘못된 입력·분할/연속 수신·반복 연결 |
| 4인 매칭·로딩 | 보스 4/5 PASS | 보스 4/5 PASS | 전원 로딩 완료·첫 NPC 알림 |
| 초기화 실패 회수 | 4종 PASS | 4종 PASS | 리소스 누락 2종·로비 미실행·포트 충돌 |
| 로컬 시작 smoke | 클라이언트 창 PASS | 서버 연결 PASS | 전투 검사는 별도 |
| 클라이언트 빌드 | PASS | 이번 단계 미실행 | 공통 Protocol 참조 |

7개 통합 시나리오의 종료에서 Core 잔여 수 0을 확인했다.
에이전트 환경 검사와 Serena 조회도 통과했다.
compilation database는 226개 소스이며 누락 경로는 0개다.

Core 검사는 프레임·부분 송신·즉시 실패·풀 반환·다중 worker·재접속 세대·작업 budget을 포함한다.
통합 검사는 잘못된 프레임, 반복 연결, 두 보스의 4인 매칭과 로딩 완료를 검사한다.
리소스 누락·로비 미실행·포트 충돌의 실패 종료도 확인한다.

4인 검사는 TCP 참가자 4개이며 실제 렌더링 클라이언트 4개 실행은 아니다.
DB 없는 `LOCAL_TEST`만 사용한다.
운영 DB·블록체인·전투 전체·장시간 부하는 미검증이다.
서버 메모리·CPU·처리량의 전후 수치는 이번에 측정하지 않았다.
기존 서버/클라이언트 경고는 빌드 로그에 남겼으며 성공으로 숨기지 않는다.

## 후속 단계

1단계의 코드 공통화가 후속 도구의 기반이다.
`IocpService::Stats()`는 프로세스 내부 관측값을 제공한다.
새 원격 관측 API·GUI·월드 렌더링은 아직 구현하지 않았다.

2~4단계에서 신규 시나리오 도구와 agent/GUI 제어·월드 관찰을 만든다.
이번 Python 검사는 회귀용이며 기존 더미 제품을 재사용한 신규 도구가 아니다.
5단계 전체 서버 문서화와 6단계 콘텐츠 데이터화는 이후 순차 진행한다.
