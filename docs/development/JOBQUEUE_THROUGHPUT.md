# JobQueue 추가 개선 실측

> 과거 실험 기록: 2026-10-12에 해당 큐 후보와 전용 도구를 새 작업 브랜치에서 제외했다. 현재 JobQueue는 main 구현이며, 이 문서의 후보·도구·명령은 당시 결과를 설명한다. 원시 결과와 복구 사본은 artifacts에 보존한다.

후속 판정(2026-10-12): 실제 서버 개선으로 채택할 근거가 부족하다. [교차 재측정·성능 악화 원인 조사](JOBQUEUE_REGRESSION.html)를 먼저 확인한다. 아래는 당시 결과를 보존한 기록이다.

[HTML 요약](JOBQUEUE_THROUGHPUT.html) · [소유권 흐름도](../diagrams/jobqueue-throughput/README.md)

## 결과

큐 단위 28조건 중 처리량 중앙값이 10% 넘게 증가한 조건 17개, 10% 넘게 감소한 조건 1개. 실제 서버의 고정 부하는 별도로 비교했다. 로비 종료 구간의 미시작 패킷은 중앙값 279개로, 이동 처리의 잔여 대기는 남아 있다. 서버 준비 실패 1회가 있어 게임 전후 표본 수가 다르며, 서버 성능 향상 확정 근거로 사용하지 않는다.

before-micro: 175/175 PASS · after-micro: 175/175 PASS · before-server: 10/10 PASS · after-server: 9/10 PASS

## 조건과 근거

- 기준: main 3e34e71a, 브랜치 codex/jobqueue-throughput-optimization. 기존 FIFO/ready 순환·IOCP 배치·Poll 책임 분리를 포함한다.
- 원시 경로: `artifacts/logs/jobqueue-throughput-20261011`. 전후 metadata.json·runs.jsonl·summary.json·서버 report/종료 로그와 comparison.json을 보존한다.
- before-micro/after-micro: 200,000작업·35조건·5회·seed 20261011. 계측 켬/끔·상세 대기·공정성·idle 포함. burst는 preload를 제외한 drain 처리량이다.
- before-server/after-server: 64명 로비/게임 각 seed 1~5·계측 켬. 기존 perf-lobby/perf-match hash 동일, 워밍업 5초·측정 20초·250ms 입력, 입장 제한 해제, 게임 매치당 4명 유지.
- 실제 서버는 64명에 집중했다. 4/16명 성능·계측 끔 서버 대조·최대 처리량·장시간 전투는 이번 추가 측정 범위 밖이다. 큐 단위 계측 켬/끔 비교와 구분한다.
- 전후를 순차 실행했으며 affinity·온도·백그라운드 부하는 통제하지 않았다. 작은 변화는 효과 보장으로 해석하지 않는다.

## 큐 단위 처리량

단위: 백만 jobs/s, 각 5회 중앙값. 원시 실행/요약에 최소·최대·CPU·지연 분포가 있다.

| 부하 | 정책 | 계측 | 개선 전 | 개선 후 | 변화 |
| --- | --- | --- | --- | --- | --- |
| single | Five | 끔 | 6.749 | 6.614 | -2.0% |
| producers4 | Five | 끔 | 3.884 | 6.074 | +56.4% |
| consumers4 | Five | 끔 | 3.518 | 5.946 | +69.0% |
| queues32 | Five | 끔 | 23.593 | 22.073 | -6.4% |
| burst | Five | 끔 | 13.325 | 15.120 | +13.5% |
| preallocated | Five | 끔 | 9.533 | 10.833 | +13.6% |
| mixed | Five | 끔 | 22.934 | 26.056 | +13.6% |
| single | Five | 켬 | 5.067 | 6.146 | +21.3% |
| producers4 | Five | 켬 | 2.828 | 5.515 | +95.0% |
| consumers4 | Five | 켬 | 2.495 | 5.006 | +100.6% |
| queues32 | Five | 켬 | 10.793 | 9.852 | -8.7% |
| burst | Five | 켬 | 6.535 | 7.101 | +8.7% |
| preallocated | Five | 켬 | 4.389 | 6.373 | +45.2% |
| mixed | Five | 켬 | 9.461 | 9.700 | +2.5% |
| single | Snapshot | 끔 | 7.043 | 6.107 | -13.3% |
| producers4 | Snapshot | 끔 | 3.945 | 6.160 | +56.1% |
| consumers4 | Snapshot | 끔 | 4.040 | 6.729 | +66.6% |
| queues32 | Snapshot | 끔 | 21.920 | 21.721 | -0.9% |
| burst | Snapshot | 끔 | 14.388 | 15.085 | +4.8% |
| preallocated | Snapshot | 끔 | 10.252 | 10.508 | +2.5% |
| mixed | Snapshot | 끔 | 25.877 | 29.823 | +15.2% |
| single | Snapshot | 켬 | 5.233 | 6.175 | +18.0% |
| producers4 | Snapshot | 켬 | 2.908 | 6.010 | +106.7% |
| consumers4 | Snapshot | 켬 | 2.880 | 5.860 | +103.5% |
| queues32 | Snapshot | 켬 | 10.099 | 10.661 | +5.6% |
| burst | Snapshot | 켬 | 7.184 | 6.917 | -3.7% |
| preallocated | Snapshot | 켬 | 4.608 | 6.650 | +44.3% |
| mixed | Snapshot | 켬 | 8.795 | 9.823 | +11.7% |

