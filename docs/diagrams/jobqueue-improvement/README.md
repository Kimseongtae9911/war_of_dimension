# JobQueue 공정성과 게임 전송 적체 개선 흐름

[JSON](jobqueue-improvement.architecture.json), [HTML](jobqueue-improvement.architecture.html), [원인·실측 비교](../../development/JOBQUEUE_IMPROVEMENT.md).

왼쪽은 로비의 작업 게시·FIFO·ready 대상 순환·worker 실행이다. 게임 CMatch는 IOCP tick 안에서 Snapshot 큐를 실행하며 ready 스케줄러를 쓰지 않는다.
오른쪽은 두 서버에 공통인 연결별 대기 프레임 묶음과 부분 전송·개별 완료 회수다. 서로 다른 원인과 실행 경로를 두 열로 표시했다.
근거: Core `Concurrency.h`의 JobQueue/JobScheduler, `Net.h`의 SocketState, `Net.cpp`의 IssueSend/Poll/Close/Disconnect,
로비 `CPacketMgr.cpp`의 MatchPacket과 게임 `CMatch.cpp`의 InGameUpdate, `Telemetry.cpp`와 `tools/server_lab/baseline.py`.
게임의 입력 누락/좌표 미갱신 가설과 TCP_NODELAY 단독 변경은 진단에서 배제했다.

Archify 2.16 validate/deliver showcase 9/9·composition 오류/경고 0.
1440×900·1600×1000·1920×1080·2048×1320 containment PASS. 최소/최대 light/dark PNG 4장을 직접 점검했다.
자동 receipt의 visualReview=pending은 유지한다. 고정 Viewer UI·HTML lang은 영어다. PNG·visual-check sidecar는 Git 제외 생성물이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/jobqueue-improvement/jobqueue-improvement.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/jobqueue-improvement/jobqueue-improvement.architecture.json docs/diagrams/jobqueue-improvement/jobqueue-improvement.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/jobqueue-improvement/jobqueue-improvement.architecture.html --json
```

JSON SHA-256 `a302e352a82f6a55b6c19091f5127d4226a2ae06628942c2ca2ca6116a08d09e` (3,844 bytes).
HTML SHA-256 `a3af3de30567d768ec85d8e7cad166c2777b154412b862bdca45512a0f140cf2` (708,650 bytes).
