# DB 장애 복구와 요청 재개

[HTML 구조도](recovery.html) · [JSON 원본](recovery.json) · [검증 보고서](../../development/DATABASE_RECONNECT.html) · [정책/재현](../../development/DATABASE_RECONNECT.md)

## 구현 의도와 선택 이유

| 문제 | 선택 | 단순한 대안의 한계 | 보장 | 비용·한계 |
|---|---|---|---|---|
| 정상 요청의 불필요한 왕복과 연결 장애 | 실제 SQL 실패 후에만 진단, 연결 소유 worker에서 재접속 | 매 요청 probe는 왕복을 추가하지만 확인 직후 단절을 막지 못함 | 정상 경로 추가 SQL/진단 없음, SQL 동안 로비 worker/world lock 반환 | 첫 실패가 쓰기이면 불명확한 결과일 수 있음 |
| 장애 중 미실행 요청 유실·순서 역전 | bounded FIFO 선두 보존 | 실패 작업을 큐 끝에 추가하면 후속 쓰기가 앞질러 실행됨 | 수락한 SQL 본문 실행 순서 유지 | 최대4096건, 일반 기한30초 |
| 같은 세션의 DB 결과보다 다음 요청이 먼저 실행 | 기존 Suspend→Resume 앞 삽입 유지 | SQL FIFO만으로 로비 결과 적용 순서는 보장되지 않음 | 결과 반영 뒤 해당 세션 후속 작업 | 세션 후속 동작도 선행 결과를 기다림 |
| COMMIT 응답 유실의 중복 차감 | 명시적인 replay-safe 경로만 재실행 | 모든 예외의 자동 재시도는 이미 확정된 쓰기를 반복 | 일반 쓰기 결과 불명확이면 성공/재실행 금지 | 일반 쓰기의 자동 정산·요청 원장은 후속 |
| 오래된 보상이 일반 요청 만료를 방해 | 보상은 무기한 보존, 일반 기한은 큐 전체에서 검사 | 선두 기한만 보면 내구성 보상 뒤 일반 요청이 만료되지 않음 | SQL 실행 순서를 바꾸지 않고 만료 알림 전달 | 만료 검사 최악4096건 탐색 |
| 장애 중 종료가 끝나지 않음 | condition_variable 깨우기, 오프라인 일반 요청 취소 | sleep 기반 재접속은 최대30초 타이머를 기다림 | backoff 즉시 해제, 보상 파일 보존 | 실행 중 ODBC 호출은 최대 timeout까지 기다릴 수 있음 |

순서 예시: A의 구매→채널 변경은 구매 결과 반영 뒤 채널 변경을 실행한다. 장애 중 B의 채널 변경은 DB가 필요하지 않으므로 실행 가능하다. 보상은 같은 작업 ID를 반복해도 SQL 원장이 한 번만 적용하지만, 일반 구매에는 그 계약이 없어 같은 방식으로 반복하지 않는다.

11/2부터 사전 probe를 제거했다. A의 안전한 조회가 장애를 감지하면 뒤의 미실행 구매는 대기 후 재개한다. 첫 장애 요청 자체가 구매이면 자동 재실행하지 않고 세션 종료 후 DB 상태를 다시 조회한다. 이 둘을 같은 성공 재개 계약으로 취급하지 않는다.

## 코드와 검사

- [복구 큐](../../../Server/ServerCore/include/ServerCore/RecoveringWorkQueue.h), [ODBC 경계](../../../Server/Lobby_Server/CDataBase.cpp), [worker 수명](../../../Server/Lobby_Server/CDataBaseThread.cpp)
- [세션 결과·실패 적용](../../../Server/Lobby_Server/CClient.cpp), [재시도 허용 호출부](../../../Server/Lobby_Server/CPacketMgr.cpp)
- [Core 경계 검사](../../../Server/ServerCore/tests/DatabaseQueueTests.cpp), [실DB 장애 검사](../../../tools/server_lab/tests/database_reconnect.py), [실행 스크립트](../../../scripts/Test-DatabaseReconnect.ps1)

## 재생성과 검증

Archify2.16의 lifecycle이다. 설명은 한글이며 고정 Viewer UI와 html lang은 영어 fallback이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate lifecycle docs/diagrams/database-reconnect/recovery.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver lifecycle docs/diagrams/database-reconnect/recovery.json docs/diagrams/database-reconnect/recovery.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/database-reconnect/recovery.html --json
```

- specification_sha256: `5adeaa76525e853c225b10386976068638cb99cd8393b9475638fccc2eb0270b`
- artifact_sha256: `fd191f85837013ca296dd4c0c56af3220980e5409ea199a33d80a1c25980c92e`
- validation: 9/9 showcase, 0 errors, 0 warnings
- visual_review: passed. 1440×900 light / 2048×1320 dark 실제 PNG 확인. 네 desktop 크기 자동 containment PASS와 수동 점검을 구분한다.
- correction_rounds: 0 (11/2 수정). 기존 검증 배치를 유지하고 실행 노드와 설명만 현재 정책으로 갱신했다.