## 실제 서버

| 서버 | 지표 | 개선 전 | 개선 후 |
| --- | --- | --- | --- |
| 로비 | packets/s | 253.605 (n=5) | 252.058 (n=5) |
| 로비 | jobs/s | 511.556 (n=5) | 503.567 (n=5) |
| 로비 | CPU % | 16.692 (n=5) | 16.384 (n=5) |
| 로비 | 작업 대기 p99 상한 ms | 4,194.304 (n=5) | 4,194.304 (n=5) |
| 로비 | 패킷 대기 p99 상한 ms | 4,194.304 (n=5) | 4,194.304 (n=5) |
| 로비 | tick 지연 p99 상한 ms | 2,097.152 (n=5) | 2,097.152 (n=5) |
| 로비 | 종료 시점 미시작 패킷 | 220.000 (n=5) | 279.000 (n=5) |
| 게임 | packets/s | 255.960 (n=5) | 255.681 (n=4) |
| 게임 | jobs/s | 255.960 (n=5) | 255.681 (n=4) |
| 게임 | CPU % | 45.212 (n=5) | 40.421 (n=4) |
| 게임 | 작업 대기 p99 상한 ms | 131.072 (n=5) | 262.144 (n=4) |
| 게임 | 패킷 대기 p99 상한 ms | 131.072 (n=5) | 262.144 (n=4) |
| 게임 | tick 지연 p99 상한 ms | 131.072 (n=5) | 262.144 (n=4) |
| 게임 | 종료 시점 미시작 패킷 | 0.000 (n=5) | 0.000 (n=4) |

p99는 실행별 histogram bucket 상한의 중앙값이며 정확한 RTT가 아니다. 로비 tick은 이동 갱신 간격−1ms, 게임 tick은 15ms 예약 목표 초과이므로 서로 직접 비교하지 않는다. CPU는 전체 논리 CPU 기준이다. 고정 입력률은 최대 TPS가 아니며 jobs/s에는 로비의 자기 재등록 작업이 포함된다.

## 추가 대응

| 항목 | 영향 | 상태/근거 | 다음 대응 |
| --- | --- | --- | --- |
| 게임 로그인 응답 누락 | 준비 실패로 해당 실행의 성능 표본 없음 | Connected 로그·미시작 작업 0, 원인 미확정 | 로그인/송신/수신 경계를 별도 재현·추적 |
| 로비 이동 자기 재등록 | 작업·전송·시야 처리 비용이 계속 발생 | 기존 경로 유지 | TODO 78의 주기/이동 재설계 |
| 실제 서버 대기/지연 악화 | 로비 잔여 패킷·게임 p99 증가 관측 | 게임 표본 수 차이·원인 미확정 | 동일 seed 쌍·서버 준비/타이머/송신 지연 추가 분해 |
| 조건별 편차/악화 | 모든 부하에 같은 효과를 보장하지 않음 | 10% 초과 감소 1조건 | 범위와 계측 비용을 함께 확인 |
| 추가 대기열 메모리 | 큐마다 deque 하나와 카운터 추가 | 상시 구조 비용 증가 | 저부하/최대 세션 메모리 별도 평가 |
| 실제 서버 검사 범위 | 64명 고정 입력률·계측 켬 | 최대 TPS/전투/장시간 검사가 아님 | 후속 한계 부하와 재설계 전후 측정 |
| 브라우저 검증 | 실제 HTML 화면/containment 미확인 | 기존 file URL 보안 제한 | 정적 검사와 실제 화면 검증 구분 |

## 재생성

```powershell
.runtime/report-venv/Scripts/python.exe scripts/Build-JobQueueThroughputReport.py --input artifacts/logs/jobqueue-throughput-20261011
```

