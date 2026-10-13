# NavMesh·HeightMesh 공간 검색 개선 근거

실제 비교 29/30 PASS. 보고서는 [HTML](TERRAIN_SPATIAL_INDEX.html), 구조는 [흐름도](../diagrams/terrain-spatial-index/README.md)다. 기존 기준 분석은 덮어쓰지 않았다. 실패: before/64명/seed1: client 4 / match-transfer: client 4 / game-login: SC_LOGIN_INFO: 기대 응답 없음 (10s).

## 실제 측정

원시 자료: `artifacts/logs/terrain-spatial-compare-20261012/`. `metadata.json`에 전후 실행 파일·소스·자원 hash, `source-snapshot/`에 개선 후 소스/실행 파일, `binaries/`에 비교 실행 파일을 보존했다. 기준 실행 파일은 이전 `game-tick-profile-20261012/source-snapshot/`에서 가져오며 데이터·시나리오 hash를 대조한다. 각 실행 `report.json`·원시 `lab/runs/*.json`·종료 로그, `before/analysis.json`·`after/analysis.json`·전체 `analysis.json`과 검증 JSON을 함께 보존한다. 원시 실행 로그는 Git 제외다.

Release x64 LOCAL_TEST, Ryzen 5 7500F, Windows11, 12논리 코어. 1/4/16매치 × 4명, 조건별5쌍 시도·seed1~5·홀수 후/전·짝수 전/후 교차. 5초 워밍업/20초 측정/250ms 이동·정지, 매회 서버 재시작·입장 소프트 제한 해제·양쪽 TickProfile 켬. 고정 부하 처리 성능이며 최대 처리량이 아니다. 수치는 실행별 평균의 중앙값 [최소–최대], 예약 지연과 실행 시간을 분리한다.

| 매치 | 전후 | tick 실행 ms [범위] | tick 예약 지연 ms | CPU % [범위] | 갱신 Hz/매치 | HeightRay ms/tick | NavRay ms/tick | PASS |
|---|---|---|---|---|---|---|---|---|
| 1 | 전 | 20.7220 [20.3917–20.8522] | 8.578 | 7.5765 [7.3120–7.7964] | 42.360 | 17.3232 | 2.1062 | 5/5 |
| 1 | 후 | 2.3861 [2.3397–2.4026] | 1.113 | 1.6508 [1.5842–1.7878] | 62.053 | 0.7542 | 0.3346 | 5/5 |
| 4 | 전 | 24.0087 [23.7985–24.1139] | 11.074 | 31.0167 [30.7170–31.1199] | 38.338 | 20.6810 | 2.1549 | 5/5 |
| 4 | 후 | 3.7216 [3.7088–3.7581] | 1.291 | 6.4760 [5.0563–8.8708] | 61.374 | 0.8490 | 0.3260 | 5/5 |
| 16 | 전 | 31.0661 [30.9428–31.1103] | 68.462 | 49.5889 [49.4994–49.7312] | 11.972 | 28.3710 | 2.2422 | 4/5 |
| 16 | 후 | 2.6277 [2.6133–2.6568] | 8.346 | 14.7256 [11.7655–16.7981] | 42.823 | 1.0305 | 0.3226 | 5/5 |

CPU는 논리 코어12개로 정규화한 프로세스값이다. Hz는 같은 프로파일 표본 구간의 완료 tick / 경과 초 / 매치 수다. 상세 cycle·stage·NPC 호출·후보 방문량과 observer CPU·generator lag는 전체 analysis에 보존한다. stage는 중첩하므로 합산하지 않는다. 매치 나이/활성 NPC는 순차 준비에 따라 달라지며 차이를 순수 경합 효과로 해석하지 않는다. 실패도 분모에 유지한다.

성능 표·그래프는 동일 seed의 양쪽 성공 쌍만 사용한다. 짝과 수치는 `paired-analysis.json`, 전체 성공/실패와 비대응 실행은 `analysis.json`에 보존한다. 조건별 대응 seed: `{4: [1, 2, 3, 4, 5], 16: [1, 2, 3, 4, 5], 64: [2, 3, 4, 5]}`. 실패 실행에 측정 구간이 없으면 성능을 0으로 채우지 않는다.

대표 조건 observer CPU 중앙값은 0.817→2.620%다. 개선 후 수신/관측 처리량도 증가하므로 게임 프로세스 CPU와 구분한다. 원시 generator lag와 입력/패킷 처리량을 함께 보존하며 외부 시스템 부하 전체를 통제했다고 주장하지 않는다. 대표 조건 미니언 Update는 4.561→4.526마리/tick이다.

## 구현 계약

