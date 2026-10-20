# 로비 채널 병렬화 사전 검토

2026-10-18 · 기준 `main@54af931c` · 작업 브랜치 `codex/lobby-channel-parallelism`

이 문서는 수정 전 사전 검토 기록이다. 후속 [구현·수정·검증 현황](LOBBY_CHANNEL_PARALLELISM.md)을 별도로 관리한다. 아래의 현재/미구현 표현은 검토 당시 기준이며 이후의 적용 상태를 뜻하지 않는다.

**채널별 직렬 큐를 제한된 worker pool에서 병렬 실행하는 방향을 권장한다. 현재 유저 큐의 소비자 수만 늘리면 안전하지 않다.** 한 채널의 이동·시야·가입·탈퇴는 같은 실행 소유자가 처리하고, 전역 매칭과 채널 이동은 별도 조정 절차를 거쳐야 한다.

이번 단계는 검토다. 런타임 소스·worker 수·프로토콜은 변경하지 않았다. [소유권 제안 구조도](../diagrams/lobby-channel-parallelism/README.md)는 미구현 설계다.

## 먼저 해결할 문제

| 우선순위 | 발견 내용 | 근거 수준 | 대응 |
|---|---|---|---|
| 1 | 종료한 유저의 제거 패킷 ID가 `-1` | 현재 단일 worker에서 실제 수신 | 초기화 전에 기존 ID와 관찰자를 확정하고 시야에서 제거 |
| 1 | 채널 이동 시 제거 패킷이 기존 관찰자 대신 이동한 본인에게 전달 | 현재 단일 worker에서 실제 수신 | 기존 관찰자의 sender 사용, 출발 채널에서 시야 정리 |
| 1 | 접속 초기화와 종료가 같은 채널 슬롯을 서로 다른 잠금 규칙으로 수정 | 정적 경로 확인 | 초기화·종료도 소유 채널 작업으로 전달 |
| 1 | 음수 채널 번호를 검사하지 않고 배열 접근 | 정적 경로 확인 | 목적지 하한·상한·현재 채널·가득 참 검사 |
| 2 | 서로 다른 유저 큐가 같은 채널의 좌표·외형·시야를 접근 | worker 추가 시 충돌 경로 | 유저별 실행 잠금 위에 채널 직렬 실행 경계 도입 |
| 2 | 연결 generation은 채널 변경을 식별하지 못함 | 타이머/채널 이동 경로 확인 | 소유 채널과 `routeEpoch`, 이전 작업 차단·전달 절차 |
| 2 | 전역 매칭 큐·인원·매치 번호의 복합 연산에 직렬화 없음 | worker 추가 시 충돌 경로 | 전역 매칭 조정자에서 참가자 예약과 번호 할당 |
| 3 | 공유 RNG·채널 간 거래·선택 DB/블록체인 경로 | 정적 검토, 일부 비활성 | 소유자 메시지/전용 서비스로 분리 후 기능별 검증 |

## 현재 실행 경계

[CServer::Run](../../Server/Lobby_Server/CServer.cpp)은 IOCP 4개, 타이머 1개, 전역 `GPacketJobQueue->ProcessJob()` 소비자 1개를 시작한다. `WITH_DATABASE`일 때 DB worker가 추가되며 계측 reporter도 별도로 동작한다. 로직 소비자는 하나지만 IOCP 초기화까지 같은 스레드에서 실행되는 것은 아니다.

```text
현재: IOCP → 각 유저 JobQueue → 전역 로직 worker 1개
      타이머 ────────────────┘
      IOCP Accept → 유저 초기화·채널 슬롯 등록  ← 별도 쓰기 경로

제안: IOCP/타이머/가입·종료 → 소유 채널 큐 → 제한된 worker pool
                                      채널 A는 직렬, A와 B는 병렬
```

[Core JobQueue](../../Server/ServerCore/include/ServerCore/Concurrency.h)는 `m_execute`로 **각 큐**의 실행을 보호한다. 이 보호는 존재하지만 이웃 유저나 채널 전체를 보호하지는 않는다. 여러 소비자가 서로 다른 유저 큐를 실행하면 같은 채널의 유저 상태를 동시에 읽고 쓸 수 있다. `JobScheduler`의 enqueue 중복 방지도 채널 실행 소유권과 다르다.

