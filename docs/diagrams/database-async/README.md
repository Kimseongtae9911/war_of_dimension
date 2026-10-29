# DB 전용 FIFO와 세션 완료 경계

[HTML 구조도](implementation.html), [JSON 원본](implementation.json), [측정 보고서](../../development/DATABASE_IMPROVEMENT.html)를 함께 관리한다. 현재 구현이며 Archify 2.16을 사용했다.

## 구현 의도와 설계 선택 이유

| 해결할 문제 | 선택한 방식 | 단순한 대안으로 부족한 이유 | 보장 계약 | 비용·한계 |
|---|---|---|---|---|
| 패킷 처리 중 ODBC 대기·DB thread의 전역 잠금 | DB worker 하나가 연결을 생성·사용·파괴, SQL은 로비 잠금 밖에서 실행 | 별도 thread만 만들어도 `LockGlobal`을 잡고 SQL을 실행하면 모든 채널이 기다림 | SQL 중 로비 worker·world lock을 점유하지 않음 | 단일 연결 처리량, DB 요청 사이의 head-of-line 대기 |
| 읽기·쓰기 경로 분리와 같은 시각 이벤트의 불명확한 순서 | `OrderedWorkQueue`의 mutex 수락 순서대로 FIFO | priority queue의 시간은 순서 번호가 아니며 같은 시각 요청의 안정성도 없음 | 수락한 전체 작업의 순서, 작업 내부 트랜잭션 경계 | 서로 다른 producer의 실제 도착 시각까지 정렬하지 않음 |
| 비동기 로그인보다 다음 요청이 먼저 실행 | 세션 큐 Suspend → 완료를 맨 앞에 Resume → 후속 요청 | 완료를 큐 뒤에 넣으면 로그인·잔액 변경보다 다음 구매/조회가 먼저 처리됨 | 완료 적용 이후 같은 세션의 후속 요청 실행 | 요청한 세션은 DB 대기를 유지, 다른 세션은 계속 실행 |
| worker가 반환하기 전에 빠른 DB 완료가 도착하는 경합 | 세션 mutex 아래 `m_scheduled`·`m_paused`·inbox를 함께 변경 | paused bool 하나만으로는 스케줄 누락 또는 이중 예약 가능 | Resume가 epilogue보다 먼저/나중 도착하는 두 경우 검사 | 기존 채널/전역 scheduler를 그대로 사용 |
| 연결 종료·소켓 재사용·객체 수명 | 세션 `Generation` 확인, cleanup도 같은 세션 큐, producer join 후 DB drain | raw client만 캡처하면 재사용된 세션에 오래된 결과 적용 가능 | client 해제 전 accepted 작업 완료, 오래된 generation은 적용 제외 | 서버 종료 시 아직 수락되지 않은 inbox 요청과 UI 응답은 취소 |
| 큐 적체·SQL 실패·불확실한 커밋 | outstanding 4096건 상한, 수락 거부 시 세션 종료, SQL 실패 시 성공 응답 금지, 명시적 rollback | 무제한 큐는 메모리를 늘리고 자동 재시도는 중복 쓰기를 만들 수 있음 | 수락/완료/거부/실패 카운터, 5초 SQL timeout, 연결 상태 불명확 시 재사용 금지 | 내구성 큐·자동 재접속·재시도 없음, 프로세스 강제 종료의 미완료 작업 복구 미지원 |
| 종료 snapshot이 오래된 잔액을 덮어씀 | 변경 요청의 SQL 성공 후 메모리 반영, 외형만 최신 DB 잔액 보존 저장 | disconnect에서 전체 PlayerInfo를 저장하면 비동기 보상·거래를 덮어쓸 수 있음 | 구매/보상 확정 잔액을 종료 snapshot으로 복구하지 않음 | 같은 계정의 복수 로그인 메모리 일관성은 후속 과제 |

예: `구매 → 구매 → 조회`를 한 TCP batch로 보내도 각 구매 완료가 다음 요청보다 먼저 적용된다. DB가 느릴 때 같은 세션은 기다리지만, 다른 세션의 채널 변경 요청은 DB 큐를 거치지 않고 응답한다. FIFO는 저장 프로시저 내부의 원자성을 대신하지 않으므로 구매는 거래/잔액 변경을 한 트랜잭션으로 묶는다.

## 코드와 검증 근거

- [DB 큐](../../../Server/ServerCore/include/ServerCore/OrderedWorkQueue.h), [세션 dispatcher](../../../Server/Lobby_Server/LobbyDispatcher.h), [client 완료 경계](../../../Server/Lobby_Server/CClient.cpp)
- [DB 연결 수명](../../../Server/Lobby_Server/CDataBaseThread.cpp), [패킷의 snapshot/SQL/완료](../../../Server/Lobby_Server/CPacketMgr.cpp), [ODBC·트랜잭션](../../../Server/Lobby_Server/CDataBase.cpp)
- [FIFO·거부·drain·예외·빠른/느린 완료 검사](../../../Server/ServerCore/tests/DatabaseQueueTests.cpp), [실제 DB 검사](../../../tools/server_lab/tests/database.py)
- [측정과 source/binary SHA-256](../../development/evidence/database-improvement-20261029.json)

## 재생성과 수락 영수증

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/database-async/implementation.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/database-async/implementation.json docs/diagrams/database-async/implementation.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/database-async/implementation.html --json
```

- diagram_type: architecture
- specification_sha256: `17cf625207a03b97a860082d66921e5bed4fa50001ae92e9e169f4dc7e03a75f`
- artifact_sha256: `ebd8a13ad20ce2554e0a184267adb8c0fe80f60b6f42373ab70df149862983a6`
- validation: 9/9 showcase, 0 errors, 0 warnings
- visual_review: passed (1440×900 light / 2048×1320 dark 실제 PNG 확인)
- correction_rounds: 1 (진단된 완료값 label의 위치만 수정)

1440×900, 1600×1000, 1920×1080, 2048×1320의 자동 containment 검사를 통과했다. 자동 영수증의 `visualReview: pending`과 실제 이미지 점검을 구분한다. 고정 Viewer UI는 영어이며 설명은 한글이다.
