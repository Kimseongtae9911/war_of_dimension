# DB 정합성 수정과 실패 복구 검증

[HTML 결과 보고서](DATABASE_CONSISTENCY.html) · [수치·소스/바이너리 SHA-256](evidence/database-consistency-20261029.json) · [구조와 선택 이유](../diagrams/database-consistency/README.md)

초기 비동기 DB 개선은 `88d310a5189324bd23a99357fd0c0e7b5c3260e3`로 `codex/db-setup-async`와 `main`에 push·fast-forward 병합했다. 이 문서는 그 뒤 `codex/db-consistency-fixes`에서 수정한 정합성 결함을 기록한다. 초기 성능 수치는 [이전 보고서](DATABASE_IMPROVEMENT.html)의 snapshot이며 현재 결과와 혼합하지 않는다.

## 결과

| 수정 | 실제 확인 |
|---|---|
| 만료 경매의 재고 손실 | 동시 정산 2개 × 5회/구성에서 아이템 1개 반환, 만료 행 0개. 구매/만료 경합 4회/구성 모두 재고1개·합계4000 Token 보존 |
| 중복 로그인·오래된 잔액 | 단일 계정 소유 예약, ASCII 대소문자 별칭 차단, 연결 종료 후 해제. DB2100에서 구매100 뒤 응답/DB2000 일치 |
| 보상 실패 유실·재생 중복 | SQL 실패 기록 보존 → 다음 시작에23 Token 반영 → 확정 기록 재생에도 잔액2023·원장1개 |
| DB 대기 중 입력 적체 | 세션256건·65536 bytes 상한. 400건 flood 후 disconnect·cleanup·재접속, Core에서 바이트/건수 상한 각각 검사 |
| 종료 도구 강제 중단 | 1초 timeout에도 실제 프로세스·실행 기록 유지, SQL 잠금 해제 후 정상 drain·자원0 |
| 평문 비밀번호 | PBKDF2-HMAC-SHA256 600000회, 무작위 salt16 bytes·hash32 bytes. 기존 ASCII 평문 이전과 오류 rollback·native 검증 |
| 경매 최초 페이지 | 최초 요청을0으로 통일. Debug/Release 클라이언트 빌드 통과, GUI 클릭은 미검사 |

## 원인과 해결

`GetAuctionInfo`의 직접 DELETE는 등록 시 차감한 재고를 반환하지 않았다. `SettleExpiredAuctions`가 DELETE OUTPUT으로 확보한 행과 재고 반환을 같은 트랜잭션으로 처리하고, 목록·건수 조회에서 정산한다. 구매 SQL의 행 잠금과 경쟁해도 아이템 소유권과 잔액 합계를 보존한다.

계정 캐시 차감 대신 SQL이 확정한 Token과 `BalanceVersion rowversion`을 읽는다. 로그인 전 소유 세션을 예약하고 실패·disconnect 시 해제하며, 보상·판매자 갱신은 해당 소유자에게 전달한다. 오래된 완료값은 version 비교로 제외한다. 로컬 SQL의 ASCII 대소문자·후행 공백 동일성만 맞췄으며 임의 Unicode collation이나 여러 로비 프로세스의 분산 소유권을 보장하지 않는다.

보상은 별도 writer에서 작업 ID·계정·delta·SHA-256 checksum을 파일에 쓰고 flush 후 승격한다. SQL 원장은 작업 ID를 한 번만 적용하고 계정/delta 충돌을 거부한다. SQL 확정 후 기록을 삭제하며 실패 기록은 다음 시작에 재생한다. 손상 파일은 보존하고 시작을 차단한다. 종료는 ingress writer join → SQL drain → dispatcher 정리 순서다. 실행 중 자동 재접속·주기 재시도는 구현하지 않았다.

비밀번호의 CNG PBKDF2 검증과 SQL 조회는 DB worker에서 실행한다. 원문 비밀번호를 SQL 매개변수로 전달하지 않는다. 스키마 v3 이전은 DDL·자격 증명 변환·버전 갱신을 한 트랜잭션으로 실행한다. 비ASCII 기존 평문은 인코딩을 추측하지 않고 실패·rollback한다. wire 길이와 암호화되지 않은 기존 TCP 인증 프로토콜은 그대로다.

## 실제 검증과 재현

Windows x64, MSVC v145, ODBC Driver17, 격리 LocalDB `NewWodTest`/`NewWodTestRecovery`와 합성 계정을 사용했다. 검사 후 자신이 만든 행만 정리한다. 서버 포트를 사용하는 검사는 동시에 실행하지 않는다. Python3.12 이상과 `pyodbc`가 필요하다. 보고서 생성은 `matplotlib`도 사용한다.

