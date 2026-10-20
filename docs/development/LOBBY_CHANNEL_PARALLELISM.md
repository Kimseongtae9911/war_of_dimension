# 로비 채널별 직렬 큐와 공통 worker pool

2026-10-18 · `codex/lobby-channel-parallelism` · 기준 `main@54af931c`

2026-10-19 명칭 정리: 유저별 작업 보관함은 `SessionWorkQueue`, 보유 멤버는 `m_workQueue`로 부른다. 이 큐는 서버 `CClient`가 보유하며 연결 세대 검사는 별도 generation으로 유지한다. 기존 성능 표·raw snapshot은 명칭 변경 전 실행의 근거로 보존하며 이번 명칭 정리로 재측정한 수치가 아니다.

**기존 시야 제거 결함을 수정하고, 같은 채널은 직렬·다른 채널은 병렬로 처리하도록 바꿨다.** 기본 로직 worker는 4개다. 매칭·채널 이동·가입/정리·전체 채팅·DB/부가 기능은 첫 구현에서 전역 배타 구간으로 보호한다. 따라서 전역 기능이 오래 실행되면 일반 채널도 기다린다.

[수정 전후 요약 보고서](LOBBY_CHANNEL_PARALLELISM.html) · [구현 구조도](../diagrams/lobby-channel-parallelism/implementation.html) · [사전 검토 당시 발견 사항](LOBBY_CHANNEL_CONCURRENCY.md) · [구조도 근거](../diagrams/lobby-channel-parallelism/README.md)

## 먼저 수정한 기존 결함

| 문제 | 수정 결과 |
|---|---|
| 종료 후 상대 시야가 초기화된 ID `-1`을 다시 조회 | ID 초기화 전에 상대 시야를 제거하고 이전 ID를 관찰자에게 통지 |
| 채널 이동 시 본인 sender로 상대 제거 통지 | 출발 채널 관찰자의 sender 사용, 새 채널 시야 재구성 |
| 접속 등록과 종료의 슬롯 잠금 불일치 | 전역 소유 구간에서 입퇴장, 슬롯 mutex 접근 통일 |
| 음수/가득 찬 목적지·미배정 연결·중복 가입 슬롯 | 하한/상한·정원 검사, 등록 실패 시 disconnect, 중복 배정 방지 |
| 매칭 취소 때 카운터만 감소하고 큐 항목 잔존 | 취소 시 항목 제거, 재등록 중복 제거, generation/상태 검증 후 3+1 선택 |
| 선택된 유저가 후속 등록으로 다른 매치에 중복 포함될 가능성 | 전송 대기 상태를 `ST_INGAME`으로 바꾸고 후속 일반 로비 명령 차단 |
| DB 없는 모드의 일부 요청이 null DB 포인터 사용 | 지원하지 않는 DB 요청은 실행하지 않음. 회원가입은 기존 실패 응답 사용 |
| 수신 제거 ID를 클라이언트가 무검사 인덱싱 | 클라이언트 제거 처리에 ID 범위 검사 추가 |

초기 TCP 2클라이언트 재현에서 종료 제거는 `[-1] → [1]`, 채널 이동은 `관찰자 [] / 본인 [1] → 관찰자 [1] / 본인 []`로 바뀌었다. 이 비교는 병렬화 전에 단일 worker에서 수행했다. 이후 실제 다중 채널 검사로 다시 확인했다.

## 실행과 소유권

```text
IOCP 패킷·종료 ─┐
이동 타이머 ────┴→ 유저 SessionWorkQueue → 채널별 큐 ──→ 공통 worker pool
                              → 전역 큐 ────→ 공유/배타 실행 barrier
신규 접속·게임 서버 메시지 ─────→ 전역 큐
```

[LobbyDispatcher](../../Server/Lobby_Server/LobbyDispatcher.h)는 기존 Core `JobScheduler`에 채널별 `JobTarget`과 전역 target을 등록한다. 각 target은 기존 `Five` budget을 사용해 최대 5작업 뒤 남은 작업을 뒤에 등록한다. Core 큐 알고리즘과 게임 서버의 스케줄링 정책은 바꾸지 않았다.

유저별 [CClient](../../Server/Lobby_Server/CClient.cpp)는 별도 FIFO `SessionWorkQueue`를 갖는다. 전체 입력 중 선두 한 작업만 실제 실행 큐에 발행한다. 작업이 끝나면 **그때의 소유 채널**을 읽고 다음 작업을 발행한다.

