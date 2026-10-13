# 실제 서버 동접 한계 조사 근거

[HTML 보고서](SERVER_CAPACITY.html) · [측정 흐름](../diagrams/server-capacity/README.md).

## 결과

- game: 1000명 목표 중 실제 준비596명, generator-limited; 추가 로그인 중 더미 발행 지연 1초 초과; 서버 종료 True
- lobby: 128명 목표 중 실제 준비128명, failed; 이동/정지 실패 11/128명; 서버 종료 True

게임 임시 품질5회 통과: 5회 모두 충족한 인원 없음.

로비120초 입력 처리량 유지 기준의 최대 확인 인원: 64명. 96명부터 입력보다 처리량이 낮아 적체가 관측됐다. 게임512명은128개의4인 매치이며, 596명 확장 중 더미 지연1.484초로 중단해 서버 절대 최대를 확정하지 못했다.

## 실행 자료

기준 커밋 `012d1046352a35c5edd255e1789800e3db996fed`, main 작업 트리의 추가 계측·더미 도구를 사용했다. Ryzen5 7500F(6코어/12논리 스레드), RAM33,933,996,032bytes, Windows11, 균형 전원 계획, Release x64 LOCAL_TEST, DB 없음. 게임250매치×4명=1000명/로비3000명의 고정 저장 공간은 유지하고 WOD_PERF_UNCAPPED=1로 입장 소프트 제한을 풀었다. 실행마다 metadata의 파일 SHA-256, 소스/실행 파일 snapshot 또는 기존 Git 기준과 source.patch, 종료 상태를 보존했다.

