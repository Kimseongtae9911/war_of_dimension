# 서버 리팩토링 1단계: ServerCore 구현 계획

작성일: 2026-10-07. 조사 기준: `86f83a94f76e7218c4cd2f34d992f9d2d02174f1`.

이 문서는 **구현 전 설계·조사 기록**이다. 당시 서버 코드·패킷·프로젝트 설정은 변경하지 않았다.
이후 1단계를 적용했으며 현재 상태와 실제 검증은 [ServerCore 구현](SERVER_CORE.md)을 따른다.

## 목표와 진행 순서

로비·게임 서버의 공통 기반을 `ServerCore` 정적 라이브러리로 분리한다.
각 서버는 이 라이브러리를 연결하고 콘텐츠와 상태를 직접 소유한다.
두 서버의 프로세스나 메모리를 합치는 작업은 아니다.

| 단계 | 작업 | 결과물·완료 조건 |
|---|---|---|
| 1 | ServerCore 공통화 | 공통 라이브러리, 두 서버 어댑터, 계약·회귀 테스트, 이전 기록 |
| 2 | 새 시나리오 더미 제작 | 신규 연결·상태 관리, 시나리오 실행·assert·timeout·결과 저장. 기존 더미 구현은 재사용하지 않음 |
| 3 | agent와 GUI 제어 | 같은 실행 서비스를 사용하는 기계 판독 인터페이스와 GUI. 상태 조회·시작·중지·결과 확인 |
| 4 | 서버 월드 관찰 | 서버에서 제공한 월드 snapshot을 간단히 렌더링. 객체 ID·위치·상태·갱신 시각 확인 |
| 5 | 서버 구조 문서화 | 전반 흐름·콘텐츠·DB·NPC·스레드·소유권·실패 경로 문서와 그래프·흐름도·표 |
| 6 | 콘텐츠 데이터화 완료 | 기존 CSV·텍스트·상수 조사, 데이터 계약·검증·로더, 기존 동작 동등성 확인 |

각 단계의 완료 조건을 확인한 뒤 다음 단계로 진행한다.
1단계의 최소 회귀 테스트는 신규 더미 제품을 미리 만드는 작업과 구분한다.
전체 서버 기능·DB·NPC의 상세 분석은 5단계에서 작성한다.

## 현재 코드 조사

[조사 원본](evidence/server-core-audit-20261007.json)에 파일 쌍·SHA-256·출처를 기록한다.
아래 내용은 정적 코드 조사다. 장애 재현·부하 실측 결과로 해석하지 않는다.

### 이름이 같아도 동작은 다르다

두 서버 최상위 `.h`·`.cpp` 중 이름이 같은 파일은 **35쌍**이다.
바이트까지 같은 파일은 **4쌍**, 다른 파일은 **31쌍**이다.

| 분류 | 파일·현재 차이 | 공통화 방향 |
|---|---|---|
| 동일 내용 | `SockAddr.h/.cpp`, `Job.cpp`, `JobQueue.cpp` | SockAddr부터 추출. Job의 핵심 동작은 서로 다른 헤더에 있으므로 별도 조사 |
| 작업 실행 | `Job.h`, `JobQueue.h` | 공통 Job·큐 저장/실행, 서버별 처리 예산 분리 |
| 소켓·I/O | `SocketUtil`, `TCPSocket`, `OverlapEx` | Win32 자원·비동기 요청·연결·프레임 처리 공통화 |
| 풀 | `Resource` | I/O 컨텍스트·일반 세션의 풀 원리만 공통화. ID·객체 reset·접속 정책은 서버에 유지 |
| 오류·진단 | `LogUtil` | 오류 정보·진단 sink 공통화. 서버별 로그 내용은 어댑터에 유지 |
| 혼합 클래스 | `CClient`, `CNetworkMgr`, `CServer` | 클래스 전체 이동 대신 통신·수명 기능만 Core에 위임 |
| 도메인 | `CMatchMgr`, `CPacketMgr`, `CPacketSender`, `GameUtil`, `Global`, `enum.h` | 매칭·패킷 의미·콘텐츠 상태는 각 서버에 유지 |

