# 실제 서버 기준 측정 구조

현재 코드의 관측/측정 경로이며 큐 최적화 설계가 아니다.
근거는 `tools/server_lab/baseline.py`·`performance.py`·`runner.py`·`monitor.py`, `scripts/Measure-ServerPerformance.ps1`·`Start-ServerLab.ps1`,
Core `Concurrency.h`·`Metrics.h`·`Telemetry.cpp`, 로비 `CClient::Move/DispatchFrame`과 게임 `CNetworkMgr::ProcessTimerEvent/MatchUpdate`다.
[실측·계약 문서](../../development/SERVER_BASELINE.md)를 함께 참조한다.

Archify 2.16 validate/deliver showcase 9/9·composition 오류/경고 0.
1440×900·1600×1000·1920×1080·2048×1320 containment PASS이며 최소/최대 light/dark PNG 4장을 직접 확인했다.
자동 receipt의 visualReview=pending은 유지하며 수동 점검과 구분한다. 고정 Viewer UI와 HTML lang은 영어다.
PNG·visual-check sidecar는 Git 제외 생성물이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/server-baseline/server-baseline.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/server-baseline/server-baseline.architecture.json docs/diagrams/server-baseline/server-baseline.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/server-baseline/server-baseline.architecture.html --json
```

- JSON SHA-256: `a0acaa861db78a3e1773225ba584b70e7d64df93e06a83af1897890856ed7c0f` (2,119 bytes)
- HTML SHA-256: `1a9be8a832d1c6719ec495c6a47637f632b2d1c712f529e2d1216cc579cf9d5a` (706,172 bytes)
