# 지형 정적 계산 캐시·품질 진단 근거

[HTML 보고서](TERRAIN_CACHE.html) · [흐름도](../diagrams/terrain-cache/README.md).

## 결과와 실제 측정

실제 서버 30/30회 성공. 이전 공간 인덱스 완료 바이너리를 기준으로 동일 seed의 양쪽 성공 쌍만 비교한다. 대응 seed: `{4: [1, 2, 3, 4, 5], 16: [1, 2, 3, 4, 5], 64: [1, 2, 3, 4, 5]}`. 실패는 `없음`이며 원시 결과에 남긴다.

CPU 중앙값은 1매치에서 증가했습니다. 실행 간 변동과 함께 판단해야 합니다.

| 매치 | 성공 쌍 | tick ms | CPU % | 예약 지연 ms | Hz/매치 |
|---|---:|---:|---:|---:|---:|
| 1 | 5 | 2.327 → 2.171 | 1.099 → 1.593 | 1.740 → 1.695 | 59.722 → 59.890 |
| 4 | 5 | 3.767 → 3.624 | 7.013 → 5.949 | 1.845 → 1.792 | 59.356 → 59.546 |
| 16 | 5 | 2.652 → 2.441 | 15.851 → 13.320 | 8.591 → 8.585 | 42.384 → 42.406 |

Release x64 LOCAL_TEST·4인 매치·seed1~5 교차·매회 재시작·5초 워밍업/20초 측정/250ms 입력·입장 소프트 제한 해제·양쪽 profile 켬. 실행별 평균의 중앙값이며 최대 처리량이 아니다. 프로세스 CPU는12논리 코어로 정규화한다. NPC 상태·외부 부하가 완전히 고정되지 않으며 stage wall/cycle은 포함 관계여서 합산하지 않는다.

16매치 private bytes 표본 최대값 중앙값은 34.75→41.42MiB다. 초기 로드 순간 peak와 구분한다. 같은 위치의 공간 검색 규칙은 유지하지만 실행 시점·이동 위치에 따라 Ray별 실제 방문 수는 달라질 수 있다.

원시 자료 `artifacts/logs/terrain-cache-compare-20261013`: metadata/source-snapshot/binaries, 전후 report 및 lab/runs, analysis/paired-analysis, verification, report-charts. 원시 로그·실행 파일은 Git 제외다. 새 기준을 기존 선형 검색 결과와 혼동하지 않는다.

동일 캐시 바이너리 1매치 계측 끔→켬: tick 1.983→2.202ms, CPU 1.545→1.764%. 10회 성공, 실행 평균 중앙값. 계측 차이와 실행 간 변동이 함께 포함되며 다중 매치의 계측 끔 대조는 미측정입니다. 원시 대조: `artifacts/logs/terrain-cache-control-20261013/after`. 실제 데이터와 실행별 범위는 해당 report.json/analysis.json에 보존한다.

계측을 모두 끈 기존→캐시 1매치 대조: tick 2.130→1.983ms, CPU 1.159→1.545%. 동일 seed 5쌍. 1매치 CPU 상승은 계측 끔 대조에도 남아 있어 전체 CPU 개선 완료로 판단하지 않습니다. 기존 끔→캐시 끔→캐시 켬 순서와 역순을 seed별로 번갈아 실행했다. tick 구간 cycle은 프로세스 전체 CPU와 범위가 달라 직접 합산·치환하지 않는다.

## 구현과 검증

TerrainSpatialIndex는 삼각형별 법선·변2개·내적3개·분모 역수를 원본 float 순서로 계산해52byte로 저장한다. RayCast는 원본 배열 ID로 읽으며 캐시 없음/무효이면 원래 계산을 사용한다. 초기 로드 뒤 불변이고 Reset 시 회수한다. `terrain_index.nav/height.prepared_bytes`는 캐시 vector capacity, `prepared_triangles`는 유효 캐시 개수다. 기존 `bytes`는 공간 인덱스만 뜻한다. 추가 저장량6,941,948bytes(6.620MiB), 공간 인덱스 포함8.172MiB이며 전체 프로세스 메모리와 구분한다.