동일한 `Job.cpp`·`JobQueue.cpp`가 핵심 구현의 동일성을 의미하지 않는다.
같은 이름의 클래스도 API와 데이터의 의미를 먼저 비교한다.

### 이전 전에 고정해야 할 차이

| 영역 | 로비 | 게임 | 계획 |
|---|---|---|---|
| 큐 처리 예산 | 호출당 최대 5개 | 호출 시작 시 큐 크기만큼 시도 | 명시적 정책으로 보존 |
| 큐의 객체 책임 | `PacketJobQueue`가 `CClient*`, `CUserMgr`에 결합 | 매치 단위 Job 실행 | 콘텐츠 스케줄러는 각 서버에 유지 |
| 지연 패킷 처리 | 로비 Job이 `this`를 캡처 | 게임은 `shared_from_this()` 캡처 | 연결 수명·세대 검증 후 실행 |
| 소켓 풀 | SOCKET·시각을 담은 값 객체 | shared_ptr Session 기반 풀 | 자원 소유와 서버 ID 정책 분리 |
| I/O 태그 | `OP_SERVER_CONNECT`가 포함됨 | 매치·스킬·NPC 이벤트가 포함됨 | transport 태그와 앱 이벤트를 별도 타입으로 분리 |
| 시작·접속 | 최초 접속을 게임 서버 연결로 취급 | 로비로 outgoing 연결 | 역할 판정·연결 순서는 서버 어댑터에 유지 |
| 스레드 | IOCP 4개, 패킷 worker 1개, timer 1개. DB는 조건부 | IOCP `hardware_concurrency()/2`, timer 1개 | 최초 이전 시 역할·개수 보존, 생성한 스레드만 join |

현재 두 JobQueue는 `concurrent_priority_queue<shared_ptr<IJob>>`를 사용한다.
Job의 시간 비교 함수가 실제 큐에서 적용된다고 단정할 수 없다.
FIFO 또는 예약 시각 순서로 바꾸는 작업은 공통화와 분리한다.

### 확인한 위험과 회귀 테스트

| 출처 | 코드에서 확인한 사항 | 구현 시 확인·수정 |
|---|---|---|
| 게임 `CNetworkMgr::Recv` | 남은 데이터 이동 조건이 `remaindata < 0` | 서버 간 분할·합쳐진 패킷 및 꼬리 보존 테스트 |
| 양쪽 `CClient`·`CNetworkMgr` 수신 | 최소 헤더·0 길이·패킷별 길이를 확인하기 전에 cast/반복 | 공통 FrameDecoder와 방향별 패킷 검증 |
| 양쪽 `Session::Send/Recv` | WSASend/WSARecv 반환 오류 처리가 부족함 | 즉시 실패·pending·부분 송신·취소 테스트 |
| 양쪽 `IOCPFunc` | 실패 또는 null OVERLAPPED를 구분하기 전에 접근 가능 | timeout·제어 완료·취소·peer close를 구분 |
| 로비 `SocketUtil::PrintError` | 오류 출력 후 `while (true);` | 오류 반환과 제한 시간 내 종료 테스트 |
| 양쪽 `TCPSocket.h` | 빈 소멸자, 핸들 소유권이 명시되지 않음 | pending I/O drain 뒤 소켓·IOCP·WSA 해제 |
| 로비 `CServer::Run` | 생성은 4/1개지만 join은 `hardware_concurrency()/2` 기준 | 실제 컨테이너 기준 join, 부분 초기화 해제 |
| 양쪽 Job 처리 | 지연 실행·풀 재사용·남은 작업 재등록 경계 | 중복 실행·누락·이전 세대 callback 검사 |