```cpp
// 개념을 줄인 코드. 실제 구현은 LobbyDispatcher::Run/ScheduleLocked.
work.Execute();                    // 채널 또는 전역 소유 구간
workQueue.scheduled = false;
ScheduleNext(workQueue.currentChannel); // 이동이 끝난 뒤 목적지로 발행
```

채널 작업끼리는 실행 barrier를 공유로 획득하고, 전역 작업은 배타로 획득한다. 같은 채널 target의 Core 실행 잠금이 채널 내부 동시 실행을 막는다. 전역 작업이 채널의 가변 상태를 직접 읽고 바꾸는 기존 코드는 배타 구간에 들어가므로 여러 채널과 경합하지 않는다. 전역 작업의 비동기 서비스별 메시지 분리는 후속 최적화다.

| 채널 작업 | 전역 배타 작업 |
|---|---|
| 이동·회전·외형·RTT·채널 채팅 | 신규 접속·종료 정리·채널 이동·매칭 |
| DB 없는 일반 로그인 | DB 로그인·상점·거래·전체 채팅·P2P 기능 |
| 이동 타이머의 실제 갱신 | 게임 서버에서 온 전역 메시지·DB worker의 DB 실행 |

핸들러 분류는 [CPacketMgr::RequiresGlobal](../../Server/Lobby_Server/CPacketMgr.cpp)에 모았다. 읽기 전용 handler map은 const 조회를 사용한다. 공유 RNG와 채널 간 판매자 갱신도 전역 구간에 포함한다. 미분류 패킷은 기본적으로 전역 작업으로 보낸다.

## 설계 선택 이유

목표는 **같은 채널의 상태 변경을 직렬로 보호하면서, 한 유저가 채널/전역 큐를 오갈 때도 입력 순서를 유지하는 것**이다. 채널 큐는 공유 상태의 실행 소유권, SessionWorkQueue는 유저별 작업의 선후 관계를 담당한다. 다음 표의 대안은 설계 차이를 설명하며 별도 성능 대조를 수행했다는 뜻은 아니다.

