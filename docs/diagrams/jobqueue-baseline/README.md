# JobQueue 기준 측정 구조

현재 측정 도구의 구조다. 운영 큐의 개선 설계도와 구분한다.
근거는 `scripts/Measure-JobQueue.ps1`·`Measure-JobQueue.py`, `Server/ServerCore/tests/JobQueueBenchmark.cpp`, Core `Concurrency.h`·`Metrics.h`다.
[실측/개선 검토](../../development/JOBQUEUE_BASELINE.md)와 [성능 검사 지침](../../guides/SERVER_PERFORMANCE.md)을 함께 참조한다.

Archify 2.16, architecture showcase 9/9·composition 오류/경고 0으로 validate/deliver했다.
1440×900·1600×1000·1920×1080·2048×1320 containment PASS. 최소/최대 크기 light/dark PNG 4장을 직접 확인했다.
자동 receipt의 visualReview=pending은 유지한다. 한글 설명을 사용하며 고정 Viewer UI와 HTML lang은 영어다.
PNG·visual-check sidecar는 Git 제외 생성물이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/jobqueue-baseline/jobqueue.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/jobqueue-baseline/jobqueue.architecture.json docs/diagrams/jobqueue-baseline/jobqueue.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/jobqueue-baseline/jobqueue.architecture.html --json
```

- JSON SHA-256: `732ec197f7e26c923ff29712302defdf96b9d38a20766d69b0bca007fe64e42d` (2,070 bytes)
- HTML SHA-256: `34912d26735dae3a89175b3af3bb632ae2d8f35a0d1ec7398a468ed3cf40111e` (705,708 bytes)