기존 결함을 정상 동작으로 보존하지 않는다.
먼저 실패를 재현하는 작은 테스트를 만들고, 결함 수정과 단순 코드 이전을 구분한다.
성능 개선량은 구현 후 같은 조건으로 측정한다. 현재 계획에서 수치를 가정하지 않는다.

## 목표 구조

[인터랙티브 설계도](../diagrams/server-core-plan/server-core.architecture.html),
[JSON 원본·검증 근거](../diagrams/server-core-plan/README.md)를 함께 관리한다.
화살표는 소스 의존성이다. 실제 서버 간 TCP 연결도는 기존 [현재 구성](OVERVIEW.md)을 따른다.

```text
Server/
  ServerCore/
    ServerCore.vcxproj             # 신규 StaticLibrary
    include/ServerCore/
      Net/                        # 주소, 소켓, IOCP, 연결, 프레임
      Concurrency/                # Job, JobQueue, 실행 예산
      Diagnostics/                # 오류·통계·진단 sink
    src/
    tests/ServerCore.Tests.vcxproj # 신규 단위·loopback 테스트
  Lobby_Server/                   # 로비 서비스와 어댑터
  Game_Server/                    # 게임 서비스와 어댑터
Shared/
  Protocol/protocol.h             # 기존 wire 선언의 단일 원본
```

이 구조는 제안이며 아직 디렉터리·프로젝트를 만들지 않았다.
Core의 public header는 필요한 표준·Windows 헤더를 직접 포함한다.
두 서버의 `pch.h`나 전역 싱글턴 없이 단독으로 컴파일되어야 한다.

| Core가 소유할 것 | 각 서버에 남길 것 |
|---|---|
| WSA 초기화·해제, SOCKET/HANDLE 관리 | 포트·접속 역할·최대 접속 수 등 설정 |
| 주소, listener, TCP 세션, IOCP 실행 기반 | 로그인·매칭·로비→게임 연결 절차 |
| pending I/O 컨텍스트, 송수신 버퍼, 프레임 분리 | 방향별 허용 패킷·CPacketMgr·PacketSender |
| Job 표현·큐 저장·예산에 따른 실행 | 유저/존/매치의 실행·재등록·상태 규칙 |
| 자원 계수·오류·진단 snapshot | NPC·스킬·아이템·충돌·월드·콘텐츠 timer |
| 앱 제어 이벤트의 전달·수명 기반 | DB 스레드·ODBC·저장 프로시저·블록체인 |

Core에서 `Game_Server`·`Lobby_Server` 헤더를 역참조하지 않는다.
`Interface.h`는 generic singleton과 콘텐츠 인터페이스가 섞여 있어 통째로 이동하지 않는다.
`MathUtil.h`, DirectX 수학·콘텐츠 타입·외부 라이브러리도 필요 없이 Core에 가져오지 않는다.

### 공통 API와 어댑터

다음은 책임을 고정하기 위한 API 초안이다. 최종 명칭은 구현 시 정리한다.

| API | 책임·주의점 |
|---|---|
| `Start(config, sink)` / `Stop()` | 상태·오류 반환, 중복 호출 안전, 부분 시작 실패 cleanup |
| `TcpSession::Send(bytes)` | 데이터 소유권 확보, 순서·부분 송신·즉시 실패 처리 |
| `FrameDecoder::Feed(bytes, policy)` | 타입 의미와 분리된 헤더/길이 처리, 완성 프레임과 남은 꼬리 관리 |
| `OnConnected/OnPacket/OnClosed/OnError` | IOCP callback임을 명시. 서버 어댑터가 기존 실행 위치로 전달 |
| `JobQueue::Process(budget)` | 로비 5개, 게임 시작 시 개수 정책. 실행 후 잔여 상태 반환 |
| `PostAppEvent(envelope)` | transport와 별도 앱 이벤트. 수명 보장된 payload와 핸들러 |
| `StatsSnapshot()` | 연결·pending I/O·풀·바이트·오류 계수. 콘텐츠 상태를 소유하지 않음 |