또한 현재 scheduler는 끊어진 target의 큐를 `Clear`한 뒤 그 실행 잠금 바깥에서 `OnJobQueueDisconnected`를 호출한다. 병렬화할 때 이 정리도 채널 소유 작업으로 옮겨야 한다. 유저 하나의 종료를 채널 전체 target의 종료로 취급해서는 안 된다.

## 공유 데이터와 필요한 소유권

| 상태 | 현재 보호와 한계 | 제안 소유자 |
|---|---|---|
| 채널 슬롯 `m_channelClient` | 등록/이동/조회는 전역 mutex, `ClientReset`의 제거는 잠금 없음 | 채널 등록 조정자 + 채널 소유 작업 |
| 유저 위치·방향·외형·이동 예약 | 본인 큐는 직렬이나 시야에서 이웃의 가변 상태 접근 | 해당 채널 |
| 시야 집합 | 집합 mutex만 존재. 상대 좌표/ID/외형의 일관성까지 보장하지 않음 | 해당 채널 |
| 유저 객체 목록 | concurrent map, 객체는 미리 생성되어 종료 때까지 유지. 값의 가변 필드는 별도 문제 | 목록 수명은 서버, 활성 상태는 채널 |
| 매칭 큐·취소·참가자·매치 번호 | 일반 priority_queue/카운터, 복합 트랜잭션 없음 | 전역 매칭 서비스 |
| 전체 채팅·경매 판매자 갱신 | 다른 채널의 유저 포인터를 직접 순회/갱신 | 불변 메시지와 목적지 채널 명령 |
| 상점 난수 `m_randomEngine` | 전역 `CPacketMgr`의 가변 engine | 채널별 engine 또는 직렬 상점 서비스 |
| NavMesh·HeightMesh·공간 인덱스 | 스레드 시작 전 로드, 이후 조회 | 불변 공유 데이터 유지 |
| `GameUtil::m_sections` | 셀별 mutex 있으나 채널 구분 없음, 활성 제거 호출 없음 | 사용 전 채널별 occupancy로 분리 |
| 전송 큐·풀 | Core 내부 잠금 존재, 클라이언트 논리 수명까지 보장하지 않음 | 공유 transport 유지 + 유효 수신자 handle |
| 계측 | 원자 집계/히스토그램 잠금, TickProfile의 현재 frame은 thread_local | 공유 집계 + 소유자가 발행한 채널 snapshot |

`GetChannelClients`는 mutex 아래 **포인터 목록을 복사**한다. 이후 그 포인터로 읽는 위치·상태·외형은 snapshot이 아니다. `m_stateLock`으로 상태만 확인한 뒤 잠금을 풀고 좌표를 읽는 [CViewList::CheckViewList](../../Server/Lobby_Server/CViewList.cpp)도 채널 병렬화의 보호 경계가 될 수 없다.

정적 지형을 채널마다 복제할 필요는 없다. 반대로 `m_sections`의 현재 셀에는 여러 채널의 socket ID가 섞일 수 있다. 현재 시야 탐색은 이 섹션을 사용하지 않으므로 이를 현재 시야 오표시의 원인으로 단정하지 않는다.

## 실제 재현한 기존 결함

Release `LOCAL_TEST`, DB 없는 실행, 기본 채널 정원 4명, 기존 로직 worker 1개에서 TCP 클라이언트 2개로 확인했다. GUI 화면 재현이나 race detector 결과가 아니다.

| 검사 | 기대 | 실제 수신 |
|---|---|---|
| 같은 채널의 ID 1 유저 종료 후 관찰자 이동 | 관찰자가 ID 1 제거 수신 | 관찰자가 `SC_REMOVE_PLAYER.id = -1` 수신 |
| ID 1 유저가 채널 0→1 이동 | 출발 채널 관찰자가 ID 1 제거 수신 | 관찰자 수신 없음, 이동한 본인이 ID 1 제거 수신 |

