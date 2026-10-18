# 로비 이동 예약·최신 상태 전달

Archify 2.16, sequence v1. 본문은 한글이며 고정 Viewer UI와 HTML lang은 영어 fallback이다.
[HTML](movement.html) · [JSON 원본](movement.sequence.json) · [구현·실측](../../development/LOBBY_MOVEMENT.md)

## 구현 근거

- `Server/Lobby_Server/CPacketMgr.cpp::MovePacket`: 기존 유저 FIFO에서 `ApplyMovementInput` 호출.
- `CClient.cpp::ApplyMovementInput`: 이전 방향 이동 적분, 새 방향 적용, 본인에게 즉시 응답, dirty 표시.
- `CClient.cpp::ScheduleMovementUpdate`: 예약/실행 대기 플래그 한 개, 15ms 뒤 기존 큐에 generation 보호 작업 추가.
- `CClient.cpp::RunMovementUpdate`: 최신 상태 전송, 정지 뒤 예약 종료, 실행 완료 뒤 다음 한 번 예약.
- `CNetworkMgr.cpp::TimerFunc/PrepareStop`: steady_clock·condition_variable 대기/중단, 타이머는 위치를 수정하지 않는다.
- `Server/ServerCore/include/ServerCore/Concurrency.h`: 기존 FIFO·budget·단일 소비자 그대로 사용한다.

본인 응답은 입력 처리 시점에 전송 요청한다. 실제 네트워크 수신 시간의 상한을 보장하지 않는다.
주변 유저는 예약 갱신 시 최신 상태를 받으므로 빠른 입력의 중간 방향을 모두 받지는 않는다.
지연 중 누락된 갱신 횟수를 나중에 재생하지 않으며 과부하에서 66.7Hz를 보장하는 구조가 아니다.
세션 generation이 달라진 옛 작업은 실행하지 않는다. CClient 수명은 producer join 이후 종료한다.

## 생성·검증

```powershell
node .agents/skills/archify/bin/archify.mjs validate sequence docs/diagrams/lobby-movement/movement.sequence.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver sequence docs/diagrams/lobby-movement/movement.sequence.json docs/diagrams/lobby-movement/movement.html --quality showcase --json
```

validate/deliver showcase 9/9·오류0·경고0. receipt는 `artifacts/logs/lobby-movement-diagram-receipt.json`에 보존한다.
JSON SHA-256 `a17a5ae3301aa48e4fa59d6732a9d4e502ca0fc1e8e54dd453be85fd9494d198`,
HTML SHA-256 `c0d9f1bdba9625fe2c43652fd4e259a0a26917ba403b8b8295951000a704cea2`.
이 대화의 로컬 파일 접근 차단 이력으로 visual-check·전체 브라우저 시각 검사는 수행하지 않았다.
