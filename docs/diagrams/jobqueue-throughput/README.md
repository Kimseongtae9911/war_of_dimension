# JobQueue 게시와 실행 대기열 소유권 분리

> 과거 실험의 흐름도다. 2026-10-12 큐 후보와 전용 도구는 작업 트리에서 제외했으며 현재 구현을 나타내지 않는다. JSON/HTML은 당시 검증된 원본으로 보존한다.

[JSON](ownership.workflow.json) · [HTML](ownership.workflow.html) · [전후 실측](../../development/JOBQUEUE_THROUGHPUT.md).

생산자는 게시 mutex로 `m_jobs`를 보호한다. 소비자는 실행 mutex를 보유한 상태에서 `m_readyJobs`를 소진하며 비었을 때만 게시 mutex 안에서 swap한다. 개별 작업의 pop과 callback 실행은 게시 mutex를 사용하지 않는다. 남은 실행 대기열을 새 게시 작업보다 먼저 실행해 FIFO를 유지한다.

`m_queued`는 양쪽 대기열의 미시작 수를 포함하며 `HasJobs`가 조회한다. Five는 최대 5개, Snapshot은 진입 시 관측한 수가 예산이다. 예외/예산 소진 뒤에도 남은 작업을 보존한다. Clear는 실행 mutex를 획득한 뒤 게시 mutex 안에서 두 큐를 분리하고, 게시 mutex 밖에서 폐기한다. 그 파괴자가 게시한 새 작업은 보존한다. 같은 큐의 실행/Clear 재진입은 지원하지 않는다.

그림은 공통 큐 내부만 보여준다. 로비 JobScheduler의 ready 순환과 게임 CMatch의 tick 기반 호출 방식은 변경하지 않았다. 과거 [FIFO/전송 배치 구조도](../jobqueue-improvement/README.md)와 함께 읽는다.

근거: [Concurrency.h](../../../Server/ServerCore/include/ServerCore/Concurrency.h)의 JobQueue·JobScheduler, [회귀 검사](../../../Server/ServerCore/tests/ExtractionTests.cpp)의 BufferedQueueContracts·SchedulerHandoff·FairQueues, [단위 측정](../../../Server/ServerCore/tests/JobQueueBenchmark.cpp), [서버 측정](../../../tools/server_lab/baseline.py).

Archify 2.16 validate/deliver showcase 9/9, composition 오류/경고 0. 한글 노드와 설명을 사용하며 고정 Viewer UI·HTML lang은 영어로 fallback한다.
실제 브라우저 가독성·4개 desktop 크기의 containment·light/dark 시각 검토는 미수행이다. 앞선 local file URL의 브라우저 보안 거절을 다른 제공 방식으로 우회하지 않았다. 이는 정적 showcase 통과와 구분한다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/jobqueue-throughput/ownership.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/jobqueue-throughput/ownership.workflow.json docs/diagrams/jobqueue-throughput/ownership.workflow.html --quality showcase --json
```

JSON SHA-256 `099647eff98cb1ff2b0e16a50b9c4f89ded468617e6798bf630ec502149fa830` (2,300 bytes).
HTML SHA-256 `89078fccdbd14e0d48cb811d831e98dc69173fedec8ff69342faf8887cb41f6e` (705,856 bytes).