- `artifacts/logs/capacity-probe-game-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-ramp-game-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-ramp-game-v2-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-ramp-game-headless-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-ramp-lobby-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-boundary-game-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-boundary-lobby-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-control-profileoff-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.
- `artifacts/logs/capacity-control-metricsoff-20261013`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.

원시 결과는 Git 제외이며 실패/취소도 보존했다. 두 전체 월드 누적 실험은 도구 비용 조사 후 수동 중단했으며 서버 한계 실패로 집계하지 않는다. headless는 NPC 위치 의미 검증을 생략하지만 서버 NPC 처리와 수신/플레이어 검사는 유지한다.

## 실행별 수치

### game

| 요청 인원 | 준비 인원 | 상태 | Hz/매치 | 서버 CPU % | tick 실행 ms | 타이머 초과 ms | Post→handler ms | 오류 |
|---|---|---|---|---|---|---|---|---|
| 4 | 4 | passed | 59.68 | 1.19 | 2.44 | 1.72 | 0.03 |  |
| 16 | 16 | passed | 60.73 | 10.60 | 4.16 | 1.40 | 0.07 |  |
| 32 | 32 | passed | 51.58 | 7.65 | 3.22 | 3.89 | 0.49 |  |
| 64 | 64 | passed | 42.86 | 14.05 | 2.66 | 6.86 | 1.49 |  |
| 128 | 128 | passed | 37.80 | 22.34 | 2.39 | 8.51 | 2.94 |  |
| 256 | 256 | passed | 26.24 | 36.45 | 2.45 | 7.42 | 15.69 |  |
| 512 | 512 | passed | 11.86 | 32.31 | 2.44 | 7.97 | 61.37 |  |
| 1000 | 596 | generator-limited | 미수집 | 미수집 | 미수집 | 미수집 | 미수집 | 추가 로그인 중 더미 발행 지연 1초 초과 |

### lobby

| 요청 인원 | 준비 인원 | 상태 | Hz/매치 | 서버 CPU % | tick 실행 ms | 타이머 초과 ms | Post→handler ms | 오류 |
|---|---|---|---|---|---|---|---|---|
| 4 | 4 | passed | 미수집 | 12.63 | 미수집 | 미수집 | 미수집 |  |
| 16 | 16 | passed | 미수집 | 14.80 | 미수집 | 미수집 | 미수집 |  |
| 32 | 32 | passed | 미수집 | 13.86 | 미수집 | 미수집 | 미수집 |  |
| 64 | 64 | passed | 미수집 | 14.90 | 미수집 | 미수집 | 미수집 |  |
| 128 | 128 | failed | 미수집 | 18.07 | 미수집 | 미수집 | 미수집 | 이동/정지 실패 11/128명 |

## 경계 반복과 계측 비용

| 게임 동접 | 기능/종료 | 임시 품질 | Hz 중앙값 | Hz 범위 |
|---|---|---|---|---|
| 16 | 5/5 | 1/5 | 58.28 | 57.11–60.29 |
| 20 | 5/5 | 2/5 | 59.99 | 58.36–60.42 |
| 24 | 5/5 | 0/5 | 58.51 | 57.79–58.81 |

| 로비 동접 | 기능/종료 | 패킷 대기 p99 상한 중앙값 ms | 끝 미처리 중앙값 | 실패 |
|---|---|---|---|---|
| 64 | 5/5 | 2,097.15 | 132 | 없음 |
| 128 | 5/5 | 8,388.61 | 2,555 | 없음 |

| 16명 조건 | 기능/종료 | CPU 중앙값 % | CPU 범위 % | Hz 중앙값 |
|---|---|---|---|---|
| 계측/구간 프로파일 켬 | 5/5 | 7.09 | 6.67–7.53 | 58.28 |
| 계측 켬/구간 프로파일 끔 | 5/5 | 6.48 | 5.60–7.34 | 60.84 |
| 계측/구간 프로파일 끔 | 5/5 | 5.68 | 4.72–7.99 | 미계측 |

20초 경계 실행은 매번 서버를 재시작하고 seed1~5, 워밍업5초/입력250ms다. 누적 탐색과 달리 준비 중 기존 참가자에게 이동 입력을 주지 않는다. 순차 실행한 계측 켬/끔은 OS 변동을 완전히 통제하지 못하며 두 신규 histogram만의 오버헤드가 아니다. 누락 지표는 0으로 대체하지 않는다.

## 로비120초 유지와 최종 입력 정리

| 동접 | 성공 | 처리 패킷/s | 처리량 유지 | 측정 끝 대기 | p99 상한 ms | 정리 판정 | 정리 ms | 오류 |
|---|---|---|---|---|---|---|---|---|
| 64 | True | 256.06 | True | 164 | 4,194.30 | passed | 4,219.00 | 없음 |
| 96 | True | 369.27 | False | 2036 | 8,388.61 | passed | 16,328.00 | 없음 |
| 112 | False | 399.07 | False | 6438 | 33,554.43 | failed | 20,062.00 | 최종 입력 처리/패킷·송신 대기열 정리 timeout: {"timestamp_ms": 1791869314606, "sequence": 151, "processed_packets": 55713, "pending_packets": 511, "queued_sends": 0} |
| 128 | False | 433.54 | False | 10350 | 33,554.43 | failed | 20,062.00 | 최종 입력 처리/패킷·송신 대기열 정리 timeout: {"timestamp_ms": 1791868960809, "sequence": 150, "processed_packets": 61514, "pending_packets": 2742, "queued_sends": 0} |

`artifacts/logs/capacity-soak-lobby-20261013`(64/128명)·`capacity-soak-refine-20261013`(96/112명)의 각1회 seed3 진단이다. 처리량 유지 기준은 완료 패킷/s가 동접×4/s의99% 이상, 정리/기능/종료 성공, 발행 p99<=250ms/더미 CPU<0.9코어다. 64/128명 결과 이후 세분화 전에 정했으며 낮은 지연을 보장하는 SLO가 아니다. 5초 워밍업/120초 유지 후20초 내 최종 입력 수 이상 처리·패킷/송신 대기0을 새 reporter 표본3개로 확인한다. 기존20초/누적 검사는 이 조건이 없어 성공을 최종 명령 완료 보장으로 해석하지 않는다. 신규 settlement 코드/표본 전달이 포함된 별도 소스 hash를 보존했으며 서버 실행 파일은 동일하다. 운영 장시간·다른 참가자가 있는 서버의 개별 ACK 검증은 아니다.

## 원인과 코드 근거

- 게임 `CNetworkMgr.cpp` TimerFunc/ProcessTimerEvent/MatchUpdate: 미래 이벤트 pop/push+1ms sleep, 전달 전 초과와 전달 후 대기를 분리. 512명 tick 본문2.44ms에 비해 IOCP 진입 이후 대기61.37ms. 단순 CPU 전체 사용률만으로 판단하지 않는다.
- 게임 `CMatch.cpp` InGameUpdate: 같은 SendMovePacket을 연속 두 번 호출. 제거 효과는 아직 측정하지 않았다.
- 로비 `CServer.cpp` Run: IOCP4개와 별개로 JobScheduler 소비자1개. `CClient.cpp` ProcessUpdate는 이동 중 자신을 다시 등록하고 Move에서 충돌/가시성 검사를 수행한다. `CViewList.cpp`는 채널 전체를 순회한다. 밀집 참가자 증가에 따라 이 단일 경로의 처리량·공정성이 제한되는 구조이며 세부 함수별 CPU 비율은 미계측이다.
- 로비128명 누적 단계: 미처리3802개, 입력 목표512/s 대비 완료418.995/s, 11명의 이동 응답/위치 변화 검증 실패. 20초 동안4명은 자기 이동 응답0회, 7명은1회로 두 위치 간 변화 검증 불가였다. 더미 CPU31.02% of one core·발행 주기 시작 p99 지연0ms로 생성기 포화 판정에 해당하지 않았다.
- 로비 `CClient.h` m_updateTime={} → Initialize에서 미설정 → LoginPacket의 ProcessUpdate → Move: 첫 경과 시간이 epoch 규모로 기록된다. 전체 누적 histogram을 오염시키며 신규 참가자가 없는 측정 구간 차분과 구분해야 한다.
- 로비 `CViewList.cpp` CheckViewList: 호출부가 nearClient의 m_viewLock을 shared 획득하고 내부 CheckViewList(int)도 같은 lock_shared를 수행한다. 중첩 잠금은 코드에서 확인했지만 이번 실패를 교착 때문이라고 확정하지 않는다.
- `capacity-ramp-game-20261013/thread-cpu.jsonl`: 58.56초 동안 로비 최대 스레드 약0.98코어. TimerFunc 빈 큐 yield 루프와 일치하는 가설이며 함수 주소를 식별하지 않아 원인 확정이 아니다.

| 게임 동접 | Jobs CPU cycle % | 미니언 % | HeightRay % | NavRay % | 플레이어 송신 % |
|---|---|---|---|---|---|
| 4 | 0.09 | 47.50 | 29.93 | 13.93 | 22.00 |
| 16 | 0.05 | 53.50 | 32.88 | 12.31 | 20.59 |
| 32 | 0.06 | 62.31 | 43.58 | 14.92 | 9.74 |
| 64 | 0.06 | 63.49 | 44.58 | 15.52 | 9.58 |
| 128 | 0.07 | 64.10 | 46.68 | 15.49 | 8.96 |
| 256 | 0.09 | 66.77 | 48.89 | 16.30 | 6.85 |
| 512 | 0.19 | 70.04 | 52.38 | 16.81 | 5.53 |

## 재현

```powershell
python tools/server_lab/capacity.py --scenario capacity-match --clients 1000 --seed 3 --profile --headless --output artifacts/logs/capacity-game-new
python tools/server_lab/capacity.py --scenario capacity-lobby --clients 3000 --seed 3 --headless --output artifacts/logs/capacity-lobby-new
python scripts/Measure-CapacityBoundary.py --scenario perf-match --clients 16 20 24 --repeats 5 --profile --headless --output artifacts/logs/capacity-boundary-new
python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 64 128 --repeats 5 --headless --output artifacts/logs/capacity-lobby-boundary-new
python scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 96 112 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/capacity-soak-new
```

## 검증 범위

Release 서버 빌드·Core1479검사/ABI110종·106상수/실제 지형27904비교, Python49검사 통과. 기존 컴파일 경고 유지. 정상 종료는 각 실행 report의 shutdown_ok와 pending=0/sockets=0/leased=0 로그로 판정한다. 실패 후 정상 종료는 미처리 업무 완료를 뜻하지 않으며 settlement 실패를 유지한다. 전체 GUI·운영DB·전투와 HTML 실브라우저 시각 검사는 수행하지 않았다. 보고서 PNG와 HTML 구조/링크는 확인했다. 기본15ms 예약의 이론66.7Hz와 실측 갱신률을 구분한다. 구간 프로파일은 중첩하므로 합산하지 않는다.