종료 원인은 [CClient::Reset](../../Server/Lobby_Server/CClient.cpp)이 본인 시야만 비우고 ID를 `-1`로 바꾼 뒤에도 상대 시야에는 socket 참조가 남기 때문이다. 상대의 다음 시야 계산이 재조회한 ID는 이미 `-1`이며, [SendRemovePlayerPacket](../../Server/Lobby_Server/CPacketSender.cpp)은 이를 wire ID로 보낸다.

```text
B 종료 → B.id = -1 → A의 시야에는 B.socket이 남음
       → A 이동 → B의 현재 ID 조회 → REMOVE(-1)
```

[클라이언트 RemovePlayerPacket](../../Client/WarOfDimension/NetworkManager.cpp)은 받은 ID로 배열을 바로 인덱싱하므로 범위 밖 접근 위험도 있다. 실제 GUI 충돌은 검사하지 않았다. 서버가 올바른 이전 ID를 보내도록 수정하고 클라이언트의 범위 검사도 후속 방어 항목으로 둔다.

채널 이동의 수신자 오류는 [CUserMgr::ChangeChannel](../../Server/Lobby_Server/CUserMgr.cpp)에 있다. 아래는 문제 지점을 줄인 코드다.

```cpp
observer->GetViewList()->DeleteFromView(
    movingClient->GetID(), movingClient->GetSocketID(),
    movingClient->GetPacketSender()); // 제거 수신자는 observer여야 함
```

이 코드는 관찰자의 시야에서는 이동자를 삭제하면서 패킷은 이동자에게 보낸다. 이후 관찰자 시야에 제거 대상 자체가 없으므로 나중 계산으로도 제거 통지가 복구되지 않는다. 두 결함 모두 새 병렬화 때문에 생긴 문제가 아니다.

## 채널 이동·연결 수명 계약

연결 `generation`은 종료 후 재사용을 구분한다. 연결을 유지하는 채널 이동에서는 바뀌지 않으므로 이전 채널의 예약 이동 작업도 generation 검사에 통과한다. `ClientHandle(SessionRef, generation)`과 별도로 소유 채널/`routeEpoch`가 필요하다. 매칭 취소·재등록을 구분하는 ticket도 연결 generation과 다른 개념이다.

권장 전달 순서는 다음과 같다. 아직 구현하지 않았다.

1. 목적지 범위·현재 상태·정원·같은 채널 요청을 검사하고 슬롯을 예약한다.
2. 출발 채널의 경계 작업 이전 입력을 완료한다. 이동 중 새 입력은 연결 순서대로 보관한다.
3. **이전 ID·generation·관찰자**를 고정한 뒤 출발 시야/섹션을 정리하고 올바른 수신자에게 제거를 보낸다.
4. 실행 소유권을 목적지로 넘긴 뒤 ID/채널을 설정하고 새 `routeEpoch`를 발행한다. 양쪽이 동시에 객체를 수정하지 않는다.
5. 목적지에서 시야를 만들고 기존 로그인/채널 응답을 보낸 후 보관한 입력을 순서대로 처리한다.
6. 이동 중이면 이동 타이머를 한 번만 이어간다. 실패·접속 종료·서버 종료 시 예약과 보관 입력을 정리한다.

오래된 epoch 작업을 버리기만 하면 `m_updateScheduled=true`가 남아 이동이 멈출 수 있다. 소유권 전달 시 예약 플래그의 무효화·재등록도 함께 설계해야 한다. 두 채널 실행 잠금이나 두 세션 generation 잠금을 중첩해서 잡는 방식 대신 메시지와 단계별 응답으로 전달한다.

매칭 중 채널 이동을 허용할지는 구현 전 확정할 정책이다. 초기 구현에서는 이동을 거절하거나, 매칭 ticket 취소가 확정된 뒤 이동하는 방식 중 하나가 필요하다. 사용 중인 wire 계약을 임의로 바꾸지 않는다.

## 전역 서비스와 부가 경로