Core 연결 핸들은 slot과 generation을 포함한다.
기존 wire상의 유저·소켓·매치 ID는 서버 어댑터에서 매핑하며 바꾸지 않는다.
타입이 다른 `OP_TYPE` 값을 숫자로 직접 변환하지 않는다.

게임의 로그인·RTT·테스트 패킷 직접 처리와 나머지 매치 Job 전달을 보존한다.
로비의 유저 큐, 게임의 매치 큐, 도메인 timer도 실행 경계를 유지한다.
일반화 때문에 모든 콘텐츠 callback을 IOCP 스레드에서 실행하지 않는다.

## 프로토콜 호환성

현재 단일 원본은 `Server/Game_Server/protocol.h`다.
로비 `pch.h`와 클라이언트 `stdafx.h`가 이를 상대 경로로 참조한다.
1단계에서 선언을 `Shared/Protocol`로 이동하고 소비자를 함께 갱신한다.
이전 중에는 기존 경로의 forwarding header를 사용할 수 있다.

- `size/type`, 패킷 상수, 필드 순서, packing, 실제 ABI를 유지한다.
- 현재 BASE_PACKET은 packed 2byte이며 길이는 1byte다. 표현 가능한 최대 길이는 255byte다.
- `BUF_SIZE=256`과 최대 wire 길이를 혼동하지 않는다.
- `WCHAR`, `time_point`, `DirectX::XMFLOAT3` 등 기존 필드의 Windows/MSVC ABI를 먼저 기록한다.
- `sizeof`·`alignof`·`offsetof`와 대표 패킷 byte fixture를 이전 전후 비교한다.
- CS/SC/LG/GL의 같은 숫자 type을 한 테이블에서 충돌시키지 않는다. endpoint·방향별 검증을 적용한다.
- `LOCAL_TEST`·`WITH_DATABASE`와 콘텐츠 상수의 의미는 유지한다. 운영 DB 검증으로 해석하지 않는다.

자체 포함에 필요한 헤더만 보완하고 ABI 변경은 별도 작업으로 분리한다.
Core는 게임 패킷 struct를 몰라도 동작하도록 길이 정책을 주입받는다.
패킷 의미·정확한 길이·가변 데이터의 범위는 서버 측 validator가 확인한다.

수신 헤더 미완성은 보관한다. 0/1 길이와 범위 밖 길이는 연결을 거부한다.
알 수 없는 type과 허용되지 않은 방향은 서버별 정책에 따라 거부·기록한다.
비동기 Job에는 mutable 수신 버퍼의 view를 넘기지 않고 소유한 패킷을 전달한다.

## 동시성과 자원 수명

| 자원 | 소유권·수명 계약 |
|---|---|
| WSA runtime | 프로세스 공통 수명. 모든 소켓 종료 뒤 Cleanup |
| listener·session socket | 명시적 RAII 소유. adopt/detach/reset으로 기존 풀과 경계 표시 |
| IOCP handle | 실행기가 소유. 완료 처리와 worker join 뒤 CloseHandle |
| OVERLAPPED·send buffer | 요청부터 최종 완료까지 주소·내용 유지. 성공·실패·취소 모두 한 번만 반환 |
| receive buffer | 세션 소유. 동일 세션의 중첩 수신·꼬리 이동 경계 보호 |
| send queue | 연결별 순서 유지. 부분 송신은 offset을 갱신해 재요청 |
| delayed Job·앱 이벤트 | 객체 수명과 generation 검사. reset된 이전 유저에 실행하지 않음 |
| 풀 | pending 요청이 있는 객체를 재사용하지 않음. 용량·상한·고갈 정책은 설정으로 전달 |

