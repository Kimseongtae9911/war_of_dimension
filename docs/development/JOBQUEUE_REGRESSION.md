# JobQueue 성능 악화 원인 조사 (2026-10-12)

> 과거 실험 기록: 2026-10-12에 해당 큐 후보와 전용 도구를 새 작업 브랜치에서 제외했다. 현재 JobQueue는 main 구현이며, 이 문서의 후보·도구·명령은 당시 결과를 설명한다. 원시 결과와 복구 사본은 artifacts에 보존한다.

[HTML 요약](JOBQUEUE_REGRESSION.html) · [실험 종료·복구 기록](CLEANUP.md#jobqueue-추가-실험-종료-2026-10-12) · [흐름도](../diagrams/jobqueue-regression/README.md) · [기존 결과](JOBQUEUE_THROUGHPUT.md).

## 1. 결과

1생산자·1소비자에서 큐 처리량 감소를 재현했다. 메모리 배치·경합 비용이 유력하며, 게임 tick 악화의 직접 원인은 미확정이다.
실제 시나리오 20/20 통과. 게임 전후 CPU 49.60→49.72%, tick 평균 실행 30.76→30.65ms다. 실제 서버 개선을 입증하지 못한 후보로 유지한다.

## 2. 분석한 원인

로비 `CClient::ProcessUpdate`는 작업 실행 중 이동을 이어갈 작업을 하나 다시 게시한다. 개선 큐는 ready가 매번 비는 조건에서 게시 mutex·swap을 반복하고 추가 atomic 증감 2회도 부담한다. 하지만 단일 스레드 self-five 계측 끔 비용 변화는 -3.6%이고 계측 켬에서는 소폭 증가했다. 범위가 겹치며 atomic 제거 대조군에서도 큰 차이가 없어 악화 원인으로 확정하지 않는다. 실제 로비 전체의 저하율과 동일한 값이 아니다.
기존 벤치마크의 1생산자·1소비자 조건을 2,000,000작업×5회 교차 측정하니 계측 끔에서 Five -8.1%·Snapshot -6.3%가 재현됐다. 독립 비용 probe의 큐/잠금/카운터 64바이트 정렬 대조는 계측 끔 143.14→122.08ns/job(-14.7%)였다. 원자 연산·메모리 순서·알고리즘을 유지하고 배치만 바꾸었으므로 메모리 배치/경합 비용이 유력하다. 특정 멤버의 false sharing·캐시 미스 비율·atomic 자체 기여율은 미확정이다.
게임 `CMatch::InGameUpdate`는 큐를 비운 뒤 이동·NPC·송신을 처리한다. 실제 게임 callback wall/tick wall 비율은 최대 0.0017%다. callback 계측은 큐 잠금/atomic을 제외하며 마이크로초 절삭·동시 snapshot 근사 한계가 있다.
원본 추가 진단에서도 p99 상한 262.144ms·낮은 tick 갱신량이 나타났다. 이전 게임 악화를 이번 큐 변경에 귀속할 근거는 부족하다. 환경 영향·잠금 대기·콘텐츠 비용 중 어느 것이 지배적인지 구간별 프로파일링은 미수행이다.

## 3. 해결 방식

적용한 변경은 측정 도구·비교 방식 보완이다. 전후 교차 재시작, PDH 시스템 부하 표본, 같은 측정 구간의 실제 갱신량·평균 지연·짝별 변화율을 저장한다. 큐 변경은 추가로 수정하거나 원복하지 않았다.
후속 제안은 서버 개선으로의 채택을 보류하고 실제 tick 지배 비용을 먼저 분해하는 것이다. 작은 묶음/자기 재등록도 비교 조건에 유지한다. atomic 제거 대조군은 단일 스레드만 지원하며 제품·동시성 해결책이 아니다. FIFO·Five/Snapshot·예외/Clear·lost wakeup 계약을 유지해야 한다.

## 4. 실제 측정

AMD Ryzen 5 7500F·12논리 코어·Windows 11·균형 조정 전원·VS2026/v145 Release x64 LOCAL_TEST. 64명, 게임 16매치×4명, 입장 제한 해제, 계측 켬, 워밍업5초/측정20초/250ms 입력. 로비/게임 전후 각5회, 홀수 seed AB/짝수 BA, 매번 서버 재시작. 같은 seed는 더미 방향만 고정하며 NPC 난수·매치 시작 시각·스케줄링은 고정하지 않는다.

| 지표 | before 중앙값 | after 중앙값 | 단위 |
|---|---:|---:|---|
| 게임 · CPU | 49.60 | 49.72 | % |
| 게임 · 평균 tick 지연 | 67.60 | 67.33 | ms |
| 게임 · 평균 tick 실행 | 30.76 | 30.65 | ms |
| 게임 · 매치별 갱신 | 12.10 | 12.14 | Hz |
| 로비 · CPU | 17.45 | 17.38 | % |
| 로비 · 평균 패킷 작업 대기 | 578.67 | 506.50 | ms |
| 로비 · 대기 p99 상한 | 2,097.15 | 2,097.15 | ms |
| 로비 · 미시작 패킷 | 96.00 | 87.00 | 개 |

| 시나리오 | seed | 버전 | 판정 | CPU % | 평균 패킷 대기 ms | 평균 tick 지연 ms | 매치별 tick Hz |
|---|---:|---|---|---:|---:|---:|---:|
| perf-match | 1 | before | passed | 49.62 | 39.07 | 67.31 | 12.14 |
| perf-match | 1 | after | passed | 49.61 | 37.91 | 67.00 | 12.18 |
| perf-match | 2 | after | passed | 49.38 | 38.70 | 67.18 | 12.16 |
| perf-match | 2 | before | passed | 49.59 | 37.25 | 66.37 | 12.28 |
| perf-match | 3 | before | passed | 49.70 | 38.52 | 67.60 | 12.10 |
| perf-match | 3 | after | passed | 49.72 | 38.62 | 67.33 | 12.14 |
| perf-match | 4 | after | passed | 49.72 | 38.69 | 67.67 | 12.09 |
| perf-match | 4 | before | passed | 49.58 | 39.53 | 68.62 | 11.95 |
| perf-match | 5 | before | passed | 49.60 | 38.82 | 68.54 | 11.96 |
| perf-match | 5 | after | passed | 49.73 | 37.97 | 68.38 | 11.99 |
| perf-lobby | 1 | before | passed | 17.34 | 554.26 | 226.24 | 미수집 |
| perf-lobby | 1 | after | passed | 17.22 | 507.07 | 217.46 | 미수집 |
| perf-lobby | 2 | after | passed | 17.38 | 474.19 | 201.65 | 미수집 |
| perf-lobby | 2 | before | passed | 17.32 | 587.12 | 226.54 | 미수집 |
| perf-lobby | 3 | before | passed | 17.48 | 761.13 | 225.72 | 미수집 |
| perf-lobby | 3 | after | passed | 17.67 | 485.68 | 208.78 | 미수집 |
| perf-lobby | 4 | after | passed | 17.29 | 506.50 | 205.63 | 미수집 |
| perf-lobby | 4 | before | passed | 17.45 | 514.00 | 215.49 | 미수집 |
| perf-lobby | 5 | before | passed | 17.54 | 578.67 | 215.97 | 미수집 |
| perf-lobby | 5 | after | passed | 17.44 | 520.57 | 213.87 | 미수집 |

단일 스레드 큐 비용: Release /O2 /MD, C++20 v145, 2,000,000작업×5회, 조건별 제외 워밍업1회, 실행 순서 반전. 실제 Move·송신·JobScheduler·시간 간격은 재현하지 않는다.

| 큐 경로 | 계측 | before ns/job | after ns/job | atomic 제거 대조 ns/job |
|---|---|---:|---:|---:|
| single | 끔 | 69.21 | 63.89 | 65.41 |
| single | 켬 | 148.49 | 148.62 | 146.29 |
| burst4 | 끔 | 58.47 | 53.98 | 52.80 |
| burst4 | 켬 | 138.10 | 136.24 | 136.05 |
| self-five | 끔 | 67.90 | 65.47 | 65.60 |
| self-five | 켬 | 144.34 | 146.54 | 145.47 |

기존 벤치마크의 단일 생산자·소비자 재측정: 별도 두 스레드, 2,000,000작업×5회, 제외 워밍업, AB/BA. 위 단일 스레드 probe와 다른 검사다. 기존 200,000작업의 -13.3% 감소와 수치는 직접 합산하지 않는다.

| 정책 | 계측 | before M jobs/s | after M jobs/s | 변화 % |
|---|---|---:|---:|---:|
| Five | 끔 | 7.771 | 7.145 | -8.06 |
| Five | 켬 | 5.561 | 6.884 | 23.79 |
| Snapshot | 끔 | 7.950 | 7.449 | -6.30 |
| Snapshot | 켬 | 5.024 | 6.603 | 31.43 |

별도 배치 비용 대조: 동일 probe에서 1생산자·1소비자 Snapshot, 2,000,000작업×5회. 게시/실행 queue·mutex·카운터를 각각 alignas(64)로 분리한 대조군이다. 스레드 affinity·캐시 하드웨어 카운터는 사용하지 않았다. 범위는 원시 summary와 그래프를 참조한다.

| 계측 | before ns/job | after ns/job | 정렬 대조 ns/job |
|---|---:|---:|---:|
| 끔 | 132.57 | 143.14 | 122.08 |
| 켬 | 189.51 | 141.55 | 127.59 |

원시 결과: `artifacts/logs/jobqueue-regression-20261012/`의 metadata/environment/runs/analysis/restoration.json 및 실행별 결과/PDH JSONL. 비용 대조는 `artifacts/logs/jobqueue-regression-cost-20261012/`에 소스·헤더·실행 파일 hash·빌드 로그·조건별 결과를 보존한다. 원본/개선 서버 실행 파일의 hash는 기존 `artifacts/logs/jobqueue-throughput-20261011/source-before`·`source-after`와 일치한다. 실패를 성공 표본으로 교체하지 않는다.

```powershell
python tools/server_lab/compare.py --before artifacts/logs/jobqueue-throughput-20261011/source-before --after artifacts/logs/jobqueue-throughput-20261011/source-after --output artifacts/logs/jobqueue-regression-new --clients 64 --repeats 5
python tools/server_lab/analyze_compare.py artifacts/logs/jobqueue-regression-new
python tools/server_lab/probe_queue_cost.py --before artifacts/logs/jobqueue-throughput-20261011/source-before --after artifacts/logs/jobqueue-throughput-20261011/source-after --output artifacts/logs/jobqueue-cost-new --nonatomic-control
python tools/server_lab/probe_queue_cost.py --before artifacts/logs/jobqueue-throughput-20261011/source-before --after artifacts/logs/jobqueue-throughput-20261011/source-after --output artifacts/logs/jobqueue-layout-new --cacheline-control
python scripts/Build-JobQueueRegressionReport.py --actual artifacts/logs/jobqueue-regression-20261012 --probe artifacts/logs/jobqueue-regression-cost-20261012 --historical artifacts/logs/jobqueue-throughput-20261011 --cacheline artifacts/logs/jobqueue-regression-cacheline-20261012
```

p99는 2배 폭 histogram bucket 상한으로 정확한 p99가 아니다. 평균은 sum/count 차분이다. callback wall은 큐 잠금/atomic 제외, tick wall은 여러 worker 합산으로 elapsed와 구분한다. PDH 미수집은 0으로 대체하지 않는다. OS processor performance 비율은 GHz·온도가 아니다. 기존 측정에 이 표본이 없어 환경을 사후 역산할 수 없다.
핵심 큐 구현·서버 바이너리는 이번에 다시 빌드하지 않았다. 비용 대조용 헤더 전용 실행 파일은 별도로 새로 빌드했다. 기존 Core/ABI/통합 회귀 결과는 이전 작업의 검증이며 이번 재실행으로 간주하지 않는다.

## 5. 추가 결함·대응 필요

| 항목 | 영향 | 상태/근거 | 다음 대응 |
|---|---|---|---|
| 이동 패킷 중복 | 매 tick player별 수신자에게 같은 프레임 2회 | CMatch.cpp SendMovePacket 연속2회, 기존 코드 | 별도 수정 후 송신량/tick 비교 |
| tick 비용 | 목표15ms를 전후 모두 초과 | 6worker·16매치·표 참조 | 이동/지형·NPC·송신·잠금 구간 분해 |
| 로그인 응답 누락 | 기존 after 1/10 준비 실패 | 이번 20회 재현 없음, 원인 미확정 | 로그인/세션 등록/송신 경계 추적 |
| 콘텐츠·환경 편차 | 고정 seed로 완전 동일 실행 불가 | NPC 난수/매치 나이·기존 시스템 표본 미확보 | 상태/구간 계측과 교차 측정 유지 |

Archify 도구 흐름도 showcase9/9·오류/경고0. Matplotlib PNG 직접 검토·HTML 정적 검사와 실제 HTML 시각 검증을 구분한다. 기존 local-file 브라우저 보안 거절을 우회하지 않아 HTML/desktop containment 검토는 미수행이다. 운영 DB·전체 GUI 게임 플레이·장시간 전투·계측 끔 서버 비교·최대 처리량은 미검증. 커밋/푸시 없음.