- **매칭:** [CMatchMgr](../../Server/Lobby_Server/CMatchMgr.cpp)의 등록→가능 여부→일반 3명/보스 1명 선택→번호 할당은 전체가 직렬화되어야 한다. 각 함수에 mutex를 따로 넣어도 참가자 예약의 원자성이 생기지 않는다. 취소는 인원만 줄이고 큐 항목을 남기므로 취소/재등록·재접속의 stale/중복 항목과 빈 큐 `top()` 경로도 정리해야 한다. 이 경로는 정적 발견이며 이번에 매칭 취소 오류를 실재현한 것은 아니다.
- **다른 채널 유저:** [CPacketMgr](../../Server/Lobby_Server/CPacketMgr.cpp)의 전체 채팅·매칭 정보 조회·경매 판매자 `SetTokenNum`은 채널 소유권을 넘어간다. 불변 발신 payload와 generation을 포함한 수신자 handle, 대상 채널의 갱신 명령으로 바꿔야 한다.
- **가입/정리:** IOCP `InitializeClient`와 로직 `ClientReset`의 슬롯 접근 규칙을 통일한다. 등록 실패를 현재 void 경로가 명시적으로 반환하지 않아 채널 미배정 연결이 계속 수신할 수 있다. 공개 전에 메타데이터를 완성해야 하며 `RegisterPeer`의 IP 설정도 현재 수신 시작 뒤 수행된다.
- **입력 검사:** `CS_CHANGE_CHANNEL`은 형식 검사는 있으나 음수 값 검사가 빠져 있다. 잘못된 채널·매칭 역할 등의 값 검증을 먼저 보완한다. 음수 요청은 실제 서버에 전송하지 않았다.
- **선택 DB:** DB worker와 동기 핸들러가 같은 `CDataBase::m_hdbc`를 사용한다. statement는 각 호출에서 만들어도 연결/트랜잭션 소유권은 분리되지 않는다. DB 서비스 직렬화 또는 연결 분리와 결과의 generation/request 검증이 필요하다. `WITH_DATABASE` 분기의 오래된 식별자와 DB 없는 모드의 일부 핸들러 null 접근 가능성도 별도 점검해야 한다. 이번 빌드/실행은 DB 분기를 검증하지 않았다.
- **P2P/블록체인:** 내부 자료구조 잠금과 유저 상태 소유권은 별개다. 트랜잭션 mutex 안의 파일 출력·전체 유저 전송은 병렬 채널을 막을 수 있다. validator 선택 후 벡터 접근의 잠금 불일치는 실제 선택 호출이 주석인 잠재 경로로 구분한다.
- **Core 세션:** IOCP callback의 socket별 잠금, `WithGeneration`, 큐 실행 잠금은 서로 다른 범위다. 외부 유저가 보내는 동안의 세션 재사용, 수신 잔여 버퍼와 `Invalidate`의 교차 경로는 수명 회귀에서 확인해야 한다. 이번에 동적 race로 재현하지 않았다.
- **Core 송신:** [Net.cpp](../../Server/ServerCore/src/Net.cpp)의 서비스 공용 Send mutex는 현재 전송 순서/소유권을 보호한다. 채널 병렬화 뒤 경합 후보지만 이번에 비용을 측정하지 않았다. 채널별 transport 복제나 잠금 제거보다 잠금 대기 계측을 먼저 한다.

## 구현 순서와 검증 조건

| 단계 | 작업 | 완료 조건 |
|---|---|---|
| 1 | 종료/이동 시야 결함·입력 검증·가입 실패 정리 | 이전 ID/정확한 관찰자 제거, 잘못된 요청 거부, 슬롯 중복/누수 없음 |
| 2 | 채널 실행 큐·라우팅·연결/채널 수명 전달 | 같은 채널 실행 겹침 0, 다른 채널 병렬 실행, 유저 입력 순서 보존 |
| 3 | 전역 매칭·채널 간 작업을 조정자 메시지로 전환 | 중복 없는 4명 매칭, 고유 번호, 취소/재접속 stale 결과 폐기 |
| 4 | 제한된 worker 활성화·동일 조건 비교 | 정확성 회귀 통과 후 처리량/대기/CPU 비교 |

