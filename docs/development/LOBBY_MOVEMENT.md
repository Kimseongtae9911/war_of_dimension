# 로비 이동 예약 개선 (2026-10-13)

[HTML 보고서](LOBBY_MOVEMENT.html) · [흐름도](../diagrams/lobby-movement/README.md)

## 결과·비교 조건

기준은 1차 지형 개선 커밋 `90a9e79f`의 로비 실행 파일이며, 그때 확보한 동일 시나리오 실측을 재사용한다. 이전 지형 선형 탐색 버전과 섞지 않는다. 새 개선은 같은 브랜치의 미커밋 구현이다.
Release x64 LOCAL_TEST·DB 없음·입장 제한 해제·단일 채널·원본 OBJ·headless·계측 ON/상세 프로파일 OFF. 12논리 코어 CPU 정규화 비율이다.
20초 검사 전후128명 각5회(seed1–5), 새64명5회는 새 구현 제어 검사이며 같은 시간의 이전64명 값은 없어 직접 비교하지 않는다.
120초 유지 전후64/128명 각각1회(seed3). 워밍업5초, 이동/정지250ms. 최대 동접 검사로 해석하지 않는다.

128명20초 중앙값: 처리율 515.81→512.16패킷/s, 패킷 대기 p99 bucket 상한 1048.576→524.288ms, CPU 20.17→11.77%.
세부 중앙값/범위와 원본 행은 `artifacts/logs/lobby-movement-analysis-20261013.json`에 보존한다. histogram bucket 상한은 네트워크 응답 지연이 아니다.
더미/관측 CPU 중앙값은 0.915→0.884코어다. 기준은0.9코어 여유 기준을 넘었다. 같은 PC의 생성기/관측 한계를 서버 최대 동접으로 해석하지 않는다.
유저당 방향0이 아닌 본인 응답 평균의 실행별 중앙값은 2.182→3.357개/s다. 개선 후에는 입력 즉시 응답도 포함하므로 실제 위치 변경/화면 갱신 빈도와 다르다. 66.7Hz 유지나 시각 품질 개선을 주장하지 않는다.

| 동접 | 처리율 패킷/s | 패킷 대기 p99 상한 ms | CPU % | 측정 끝 패킷 대기 | 최종 정리 s |
|---|---|---|---|---|---|
| 64명 | 256.02 → 255.90 | 262.14 → 131.07 | 15.26 → 8.64 | 0 → 0 | 3.047 → 3.515 |
| 128명 | 512.58 → 511.89 | 2097.15 → 524.29 | 19.32 → 12.27 | 106 → 131 | 4.375 → 3.453 |

## 구현과 계약

`CClient::ApplyMovementInput`은 입력 이전 방향의 지난 이동을 적분하고 새 방향/속도를 적용한 뒤 본인에게 좌표를 전송한다. 본인 응답은 입력 순서를 유지하며 고유 입력 sequence ACK는 아니다.
`ScheduleMovementUpdate`의 플래그는 단일 content worker만 변경한다. 예약 시각은 steady_clock이다. 타이머는 위치/방향을 읽지 않고 기존 유저 FIFO에만 작업을 추가한다.
`RunMovementUpdate`는 세션 generation이 일치할 때만 실행한다. pending 해제→최신 상태 broadcast→다음 한 번 예약 순서이며 밀린 횟수의 작업을 재생하지 않는다.
빠른 입력의 중간 상태는 주변에 병합될 수 있다. 최종 방향0은 한 번 전달하고 예약을 종료한다. 타이머 대기/작업 큐 체류 시간만큼 주변 반영이 늦을 수 있다.
`CNetworkMgr::PrepareStop`은 mutex 아래 중단 플래그와 CV wake를 처리한다. producer join 이후 CClient를 해제하며 옛 generation 콜백은 새 사용자 상태를 수정하지 않는다.
`CPhysic`은 로비 방향을 정규화한 목표 속도(현재10 units/s)로 해석한다. 이전 갱신당 속도 누적·단위 벡터 감속은 제거했다. 게임 이동·전투 물리는 변경하지 않았다.
`CViewList`의 동일 shared_mutex 재잠금을 제거했다. 나머지 채널 순회/시야/전송 구조와 공통 큐는 유지한다.

## 제외한 후보와 기능 검사

입력마다 주변 전체 전송한 첫 후보는128명322.38패킷/s·p99상한16.78초로 악화되어 제외했다(`lobby-movement-pilot-20261013`).
dirty 상태 병합 뒤 예비값513.22패킷/s·p99상한0.524초(`lobby-movement-pilot2-20261013`)는 최종 반복 통계에 포함하지 않는다.
주기 변경 시 이전 속도 누적 때문에 이동 거리가 감소하는 현상을 확인해 목표 속도로 수정했다. 초기 forward 검사는 스폰 근처 NavMesh 경계에 막혀1.18units였으므로 같은 위치의 backward로 전후를 비교했다.
실제 소켓 검사는 `tools/server_lab/tests/lobby_movement.py`다. 기존 실행 파일의 `--legacy` 검사는 새 정지 응답을 요구하지 않아 전후 동일 기능 검증으로 주장하지 않는다.
분할 입력·실제 유한 좌표/거리·본인/주변 방향0·200개 입력 응답 순서·정지 뒤 갱신 중단·다시 이동·이동 중 재접속10회를 검사한다. 이전 세션 객체의 재사용 자체를 강제로 유도한 검사는 아니며 generation 보호는 기존 Core 검사와 코드 경계를 함께 근거로 한다.

