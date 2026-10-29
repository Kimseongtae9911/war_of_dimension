# DB 실행 준비·비동기 순서 개선

[HTML 보고서](DATABASE_IMPROVEMENT.html)에서 결과·전후 그래프를 확인한다. [실측 원본과 SHA-256](evidence/database-improvement-20261029.json), [실행 구조와 설계 이유](../diagrams/database-async/README.md)를 함께 관리한다.

## 결과와 적용 범위

- 현재 브랜치 `codex/db-setup-async`, 기반 커밋 `54bc0a3202d2120e9ee70cd6dc5aaa9265b660ca`.
- 이전 `codex/lobby-movement-load`의 추적/미추적 작업은 stash `012034e978eb94333c4e84be57baae6b5d532194`에 보관했다. 새 브랜치에는 이전 미커밋 내용을 적용하지 않았다. 커밋·push는 수행하지 않았다.
- 로컬 DB 준비 스크립트가 `NewWodLocal`과 저장 프로시저 16개를 준비한다. 소스의 `WITH_DATABASE` 편집과 ODBC DSN GUI 등록은 필요 없다. 기본은 `disabled`, 명시적 `-Database`에서만 활성화한다.
- 패킷의 로그인·회원가입·조회·구매·외형·스테이킹·게임 보상과 미사용 validator 조회 경로까지 직접 ODBC 접근을 없앴다. 전용 FIFO worker만 연결에 접근한다.
- SQL 실행 중 로비 world lock을 보유하지 않는다. 같은 세션은 Suspend→완료를 inbox 앞에 Resume→후속 실행한다. 완료의 generation과 객체 수명을 확인한다.
- 구매의 재고/잔액은 트랜잭션으로 확정한 뒤 성공 응답을 보낸다. 종료 시 오래된 전체 PlayerInfo snapshot을 저장하지 않고, 외형은 DB의 최신 잔액을 보존해 저장한다.

## 실측 정의

Windows 11 build 26200, AMD Ryzen 5 7500F / 논리 코어 12개, MSVC v145 x64 C++20, Python 3.12.14. 실제 DB 검사는 SQL Server 2025 Express LocalDB / ODBC Driver 17을 사용했다. Debug 결과는 기능 검사로만 사용한다.

### 경계 제어 실험

`ServerCore.Tests --db-benchmark`가 이전 코드의 **SQL 중 전역 잠금 보유 경계**를 재현하고 현재 production `OrderedWorkQueue`/`LobbyDispatcher`와 비교한다. 과거 서버 바이너리나 SQL 엔진 자체의 전후 측정은 아니다.

한 작업에 `sleep_for(10ms)` 8번을 넣는다. Windows timer 해상도로 실제 전체 대기는 약 125ms이며 80ms라는 이론값으로 대체하지 않는다. 한 쌍 워밍업 뒤 전후 실행 순서를 바꾸어 각각 5회 측정한다. 단일 로비 worker, 같은 채널의 다른 세션, 동일 입력, 계측 설정은 동일하다.

| 지표 | 이전 중앙값 / ms | 개선 중앙값 / ms | 개선 5회 범위 / ms |
|---|---:|---:|---:|
| 로비 작업 점유 | 125.5100 | 0.0032 | 0.0012–0.0038 |
| 다른 세션 응답 | 125.5170 | 0.0183 | 0.0079–0.0196 |
| 전체 작업 완료 | 125.8230 | 125.3610 | 124.2830–126.3110 |

작업 점유 중앙값은 약 99.997% 감소했다. 전체 SQL 대기·처리량이 같은 비율로 개선됐다는 뜻이 아니다. 전체 완료 시간의 작은 변동은 성능 향상으로 주장하지 않는다. 표본 5개로 p99나 최대 동접을 추정하지 않았다.

### 실제 서버 DB 잠금 검사

`NewWodTest`의 `auction_data`에 별도 연결로 250ms의 배타 잠금을 둔다. A는 `CS_OPEN_AUCTION`→`CS_CHANGE_CHANNEL`, B는 DB를 사용하지 않는 `CS_CHANGE_CHANNEL`을 보낸다. 5회 모두 A의 DB 응답이 A의 다음 응답보다 먼저 도착했다.