WSA 호출의 성공·`WSA_IO_PENDING`·즉시 실패를 구분한다.
전송 중 연결 해제, GQCS 실패, bytes=0, null OVERLAPPED를 각각 처리한다.
여러 IOCP worker가 같은 객체를 동시에 변경할 수 있는 지점을 표시한다.
같은 연결의 패킷 전달·세대 변경과 send queue는 명시적 직렬화 경계를 둔다.

`DisconnectEx`의 소켓 재사용과 단순 close를 섞지 않는다.
재사용은 해당 완료와 pending 요청 정리를 확인한 후 수행한다.
정적 라이브러리 전환만으로 풀 객체를 두 프로세스가 공유하는 것은 아니다.

정상 종료 순서는 다음과 같다.

1. 종료 상태로 전환하고 새 접속·새 Job·timer producer를 막는다.
2. 각 서버가 콘텐츠 worker/timer를 중지하고 세션 I/O를 취소한다.
3. IOCP를 계속 처리하여 취소·송신·수신 완료와 앱 이벤트를 drain한다.
4. pending=0을 확인하고 제어 완료로 대기 중인 worker를 깨워 join한다.
5. 연결·버퍼 풀·listener·IOCP·WSA를 순서대로 해제한다.

초기화 실패에도 이미 만들어진 자원은 해제한다.
종료 timeout은 실패로 보고하며 강제 kill을 정상 종료 성공으로 계산하지 않는다.
DB 전용 실행·종료는 현재 DB 없는 경로와 별도로 검증해야 한다.

## 구현 순서와 단계별 결과물

각 단위는 독립적으로 검토·검증 가능한 변경으로 나눈다.
중간에 공통 API를 확장하더라도 콘텐츠 규칙과 wire 변경은 함께 넣지 않는다.

| 순서 | 작업 | 결과물·다음 단위로 넘어갈 조건 |
|---|---|---|
| 1-A | 현재 동작·실패 경로 기준 확보 | Debug/Release 서버 빌드, LOCAL_TEST 접속 흐름, 패킷 ABI manifest, 큐 정책 테스트, 기존 경고 기록 |
| 1-B | Core 프로젝트와 의존성 설정 | v145/C++20 x64 StaticLibrary·테스트 프로젝트, 양 서버 ProjectReference, header 독립 컴파일 |
| 1-C | Protocol·주소·진단 분리 | Shared/Protocol, SockAddr, 오류 sink. ABI 비교 및 양 서버·실제 클라이언트 빌드 통과 |
| 1-D | Job·JobQueue 공통화 | 서버별 budget·도메인 adapter. callable·동시 push·정확히 한 번 실행·잔여 재등록 검사 |
| 1-E | 송수신 기반 구현 | RAII 핸들, IoContext/풀, FrameDecoder, TcpSession/listener/IOCP. loopback·오류 주입 테스트 |
| 1-F | 로비 어댑터 적용 | 유저·매칭·게임 연결 역할 유지. 5개 budget, 첫 접속 역할, 지연 Job·재접속 검증 |
| 1-G | 게임 어댑터 적용 | 로비 outgoing·클라이언트·매치/NPC 이벤트 bridge 적용. 직접 처리/매치 큐 경계 유지 |
| 1-H | 종료·중복 제거·최종 검증 | 정상 Stop 통합, 두 서버 중복 기반 구현 제거, 소유권·경고·회귀 결과 문서화 |

1-A의 기준 확보 후 실패 테스트를 추가한다.
해당 결함을 Core에서 수정할 때 기존 구현과 같은 실패를 재현했다는 근거를 남긴다.
1-F/1-G의 mixed 상태도 동일 프로토콜로 접속 가능해야 한다.
각 단위는 이전 상태로 되돌릴 수 있어야 하며, 두 구현을 장기 유지하지 않는다.
임시 호환 wrapper는 참조가 0인지 확인한 뒤 제거하고 정리 기록에 남긴다.

### 프로젝트·개발 도구 변경 대상