## 원시 근거·재현

- `artifacts/logs/lobby-throughput-short-after-20261013`
- `artifacts/logs/lobby-movement-short-after-20261013`
- `artifacts/logs/lobby-throughput-soak-after-20261013`
- `artifacts/logs/lobby-movement-soak-after-20261013`
- 기능 검사: `artifacts/logs/lobby-movement-functional-*.json`
- 기준 실행 파일: `artifacts/logs/lobby-movement-before-binaries-20261013/Lobby_Server.exe` (SHA-256 `e7852fa56a06f4fe4f09e6b9582c58462b5d779f0a2ba0e50e47b56bb0cd897e`)

```powershell
python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 64 128 --repeats 5 --headless --output artifacts/logs/lobby-movement-short-new
python scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/lobby-movement-soak-new
```

각 실행 metadata/source snapshot/hash와 shutdown 로그를 보존한다. 최종 입력 정리는 격리 서버의 총 처리량/3개 빈 표본을 사용하며 wire별 ACK가 아니다.
계측의 `movement_interval_us`·`movement_single_pending` 및 타이머/로직 지연 계약은 [Server Lab](SERVER_LAB.md#로비-이동-예약-2026-10-13)을 따른다. 이전1ms 목표와 현재15ms 목표의 tick_delay는 직접 비교하지 않는다.

## 검증 한계·후속

Release 서버 빌드, Core1,479검사·ABI110종/106상수, 네트워크12종·보스4/5 매칭·초기화 실패4종(통합7그룹), Python49검사 통과.
최종 실제 소켓3항목(재접속10회 포함)·로비4명 lobby-cycle20 assertion 통과. 마지막 기능 검사에서 약0.6초 backward 이동5.922units·방향0이 아닌 본인 응답28개·본인 정지 수신0.361ms를 관측했다(각1회, 로컬 환경).
기존 실행 파일은5.909units·590응답이었고 새 정지 응답은 없어 비교하지 않았다. 실제 유지 시간/타이머 오차가 있으므로 정확한 속도 등가 실험으로 해석하지 않는다.
실제 GET /api/state의 admission에서 이동 정책15000us/단일 대기true를 확인했다. 최종 종료 로그 양쪽pending=0 sockets=0 leased=0 및 소유 실행 기록 제거를 확인했다.
측정 완료 뒤 CPhysic/CServer의 빈 줄·줄바꿈을 정리하고 재빌드/기능 검증했다. 이후 ScheduleMovementUpdate의 중첩 람다를 EnqueueMovementUpdate/ExecuteScheduledMovementUpdate로 추출했으며 예약·세션 보유·generation 잠금·계측 경계는 유지했다. 함수 추출 뒤 Release 로비 빌드와 실제 소켓3항목(200개 입력 순서·이동 중 재접속10회 포함)을 통과했다. 근거는 `artifacts/logs/lobby-movement-lambda-refactor-20261013.json`이며 함수 추출 이후 성능은 재측정하지 않았다. 각 실측은 해당 metadata의 정확한 source snapshot/실행 파일 hash를 근거로 한다.
기존 빈 switch의C4065는 타이머 switch 제거로 사라졌고 최종 증분 서버 빌드에는 경고가 보고되지 않았다. 전체 클라이언트/clean rebuild의 기존 경고까지 해소했다는 뜻은 아니다.

idle·계측OFF·연결0·전후 각5초1회 CPU는 8.29→0.00%였다(`artifacts/logs/lobby-movement-idle-20261013.json`). 0%는 해당5초의 CPU 시간 표본 해상도 내 값이며 영구적으로 비용이 없다는 뜻이 아니다. 부하 CPU 감소의 큰 부분은 빈 타이머 busy loop 제거와 일치한다.
128명120초 개선의 더미/관측 CPU는 0.927코어로0.9코어 기준을 넘었다. 측정 끝 패킷 대기106→131개 증가는 숨기지 않으며 이후3개 빈 표본으로 정리를 확인했다.

DB/블록체인·전체 GUI 플레이·클라이언트의 시각적 이동 품질·다중 채널 병렬화·연속 충돌·운영 장시간 안정성·최대 동접은 미검증이다.
Archify showcase9/9 검증과 브라우저 시각 검증은 구분한다. 이 대화의 로컬 파일 접근 차단 이력으로 전체 보고서/다이어그램 브라우저 확인은 수행하지 않았다.
실제 기능/통합 검사 결과와 더미 CPU·갱신률의 세부 한계는 완료 검증 기록을 추가해 관리한다.