Release 중앙값: A의 SQL 대기 포함 응답 **250.6713ms**, B의 응답 **0.1280ms**. 이는 개선 후 실제 서버의 대기 격리 검사다. 과거 서버의 실측이 아니며 전체 게임 부하·수용량 측정으로 해석하지 않는다. 세부 표본은 evidence JSON에 남겼다.

## 실제 검증

| 검사 | 결과 | 확인 범위 |
|---|---|---|
| Servers Debug/Release 빌드 | PASS | 실제 MSVC v145 x64 |
| Core Debug/Release | 각각 1690 checks PASS | 새 DB 경계 검사 4개 그룹 포함: concurrent producer별 FIFO·drain, 포화·예외, 느린 완료, epilogue보다 빠른 완료 |
| Protocol ABI | 110종 / 106상수 동일 | wire 형식 미변경 |
| DB 통합 Debug/Release | 각각 20/20 PASS | 실제 TCP→ODBC→격리 LocalDB |
| DB 설정 실패 Debug/Release | 각각 3/3 PASS | 잘못된 mode·연결 누락·드라이버 누락, 종료 코드 1·자원 0 |
| 기존 Release 통합 | 7개 그룹 PASS | 네트워크 12검사, boss 4/5 각각 4인 입장, 초기화 실패 4종 |
| DB 없는 LOCAL_TEST smoke | Debug/Release PASS | 두 listener·서버 간 연결·정상 종료 |
| 에이전트 환경 | 정적 PASS | compilation database 230개 소스 |
| 정상 종료 | 양쪽 서버 자원 0 | DB 수락 27/실행 완료 27, 예상 SQL 실패 2건 포함, pending=0 sockets=0 leased=0 |
| HTML | Chromium 점검 PASS | 1440/1920/390px 가로 넘침 0, JS 오류 0, 그래프 2개. 실제 desktop/mobile PNG 확인 |
| Archify | 9/9 showcase | 네 desktop 크기 containment, 실제 light/dark PNG 확인. 영수증은 구조도 README에 기록 |

예상 SQL 실패 2건은 잔액 부족으로 구매 rollback을 강제한 1건과 약 5초 query timeout 1건이다. 오류 수를 0으로 숨기지 않았다. timeout 뒤 다음 쿼리가 진행되고, 실패 거래에는 성공 패킷을 보내지 않으며 재고·잔액이 rollback되는 것을 확인했다. 잘못된 패킷 이름을 쓴 초기 검사와 0건 DELETE의 `SQL_NO_DATA` 오류 처리 실패도 수정 후 재검증했다.

최신 빌드 로그는 증분 빌드여서 경고가 없었다. 기존 전체 빌드의 변환·미사용 변수 경고가 모두 해소됐다고 주장하지 않는다.

## 재현

```powershell
./scripts/Initialize-LocalDatabase.ps1
./scripts/Initialize-LocalDatabase.ps1 -CheckOnly
./scripts/Build.ps1 -Module Servers -Configuration Debug
./scripts/Build.ps1 -Module Servers -Configuration Release
./scripts/Test-ServerCore.ps1 -Configuration Debug
./scripts/Test-ServerCore.ps1 -Configuration Release
./scripts/Test-DatabaseStartup.ps1 -Configuration Debug
./scripts/Test-DatabaseStartup.ps1 -Configuration Release
./scripts/Test-Database.ps1 -Configuration Debug
./scripts/Test-Database.ps1 -Configuration Release
./scripts/Measure-Database.ps1
```

DB 통합 검사 전용 Python 의존성은 `pyodbc`, 보고서 그래프는 `matplotlib`이다. 실행 서버에는 Python을 요구하지 않는다. 실제 Python 경로에서 `-m pip install --target .runtime/db-test-deps pyodbc matplotlib`로 설치한다. 현재 컴퓨터에서는 번들 Python 3.12.14로 실행했다. `Test-Database.ps1`은 연결 환경변수를 복원하고 검사용 `NewWodTest`만 사용하며 생성한 두 계정만 정리한다. DB 자체를 삭제하지 않는다. 네트워크 검사들을 동시에 실행하지 않는다.

