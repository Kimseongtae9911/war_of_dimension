# DB 정합성과 실패 복구 경계

[HTML 구조도](implementation.html) · [JSON 원본](implementation.json) · [검증 보고서](../../development/DATABASE_CONSISTENCY.html) · [수치/코드 SHA](../../development/evidence/database-consistency-20261029.json)

## 구현 의도와 선택 이유

| 문제 | 선택 | 단순한 대안으로 부족한 이유 | 계약 | 비용·한계 |
|---|---|---|---|---|
| 복수 연결의 계정 캐시 불일치 | 인증 전 소유 예약, 실패/종료 시 해제, DB 잔액·rowversion 적용 | 마지막에만 중복 확인하면 인증 중 다른 완료와 교차. 캐시 증감은 외부/보상 갱신을 잃음 | 로컬 소유1개·오래된 잔액 배제 | ASCII/후행 공백 정규화, Unicode/분산 lease 별도 |
| 만료 행 삭제 후 재고 소실 | 삭제 OUTPUT과 재고 반환을 한 SQL 트랜잭션 | 반환을 별도 UPDATE하면 구매·동시 정산과 경합 | 재고1개 보존·정산1회 | 조회가 만료 정산 쓰기를 유발, 대량 정산 비용 미측정 |
| paused 세션 입력 무제한 | inbox256건/65536bytes, cleanup·Resume 별도 허용 | SQL 큐4096 상한만으로 아직 제출 안 된 세션 입력을 제한하지 못함 | 과입력 disconnect 후에도 완료·소유 해제 가능 | 전역 큐 전체 용량을 뜻하지 않음 |
| 보상 SQL 실패·프로세스 재시작 | 전용 writer의 flush 기록과 SQL ID 원장 | 메모리 재시도는 종료 시 사라짐. 바로 SQL 재시도는 확정 여부 불명확 시 중복 | 저장된 같은 작업 ID를 최대1회 반영 | 파일/원장 비용·다음 시작 재생·원본 ID/ACK 미지원 |
| 로비에서 디스크 대기 | writer ingress에 값만 제출, 기록 확정 후 SQL 큐 제출 | SQL 전용 thread만으로 파일 I/O의 로비 잠금 점유를 해결하지 못함 | 로비가 flush를 기다리지 않음, writer drain 후 SQL drain | SQL FIFO는 SQL 큐 **수락 순서**다. 다른 producer의 수신 시각/디스크 확정 전 이벤트와의 인과 순서는 별도 |
| 종료 제한보다 긴 backlog | Stop-Local timeout에서 PID/state 유지, 명시적 Force만 강제 종료 | 자동 kill은 drain 도중 수락된 일반 작업을 끊음 | 정상 종료 요청과 강제 종료 구분 | timeout은 실패로 반환, 이후 로그/종료 확인 필요 |
| 평문 비밀번호 | CNG PBKDF2 600000회·무작위 salt, native 검증 | 단순 SHA는 추측 비용이 낮음 | DB에는 verifier만 저장, 이전 실패 시 rollback | 단일 DB worker CPU 비용, wire/TLS 개선 별도 |

순서 예: 구매 A의 DB 결과가 늦게 도착한 사이 보상 B의 더 최신 rowversion이 적용됐다면 A 완료는 이전 잔액으로 되돌리지 않는다. 보상 SQL이 확정되고 파일 삭제 직전 중단되면 다음 시작에서 같은 ID를 읽어 SQL 원장으로 재적용을 막는다. 파일 확정 전 프로세스 종료나 독립적으로 두 번 받은 기존 보상 패킷까지 식별하지는 못한다.

계정 예약·완료·배포는 기존 dispatcher 전역 소유 구간에서 실행한다. AccountOwners mutex는 map 접근만 보호하며 `CClient` 상태 수명 보장 자체를 대신하지 않는다. 세션 generation·cleanup·종료 drain은 기존 [비동기 구조](../database-async/README.md)를 유지한다. 도식의 두 행은 별도 입력 경로이며 SQL/DB의 상세 공유 관계는 이 설명을 따른다. 화살표는 각 행의 처리 순서가 양 끝 이름으로 드러나므로 중복 label을 생략했다.

## 코드와 검증

- [AccountOwners](../../../Server/Lobby_Server/AccountOwners.h), [세션/잔액](../../../Server/Lobby_Server/CClient.h), [전달](../../../Server/Lobby_Server/CUserMgr.cpp)
- [DB worker·writer](../../../Server/Lobby_Server/CDataBaseThread.cpp), [복구 기록](../../../Server/Lobby_Server/RewardJournal.h), [비밀번호](../../../Server/Lobby_Server/PasswordHash.h), [SQL 계약](../../../database/local-schema.sql)
- [Core 실패/상한/암호 KAT](../../../Server/ServerCore/tests/DatabaseQueueTests.cpp), [실제 TCP·재시작](../../../tools/server_lab/tests/database_consistency.py)

## 수락·시각 검증

```powershell
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/database-consistency/implementation.json docs/diagrams/database-consistency/implementation.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/database-consistency/implementation.html --json
```

Archify2.16, architecture, showcase9/9, errors0·warnings0. 최종 spec/artifact SHA는 아래 검증 영수증 값으로 기록한다. 1440×900·1600×1000·1920×1080·2048×1320 containment와 실제1440 light/2048 dark PNG를 확인했다. 자동 영수증의 `visualReview: pending`과 실제 이미지 검토를 구분한다. 설명은 한글이며 고정 Viewer UI와 `<html lang>`은 영어 fallback이다.

- specification_sha256: `f43885c21cd1e710508afa4404853d34ba7fb223b36f78be5875da6f319e2d5b`
- artifact_sha256: `1796eee88bbc051072697518a9fbc8e25b1f16ca136802923d4b0cdaa056e89c`
- validation: 9/9 showcase, errors0, warnings0
- visual_review: passed (실제 PNG 확인, 자동 containment 영수증과 구분)