- `NewWod.slnx`, `NewWod.Servers.slnf`: Core·테스트 프로젝트와 참조 반영.
- `Directory.Build.props`: 현재 3개 앱만 포함하는 공통 설정을 Core까지 확장. 출력·중간 경로 분리.
- `Directory.Build.targets`: 공용 include와 링크 설정을 점검. Core가 게임 Include 폴더에 의존하지 않도록 분리.
- 양 서버 `.vcxproj/.filters`: 이전 파일 제거, 공용 include·ProjectReference 추가.
- Debug/Release의 실제 CRT 옵션을 평가하고 Core·테스트와 일치시킨다. `/MDd`·`/MD`를 기준으로 확인한다.
- `scripts/Development.ps1`, `New-CompilationDatabase.ps1`: 실행 앱 목록과 라이브러리/테스트의 탐색 목록 분리.
- Core를 실행 가능한 모듈로 취급하지 않는다. compilation database에는 Core·테스트 소스를 포함한다.
- `scripts/Test-ServerCore.ps1` 신규 제안: 테스트 빌드·실행·timeout·기계 판독 결과.

기존 사용자 `NewWod.slnx` 수정은 보존한다.
추후 프로젝트 추가는 현재 파일에 병합하고 저장된 경로를 임의로 되돌리지 않는다.
생성된 compilation database·빌드·로그·개인 설정은 추적하지 않는다.

## 검증 계획

### 의미 있는 자동 테스트

| 테스트 | 확인할 실패·경계 |
|---|---|
| Protocol ABI | 패킷 크기·offset·packing·상수·대표 bytes 동등. 표현 범위를 넘는 기존 선언은 별도 보고 |
| FrameDecoder | 헤더 1byte, 분할 body, 여러 프레임+꼬리, 0/1 길이, 최대 길이, 긴 스트림·receive capacity |
| endpoint validator | 방향·타입·sizeof/가변 길이·범위 오류, unknown type 정책 |
| JobQueue | JobRef와 callable overload, move-only 캡처, 로비 5개/게임 snapshot budget, 동시 push, 잔여 작업 재등록 |
| 객체 수명 | reset·disconnect·재접속 후 이전 generation Job/event가 새 사용자에 적용되지 않음 |
| I/O 오류 주입 | pending·즉시 오류·부분 송신·원격 close·취소·null 완료·풀 고갈·connect/accept 실패 |
| loopback | 양방향 분할/합친 패킷, 동시 연결, 송신 순서, 연결 반복·reset |
| 정상 종료 | 초기화 중 실패, 유휴/통신 중 Stop, 반복 Start/Stop, pending/풀 대여/소켓/worker 잔존 0 |

부분 송신·즉시 오류는 실제 네트워크에서 우연히 발생하기를 기다리지 않는다.
I/O 호출 경계의 테스트 seam으로 재현하고 loopback으로 실제 IOCP 경로도 확인한다.
회귀 harness는 신설하고 기존 `Stress_Test`·더미 소스를 제품 기반으로 재사용하지 않는다.

### 실제 실행 확인

```powershell
./scripts/Build.ps1 -Module Servers -Configuration Debug
./scripts/Build.ps1 -Module Servers -Configuration Release
./scripts/Test-Local.ps1 -ServersOnly -Configuration Debug
./scripts/Test-Local.ps1 -ServersOnly -Configuration Release
./scripts/Test-AgentEnvironment.ps1
```

Protocol 이동 이후에는 클라이언트도 빌드한다.
신규 Core 테스트는 구현 후 `Test-ServerCore.ps1`로 실행하도록 한다.
기존 `Test-Local.ps1`은 listener·서버 간 연결을 확인하는 smoke 수준이다.
강제 중지 경로를 쓰므로 정상 종료 검증을 대신하지 않는다.

LOCAL_TEST에서 로비 접속·영웅/스킬 확정·매칭·8911 전환·READY/LOADING·인게임 진입을 회귀 확인한다.
4개 클라이언트는 시험 조건을 기록하고 단계적으로 확인한다.
서버 연결 테스트 통과만으로 전투·NPC·운영 DB 전체 통과를 주장하지 않는다.