회귀 시나리오에는 두 채널 동시 이동·회전·외형 변경, 타이머/패킷 대기 중 왕복 이동, 목적지 가득 참·같은 채널·잘못된 범위, 전달 중 종료·재접속·서버 종료를 포함한다. 이전 세대/epoch 작업이 새 소유자 상태를 수정하지 않고 활성 타이머가 최대 1개인지 확인한다. 제어 가능한 barrier로 경계 순서를 검사하며 sleep만으로 안전성을 판단하지 않는다.

매칭은 여러 채널에서 동시에 3+1 등록, 취소/재등록, 연결 종료/재사용을 검사한다. DB/거래 결과는 오래된 요청 폐기와 DB 없는 모드의 기능 거부도 검사한다. GUI 전체 품질과 운영 DB 검증은 별도다.

현재 계측에는 채널/worker별 귀속이 부족하다. 구현 시 채널별 queue wait/depth/oldest·jobs/s·인원, worker busy/CPU, 실행 소유권 위반, 타이머 due→enqueue→run, 전달 시간/보관 입력/오래된 route 폐기, 전역 서비스 대기, Send 잠금 대기를 추가한다. reporter가 유저 가변 객체를 직접 순회하지 않고 소유자가 발행한 snapshot을 읽도록 한다.

## 성능 비교에서 주의할 조건

[pch.h](../../Server/Lobby_Server/pch.h)의 기본값은 정원 4명·채널 750개다. `WOD_PERF_UNCAPPED=1`은 정원을 3000으로 올리면서 **채널을 1개로 줄인다.** 기존 제한 해제 측정은 다중 채널 확장성 검증이 아니다. 채널 수·채널별 정원·총 입장 제한의 테스트 설정을 먼저 독립시켜야 한다. 채널마다 물리 스레드를 만들지는 않는다.

| 비교 부하 예시 | 고정 조건 | 비교할 worker |
|---|---|---|
| 기본 작은 채널 | 32채널 × 4명 = 128명 | 1 / 2 / 4 |
| 밀집 채널 | 별도 테스트 설정 4채널 × 32명 = 128명 | 1 / 2 / 4 |
| 단일 채널 집중 | 별도 테스트 설정 1채널 × 128명 | 1 / 2 / 4 |

각 행 내부에서 입력 주기·위치·시야 fanout·워밍업/측정 시간·계측을 같게 유지한다. 1채널 128명과 32채널 4명을 직접 비교하면 시야 전송량도 달라지므로 worker 효과로 해석할 수 없다. 단일 채널은 채널 직렬 실행 설계상 병렬 가속을 기대하지 않는다. 실제 처리량·p99·tick/타이머 지연·CPU와 부하 생성기 포화를 함께 확인한다.

## 이번 검증과 한계

- `scripts/Build.ps1 -Module Servers -Configuration Release`: 성공. 기존 소스 전체의 경고 해소나 `WITH_DATABASE` 빌드 성공을 의미하지 않는다.
- `Start-Local.ps1 -Configuration Release -ServersOnly` → 임시 TCP probe → `Stop-Local.ps1`: 두 제거 결함 관측, 양쪽 서버 `pending=0 sockets=0 leased=0` 종료. 테스트는 기본 정원 4, `WOD_PERF_UNCAPPED` 해제 상태다.
- 재현 결과: Git 제외 `artifacts/logs/lobby-channel-review-probes-20261018.json`. probe는 `.runtime/review_lobby_channels.py`에 보존했다. 로비 EXE SHA-256은 `f433da5b26146ee5a6817af8f793f4e12c96c7b22d7f2d85ec1f3231a54cd530`이다.
- 제안 구조도: Archify showcase 9/9, 오류/경고 0. 이전 로컬 파일 접근 제한을 우회하지 않았으며 브라우저의 실제 배치/가독성 검사는 수행하지 않았다.
- UTF-8·문서 링크 21개·런타임 소스 변경 0·재현 hash·구조도 원본/산출물 hash·`git diff --check` 통과. 확인 결과는 Git 제외 `artifacts/logs/lobby-channel-review-verification-20261018.json`이다.
- 다중 worker 실행·race detector·새 성능 측정·전체 GUI 플레이·운영 DB/블록체인은 미검증이다. 이번 관측은 기존 단일 worker의 기능 결함 근거이며 최대 동접이나 병렬화 성능 근거가 아니다.