```powershell
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Release -Python <실제-python.exe>
./scripts/Test-Local.ps1 -Configuration Debug -ServersOnly
./scripts/Test-Local.ps1 -Configuration Release -ServersOnly
<실제-python.exe> ./scripts/Build-DatabaseReport.py
```

원시 결과는 Git 제외 `artifacts/logs/database/`에, 기존 네트워크 회귀는 `artifacts/logs/server-core-integration-Release/`에 있다. evidence JSON에는 표본·source/binary SHA-256·구성·판정 결과만 담고 개인 연결 설정·원시 실행 로그는 담지 않는다.

## 남은 범위

단일 DB worker이므로 느린 쿼리 뒤의 다른 DB 요청은 대기한다. 상한 4096건은 메모리를 제한하지만, backlog가 크면 drain이 기존 중지 도구의 15초를 넘길 수 있다. 강제 종료·전원 장애의 복구, 내구성 큐, 자동 재접속/재시도, idempotency, 다중 프로세스 순서 보장은 구현하지 않았다. 커밋·rollback의 결과가 불명확하면 해당 연결의 재사용을 막으며 중복 쓰기 재시도를 하지 않는다.

동일 계정의 복수 로그인 메모리 상태는 후속 동기화 대상이다. 로컬 스키마는 기존 평문 비밀번호 계약을 재현하므로 운영 인증 마이그레이션이 필요하다. 기존 만료 경매 삭제는 재고 반환 정책이 없으며, 클라이언트의 최초 페이지 요청 `1`과 이후 0기반 버튼 계약도 불일치한다. 이 두 경매 정책은 이번에 변경하지 않았다. 운영 DB의 열·프로시저 버전/거래 의미는 별도 호환 검토가 필요하며 로컬 자동 스키마를 운영 DB에 적용하지 않는다.

운영 DB·블록체인·실제 GUI 전투·장시간 장애·최대 처리량은 미검증이다.

ODBC 설정 근거: [SQLDriverConnect](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqldriverconnect-function), [연결 속성·login timeout](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/connection-attributes), [statement 속성·query timeout](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/statement-attributes).

## 추가 개선 검토 · 커밋 준비 시 확인 (2026-10-29)

[재현 수치·분류·코드 위치](evidence/database-additional-review-20261029.json)를 남겼다. 이번 추가 검토는 구현을 변경하지 않았다. 격리 `NewWodTest`에서 SQL 검토는 외부 트랜잭션 rollback, 네트워크 검토는 생성한 계정/재고만 삭제하고 서버를 정상 종료했다. 운영 DB는 접근하지 않았다.

### 먼저 해결할 데이터 정합성

1. **P1 · 만료 경매의 아이템 반환 누락 — 실제 재현.** `RegisterAuction`은 재고를 1개 차감하지만 `GetAuctionInfo`의 만료 `DELETE`에는 반환이 없다. 테스트 아이템 1개 → 등록 후 0개 → 만료 삭제 후에도 0개, 경매 행 0개를 확인했다. 등록/판매/만료를 명시적 상태로 관리하고, 만료 상태 전환과 재고 반환을 같은 트랜잭션에서 한 번만 수행해야 한다. 재시작·판매와 만료의 경합도 검증 대상으로 둔다. 근거: `database/local-schema.sql:98`, `Server/Lobby_Server/CDataBase.cpp:403`.
2. **P1 · 중복 로그인 잔액 불일치 — 실제 TCP+DB 재현.** 같은 계정으로 접속한 A/B가 각각 100을 구매하면 DB는 2000→1900→1800이지만 B에게는 1900이 전송됐다. `Authenticate`가 계정의 단일 소유자를 정하지 않고 구매 완료가 각 세션의 이전 메모리 값에서 차감하기 때문이다. 우선 계정당 활성 소유 세션을 하나로 제한하고, 거래 SP가 확정 잔액/버전을 반환하도록 변경하는 안을 권장한다. DB 잔액 음수 방지는 기존 트랜잭션이 담당하지만 화면/후속 판단 일관성까지 해결하지는 않는다. 근거: `CPacketMgr.cpp:352`, `CPacketMgr.cpp:644`.
3. **P1 · 보상 실패 재처리·종료 정책 — 코드 확인.** `GL_TRANSACTIONS`는 DB 작업을 수락한 후 실패해도 queue의 실패 계수만 증가하고 보상 ACK·영속 재처리 정보가 없다. 동시에 `Stop-Local.ps1`은 15초 후 강제 종료하므로 적체된 drain이 끝나기 전에 미완료 작업이 사라질 수 있다. 거래/보상 ID, 완료 결과, 중복 방지와 영속 outbox를 먼저 정하고 drain 진행 상황에 따른 종료 유예를 연결해야 한다. 임의 재시도만 추가하면 커밋 결과가 불명확한 요청을 중복 반영할 수 있다. 장시간 적체 중 강제 종료는 이번 검토에서 실행하지 않았다. 근거: `CNetworkMgr.cpp:351`, `OrderedWorkQueue.h:76`, `scripts/Stop-Local.ps1:23`.
4. **P1 · 대기 세션 inbox 상한 — 코드 확인.** DB 큐의 4096건 제한과 별개로 `LobbyDispatcher::Post`는 paused 세션에도 제한 없이 `m_work`를 추가한다. 느린 DB를 기다리는 클라이언트가 패킷을 계속 보내면 DB 큐를 우회해 메모리가 늘 수 있다. 세션별 건수/바이트 한도와 읽기 backpressure를 두고, 완료/cleanup 전달은 포화 상태에서도 보장해야 한다. 메모리 부하 수치는 아직 측정하지 않았다. 근거: `LobbyDispatcher.h:58`.