```powershell
./scripts/Build.ps1 -Module Servers -Configuration Debug
./scripts/Build.ps1 -Module Client -Configuration Debug
./scripts/Test-ServerCore.ps1 -Configuration Debug
./scripts/Test-Database.ps1 -Configuration Debug
./scripts/Test-DatabaseStartup.ps1 -Configuration Debug
./scripts/Test-DatabaseConsistency.ps1 -Configuration Debug
./scripts/Test-Local.ps1 -Configuration Debug -ServersOnly
# Release도 각각 실행
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Release -Python <python.exe>
./scripts/Initialize-LocalDatabase.ps1
./scripts/Initialize-LocalDatabase.ps1 -CheckOnly
python ./scripts/Build-DatabaseConsistencyReport.py
```

- Debug/Release 서버·클라이언트 빌드 PASS. Core 각1694 checks·ABI110 types/106 constants 동일.
- 실제 TCP→ODBC 기존 DB 각25/25, 추가 정합성 각26/26, 설정 실패 각3/3 PASS. 추가 검사마다 서버3회 정상 종료·pending/sockets/leased=0.
- Release 기존 네트워크12개·4인 전환2종·초기화 실패4종(통합7그룹) PASS. 양 구성 DB 없는 서버 smoke PASS.
- 새 v3 로컬 DB 준비·18개 프로시저·CheckOnly·반복 준비 PASS. 이전 실패 rollback과 native verifier로 이전 계정 로그인을 검사했다.
- 로컬 보상17 Token 수신→통지 시간은 근거 JSON의 `local_end_to_end_ms`이다. 각 구성1회 관측이며 p99·처리량·과거 대비 성능 향상으로 해석하지 않는다. 기존 실제 SQL250ms 잠금 중 다른 세션 진행 검사도 DB 회귀에 포함한다.
- 보고서 Chromium1440/1920/390px, 구조도4개 desktop containment와 실제 light/dark PNG를 별도 확인했다. 그림은 출처 JSON에서 생성하며 표와 교차 확인한다.

원시 실행 로그와 화면은 Git 제외 `artifacts/logs/database*`, `test-server-core-*`, `server-core-integration-Release`, `smoke-*.json`에 있다. 공개 근거에는 연결 설정·비밀번호·검증자·개인 로그를 넣지 않는다. 기존 전체 빌드 경고와 이번 증분 빌드 결과를 구분한다.

## 추가 개선 검토

| 항목 | 영향 | 상태/근거 | 다음 대응 |
|---|---|---|---|
| 원본 보상 ID·ACK·상호 인증 | 디스크 확정 전 강제 종료·서로 다른 수신의 중복을 식별하지 못함 | 기존 account/time/delta wire와 첫 게임 연결 구분 유지 | 송신 outbox·고유 event ID·ACK·인증 계약 설계 |
| 시험 거래 지급 | 운영 재화 발급의 신뢰 경계 미완성 | `CreateTransactionPacket`은 기존 시험용 랜덤 지급 | 운영 모드 차단과 검증된 거래 증명 설계 |
| 인증 전송·시도 제한 | 저장 hash 개선만으로 완성되지 않음. PBKDF2가 단일 DB worker 처리량에 비용 추가 | TCP·기존 짧은 password field 유지, rate limit 미구현 | TLS·길이 확장·시도 제한·인증 실행 분리 검토 |
| 계정 동일성·다중 인스턴스 | Unicode/운영 collation·분산 소유 미검증 | 로컬 ASCII 예약만 검사 | canonical account ID·분산 lease |
| 단일 SQL worker·경매 조회 | 대기·대량 OFFSET/COUNT 비용 | 실데이터 실행 계획·최대 처리량 미측정 | 큐/SQL 시간 계측 후 인덱스·cursor paging·계정 shard 순으로 판단 |
| 보상 원장·디스크 운영 | 원장 증가·손상 시 시작 차단·수동 점검 필요 | 보존 상한4096 기록, checksum·exclusive lock | 보존/백업/경고·손상 복구 운영 절차 |
| 운영 스키마 | 테이블/프로시저 존재·일부 열/버전만 검사 | 운영 DB에는 적용/검증하지 않음 | 배포 전 schema signature·마이그레이션/복원 검증 |

실제 전원 차단·디스크 장애·운영 DB·블록체인·장시간/최대 부하·전체 GUI 전투는 검증하지 않았다. 로컬 GUID와 SQL 원장은 **저장된 같은 복구 기록**의 재생을 보호하며 송신부터 DB까지 exactly-once를 의미하지 않는다.

PBKDF2 기준: [OWASP Password Storage](https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html). 사용 API: [BCryptDeriveKeyPBKDF2](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptderivekeypbkdf2), [Windows flush 설명](https://learn.microsoft.com/en-us/windows/win32/fileio/flushing-system-buffered-i-o-data-to-disk).
