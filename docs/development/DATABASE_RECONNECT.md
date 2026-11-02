# DB 연결 장애 복구와 대기 작업 재개

[HTML 결과 보고서](DATABASE_RECONNECT.html) · [11/2 실측·판정·SHA-256](evidence/database-reconnect-20261102.json) · [11/1 이전 기록](evidence/database-reconnect-20261101.json) · [상태 전이와 선택 이유](../diagrams/database-reconnect/README.md)

## 적용한 계약

연결 생성·SQL·재접속·연결 파괴는 같은 DB worker가 수행한다. `RecoveringWorkQueue`가 수락 순서의 FIFO를 유지한다. 11/2 수정부터 정상 경로는 실제 SQL을 바로 실행하며, 사전 `SELECT 1`과 연결 진단을 하지 않는다. 실제 SQL 실패 후에만 `NeedsReconnect`로 연결 장애를 분류한다. 장애를 발견한 뒤에는 큐가 비어 있어도 1/2/4/8/16/30초 간격으로 재접속을 계속 시도한다. 최초 서버 시작의 연결/설정 실패는 기존대로 시작 실패이며, 이번 복구는 실행 중 장애에 적용한다.

재접속은 스키마와 `service_broker_guid|database name`을 다시 확인한다. 다른 DB에 연결해 보상 원장을 잘못 적용하지 않도록 식별값이 바뀌면 정상 상태로 전환하지 않는다. 유휴 상태의 장애는 다음 실제 요청의 실패로 감지한다. 첫 요청이 일반 쓰기이면 결과 불명확으로 종료될 수 있으며 자동 반복하지 않는다. 이후 미실행 요청은 복구까지 대기한다. 사전 확인도 확인 직후의 단절을 막지 못하므로 실행 결과와 재시도 정책이 쓰기 안전성의 기준이다.

| 작업 | 정책 |
|---|---|
| 실행 전 대기 요청 | 최대4096건(실행 중 포함), 일반 요청은 수락부터30초. SQL 실행 직전 기한·세션 종료·generation 재확인 |
| 로그인/보유 외형/스테이킹 정보/validator 조회 | 명시적으로 재실행 허용. 회원가입은 제외 |
| 경매 조회·건수 | 만료 정산이라는 쓰기가 있으므로 단순 조회 재시도 대상에서 제외 |
| 일반 쓰기의 실행 후 연결 장애 | 결과 불명확 집계·로그, 성공 응답과 자동 재실행 금지. 세션 종료 후 재로그인으로 DB 상태 재조회 |
| 보상 | 기존 작업 ID·SQL 원장으로 재실행. 일반30초 기한에서 제외하고 디스크 기록 유지. 종료·접수 거부·업무 오류의 미완료 기록은 다음 시작에서 재생 |
| 큐 포화·기한 만료 | 세션 Resume. 로그인 전에는 기존 실패코드3, 로그인 후에는 기존 SC_CHAT으로 일시 장애 안내. 새 패킷/필드·GUI 입력 비활성화는 추가하지 않음 |
| 종료 | 온라인이면 기존 수락 작업 drain. 오프라인이면 재접속 대기를 깨우고 미실행 일반 요청 취소·보상 기록 보존 |

다른 세션의 DB 없는 동작은 계속 진행한다. 같은 세션은 선행 DB 요청의 완료/실패 처리 전까지 후속 작업을 보류한다. 이 큐는 여러 계정의 DB 요청을 병렬 실행하는 연결 풀이 아니다. 기한은 SQL 실행 전 대기를 제한하며, 이미 실행 중인 SQL을 강제로 중단시키는 전체 응답 SLA가 아니다. ODBC 로그인/query timeout은 각각5초이며 재접속 호출 중에는 해당 호출 완료까지 종료·만료 처리가 늦어질 수 있다.

일반 쓰기에는 요청 원장과 원본 요청 ID가 없다. 불명확한 요청을 디스크에 저장해 자동 정산하거나 클라이언트의 수동 재요청까지 중복 제거하는 기능은 구현하지 않았다. 업무 의미를 추측한 재실행보다 실패 경계를 보존하는 방식을 선택했다.

## 실제 검증

`Test-DatabaseReconnect.ps1`은 `NewWodTestReconnect<무작위8자리>`라는 **전용 인스턴스와 전용 DB**를 만들고 해당 인스턴스만 중지한다. `MSSQLLocalDB`나 운영 DB는 중지하지 않는다. 종료 후 전용 DB 파일·설정·보상 기록은 조사/복구를 위해 보존한다. 자동 삭제하지 않는다.

- 구성별5회 실제 DB OFFLINE→안전한 조회의 실패로 장애 감지→미실행 구매/보상 대기→ONLINE→동일 로비 프로세스의 재개를 검사한다. 첫 반복에서는 전용 SQL 엔진도 중지/재시작하고 PID 변경을 확인한다. 별도로 첫 실패 요청이 일반 쓰기인 경우의 자동 반복·거짓 성공 금지도 검사한다.
- `ConnectRetryCount=0`인 시험 연결로 서버 복구 경로를 구분한다. SQL 세션 KILL, 순수 조회 SP의 지연 중 KILL, 시험 전용 구매 SP에서 COMMIT 후 응답 지연 중 KILL을 수행한다. 순수 조회는 다시 실행하고, 구매는 한 번만 차감되고 성공 패킷 없이 종료되는 것을 확인한다.
- 일반 요청30초 만료와 일시 장애 안내, 세션 후속 작업 재개, 기한 이후 내구성 보상 재개, 장애 중 종료와 기록 보존을 검사한다. 정상 종료는 `pending/sockets/leased=0`과 DB `accepted=completed, pending=0`을 확인한다.
- Core 검사는 큐 포화·FIFO·조회/쓰기 재시도 경계·보상 앞의 일반 요청 만료·종료 interrupt·종료된 세션 취소를 제어된 짧은 타이머로 검사한다. 이를 실DB 지연 측정과 혼합하지 않는다.