성능은 같은 빌드·머신·접속 수·시나리오에서 비교한다.
측정 대상은 유휴/통신 시 CPU, private bytes, 핸들·스레드, pending I/O와 풀 대여 수,
수신/송신량, 오류·큐 대기 시간, 종료 시간이다.
새 더미 기반의 대규모·콘텐츠 부하는 2~4단계에서 확장한다.

### 1단계 완료 기준

- 두 서버가 같은 Core 프로젝트를 실제 연결하고 공통 기반의 이중 구현이 남지 않는다.
- Core public header·테스트에 서버 도메인 헤더·싱글턴 역참조가 없다.
- 기존 wire ABI·LOCAL_TEST 연결 순서·콘텐츠 실행 경계가 유지된다.
- Debug/Release 빌드와 단위·loopback·실제 전환 회귀 검증이 통과한다.
- 정상 종료·초기화 실패 시 outstanding I/O·대여 객체·핸들·스레드가 정리된다.
- 기존 경고·새 경고·실행하지 않은 범위를 분리하고, 경고를 숨겨 통과시키지 않는다.
- 새 도구가 활용할 진단 API의 의미·snapshot 수명·스레드 안전 계약을 기록한다.

## 후속 단계의 연결 지점

2단계 도구는 새 코드로 연결 상태 머신·scenario runner를 만든다.
시나리오와 결과는 기계가 읽을 수 있어야 하며, timeout·오류·단계별 assertion을 포함한다.
CLI/API 방식과 GUI 기술은 2~3단계에서 결정한다.
Windows C++ wire ABI를 읽을 수 있다는 사실만으로 다른 언어의 직렬화 호환을 가정하지 않는다.

3단계에서는 agent와 GUI가 같은 제어 서비스·시나리오 모델을 사용한다.
별도의 GUI 자동화만 있어야 agent가 접근할 수 있는 구조는 피한다.
Core의 transport를 재사용하더라도 로그인·영웅·스킬·게임 상태 머신은 새 도구가 소유한다.

4단계의 전체 월드 관찰은 서버가 제공하는 snapshot을 기준으로 한다.
더미가 수신한 가시 범위의 객체를 서버 전체 월드라고 표시하지 않는다.
월드 ID·객체 ID·tick/시각·snapshot 일관성·갱신 주기를 별도 관찰 계약으로 정한다.
관찰 어댑터는 서버 도메인에 두며, 공유 객체를 GUI 스레드가 직접 변경하지 않는다.
기존 게임 wire 패킷 변경과 debug 관찰 채널은 별도로 검토한다.

6단계 조사 출발점은 `SkillCsvMgr`, `NpcCsvMgr`, `ItemCsvMgr`, `DataFile/CSV`, `Resource/*.txt`,
`protocol.h`와 콘텐츠 구현의 상수다.
이미 데이터화된 값·미연결 값·중복 원본을 확인한 뒤 스키마와 단일 로딩 경로를 확정한다.
1단계에서 콘텐츠 데이터 형식·수치·규칙을 함께 바꾸지 않는다.

## 이번 계획 작성의 검증 범위

소스 쌍·프로젝트 설정·참조·수신/작업/자원 경계를 조사했다.
compilation database와 Serena 조회 준비 상태를 확인하고 관련 구현을 인덱싱했다.
설계도는 Archify showcase 검증과 desktop 시각 검사를 수행했다.
구체 결과는 [설계도 검증 기록](../diagrams/server-core-plan/README.md)과 [작업 목록](../../tasks/todo.md)에 남긴다.

이번에는 서버 빌드·구동·부하·DB 테스트를 실행하지 않았다.
Core와 새로운 테스트·더미·관찰 기능은 모두 미구현이다.
커밋·push는 수행하지 않았다.