Python의 matplotlib과 Malgun Gothic이 필요하다. 생성된 HTML에는 외부 네트워크 의존성이 없다. 기존 JobQueue 개선 보고서와 당시 결과는 덮어쓰지 않는다. 실제 코드/회귀 검증은 tasks/todo.md의 JobQueue 추가 처리 비용 개선 항목을 참조한다.

## 전후 성공 seed 쌍

| 시나리오 | seed | 지표 | 개선 전 | 개선 후 |
| --- | --- | --- | --- | --- |
| perf-lobby | 1,2,3,4,5 | CPU % | 16.692 | 16.384 |
| perf-lobby | 1,2,3,4,5 | tick p99 상한 ms | 2,097.152 | 2,097.152 |
| perf-lobby | 1,2,3,4,5 | 패킷 대기 p99 상한 ms | 4,194.304 | 4,194.304 |
| perf-match | 1,2,4,5 | CPU % | 44.931 | 40.421 |
| perf-match | 1,2,4,5 | tick p99 상한 ms | 131.072 | 262.144 |
| perf-match | 1,2,4,5 | 패킷 대기 p99 상한 ms | 131.072 | 262.144 |

seed 교집합만 별도 대조한다. 준비 실패는 전체 성공/실패 분모에 남긴다. NPC 상태와 실행 시점까지 고정한 비교가 아니며 작은 차이를 효과로 확정하지 않는다.

## 실패 실행과 통계 분모

| 측정 | seed | 실패 |
| --- | --- | --- |
| after-server | 3 | client 11 / match-transfer: client 11 / game-login: SC_LOGIN_INFO: 기대 응답 없음 (10s) |

해당 실행은 준비 단계에서 실패해 성능 표본이 없고 전체 성공/실패 분모에 남긴다. 게임 수치는 전 seed 1~5와 후 성공 seed 1·2·4·5의 중앙값이다. 표본 차이가 있으므로 서버 성능 향상 확정에 사용하지 않는다. 실패 실행의 GameServer Connected 로그와 마지막 snapshot에서 jobs_queued=0·jobs_failed=0·io_errors=0·send_queue.count=0을 확인했으나 native 송신 성공/실제 수신까지의 구간은 확정하지 못했다. 해당 로그인 경로 ClientConnect→SendLoginPacket은 JobQueue callback 등록을 거치지 않는다. 큐 변경과 무관하다는 결론은 아직 내리지 않는다.

## 상세 대기·공정성·idle

| 부하 | 정책 | 지표 | 개선 전 | 개선 후 |
| --- | --- | --- | --- | --- |
| fairness | Five | cold 작업 대기 ms | 0.460 | 0.456 |
| fairness | Snapshot | cold 작업 대기 ms | 0.404 | 0.363 |
| idle | Five | CPU seconds · 1초 대기 | 0.000 | 0.000 |
| producers4 | Five | enqueue 시도→시작 p99 ms · 1/67 표본 | 36.162 | 1.931 |
| producers4 | Snapshot | enqueue 시도→시작 p99 ms · 1/67 표본 | 34.916 | 0.917 |
| queues32 | Five | enqueue 시도→시작 p99 ms · 1/67 표본 | 5.921 | 5.303 |
| queues32 | Snapshot | enqueue 시도→시작 p99 ms · 1/67 표본 | 7.731 | 8.918 |

상세 표본은 실행당 2,986개이며 전체 ID의 중복/누락을 검사했다. fairness는 전후 모두 cold 전에 완료한 hot 작업 수 중앙값 0이다. idle CPU 0은 Windows 프로세스 CPU 시간 해상도 미만을 포함한다.

## 처리량 반복 범위

백만 jobs/s · 중앙값 [최소–최대].