11/2 실제 장애 검사는 Debug/Release 각34/34, 기존 DB 각25/25·정합성 각26/26, Core 각1699·ABI110종/106상수, DB 없는 smoke를 확인했다. 설정 실패 각3경로와 Release 네트워크7그룹은 11/1 검증 기록이며 이번에는 재실행하지 않았다.

## 정상 요청의 전후 비용

수정 전 Release 실행 파일을 `.runtime/db-probe-before/bin/Lobby_Server.exe`에 보존하고 수정 후 바이너리와 비교했다. 전용 LocalDB·같은 계정/소유 아이템·순차1클라이언트의 `CS_OPEN_CUSTOMIZE` 요청이다. 실행마다 서버 재시작·워밍업20건 후200건, 전후 순서를 교대한5회다. 대표 지연은 실행별 중앙값5개의 중앙값이다.

| 지표 | 이전 | 수정 |
|---|---:|---:|
| 연결 확인 SELECT / 동일100요청 | 100 | 0 |
| 업무 SELECT / 동일100요청 | 100 | 100 |
| 응답 중앙값 ms | 0.3783 | 0.2547 |
| 실행별 중앙값 범위 ms | 0.35295–0.38355 | 0.23765–0.26055 |

관측 중앙값은 약32.7% 감소했다. SQL 수는 별도100건의 Extended Events로 확인했고, 지연 측정 때는 XE를 껐다. 이벤트 누락0·ring buffer 잘림 없음, 실제 아이템 응답·오류0·종료 자원0을 확인했다. 해당 probe/업무 SELECT 합계가200→100인 것이며 전체 ODBC 호출·준비/fetch·네트워크 패킷 수 감소율과 다르다. 원격 DB·동시 부하·최대 처리량·p99로 일반화하지 않는다.

테스트 전용 SP 변경은 해당 전용 DB에만 적용하고 원래 정의로 복원한다. 운영 SQL 배포 스크립트를 변경하지 않는다. 실제 전원 차단·디스크 장애·WAN 장애·전체 GUI·최대/장시간 부하와 운영 스키마는 검증 범위 밖이다.

## 재현

```powershell
./scripts/Build.ps1 -Module Servers -Configuration Debug
./scripts/Test-DatabaseReconnect.ps1 -Configuration Debug
./scripts/Test-ServerCore.ps1 -Configuration Debug
./scripts/Test-Database.ps1 -Configuration Debug
./scripts/Test-DatabaseConsistency.ps1 -Configuration Debug
./scripts/Test-DatabaseStartup.ps1 -Configuration Debug
./scripts/Test-Local.ps1 -Configuration Debug -ServersOnly
# Release에서도 위 순서로 실행한다. 서버 포트를 사용하는 검사들을 동시에 실행하지 않는다.
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Release -Python <python.exe>
./scripts/Test-AgentEnvironment.ps1 -StaticOnly
./scripts/Measure-DatabaseRequestCost.ps1 -BeforeExecutable <수정-전-Release-Lobby_Server.exe>
<python.exe> -X utf8 ./scripts/Build-DatabaseReconnectReport.py
```

Python3.12 이상·시험 전용 `pyodbc`·보고서용 `matplotlib`이 필요하다. 기존 `.runtime/db-test-deps`를 재사용한다. 원시 결과는 Git 제외 `artifacts/logs/database-reconnect/{Debug,Release}/result.json`과 서버 로그에 보존한다. 공개 근거에는 자격 증명·연결 문자열·개인 경로·복구 레코드 본문을 포함하지 않는다.

## 비용과 남은 범위

- 정상 요청의 probe 왕복은 제거했다. 로컬 순차 읽기의 전후 비용을 측정했으며, 대기 큐의 최악4096건 만료 시각 탐색·운영 query plan·최대 동접 비용은 미측정이다.
- 일반 쓰기의 자동 정산은 요청 ID·작업 원장을 확장한 뒤 구현해야 한다. 원본 게임 보상 ID·ACK·상호 인증은 이전 보고서의 후속 과제로 유지한다.
- 초기 대기 안내 문자열이 CHAT_SIZE=30을 초과하는 실패를 실제 검사에서 발견했다. 짧은 안내와 compile-time `static_assert`로 수정했다.
- 응답 유실 시험에서 클라이언트가 EOF 이후 소켓을 닫고 cleanup 완료를 기다리도록 fixture를 수정했다. 중복 Disconnect의 세대 무효화를 방지하는 guard도 추가했다. 세션 잠금은 재귀 잠금이므로 잠금 재진입 자체를 교착 원인으로 기록하지 않는다.

ODBC 계약: [Connection resiliency](https://learn.microsoft.com/en-us/sql/connect/odbc/connection-resiliency), [SQLEndTran 결과 불명확](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlendtran-function).
