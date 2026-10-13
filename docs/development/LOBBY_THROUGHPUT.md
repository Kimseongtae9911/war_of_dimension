# 로비 단일 채널 처리량 개선 근거

128명 입력 처리량 380.45 → 512.58패킷/초. 로비 CPU는 16.01% → 19.32%이며, 시야·전송 비용이 남습니다.

## 조건과 식별값

로직 소비자 1개·FIFO/budget·이동 주기·프로토콜 유지. 전후 동일 Release x64 LOCAL_TEST, 1채널, 입장 제한 해제, DB 없음.
CPU: Ryzen5 7500F, 6코어/12논리 스레드. 120초 인원별 각 1회 및 128명 20초 seed 1~5 각 5회. 상세 프로파일은 별도 1회.
Python/monitor: 3.12.14 (main, Aug 25 2026, 14:01:42) [MSC v.1944 64 bit (AMD64)]; 환경 원본은 artifacts/logs/lobby-throughput-machine-20261013.json·lobby-throughput-python-20261013.json.
소스·실행 파일·로비 OBJ·시나리오 SHA-256 및 source snapshot은 아래 metadata에 보존한다. 전후 배치는 순차 실행한다.

- before: 기준 commit `012d1046352a35c5edd255e1789800e3db996fed`, `artifacts/logs/lobby-throughput-soak-before-20261013/metadata.json`
  Lobby EXE SHA-256 `bb4edf37b17872ebdee1f2383eedac63927e855e18a64d7d03a3678b782beb7f`
- after: 기준 commit `012d1046352a35c5edd255e1789800e3db996fed`, `artifacts/logs/lobby-throughput-soak-after-20261013/metadata.json`
  Lobby EXE SHA-256 `e7852fa56a06f4fe4f09e6b9582c58462b5d779f0a2ba0e50e47b56bb0cd897e`

## 실제 측정

| 인원 | 구분 | 처리 패킷/s | 대기 개수 | p99 상한 ms | CPU % | 최종 정리 | 기능 |
|---|---|---:|---:|---:|---:|---|---|
| 64 | before | 254.53 | 412 | 4,194.30 | 17.07 | passed (6.28s) | True |
| 64 | after | 256.02 | 0 | 262.14 | 15.26 | passed (3.05s) | True |
| 96 | before | 328.36 | 6983 | 33,554.43 | 16.08 | failed (20.09s) | False |
| 96 | after | 384.55 | 35 | 1,048.58 | 19.72 | passed (3.23s) | True |
| 112 | before | 365.42 | 10760 | 33,554.43 | 16.30 | failed (20.12s) | False |
| 112 | after | 447.70 | 141 | 1,048.58 | 19.74 | passed (3.45s) | True |
| 128 | before | 380.45 | 16377 | 33,554.43 | 16.01 | failed (20.00s) | False |
| 128 | after | 512.58 | 106 | 2,097.15 | 19.32 | passed (4.38s) | True |

## 프로파일 구간과 비용

128명·20초 상세 계측 전후 각 1회. measure→verify 안의 monitor 표본 중 reporter 자체 timestamp도 측정 구간 내부인 서로 다른 sequence만 사용한다.
실제 차분 구간: 전 19.238초/2,600 Move, 후 18.224초/7,032 Move.
wall/Move는 실제 실행된 Move 전체 수로 나눈 평균이다. cycle 비중도 Move가 분모이며 전체 프로세스 CPU가 아니다. 같은 입력 조건이지만 ray 질의 좌표까지 고정한 비교는 아니다.

| 구간 | 전 wall ms/Move | 후 wall ms/Move | 전 cycle 비중 | 후 cycle 비중 |
|---|---:|---:|---:|---:|
| NavMesh | 0.005 | 0.003 | 0.07% | 0.11% |
| HeightMesh | 5.156 | 0.180 | 70.65% | 7.14% |
| 시야 탐색·주변 전송 | 2.145 | 2.381 | 28.93% | 92.07% |

## 원시 근거·재현

`artifacts/logs/lobby-throughput-{short,soak,profile}-{before,after}-20261013`의 summary.json과 실행별 report.json/lab/runs/*.json을 사용한다.
`artifacts/logs/lobby-throughput-analysis-20261013.json`은 구간 차분·반복·더미 지연·정리 판정의 통합 근거다.
```powershell
python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 128 --repeats 5 --headless --output artifacts/logs/<새-짧은-측정-폴더>
python scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 96 112 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/<새-유지-측정-폴더>
python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 128 --repeats 1 --profile --headless --output artifacts/logs/<새-프로파일-폴더>
python scripts/Build-LobbyThroughputReport.py
```

원시 JSON의 샘플은 측정 구간 안의 서로 다른 reporter sequence 차분이다. 없는 표본은 0으로 채우지 않는다. 실패 실행도 포함한다.
p99 bucket 상한은 RTT·클라이언트 체감 지연의 정확한 값이 아니다. 전체 CPU와 Move 구간 cycle 비중도 구분한다.
96명 이상 개선 후 더미 수신 CPU가 코어 하나의 90%를 넘었다. 이전 수용량 진단의 생성기 여유 기준을 통과한 결과로 해석하지 않는다. 처리량/최종 정리 기능 통과와 낮은 지연 SLO·최대 동접 판정을 구분한다.
120초 검사 각 1회는 5회 반복의 대체가 아니다. 기본 5회 반복은 짧은 128명 비교에 수행했다. 최대 동접·장시간 안정성은 미확정이다.

## 검증

Release 빌드, Core 1,479건·ABI 110종/106상수, 실제 OBJ 52,672건, 통합/실패 경로 7그룹, Python 49건, 로비 재접속 20 assertion·지형 API 확인 통과.
검사 원본: artifacts/logs/lobby-throughput-checks-20261013.json. 모든 20개 성능 실행의 종료·fingerprint 확인: artifacts/logs/lobby-throughput-verification-20261013.json.
보고서 편집은 기존 실측·회귀 결과를 사용한다. 서버 수정·새 부하 검사는 수행하지 않았다. 현재 보고서의 구조·내장 이미지·링크·원시 수치 대조 결과는 artifacts/logs/lobby-throughput-report-verification-20261013.json에 기록한다.
전체 HTML 브라우저 레이아웃과 Archify visual-check는 로컬 파일 접근 제한 이력 때문에 미검증이다. 내장 PNG 3개는 실제 이미지로 확인하고 HTML 5개 섹션·상세 펼침·링크·수치/단위 구조를 검사한다.
원본 OBJ와 첫 교차/float 높이 판정은 유지한다. 공간 인덱스와 캐시는 이미 공유된 Game_Server/MathUtil.h·TerrainSpatialIndex.h를 재사용한다.
기존 ViewList 재귀 shared lock·최초 업데이트 시간·타이머 idle CPU 문제는 이번 수정과 분리한 후속 과제다.