- `TerrainSpatialIndex.h`: 두 메시 공통 XZ 격자·CSR 후보 ID·전역 후보의 순서 병합. 빌드 후 불변, 조회 시 메모리 할당/잠금 없음. 최대셀1,048,576개·후보 참조8,388,608개 상한을 넘으면 전체 검색한다.
- `GameUtil.cpp`: NavMesh 로드, HeightMesh 기존 높이 정렬 뒤 구축. 노드/삼각형은 기존 소유자에 있고 인덱스에는 ID만 있다. worker 시작 전에 구축하며 종료 후 메시 해제 전에 초기화한다.
- `MathUtil.h`: 기본 수직 하향 Ray와 finite/절댓값1024이하 Y만 가속한다. 기존 first-hit·높이1.5 필터·일반 높이0.8/Nav0.6 보정·스킬 overload 의미·miss 출력값 유지. 근접 hit 재정렬이나 정적 판정 수식 캐시는 추가하지 않았다.
- 경계 패딩은 `0.01 + 64*float_epsilon*scale*condition`. condition>256·거의 수직·큰 좌표 등의 면은 전역 후보다. 기존 필터로 거부되는 면과 정확한 수직면은 수직 Ray 후보에서 제외한다. 비수직/비정상 좌표는 전체 검색으로 돌아간다.
- 저장 공간 `1.551MiB`는 vector capacity 기반 인덱스 배열 합이며 구축 임시 메모리·전체 프로세스 메모리는 아니다. 실제 프로세스 peak sampled private bytes도 analysis에 별도로 있다.

`GET /api/state`의 GameServer `terrain_index`와 원시 서버 `content.terrain_index`에 schema_version1, nav/height의 ready·triangles·cell_size·cells·references·global_candidates·bytes를 제공한다. startup 값 복사본이며 reporter가 메시를 순회하지 않는다. Ray visited는 검사한 후보 수이며 기존 전체 방문 수와 비교한다. 누락은 0으로 채우지 않는다.

## 검증·재현

```powershell
./scripts/Build.ps1 -Configuration Release -Module Servers
./scripts/Test-ServerTerrain.ps1 -Configuration Release
./scripts/Test-ServerTerrain.ps1 -Configuration Debug
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Release -Python <python.exe>
<python.exe> tools/server_lab/terrain_compare.py --baseline artifacts/logs/game-tick-profile-20261012 --output <새 출력 경로>
<report-venv/python.exe> scripts/Build-TerrainSpatialReport.py --input <출력 경로>
```

`TerrainReferenceRay.h`는 main `3e34e71a` Ray 수식을 보존한 테스트 oracle다. 실제 두 메시 정점/경계 nextafter/중심·임의 XZ/Y·4 overload의 hit·float bit·노드 ID 비교 27,904개 PASS. Core에는 중첩 지형·첫 hit 순서·높이 제한·miss 출력 보존·빈/무효/범위 초과 인덱스·비수직/극단 Y·비정상 float·동시 읽기 검사를 추가했다. 유한 표본 검사이며 모든 float 입력의 수학적 동등성을 증명한 것은 아니다.

Debug/Release 서버 빌드·Core 각1471·ABI110종/106상수·Python35 PASS. Release 네트워크12종/보스4·5 매치/초기화 실패4종과 Server Lab 통합8종·첫 미니언4개 경로 이동 PASS. 정상 종료 pending/sockets/leased=0, 비교 종료 후 개선 실행 파일 복원 및 fingerprint 불변을 확인한다.

기존 C4065/C4101/C4244/C4305/C4309가 남는다. 테스트에서 기존 MathUtil/고정 참조 코드의 C4100/C4189/C4505도 드러나며 숨기지 않았다. Archify showcase9/9·오류/경고0. 기존 local-file 브라우저 보안 거절을 우회하지 않아 실제 HTML 시각/containment는 미검증이다. PNG 그래프는 직접 검토한다. 운영 DB/블록체인·전체 GUI 플레이·장시간/근거리 전투·최대 수용량 미검증. 커밋/푸시는 수행하지 않았다.

## 후속 품질 개선

HeightMesh 전역 후보 3,738개를 유지했다. 이는 모두 결함이라는 의미가 아니다. 데이터의 가늘거나 수직에 가까운 면과 barycentric 수치 안정성을 조사한 뒤 경계 판정·중첩/계단/경사 기준을 정하고 축소한다. `Triangle::ContainsEdge`는 `_v1/_v2` 인자끼리/자기 자신만 비교하고 메시 정점을 사용하지 않는 기존 결함이 있다. 이번에는 수정하지 않았으며 실제 Astar 영향은 미검증이다. OBJ 입력/재로드·수명 검증, 기존 중복 송신, 근거리 전투/Astar·복수 웨이브·tick 밖 잔여 CPU는 별도 작업으로 남긴다.
