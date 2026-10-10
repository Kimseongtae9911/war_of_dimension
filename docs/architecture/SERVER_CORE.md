# ServerCore 공통 기반 구현

서버 리팩토링 1단계를 적용했다.
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
│  ├─ Net.h                          소켓·IOCP·프레임·종료 신호
│  ├─ Session.h                      세션·listener·세대 검사
│  ├─ Concurrency.h                  Job·작업 큐·객체 풀·스레드 그룹
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

공용 헤더 4개를 `ClInclude`에 등록했다.
Core와 테스트의 `.filters`에서 소스·헤더·개발 설정을 구분한다.
테스트 소스 2개는 `ServerCore.Tests` 프로젝트에서 표시한다.

Core 구현·헤더·테스트 7개에 함수 간격·다중 행 `if`·분기/`while` 뒤 빈 줄·마지막 `return` 분리를 적용했다.
연결된 `if`·`else if`·`else`의 본문은 한 문장이어도 중괄호로 감쌌다.
루트 `.clang-format`과 `AGENTS.md`를 포맷 기준으로 사용한다.
멤버는 `m_`, 함수 인자는 `_` 접두어로 구분한다.
기존 동작·패킷 계약·소유권은 유지했다. 이전 소스 해시는 1단계 구현 검증 당시의 기록이다.

| 영역 | 공통 구현 | 서버에 남은 책임 |
|---|---|---|
| 네트워크 | Winsock·소켓·IOCP·AcceptEx·DisconnectEx·송수신 | 접속 역할·사용자·매치·패킷 처리 |
| 프레임 | 분할/연속 수신·길이 검사·소유한 패킷 복사 | 패킷별 콘텐츠 처리 |
| 작업 | Job·실행 직렬화·처리 budget | 로비 사용자 스케줄러·게임 매치 큐 |
| 풀 | I/O 객체 소유·확장·임대/반환 검사 | 기존 소켓/스킬 도메인 풀 |
| 수명 | 스레드 그룹·취소/완료 배출·종료 신호 | 종료 순서·콘텐츠 자원 정리 |
| Protocol | 공통 선언·방향별 형식 검사 | 인증·거래·게임 규칙 |

서버의 `SockAddr`, `Job`, `JobQueue`, `LogUtil`, `OverlapEx`,
`TCPSocket`은 alias 또는 얇은 어댑터가 됐다.
중복 `.cpp` 12개를 제거했다. [정리 근거](../development/CLEANUP.md)를 참조한다.

로비 작업 큐는 최대 5개, 게임 큐는 실행 시작 시 크기만큼 처리한다.
기존 PPL `shared_ptr` 우선순위 비교를 유지한다.
FIFO나 예약 시간순 실행을 새로 보장하지 않는다.
로비 content worker는 조건 변수로 대기하고 남은 작업을 다시 예약한다.

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
| Winsock·IOCP | `TransportHost`·`IocpService` | 모든 worker와 완료를 정리한 뒤 |
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
