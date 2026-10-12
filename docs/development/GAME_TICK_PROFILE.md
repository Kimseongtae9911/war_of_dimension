# 게임 tick 로직 비용 분석 (2026-10-12)

[HTML 요약](GAME_TICK_PROFILE.html) · [도구 계약](SERVER_LAB.md#게임-tick-구간별-비용-분석) · [계측 흐름도](../diagrams/game-tick-profile/README.md).

## 1. 결과

HeightMesh 선형 검색이 지배 비용이다. 1매치 tick 실행 20.27ms, HeightRay 17.02ms/tick·tick CPU cycle 84.49%, 실제 기능/종료 검사 20/20. 최적화 전 분석이며 로직 최적화는 미적용이다.

## 2. 분석한 원인

MathUtil.h의 RayCast는 매 호출마다 HeightMesh127,756개/NavMesh5,743개 중 첫 hit까지 선형 순회한다. 방문 수는 높이 필터로 건너뛴 항목도 포함한다. 삼각형 법선·교차·내부 판정을 반복하며, 미니언 이동과 플레이어 이동 양쪽이 GameUtil::MapCollision을 호출한다. HeightMesh는 로드 시 평균 높이 내림차순으로 정렬되며 현재 첫 hit 순서는 게임 판정의 일부다.

## 3. 해결 방향

XZ 공간 후보 축소·정적 삼각형 계산 캐시를 우선 검토한다. 기존 first-hit 원본 순서·높이 필터·스킬/일반 Ray 차이를 유지하는 회귀가 먼저다. NavMesh의 현재 노드 활용과 정지 캐시는 위치 변경/강제 이동/지형 세대의 무효화 조건을 검증해야 한다. 모두 후속 제안이며 이번에는 미적용이다.

## 4. 실제 측정

Release x64 LOCAL_TEST·Ryzen5 7500F/논리 CPU12·입장 소프트 제한 해제·기존 perf-match·4인 매치·매회 재시작·워밍업5초/측정20초/250ms 입력. 각 인원 profile 켬5회, 최소 인원 끔5회는 seed 홀짝 실행 순서를 교차한다.

| 매치 / 인원 | tick 실행 평균 ms | 예약 지연 평균 ms | 프로세스 CPU % | 통과 |
|---|---:|---:|---:|---:|
| 1 / 4 | 20.27 [20.14–20.53] | 8.12 | 7.65 | 5/5 |
| 4 / 16 | 23.96 [23.63–24.10] | 11.01 | 30.90 | 5/5 |
| 16 / 64 | 31.03 [30.84–31.13] | 68.32 | 49.62 | 5/5 |

실행별 평균의 중앙값 [최소–최대]. CPU는 논리 CPU 전체로 정규화한다. wall은 스케줄링/대기를 포함하며 cycle은 QueryThreadCycleTime의 해당 worker 누적 차분이다. cycle을 시간으로 변환하지 않고 전체 프로세스 CPU 비중으로 해석하지 않는다. 큰 구간/세부 구간/소유자별 height_ray는 포함 관계이므로 중복 합산하지 않는다.

| 매치 | HeightRay ms/tick | tick cycle 비중 % | 방문 삼각형/Ray | 미니언 Update/tick |
|---|---:|---:|---:|---:|
| 1 | 17.02 | 84.49 | 35,799 | 3.74 |
| 4 | 20.63 | 86.75 | 39,739 | 3.95 |
| 16 | 28.34 | 91.38 | 50,261 | 4.56 |

1매치 계측 끔→켬: tick 20.34→20.27ms, CPU 7.67→7.65%. 각5회. 같은 실행 파일의 계측 플래그 대조이며, 원본 main 대비 비활성 경로 비용은 미측정이다. NPC 난수/매치 나이·활성 수가 완전히 고정되지 않으므로 차이를 순수 계측 오버헤드로 확정하지 않는다.

원시 결과: `artifacts/logs/game-tick-profile-20261012/`의 metadata/fingerprints·runs.jsonl·analysis.json·verification.json, 실행별 report.json·idle.json·lab/runs/*.json 및 서버 종료 로그. HTML 그래프는 report-charts의 Matplotlib PNG를 내장한다.

```powershell
python tools/server_lab/tick_profile.py --output artifacts/logs/game-tick-profile-new --clients 4 16 64 --repeats 5
python tools/server_lab/tick_profile.py --output artifacts/logs/game-tick-profile-new --analyze
python scripts/Build-GameTickProfileReport.py --input artifacts/logs/game-tick-profile-20261012
```

수집/API/저장 경로와 누락·오류·재시작·scope 포함 관계는 Server Lab 계약을 따른다. reporter snapshot은 동시 누적의 근사값이다. frame 합산 wall은 여러 worker의 합계이며 실제 elapsed와 구분한다. root에는 MatchUpdate 종료 경로도 포함할 수 있으므로 perf-match 측정 구간의 활성 매치만 해석한다.

실제 검증: Debug/Release 서버 빌드·Core 각218검사·Protocol ABI110종/106상수·Python33검사 PASS. Release 네트워크12종·보스4/5의4인 매치·초기화 실패4종 PASS. 성능20/20 정상 종료·양쪽 pending/sockets/leased=0, 모든 구간 cycle_errors=0, API/저장 결과 필드 확인. 소스/실행 파일/데이터340개 hash 유지 및 source-snapshot 보존. 기존 C4065/C4101/C4244/C4305/C4309 경고 유지. Astar는 이번 이동/정지 측정에서 호출0이며 전투 비용 판정에 사용하지 않는다. 그래프 PNG 직접 점검·HTML 정적 검사와 실제 HTML 시각 검토 미수행을 구분한다.

## 5. 추가 결함·대응 필요

| 항목 | 영향/상태 | 후속 |
|---|---|---|
| 이동 프레임 중복 | CMatch.cpp 동일 SendMovePacket 연속2회, 확인 | 독립 수정·프레임/tick 비교 |
| 정지 지형 검색 | CClient::Move 일반 경로에 변위0의 지형 검색 진입 가능, 코드 확인 | 정확한 정지 조건/캐시 무효화·정지 전용 측정 |
| 비동기 완료/스킬·전투 비용 | tick 밖 IOCP handler 및 근거리 전투 시나리오 미분해 | 지형 개선 후 잔여 비용 계측·전투/장시간 시나리오 |
| 다중 매치 상태 차이 | 순차 준비로 매치 나이·NPC 스폰 수가 다름 | 실제 미니언 Update/tick 함께 해석·상태 통제 후 확장 비교 |

기존 JobQueue 추가 후보/전용 도구는 제외했고 과거 보고서는 보존했다. 새 브랜치는 codex/game-tick-profiling이며 큐는 main 상태다. 커밋/푸시는 수행하지 않았다. 운영 DB·블록체인·전체 GUI 플레이·최대 처리량·장시간 전투는 미검증이다.
