"""공간 인덱스 기준과 정적 계산 캐시의 실제 비교 보고서를 만든다."""
import argparse
import importlib.util
import json
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--audit', type=Path, required=True)
    parser.add_argument('--overhead', type=Path, required=True)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('spatial_report', ROOT / 'scripts/Build-TerrainSpatialReport.py')
    shared = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(shared)
    directory = args.input.resolve()
    summary = json.loads((directory / 'analysis.json').read_text(encoding='utf-8'))
    groups, pairs = shared.paired_groups(directory, summary)
    audit = json.loads(args.audit.read_text(encoding='utf-8'))
    counts = sorted(pairs)
    charts = directory / 'report-charts'
    charts.mkdir(exist_ok=True)
    plt = shared.plt
    plt.rcParams.update({'font.family': 'Malgun Gothic', 'axes.unicode_minus': False, 'font.size': 10})
    fig, axes = plt.subplots(1, 3, figsize=(13, 4), layout='constrained')
    for axis, key, divisor, title in zip(axes, ('tick_duration_us', 'cpu_percent', 'hz_per_match'), (1000, 1, 1), ('tick 실행 (ms)', '게임 CPU (%)', '갱신 (Hz/매치)')):
        for mode, shift, color, label in [('before', -.19, '#64748b', '공간 인덱스'), ('after', .19, '#0284c7', '+ 정적 계산 캐시')]:
            entries = [groups[mode, count][key] for count in counts]
            values = [row['median'] / divisor for row in entries]
            bars = axis.bar([i + shift for i in range(len(counts))], values, width=.36, color=color, label=label,
                            yerr=[[value - row['min']/divisor for row, value in zip(entries, values)], [row['max']/divisor-value for row, value in zip(entries, values)]], capsize=3)
            axis.bar_label(bars, labels=[f'{value:.2f}' for value in values], padding=4)
        axis.set_xticks(range(len(counts)), [f'{count//4}매치' for count in counts])
        axis.set_title(title)
        axis.set_ylim(0, max(groups[mode, count][key]['max']/divisor for count in counts for mode in ('before', 'after'))*1.3)
        axis.legend(fontsize=8, frameon=False)
    fig.savefig(charts/'overview.png', dpi=150)
    plt.close(fig)
    table, markdown = [], []
    for count in counts:
        b, a = groups['before', count], groups['after', count]
        values = [f'{count//4}', str(len(pairs[count]))]
        for key, divisor in [('tick_duration_us', 1000), ('cpu_percent', 1), ('tick_delay_us', 1000), ('hz_per_match', 1)]:
            values.append(f"{b[key]['median']/divisor:.3f} → {a[key]['median']/divisor:.3f}")
        table.append('<tr>' + ''.join(f'<td>{value}</td>' for value in values) + '</tr>')
        markdown.append('| ' + ' | '.join(values) + ' |')
    high = counts[-1]
    b, a = groups['before', high], groups['after', high]
    cpu_increases = [f"{count//4}매치" for count in counts if groups['after', count]['cpu_percent']['median'] > groups['before', count]['cpu_percent']['median']]
    cpu_note = ('CPU 중앙값은 ' + ', '.join(cpu_increases) + '에서 증가했습니다. 실행 간 변동과 함께 판단해야 합니다.') if cpu_increases else 'CPU 효과는 실행 간 범위와 함께 판단해야 합니다.'
    ray = lambda group, stage: group['stages'][stage]['wall_ms_per_tick']['median']
    failures = '; '.join(f"{row.get('mode')}/{row.get('clients')}명/seed{row.get('seed')}: {row.get('error') or row.get('profile_error')}" for row in summary['failed']) or '없음'
    controls = [json.loads(path.read_text(encoding='utf-8')) for path in args.overhead.glob('*/report.json')]
    control_seeds = [{row['seed'] for row in controls if row['profile_enabled'] == enabled} for enabled in (False, True)]
    if control_seeds[0] != control_seeds[1] or any(row['clients'] != 4 for row in controls):
        raise ValueError('계측 켬/끔 대조의 seed 또는 부하 불일치')
    control_values = {}
    for enabled in (False, True):
        rows = [row['servers']['GameServer'] for row in controls if row.get('ok') and row['profile_enabled'] == enabled]
        if not rows or len(rows) != sum(row['profile_enabled'] == enabled for row in controls):
            raise ValueError('계측 켬/끔 대조 실패 또는 누락: 원시 결과 확인 필요')
        control_values[enabled] = {'tick': statistics.median(row['tick_duration_us']['mean_us']/1000 for row in rows),
                                   'cpu': statistics.median(row['cpu_percent'] for row in rows)}
    off, on = control_values[False], control_values[True]
    overhead = f"동일 캐시 바이너리 1매치 계측 끔→켬: tick {off['tick']:.3f}→{on['tick']:.3f}ms, CPU {off['cpu']:.3f}→{on['cpu']:.3f}%. {len(controls)}회 성공, 실행 평균 중앙값. 계측 차이와 실행 간 변동이 함께 포함되며 다중 매치의 계측 끔 대조는 미측정입니다."
    before_controls = [json.loads(path.read_text(encoding='utf-8')) for path in (args.overhead.parent/'before').glob('*/report.json')]
    if {row['seed'] for row in before_controls} != control_seeds[0] or not all(row.get('ok') and row['profile_enabled'] is False and row['clients'] == 4 for row in before_controls):
        raise ValueError('기존 바이너리 계측 끔 대조 실패 또는 누락')
    before_off = {key: statistics.median(row['servers']['GameServer'][field]['mean_us']/1000 if field == 'tick_duration_us' else row['servers']['GameServer'][field] for row in before_controls)
                  for key, field in (('tick', 'tick_duration_us'), ('cpu', 'cpu_percent'))}
    off_comparison = f"계측을 모두 끈 기존→캐시 1매치 대조: tick {before_off['tick']:.3f}→{off['tick']:.3f}ms, CPU {before_off['cpu']:.3f}→{off['cpu']:.3f}%. 동일 seed {len(before_controls)}쌍."
    cpu_finding = '1매치 CPU 상승은 계측 끔 대조에도 남아 있어 전체 CPU 개선 완료로 판단하지 않습니다.' if off['cpu'] > before_off['cpu'] else '1매치 CPU 방향이 주실험과 계측 끔 대조에서 달라 CPU 효과와 상승 원인은 미확정입니다.'
    quality_rows = []
    for mesh in audit['meshes']:
        edge = mesh['longest_edge_units']
        quality_rows.append(f"<tr><td>{Path(mesh['path']).name}</td><td>{mesh['triangles']:,}</td><td>{edge['min']:.3f} / {edge['median']:.3f} / {edge['max']:.3f}</td><td>{mesh['flags'].get('slender',0):,}</td></tr>")
    body = f'''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>지형 정적 계산 캐시와 메시 품질 진단</title><style>
body{{font:16px/1.6 'Malgun Gothic',sans-serif;color:#1e293b;background:#f8fafc;max-width:1150px;margin:auto;padding:28px}}section{{background:white;border:1px solid #e2e8f0;border-radius:12px;padding:22px;margin:18px 0}}h1,h2{{line-height:1.3}}img{{max-width:100%;height:auto}}table{{border-collapse:collapse;width:100%;font-size:14px}}th,td{{text-align:left;padding:9px;border-bottom:1px solid #e2e8f0}}pre{{white-space:pre-wrap;padding:16px;background:#f1f5f9}}.cards{{display:flex;gap:16px;flex-wrap:wrap}}.cards div{{padding:12px 20px;background:#e0f2fe;border-radius:8px}}a{{color:#0369a1}}</style>
<h1>지형 정적 계산 캐시 · 메시 품질 진단</h1>
<section id="result"><h2>1. 결과</h2><p>공간 후보를 유지하고 삼각형의 불변 계산을 재사용했습니다. 실제 서버 비교 <b>{summary['passed']}/{summary['runs']}회</b> 성공. {cpu_note} {cpu_finding}</p>
<div class="cards"><div>{high//4}매치 tick<br><b>{b['tick_duration_us']['median']/1000:.2f} → {a['tick_duration_us']['median']/1000:.2f}ms</b></div><div>게임 CPU<br><b>{b['cpu_percent']['median']:.2f} → {a['cpu_percent']['median']:.2f}%</b></div><div>캐시 저장량<br><b>+6.62MiB</b></div></div><p>{high//4}매치 예약 지연 {a['tick_delay_us']['median']/1000:.2f}ms·갱신 {a['hz_per_match']['median']:.2f}Hz는 남아 있습니다. Unity 메시 재추출은 원본 프로젝트·지면 포함 기준 확인 대기입니다.</p></section>
<section id="cause"><h2>2. 분석한 원인</h2><p>셀로 후보를 줄인 뒤에도 같은 삼각형의 법선·변·내적·분모를 매 Ray마다 계산했습니다.</p><pre>// 기존 RayCast의 반복 계산
normal = Normalize((v2 - v1).Cross(v3 - v1));
edge1 = v2 - v1; edge2 = v3 - v1;
invDenom = 1.0f / (dot11 * dot22 - dot12 * dot12);</pre></section>
<section id="solution"><h2>3. 해결 방식</h2><pre>// 개념 흐름
초기 로드: 삼각형 ID → float 계수 52byte 저장
Ray 조회: 셀/전역 후보 → 캐시 계수 읽기 → 기존 교차 계산
캐시 없음·무효: 기존 계산 경로 사용</pre><p>원본 순서·후보 수·높이/스킬 규칙을 유지합니다. <a href="../diagrams/terrain-cache/README.md">캐시 흐름도</a> · <a href="TERRAIN_CACHE.md">검증 근거</a></p></section>
<section id="measure"><h2>4. 실제 측정</h2><p>Release LOCAL_TEST·4인 매치·동일 seed AB/BA·조건별5쌍 시도·5초 워밍업/20초 측정/250ms 이동·정지·양쪽 profile 켬. 동일 seed 양쪽 성공 쌍의 실행 평균 중앙값이며 그래프 오차선은 최소–최대입니다.</p>
{shared.image(charts/'overview.png','공간 인덱스 기준과 정적 계산 캐시의 tick·CPU·갱신 비교')}
<table><tr><th>매치</th><th>성공 쌍</th><th>tick ms</th><th>CPU %</th><th>예약 지연 ms</th><th>Hz/매치</th></tr>{''.join(table)}</table>
<p>{high//4}매치 HeightRay {ray(b,'height_ray'):.3f}→{ray(a,'height_ray'):.3f}ms/tick, NavRay {ray(b,'nav_ray'):.3f}→{ray(a,'nav_ray'):.3f}ms/tick. stage는 중첩하므로 합산하지 않습니다.</p>
<p>{high//4}매치 프로세스 private bytes 표본 최대값의 중앙값: {b['private_bytes_peak_sampled']['median']/1048576:.2f}→{a['private_bytes_peak_sampled']['median']/1048576:.2f}MiB. 초기 로드의 순간 최대값은 포함하지 않습니다.</p>
<p>{overhead}</p>
<p>{off_comparison} {cpu_finding}</p>
<p>실패: {shared.html.escape(failures)}. Debug/Release Core 각1,478검사·ABI110종/106상수·실제 메시27,904비교 및 Python37검사 통과. 전체 GUI/근거리 전투·Unity 씬 재추출은 미검증입니다.</p></section>
<section id="remaining"><h2>5. 품질 진단과 남은 작업</h2><table><tr><th>메시</th><th>삼각형</th><th>최장변 최소/중앙/최대</th><th>가늘기 진단</th></tr>{''.join(quality_rows)}</table>
<p>크기 차이 자체는 결함이 아닙니다. 진단은 double 기하 기준이고 서버의 float 전역 후보 분류와 구분합니다. 기존 OBJ에는 원본 오브젝트 그룹 정보가 없습니다.</p>
<table><tr><th>항목</th><th>영향</th><th>확인 상태</th><th>다음 대응</th></tr><tr><td>1매치 CPU 증가</td><td>저부하 CPU 개선 미달</td><td>계측 끔 대조에서도 중앙값 증가, 원인 미확정</td><td>tick 밖 스레드 비용·프로세스 CPU를 분리 계측</td></tr><tr><td>불필요한 HeightMesh</td><td>후보 비용·높이 품질</td><td>원본 오브젝트 출처 없음</td><td>Unity에서 지면 포함 목록/레이어를 확정하고 출처를 보존해 재추출</td></tr><tr><td>가늘거나 수직인 면</td><td>판정 안정성·후보 증가</td><td>진단 완료, 제거 타당성 미확정</td><td>원본 모델과 실제 높이 hit 확인 후 단순화·제외 결정</td></tr><tr><td>NavMesh 높이 정밀도</td><td>상세 지면과 오차 가능</td><td>CalculateTriangulation에 배치용 상세 높이 없음</td><td>HeightMesh와 역할 분리</td></tr><tr><td>잔여 지연·송신</td><td>16매치 갱신 약42Hz</td><td>예약 지연 유지</td><td>기존 TODO 유지, 타이머·송신 비용 별도 분석</td></tr></table></section></html>'''
    (ROOT/'docs/development/TERRAIN_CACHE.html').write_text(body,encoding='utf-8')
    evidence=f'''# 지형 정적 계산 캐시·품질 진단 근거

[HTML 보고서](TERRAIN_CACHE.html) · [흐름도](../diagrams/terrain-cache/README.md).

## 결과와 실제 측정

실제 서버 {summary['passed']}/{summary['runs']}회 성공. 이전 공간 인덱스 완료 바이너리를 기준으로 동일 seed의 양쪽 성공 쌍만 비교한다. 대응 seed: `{pairs}`. 실패는 `{failures}`이며 원시 결과에 남긴다.

{cpu_note}

| 매치 | 성공 쌍 | tick ms | CPU % | 예약 지연 ms | Hz/매치 |
|---|---:|---:|---:|---:|---:|
{chr(10).join(markdown)}

Release x64 LOCAL_TEST·4인 매치·seed1~5 교차·매회 재시작·5초 워밍업/20초 측정/250ms 입력·입장 소프트 제한 해제·양쪽 profile 켬. 실행별 평균의 중앙값이며 최대 처리량이 아니다. 프로세스 CPU는12논리 코어로 정규화한다. NPC 상태·외부 부하가 완전히 고정되지 않으며 stage wall/cycle은 포함 관계여서 합산하지 않는다.

{high//4}매치 private bytes 표본 최대값 중앙값은 {b['private_bytes_peak_sampled']['median']/1048576:.2f}→{a['private_bytes_peak_sampled']['median']/1048576:.2f}MiB다. 초기 로드 순간 peak와 구분한다. 같은 위치의 공간 검색 규칙은 유지하지만 실행 시점·이동 위치에 따라 Ray별 실제 방문 수는 달라질 수 있다.

원시 자료 `{directory.relative_to(ROOT).as_posix()}`: metadata/source-snapshot/binaries, 전후 report 및 lab/runs, analysis/paired-analysis, verification, report-charts. 원시 로그·실행 파일은 Git 제외다. 새 기준을 기존 선형 검색 결과와 혼동하지 않는다.

{overhead} 원시 대조: `{args.overhead.as_posix()}`. 실제 데이터와 실행별 범위는 해당 report.json/analysis.json에 보존한다.

{off_comparison} {cpu_finding} 기존 끔→캐시 끔→캐시 켬 순서와 역순을 seed별로 번갈아 실행했다. tick 구간 cycle은 프로세스 전체 CPU와 범위가 달라 직접 합산·치환하지 않는다.

## 구현과 검증

TerrainSpatialIndex는 삼각형별 법선·변2개·내적3개·분모 역수를 원본 float 순서로 계산해52byte로 저장한다. RayCast는 원본 배열 ID로 읽으며 캐시 없음/무효이면 원래 계산을 사용한다. 초기 로드 뒤 불변이고 Reset 시 회수한다. `terrain_index.nav/height.prepared_bytes`는 캐시 vector capacity, `prepared_triangles`는 유효 캐시 개수다. 기존 `bytes`는 공간 인덱스만 뜻한다. 추가 저장량6,941,948bytes(6.620MiB), 공간 인덱스 포함8.172MiB이며 전체 프로세스 메모리와 구분한다.

Debug/Release Core 각1478검사·ABI110종/106상수·기존 참조 대비27904개 실제 메시 hit/height bit/node 비교 PASS. 캐시 없음·크기 불일치·Reset·비수직/극단 입력을 포함한다. Python37검사는 진단의 원본 보존·잘못된 입력 거부·그룹/중복·퇴화 분류를 포함한다. 기존 경고를 유지하고 전체 GUI/운영 DB/근거리 전투·Unity 프로젝트 통합은 미검증이다.

## 메시 품질

현재 `Script/HeightMeshExporter.cs`는 Nav 태그 MeshFilter의 원본 렌더 메시를 합치고, `Script/ExportNavMeshToObj.cs`는 CalculateTriangulation을 내보낸다. 원본 프로젝트와 지면 포함/제외 기준은 사용자 확인 대기다. OBJ에는 o/g가 없어 오브젝트별 불필요한 면을 안전하게 제거할 수 없다. 자원 파일은 변경하지 않았다.

`terrain_audit.py`는 읽기 전용 진단으로 최장변·면적·XZ면적 분포, 정확한 중복 좌표 면·퇴화·수직·가늘기, o/g 출처를 JSON에 남긴다. double 기하 진단과 float 서버 후보를 동일시하지 않는다. 결과 `{args.audit.as_posix()}`. 삼각형 크기 통일을 목표로 삼지 않고 지면 오차·경계·계단/경사·겹친 층·실제 이동 품질로 판단한다.

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
'''
    (ROOT/'docs/development/TERRAIN_CACHE.md').write_text(evidence,encoding='utf-8')


if __name__ == '__main__':
    main()