Debug/Release Core 각1478검사·ABI110종/106상수·기존 참조 대비27904개 실제 메시 hit/height bit/node 비교 PASS. 캐시 없음·크기 불일치·Reset·비수직/극단 입력을 포함한다. Python37검사는 진단의 원본 보존·잘못된 입력 거부·그룹/중복·퇴화 분류를 포함한다. 기존 경고를 유지하고 전체 GUI/운영 DB/근거리 전투·Unity 프로젝트 통합은 미검증이다.

## 메시 품질

현재 `Script/HeightMeshExporter.cs`는 Nav 태그 MeshFilter의 원본 렌더 메시를 합치고, `Script/ExportNavMeshToObj.cs`는 CalculateTriangulation을 내보낸다. 원본 프로젝트와 지면 포함/제외 기준은 사용자 확인 대기다. OBJ에는 o/g가 없어 오브젝트별 불필요한 면을 안전하게 제거할 수 없다. 자원 파일은 변경하지 않았다.

`terrain_audit.py`는 읽기 전용 진단으로 최장변·면적·XZ면적 분포, 정확한 중복 좌표 면·퇴화·수직·가늘기, o/g 출처를 JSON에 남긴다. double 기하 진단과 float 서버 후보를 동일시하지 않는다. 결과 `artifacts/logs/terrain-cache-20261013/quality-audit.json`. 삼각형 크기 통일을 목표로 삼지 않고 지면 오차·경계·계단/경사·겹친 층·실제 이동 품질로 판단한다.

Unity의 [CalculateTriangulation 계약](https://docs.unity.com/en-us/engine/6000.0/script-reference/unityengine/ai/navmesh/calculatetriangulation)에 따르면 배치용 상세 높이 정보가 없다. NavMesh 이동 가능 영역과 HeightMesh 지면 높이를 구분하고, 원본 프로젝트의 버전·NavMeshSurface/agent·layer 설정을 확인한 뒤 재베이크/추출한다.

다음 품질 개선의 적용 순서는 아래와 같다. 현재 구현 완료 항목으로 해석하지 않는다.

1. HeightMesh는 실제 보행 지면·계단·경사·발판을 명시적으로 포함한다. 벽·장식물의 제외는 원본 오브젝트 기준으로 결정한다. NavMesh 베이크에는 같은 벽이 장애물로 필요할 수 있어 별도 선택 기준을 사용한다.
2. exporter는 월드 변환, 로케일 독립 숫자, 원본 오브젝트/면 출처, 누락·비정상 좌표·인덱스 검사와 추출 설정을 보존한다. 현재 서버가 지원하는 양수 인덱스 삼각형 OBJ 계약에 맞춘다.
3. 삼각형 크기를 일률적으로 맞추지 않는다. 평평한 영역의 불필요한 면을 줄이되, 경계·계단·경사·겹친 층의 높이 오차와 NavMesh 연결성을 우선한다. 큰 면을 무조건 분할하면 면 수와 후보 비용이 늘어날 수 있다.
4. 새 자원은 별도 파일로 추출하고 기존 자원과 비교한다. 지형 변경은 기존 판정의 bit 일치를 목표로 삼을 수 없으므로 위치별 높이 오차·hit 누락·원치 않는 층 선택, 경계 이동·점프/강제 이동·NPC 경로·실제 GUI 확인과 동일 부하 성능 검사를 별도로 수행한다. 허용 오차는 원본 Unity 씬과 게임 요구를 확인한 뒤 확정한다.

현재는 원본 Unity 프로젝트 경로/버전과 포함할 지면 기준이 없어 실제 자원 재추출 및 전역 후보 감소를 완료하지 않았다. `ContainsEdge`의 기존 결함과 Astar 품질 검증도 별도 TODO로 유지한다.

```powershell
python tools/server_lab/terrain_audit.py Server/Game_Server/Resource/NavMeshData3.obj Server/Game_Server/Resource/HeightMesh.obj --output artifacts/logs/terrain-quality-new.json
./scripts/Test-ServerTerrain.ps1 -Configuration Release
python tools/server_lab/terrain_compare.py --baseline artifacts/logs/terrain-spatial-compare-20261012 --output artifacts/logs/terrain-cache-new
```

기존 local-file 보안 거절을 우회하지 않아 실제 HTML 시각/containment 검사는 수행하지 않는다. 그래프 PNG·정적 링크/구문·수치 대조 결과는 최종 검증에 별도 기록한다.