| 부하 | 정책 | 계측 | 개선 전 | 개선 후 |
| --- | --- | --- | --- | --- |
| single | Five | 끔 | 6.749 [6.254–7.729] | 6.614 [6.216–6.863] |
| producers4 | Five | 끔 | 3.884 [3.805–4.134] | 6.074 [5.753–6.552] |
| consumers4 | Five | 끔 | 3.518 [3.345–3.542] | 5.946 [5.854–6.090] |
| queues32 | Five | 끔 | 23.593 [21.562–25.629] | 22.073 [5.281–23.803] |
| burst | Five | 끔 | 13.325 [11.502–13.731] | 15.120 [13.887–16.850] |
| preallocated | Five | 끔 | 9.533 [8.438–11.384] | 10.833 [9.590–12.593] |
| mixed | Five | 끔 | 22.934 [21.640–25.244] | 26.056 [14.513–27.671] |
| single | Five | 켬 | 5.067 [5.013–5.260] | 6.146 [5.899–6.589] |
| producers4 | Five | 켬 | 2.828 [2.755–2.850] | 5.515 [5.241–5.676] |
| consumers4 | Five | 켬 | 2.495 [2.470–2.518] | 5.006 [4.937–5.234] |
| queues32 | Five | 켬 | 10.793 [9.997–10.824] | 9.852 [7.090–10.681] |
| burst | Five | 켬 | 6.535 [5.755–6.867] | 7.101 [6.318–7.577] |
| preallocated | Five | 켬 | 4.389 [4.196–4.510] | 6.373 [6.303–6.554] |
| mixed | Five | 켬 | 9.461 [9.157–10.229] | 9.700 [9.560–9.889] |
| single | Snapshot | 끔 | 7.043 [6.363–7.903] | 6.107 [4.746–6.757] |
| producers4 | Snapshot | 끔 | 3.945 [3.732–4.155] | 6.160 [5.514–6.314] |
| consumers4 | Snapshot | 끔 | 4.040 [3.750–4.182] | 6.729 [6.355–6.819] |
| queues32 | Snapshot | 끔 | 21.920 [19.957–23.708] | 21.721 [20.774–23.143] |
| burst | Snapshot | 끔 | 14.388 [13.054–14.679] | 15.085 [13.940–16.825] |
| preallocated | Snapshot | 끔 | 10.252 [9.392–11.930] | 10.508 [9.091–13.244] |
| mixed | Snapshot | 끔 | 25.877 [10.028–26.720] | 29.823 [23.503–31.839] |
| single | Snapshot | 켬 | 5.233 [4.995–5.338] | 6.175 [5.264–7.079] |
| producers4 | Snapshot | 켬 | 2.908 [2.831–3.024] | 6.010 [5.361–6.301] |
| consumers4 | Snapshot | 켬 | 2.880 [2.745–2.961] | 5.860 [5.385–6.236] |
| queues32 | Snapshot | 켬 | 10.099 [8.304–11.270] | 10.661 [10.299–11.481] |
| burst | Snapshot | 켬 | 7.184 [6.181–7.670] | 6.917 [6.358–7.599] |
| preallocated | Snapshot | 켬 | 4.608 [4.458–4.782] | 6.650 [6.371–6.964] |
| mixed | Snapshot | 켬 | 8.795 [7.916–9.842] | 9.823 [8.853–10.213] |

## 소스/데이터 식별

큐 헤더 SHA-256 전: `5f75880c975e996774132cb39e4cc03356fa8135288fc5abfa774e723b41ade2`, 후: `97932c21e9fb9ed223f70e37bf841ebdf84388766d59f19d935c1f7f2ba03da5`.

기준 바이너리와 원본 큐 헤더는 `source-before/`에 별도 보존했다. 기준 서버 측정이 끝난 뒤 구현 바이너리를 빌드했으며 측정 중에는 실행 파일을 교체하지 않았다. 측정기·계측 헤더·시나리오 hash 전후 동일, 서버 Resource CSV/OBJ/TXT 35개 hash 동일을 생성 시 확인한다. 자세한 실행 파일 hash는 각 metadata.json에 있다.

## 실제 검증과 추가 진단

Debug/Release Core 각 223 checks · ABI 110타입/106상수 · Release 네트워크 12개/보스 4·5/초기화 실패 4경로 PASS. 기존 빌드 경고는 남아 있다.

원본 서버 바이너리로 추가 진단 3회: 실패 0회. 로그인 실패는 재현되지 않았으며 원인과 이번 변경의 관련성은 미확정이다. 이 진단을 기본 성능 표본에 합치지 않았다.

검증 원문은 `verification.json`·`provenance.json`, `artifacts/logs/test-server-core-{Debug,Release}/`, `artifacts/logs/server-core-integration-Release/`에 보존한다. 원본 진단의 바이너리 hash는 최초 서버 기준 측정과 일치하고 작업 폴더의 헤더는 개선 후보이므로 진단의 소스는 보존한 원본 헤더/바이너리로 식별한다. 진단 뒤 개선 후보 실행 파일 복원을 SHA-256으로 확인했다.

회귀는 두 대기열 FIFO·Five/Snapshot 예산·callback 예외 뒤 보존·양쪽 Clear·캡처 파괴 중 재등록·512회 scheduler handoff·기존 다중 생산/소비 직렬성을 포함한다. HTML/SVG 정적 검사와 Matplotlib 그래프 PNG 검토만 수행했고 실제 HTML 브라우저/containment 검토는 미수행이다. 운영 DB·블록체인·전체 GUI 게임 플레이·Debug 서버 통합·장시간/전투 부하는 미검증이다.