### 운영 전 준비와 측정 후 적용할 성능 개선

| 우선순위 | 검토 항목 | 확인 상태 | 권장 다음 작업 |
|---|---|---|---|
| 외부 운영 전 필수 | 로컬 평문 비밀번호·짧은 자격 증명 계약 | `Player_Password varchar(10)` 및 원문 비교를 코드에서 확인 | salt를 포함한 적응형 비밀번호 해시·자격 증명 버전·암호화 전송을 함께 마이그레이션. 로컬 `Encrypt=No` 설정을 외부 서버에 복사하지 않음 |
| P2 | 스키마 사전 검사·마이그레이션 | 시작 시 테이블/프로시저 이름 존재만 검사. 초기화는 여러 `GO` batch로 분리 | 열/타입/프로시저 signature와 version을 검증하고, 각 마이그레이션의 원자성·동시 초기화 잠금·중단 복구 검사 추가 |
| P2 | 로그인 왕복 4회 | 정상 경로의 CheckIdExists→CheckPassword→GetPlayerInfo→GetFullNodeInfo 호출을 확인 | 인증 결과/PlayerInfo/node 정보를 단일 SP의 명시적 결과로 반환. 실제 DB 지연 조건에서 전후 측정 후 채택 |
| P2 | 페이지 조회마다 만료 삭제 | 조회 경로의 직접 DELETE와 Deadline 정렬, 별도 Deadline index 부재를 확인 | 만료 정산을 분리하고 데이터 규모별 실행 계획·logical reads·p95/p99 측정 후 index/커서 페이지 방식 판단. 아직 index 성능 향상을 측정하지 않았음 |
| P2 | 경매 첫 페이지 1/0 불일치 | `Scene.cpp:932`의 최초 요청1, `stdafx.h:351`/Shader 버튼의 0기반 상태를 확인 | 서버·클라이언트의 0기반 계약으로 통일하고 첫 화면/이전·다음 페이지 검사 |
| 측정 후 판단 | 단일 DB worker의 처리량 | 정합성 경계 검사와 기존 실측은 존재. 실DB 최대 처리량은 미측정 | queue wait·SQL 시간·완료 적용 대기를 분리 계측. 병목 확인 후 계정 key별 직렬화와 연결 pool을 검토 |

추천 적용 순서는 **만료 정산·계정 권위 → 보상 내구성·종료/inbox 제어 → 스키마/인증 마이그레이션 → 쿼리/연결 병렬화**다. 현재 `main` 병합 범위는 앞서 검증한 로컬 DB 준비·비동기 FIFO 개선과 이 검토 기록이며, 위 후속 개선을 구현 완료한 것으로 보고하지 않는다.