| 선택 | 해결할 문제·단순한 대안의 한계 | 보장하는 규칙 | 비용·한계·근거 |
|---|---|---|---|
| 채널별 직렬 큐 | 이동/시야 갱신은 이웃 유저 상태도 읽고 바꾸므로 본인 객체 잠금만으로 부족하다. | 같은 채널 callback은 하나씩 실행하고 다른 채널은 병렬 실행한다. | 밀집 채널 하나의 연산은 분할하지 못한다. [LobbyDispatcher::Target/Run](../../Server/Lobby_Server/LobbyDispatcher.h), 기존 Core target 실행 정책 사용. |
| 유저별 SessionWorkQueue | 채널 큐와 전역 큐가 각각 FIFO여도 두 큐 사이 순서는 보장되지 않는다. 채널 이동 뒤 처리해야 할 입력이 이전 채널에 미리 쌓일 수 있다. | 선두 한 작업만 발행하고 완료 후 현재 채널을 읽어 다음 작업의 큐를 고른다. | 앞 작업이 느리면 그 유저의 후속 작업도 기다린다. [Post/ScheduleLocked/Run](../../Server/Lobby_Server/LobbyDispatcher.h). |
| SessionWorkQueue mutex | IOCP·타이머의 작업 등록과 worker의 꺼내기/재예약이 겹친다. 채널 실행 직렬화만으로 등록 스레드를 보호하지 못한다. | 목록·`m_scheduled`·`m_channel`을 함께 보호해 선두 중복 발행과 예약 누락을 막는다. | 등록/재예약 때 경합 비용이 있다. callback 실행은 SessionWorkQueue mutex 밖이다. `Post`, `SessionWorkQueue::SetChannel`, `Run`의 잠금 범위가 근거다. |
| 전역 배타 barrier | 매칭·채널 이동·전체 채팅 등은 여러 채널의 가변 상태에 접근한다. 전역 큐 하나만 직렬화해도 일반 채널 작업과 겹칠 수 있다. | 일반 채널은 공유로 진입하고 전역 작업은 배타로 진입해 채널 변경과 동시에 실행되지 않게 한다. | 긴 전역/DB 작업은 모든 채널을 지연시킨다. 이 잠금 자체가 큐 사이 FIFO나 공정성을 보장하지 않는다. [PostGlobal/Run/LockGlobal](../../Server/Lobby_Server/LobbyDispatcher.h). |
| 공통 worker pool | 채널마다 스레드를 만들면 채널 수에 비례해 스레드 수가 늘어난다. | 준비된 채널 target을 제한된 worker가 나눠 실행하며 target budget 뒤 기회를 넘긴다. | worker4가 최적이라는 성능 보장은 없다. 실제 밀집 재측정에서는 CPU·송신 적체가 증가했다. [CServer::Run](../../Server/Lobby_Server/CServer.cpp), [밀집 재측정](#채널당-인원-증가-재측정). |
| 연결 generation 검사 유지 | SessionWorkQueue의 입력 순서는 종료 뒤 재사용된 연결의 오래된 frame/타이머를 식별하지 못한다. | 실행 때 같은 연결 세대인지 검사해 이전 연결 작업을 적용하지 않는다. | generation은 채널 변경 번호가 아니다. 채널 변경의 순서는 SessionWorkQueue로 해결한다. [CClient::DispatchFrame/EnqueueMovementUpdate](../../Server/Lobby_Server/CClient.cpp). |

공유 barrier는 여기서 읽기 전용이라는 뜻이 아니다. 서로 다른 채널이 각자의 상태를 바꿀 수 있게 허용하며, 같은 채널의 쓰기는 채널 큐가 직렬로 제한한다. 전역 배타 구간은 그 모든 채널 실행을 배제한다.

### 채널 큐만 사용할 때의 실패 예시

유저가 `이동1 → A에서 B로 채널 변경 → 이동2`를 요청한다고 가정한다. 등록 때 현재 채널만 보고 바로 실행 큐에 넣으면 다음처럼 나뉜다.

```text
채널 A 큐: [이동1, 이동2]
전역 큐:   [채널 변경]
```

전역 큐와 A 큐 사이에는 실행 순서 계약이 없어 이동2가 채널 변경보다 먼저 처리될 수 있다. 배타 barrier는 동시 실행을 막지만 그 요청 순서를 복구하지는 않는다. 현재 SessionWorkQueue는 아래 순서를 만든다.

```text
SessionWorkQueue: [이동1, 채널 변경, 이동2]
이동1만 A 큐에 발행 → 완료
채널 변경만 전역 큐에 발행 → 완료, 소유 채널=B
이동2만 B 큐에 발행 → 완료
```

FIFO 기준은 **같은 SessionWorkQueue에 실제 등록된 순서**다. 서로 다른 유저 사이의 전체 순서나 IOCP·타이머의 벽시계 발생 순서까지 보장하지 않는다. 신규 접속/게임 서버 메시지처럼 `PostGlobal`에 직접 넣는 작업에는 유저 SessionWorkQueue의 선후 계약을 자동 적용하지 않는다.

### SessionWorkQueue lock과 실행 lock을 나눈 이유

`Post`에서 `push_back → m_scheduled 확인 → 선두 예약`을 같은 mutex 안에서 처리한다. 그렇지 않으면 두 등록 스레드가 모두 `m_scheduled=false`를 읽고 실행 envelope를 중복 예약할 수 있다. `Run`도 선두를 꺼내는 구간과 완료 후 다음 작업을 예약하는 구간을 같은 mutex로 보호한다.

**실제 callback 동안에는 SessionWorkQueue mutex를 잡지 않는다.** 후속 입력을 계속 등록할 수 있도록 목록 보호와 게임 상태 실행 보호를 나눈 것이다. 작업의 실제 상태 변경은 채널 직렬 실행과 공유/배타 barrier 안에서 수행한다. `m_scheduled`는 목록이 비었다는 표시가 아니라 선두가 이미 예약되었거나 실행 중이라는 표시다.

`std::deque` 자체는 이 설계의 필수 조건이 아니다. 현재 필요한 것은 뒤에 추가·앞에서 꺼내기·종료 후 비우기이며, 직접 `clear()`할 수 있는 편의 외에는 `std::queue`로도 같은 계약을 표현할 수 있다. 컨테이너 선택에 별도 성능 우위를 주장하지 않는다.

## 채널 이동·타이머·재접속

이 구현은 사전 검토안의 `routeEpoch` 방식 대신 **유저당 선두 한 작업 발행 + 전역 배타 이동**으로 같은 문제를 해결한다. 이전 채널에 후속 입력 여러 개를 직접 등록하지 않으므로, 전달 완료 뒤 새 입력이 이전 입력을 추월하지 않는다.

1. 채널 이동 명령 앞의 본인 입력을 완료한다.
2. 전역 배타 구간에서 목적지와 현재 상태를 확인한다. 매칭 중 이동은 기존 실패 응답으로 거절하고 같은 채널 요청은 성공하는 무변경 요청으로 처리한다.
3. 이전 ID로 출발 시야를 정리한 뒤 슬롯·ID·소유 채널을 변경한다. registry mutex 안에서는 송신하지 않는다.
4. 기존 채널 변경/로그인 응답을 보내고 목적지 시야를 구성한다.
5. 다음 SessionWorkQueue 작업은 새 소유 채널에서 실행한다. 타이머도 같은 SessionWorkQueue를 통과하므로 이동 중 예약 하나를 목적지에서 이어간다.

연결 generation은 계속 필요하다. 프레임과 타이머는 `SessionRef`와 generation을 보유하고 실제 실행 때 검사한다. 이미 종료된 연결의 예약은 새 연결에 적용하지 않는다. IOCP 수신의 decode/rearm도 같은 generation mutex 아래 실행해 정리의 `Invalidate`와 수신 잔여 버퍼 접근을 분리했다.

종료 completion은 원자적으로 연결 종료를 표시하고 정리 작업을 SessionWorkQueue에 한 번만 넣는다. DB 저장 snapshot·시야/매칭/섹션/슬롯 정리·객체 초기화는 전역 작업에서 실행한다. socket을 반환한 뒤 같은 객체에 새 접속이 들어와도 이전 SessionWorkQueue 작업과 새 입력의 순서는 유지되고 오래된 generation은 폐기된다. 객체는 전체 producer/worker 종료까지 생존한다.

서버 종료는 신규 작업/타이머 중단 → worker·IOCP join → 실행 큐/SessionWorkQueue 폐기 → transport 자원 정리 순서다. 실행 중 큐를 비우는 방식은 사용하지 않는다.

## 설정과 에이전트 조회

기본은 정원 4명·750채널·로직 worker 4개다. `LOCAL_TEST`에서만 다음 설정을 덮어쓸 수 있다. 정원/채널 수는 실험용이며 실제 클라이언트의 4명 배열 계약을 확장하지 않는다.

| 환경 변수 | 범위/동작 |
|---|---|
| `WOD_LOBBY_WORKERS` | 1~32, 기본 4 |
| `WOD_LOBBY_CHANNEL_CAPACITY` | 1~3000, 명시하면 기존 `WOD_PERF_UNCAPPED` 정원보다 우선 |
| `WOD_LOBBY_CHANNELS` | 1~`3000 / 정원`, 기본은 그 상한 |
| `WOD_PERF_UNCAPPED=1` | 정원을 따로 지정하지 않으면 이전처럼 3000명·1채널 |

예를 들어 32채널 × 정원4에서 같은 128명을 비교할 수 있다. 총 입장 상한은 3000 이하로 유지한다. 테스트에서 채널을 가득 채우는 것과 전체 서버 입장 상한은 구분한다.

```powershell
$env:WOD_LOBBY_WORKERS = '4'
$env:WOD_LOBBY_CHANNEL_CAPACITY = '4'
$env:WOD_LOBBY_CHANNELS = '32'
./scripts/Start-ServerLab.ps1 -Configuration Release -UncapAdmission
$state = Invoke-RestMethod http://127.0.0.1:8790/api/state
$state.servers | Where-Object role -eq LobbyServer | ForEach-Object { $_.admission.dispatcher }
./scripts/Stop-ServerLab.ps1
```

원본 reporter의 `content.dispatcher`는 monitor API·저장 samples에서 `admission.dispatcher`다. `schema_version=1`이며 모든 통계는 프로세스 시작부터 누적한다. 유저명·소켓 원본은 발행하지 않는다.

| 필드 | 의미 |
|---|---|
| `channel_count`, `logic_workers` | 전자는 dispatcher 안, 후자는 admission 안의 실제 실행 설정 |
| `max_parallel` | 동시 진입한 채널 callback 범위의 최대 개수. CPU 코어 이용률이 아님 |
| `owner_violations` | 같은 채널의 실행 소유권 중복 감지. 감지 시 worker failure로 중지 |
| `inbox_pending`, `global_pending`, 채널 `pending` | 아직 시작하지 않은 작업의 근사 순간값. 전역 직접 작업과 유저 SessionWorkQueue의 집계 범위는 다름 |
| `global_jobs`, 채널 `jobs` | wrapper callback 완료 수. 유효한 비즈니스 패킷 처리량은 기존 `packets_processed` 사용 |
| `wait_us`, `max_wait_us` | SessionWorkQueue 게시부터 실행 barrier 획득 후까지의 경과 대기. 원자 누적/최댓값 |
| `busy_us`, `global_busy_us` | callback의 경과 시간. 송신/세션 잠금 대기 포함, CPU cycle과 다름 |
| `timing_enabled` | 기존 JobMetrics 계측 활성 상태. false이면 시간 값 0은 미수집으로 해석 |

채널 목록은 작업이 완료된 적 있는 채널만 포함하고 최대 구성 채널 수로 제한된다. reporter는 원자 카운터만 읽으며 가변 유저 상태를 순회하지 않는다. 전역/채널 대기와 실행은 작업별 시간을 합산하므로 벽시계 시간과 직접 나눠 CPU 사용률로 해석하지 않는다.

`inbox_pending`은 기존 관측 JSON/저장 결과와의 호환을 위해 남긴 키 이름이다. 현재 `SessionWorkQueue`의 미시작 작업을 뜻하며 이름 변경으로 schema나 계수 의미를 바꾸지 않았다.

`global_*` 시간/작업 수는 전역 target callback 범위다. 선택 DB worker가 직접 획득하는 배타 구간의 시간은 여기에 포함되지 않으므로 DB 활성 성능 검사는 별도 DB 계측이 필요하다.

## 재현 명령

```powershell
./scripts/Build.ps1 -Module Servers -Configuration Release
./scripts/Test-ServerCore.ps1 -Configuration Release
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Release -Python <python.exe>
./scripts/Start-Local.ps1 -Configuration Release -ServersOnly
<python.exe> tools/server_lab/tests/lobby_channels.py --output artifacts/logs/lobby-channels.json
<python.exe> tools/server_lab/tests/lobby_movement.py --output artifacts/logs/lobby-channel-movement.json
./scripts/Stop-Local.ps1
<python.exe> scripts/Measure-LobbyChannels.py --channels 32 --capacity 4 --clients 128 --workers 1 4 --repeats 3 --output artifacts/logs/lobby-channel-comparison
```

실행 설정을 바꾸는 부하 비교는 매번 서버를 재시작한다. seed·시나리오·실행 파일/소스 hash를 보존하고 반복마다 worker 순서를 바꾼다. 같은 분포 안에서만 비교하며 한 채널 128명과 32채널 4명의 결과를 worker 효과로 섞지 않는다. 원본 결과와 단계별 실패도 보존한다.

## 검증 결과와 남은 한계

Release 전체 프로젝트·Debug 서버 빌드, Release/Debug Core 각 **1,686검사**와 ABI 110종/106상수, Python 49검사에 통과했다. 빌드에는 기존 형변환/미사용 변수 경고 `C4101/C4244/C4305/C4309`가 남아 있다. 신규 dispatcher와 제어 동시성 검사의 성공을 경고 전체 해소로 해석하지 않는다.

제어된 C++ 검사에서 서로 다른 채널 병렬 실행·같은 채널 배제·전역 barrier 배제·200작업의 채널 이동 FIFO·종료 후 게시 거부·대기 작업 캡처 반환을 확인했다. 실제 네트워크 회귀 결과는 다음과 같다.

| 검사 | 결과 |
|---|---|
| Release 기존 통합 | 네트워크12종, 보스4/5의 4인 전환, 초기화 실패4종 통과 |
| Release/Debug 신규 채널 검사 | 각9항목 통과: 잘못된/가득 찬/같은 채널, 종료 ID, 이동 중13회 전달/정지, 다중 채널200입력, 전체 채팅, DB 없는 요청, 매칭 취소/재등록/중복/종료, 역할 검증, legacy dummy 정책 |
| Release/Debug 기존 이동 검사 | 분할 입력·본인/주변 정지 일치,200입력 FIFO, 이동 중재접속10회 통과 |
| 정원1·채널1 입장 실패 | 미배정 연결 종료, 기존 유저 생존, 종료 후 슬롯 재사용 통과 |
| 종료 | 모든 부하 실행의 양쪽 서버 `pending=0 sockets=0 leased=0`, 정상 종료 |

### 같은 분포에서 worker 비교

정원4·32채널에 128명, 250ms마다 이동/정지, 5초 워밍업·20초 측정, 계측 ON·headless 관측, 매회 재시작 조건이다. CPU는 논리 코어12개로 정규화한 프로세스 사용률이며 percentile은 누적 histogram 차분의 **버킷 상한**이다.

| 지표 | worker1 / worker4 · 각3회 중앙값 | 최종 실행 파일 · 각1회 대조 |
|---|---:|---:|
| 패킷 대기 p99 상한 | 65.536 / 16.384ms | 65.536 / 16.384ms |
| CPU | 5.23 / 7.32% | 5.24 / 7.04% |
| 실제 처리 패킷/s | 513.46 / 507.46 | 506.76 / 514.28 |
| 타이머 dispatch 대기 p99 상한 | 32.768 / 16.384ms | 별도 원본 보존 |
| 이동 갱신 간격 p99 상한 | 65.536 / 65.536ms | 별도 원본 보존 |
| 최대 동시 채널 callback / 소유권 위반 | 1·0건 / 4·0건 | 동일 |

3회 CPU 범위는 worker1 4.85~5.30%, worker4 6.45~8.74%다. 대기는 줄었지만 CPU는 증가했다. 갱신 간격 p99의 버킷은 같으므로 화면 품질이나 66.7Hz 보장으로 해석하지 않는다. 입력은 초당 약512패킷으로 고정돼 있고 PPS 비교 방향도 대조 실행에서 바뀌므로 **최대 처리량 증가를 입증한 측정은 아니다.** 양쪽 모두 기능 assertion과 최종 정지 검사를 통과했다.

반복 당시 jobs/s 중앙값은 3,076.57 / 3,157.96으로 비슷하다. CPU 증가를 갱신 작업 증가만으로 설명할 수 없으며 worker 깨움/전역 송신 잠금 등의 비용 분해는 후속이다. 계측 OFF 각1회에서도 CPU 5.14 / 8.06%로 같은 증가 방향을 보였다. OFF 반복 수가 적고 ON도 분산이 있으므로 정확한 계측 오버헤드 비율은 확정하지 않는다. 부하 생성기 CPU는 ON 반복에서 0.205~0.269코어로 관측했다.

3회 반복 뒤 채널 이동의 legacy dummy 전송 플래그를 유지하도록 `ProcessUpdate()` 호출을 `Move()`로 바꾼 후 양쪽 구성의 채널 회귀와 최종1회 대조를 추가했다. 일반 `perf-lobby`는 채널 이동을 하지 않으며 그 이동 알고리즘/큐 정책은 이 마지막 수정에서 바꾸지 않았다. 표의 두 묶음은 실행 파일 hash가 달라 합산하지 않는다. 최종 Release 로비 EXE SHA-256은 `a239a939091b085d21f86ced9fd429316ddbf9b6f0263853260a0ed7edd74a9c`다.

원시 결과는 Git 제외 `artifacts/logs/lobby-channel-final-comparison-20261018/`(ON6회), `lobby-channel-final-metrics-off-20261018/`(OFF2회), `lobby-channel-final-verification-20261018/`(최종2회)의 metadata/source-snapshot/summary와 각 run report·원본 samples·종료 로그에 보존한다. 각 묶음의 실행 중 소스/실행 파일 hash는 유지됐다. 초기 도구 집계 키 수정과 개발 중 소스 변경 감지로 끝난 예비 실행은 이 표에서 제외했다.

기능 결과는 `artifacts/logs/lobby-channels-{release,debug}-20261018.json`, 기존 이동은 `lobby-channels-movement-{release,debug}-20261018.json`, 입장 실패는 `lobby-admission-release-20261018.json`, 기존 통합은 `server-core-integration-Release/summary.json`이다. Archify 구현/제안 구조도 모두 showcase9/9·오류/경고0이며 실제 브라우저 레이아웃은 기존 접근 제한 때문에 미수행이다.

최종 UTF-8·Python AST·프로젝트 XML·문서35링크·구조도 원본/HTML hash·최종 실행/소스 hash·diff 검사를 통과했다. 종합 확인은 Git 제외 `artifacts/logs/lobby-channel-implementation-verification-20261018.json`에 보존한다. 일반 `job_wait_us`는 채널 target에 발행한 envelope 대기이므로 유저 SessionWorkQueue 전체 대기는 기존 `packet_job_wait_us` 또는 dispatcher의 `wait_us`로 확인한다.

측정 뒤 CPacketMgr/LobbyDispatcher의 분기 뒤 빈 줄과 C++검사 파일 포맷을 정리했다. 실행 파일 hash는 유지하며, 해당 두 런타임 소스는 보존한 측정 snapshot과 공백을 제외한 내용이 동일함을 검사했다. 이 정리 뒤 부하 측정을 다시 수행했다고 주장하지 않는다.

후검증 시점의 전체 구현 diff와 미추적 C++/Python 소스도 `artifacts/logs/lobby-channel-implementation-source-20261018/`에 별도 보존했다. 이는 측정 중 수집한 hash 목록을 소급 확장한 것이 아니며, 기존 commit에서 Shared 검증 변경과 신규 파일까지 복구하기 위한 보조 자료다.

추가 대응은 전역 기능의 오래 걸리는 I/O 분리, 전송 서비스 공용 잠금/worker CPU의 비용 분해, worker2를 포함한 튜닝, 채널별 공간 occupancy, SessionWorkQueue/전역 큐의 상한·backpressure다. `WITH_DATABASE`·운영 DB/블록체인·전체 GUI 플레이·race detector·최대 동접은 미검증이다. 단일 밀집 채널 내부는 여전히 직렬이며 이번 구조만으로 병렬 가속되지 않는다.

## 채널당 인원 증가 재측정

기존4명/채널 결과를 밀집 조건의 개선 효과로 확대할 수 없어 같은 개선 코드에서 채널 정원을 늘려 비교한다. 현재 소스를 Release 서버로 다시 빌드했으며 기존4명/채널 실행 파일과 결과는 합산하지 않는다. 로직은 변경하지 않았고 서버 소스의 차이는 앞서 기록한 공백 정리다. 실제 클라이언트의4명 배열 계약을 확장한 검증은 아니다.

```powershell
<python.exe> scripts/Measure-LobbyChannels.py --channels 4 --capacity 32 --clients 128 --workers 1 4 --repeats 3 --output artifacts/logs/lobby-density-c4-p32-20261018
<python.exe> scripts/Measure-LobbyChannels.py --channels 4 --capacity 64 --clients 256 --workers 1 4 --repeats 3 --output artifacts/logs/lobby-density-c4-p64-20261018
<python.exe> scripts/Measure-LobbyChannels.py --channels 4 --capacity 128 --clients 512 --workers 1 4 --repeats 3 --output artifacts/logs/lobby-density-c4-p128-20261018
<python.exe> scripts/Measure-LobbyChannels.py --channels 1 --capacity 128 --clients 128 --workers 1 4 --repeats 3 --output artifacts/logs/lobby-density-c1-p128-20261018
```

위 명령은 계획한 조건이며 실제 수행 여부·중단·반복 수는 아래 결과와 각 `summary.json`으로 구분한다. 모든 조건은 5초 워밍업/20초 측정/250ms 입력, LOCAL_TEST·DB없음·계측ON·headless·매회 재시작이다. 반복 순서는1/4→4/1→1/4, seed1~3이며 기존 비교와 같은 각3회를 밀집 탐색에 사용했다(일반 목표5회의 예외). `generator_limit`·기능 실패가 발생하면 후속 증원을 중단한다.

더미의 실제 `SC_CHANGE_CHANNEL` 성공 응답을 `peers[].lobby_channel`로 보존하고, 측정 도구의 `channel_population`에서 채널별 실제 인원이 예상과 일치하는지 검증한다. 발행 p99>250ms 또는 관측/더미 프로세스CPU≥0.9코어는 생성기 한계이며 서버 동접 한계와 구분한다. 기능 실패·입장 분포 불일치·정상 종료 실패도 결과에서 제외하지 않는다. 이 도구 보완의 소켓 검사 포함 Python50개가 통과했다.

### 실제 결과

| 분포 | 실행 worker1/4 | 패킷 대기 p99 상한 ms · worker1/4 | CPU % · worker1/4 | 송신 대기 표본 최고 · worker1/4 |
|---|---|---:|---:|---:|
| 4채널×32명=128명 | 각3회 모두 통과 | 131.072 / 131.072 | 8.80 / 14.23 | 31 / 1,047 |
| 4채널×64명=256명 | 각3회 모두 통과 | 524.288 / 524.288 | 11.82 / 22.82 | 65 / 1,261,391 |
| 4채널×128명=512명 | 1회 통과 / 2회 중1회 실패, 중단 | 2,097.152 / 524.288~2,097.152 | 12.09 / 18.59~33.10 | 123 / 5,540,022 |
| 1채널×128명=128명 | 각3회 모두 통과 | 262.144 / 262.144 | 12.55 / 12.80 | 127 / 133 |

완료된3회 묶음은 실행별 중앙값이며 최고 송신 대기는 반복 전체의 표본 최댓값이다. **512명은 계획6회 중3회에서 중단한 진단이며, worker4 값은 실패를 포함한2회의 범위다.** 불균등한 표본으로 worker 효과를 판정하지 않는다. 21회 중20회 검증 통과·1회 실패이며 실패를 제외한 성과만 제시하지 않는다. 인원/채널을 모두 바꾼 행끼리 worker 개선 효과를 비교하지 않는다.

이동 갱신 간격 p99 상한의 중앙값도32명/채널131.072ms,64명/채널524.288ms,단일128명262.144ms로 두 worker 설정에서 같았다. 단일128명은 worker를4개로 설정해도 최대 동시 채널 callback이1개였고,4채널에서는1→4개였다. 모든21회에서 소유권 위반0건·실제 배정 분포 일치를 확인했다. 이는 다른 공유 데이터의 race가 없다는 증명은 아니다.

64명/채널·worker4에서는 송신 대기가 반복별646 / 478,788 / 1,261,391건까지 관측됐다. 마지막 측정 표본에도267 / 478,788 / 1,261,391건이 남았다. 더미별 이동·최종 정지 통과가 전체 방송 프레임의 지연이나 송신 큐 해소를 보장하지 않는다. 따라서256명을 안정적인 수용 인원으로 확정할 수 없다. CPU 증가와 전송 적체의 세부 원인, 서버/OS/더미 수신 경로별 비용 분해는 미수행이다.

512명·worker4의 두 번째 실행은 **120명 최종 정지 관찰 timeout**, 마지막 표본 송신 대기5,540,022건·로비 private bytes 최고3,158,249,472bytes(3.16GB)였다. 정상 종료도15초 제한을 초과했다. `ServerCore stop pending=0 sockets=0 leased=0` 로그가 있어도 프로세스 정상 종료 실패를 성공으로 바꾸지 않는다. 해당 조건의 나머지3회와 추가 증원은 중단했다. 이후 총인원을128명으로 낮춘 단일 채널 대조만 진행했다.

생성기의 발행 p99는 전체 실행에서0~140ms, 관측/더미 프로세스CPU는0.562~0.887코어로 이번 생성기 중단 기준을 넘지 않았다. 동일 PC의 수신/관측 영향을 완전히 배제한 것은 아니며 전용 부하 발생기와 장시간 안정성은 후속이다. 정상 종료는20/21회이며 검사 프로세스는 모두 정리됐다. 실패 원본·중단된 반복 수·종료 오류는 각 run report와 로그에 보존한다.

추가 측정의 로비 EXE SHA-256은 `45c7b6b8e76078382700821f7a316c1f8db798dadab2033e1dc70dec5cff537a`다. 네 분포 묶음은 같은73개 실행 파일/로비 소스/도구/시나리오 fingerprint를 유지했다. 공백 정리 전의 기존4명/채널 결과와 실행 파일 hash가 다르며 합산하지 않는다. 환경은 Ryzen5 7500F(6코어/12논리 코어),Windows11,균형 전원이며 구성·Python·working.diff와 소스 snapshot은 각 metadata에 보존한다.

다음 대응은 채널 내 시야/방송 비용, 송신 대기/bytes 상한과 backpressure, 중간 이동 상태의 병합, 전체 송신 drain 판정, 대량 취소/반환과 프로세스 종료 시간의 분해다. 이번 재측정에서는 서버 로직을 수정하거나 timeout을 완화하지 않았다.
