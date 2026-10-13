"""실제 공간 검색 전후 실측에서 HTML 보고서와 근거 문서를 생성한다."""
import argparse
import base64
import html
import json
import statistics
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]


def image(path, alt):
    return f'<img src="data:image/png;base64,{base64.b64encode(path.read_bytes()).decode()}" alt="{html.escape(alt)}">'


def value(group, key, divisor=1):
    return group[key]['median'] / divisor


def stats(values):
    values = [value for value in values if value is not None]
    return {'median': statistics.median(values), 'min': min(values), 'max': max(values)} if values else None


def paired_groups(directory, summary):
    records = {mode: json.loads((directory / mode / 'analysis.json').read_text(encoding='utf-8'))['records'] for mode in ('before', 'after')}
    groups, pairs = {}, {}
    for count in sorted({group['clients'] for group in summary['groups']}):
        valid = {mode: {row['seed']: row for row in rows if row['clients'] == count and row.get('ok') and 'profile_error' not in row and (row.get('tick_profile') or {}).get('ticks')}
                 for mode, rows in records.items()}
        seeds = sorted(valid['before'].keys() & valid['after'].keys())
        if not seeds:
            raise ValueError(f'{count}명 성공한 전후 쌍 없음')
        pairs[count] = seeds
        for mode in ('before', 'after'):
            original = next(group for group in summary['groups'] if group['clients'] == count and group['mode'] == mode)
            group = {**original, 'paired_runs': len(seeds), 'paired_seeds': seeds}
            rows = [valid[mode][seed] for seed in seeds]
            for key in ('cpu_percent', 'private_bytes_peak_sampled', 'packets_per_s'):
                group[key] = stats([row['servers']['GameServer'][key] for row in rows])
            for key in ('tick_duration_us', 'tick_delay_us'):
                group[key] = stats([row['servers']['GameServer'][key]['mean_us'] for row in rows])
            profiles = [row['tick_profile'] for row in rows]
            group['hz_per_match'] = stats([profile['ticks'] / profile['seconds'] / group['matches'] for profile in profiles])
            group['observer_cpu_percent'] = stats([row['observer_cpu_percent'] for row in rows])
            group['stages'] = {name: {field: stats([profile['stages'][name][field] for profile in profiles])
                                     for field in ('wall_ms_per_tick', 'cycle_share_percent', 'calls_per_tick', 'work_per_call')}
                               for name in profiles[0]['stages']}
            groups[mode, count] = group
    (directory / 'paired-analysis.json').write_text(json.dumps({'pairs': pairs, 'groups': list(groups.values())}, ensure_ascii=False, indent=2), encoding='utf-8')
    return groups, pairs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    args = parser.parse_args()
    directory = args.input.resolve()
    data = json.loads((directory / 'analysis.json').read_text(encoding='utf-8'))
    checks = json.loads((directory / 'final-verification.json').read_text(encoding='utf-8'))
    terrain = checks['terrain_assets']['meshes']
    counts = sorted({group['clients'] for group in data['groups']})
    groups, pairs = paired_groups(directory, data)
    for count in counts:
        if any((mode, count) not in groups or 'stages' not in groups[mode, count] for mode in ('before', 'after')):
            raise ValueError('전후 구간 실측 누락')
    before, after = groups['before', counts[-1]], groups['after', counts[-1]]
    charts = directory / 'report-charts'
    charts.mkdir(exist_ok=True)
    plt.rcParams.update({'font.family': 'Malgun Gothic', 'axes.unicode_minus': False, 'font.size': 10,
                         'figure.facecolor': '#f8fafc', 'axes.facecolor': '#f8fafc', 'savefig.facecolor': '#f8fafc',
                         'axes.spines.top': False, 'axes.spines.right': False})
    fig, axes = plt.subplots(1, 3, figsize=(13, 4), layout='constrained')
    metrics = [('tick_duration_us', 1000, 'tick 실행 (ms)', '작을수록 좋음'), ('cpu_percent', 1, '게임 프로세스 CPU (%)', '작을수록 좋음'), ('hz_per_match', 1, '매치별 갱신 (Hz)', '클수록 좋음')]
    for axis, (key, divisor, title, direction) in zip(axes, metrics):
        for mode, shift, color, label in [('before', -.19, '#64748b', '개선 전'), ('after', .19, '#0284c7', '개선 후')]:
            values = [value(groups[mode, count], key, divisor) for count in counts]
            low = [number - groups[mode, count][key]['min'] / divisor for count, number in zip(counts, values)]
            high = [groups[mode, count][key]['max'] / divisor - number for count, number in zip(counts, values)]
            bars = axis.bar([index + shift for index in range(len(counts))], values, width=.36, color=color, label=label, yerr=[low, high], capsize=3)
            axis.bar_label(bars, labels=[f'{number:.2f}' for number in values], padding=5, fontsize=9)
        axis.set_xticks(range(len(counts)), [f'{count // 4}매치' for count in counts])
        axis.set_title(title + '\n' + direction)
        axis.set_ylim(0, max(groups[mode, count][key]['max'] / divisor for mode in ('before', 'after') for count in counts) * 1.25)
        if key == 'tick_duration_us':
            axis.axhline(15, linestyle='--', color='#b91c1c', label='15ms 예약 간격')
        axis.legend(fontsize=9, frameon=False)
    fig.savefig(charts / 'overview.png', dpi=150)
    plt.close(fig)
    fig, axes = plt.subplots(1, 2, figsize=(10.6, 3.5), layout='constrained')
    for axis, stage, name in zip(axes, ('height_ray', 'nav_ray'), ('HeightMesh', 'NavMesh')):
        values = [group['stages'][stage]['work_per_call']['median'] for group in (before, after)]
        bars = axis.bar(['개선 전', '개선 후'], values, color=['#64748b', '#0284c7'])
        axis.bar_label(bars, labels=[f'{number:,.0f}개' for number in values], padding=5)
        axis.set(title=f'{name} · Ray 1회 방문 삼각형', ylabel='방문 수 (개)', ylim=(0, max(values) * 1.25))
    fig.savefig(charts / 'candidates.png', dpi=150)
    plt.close(fig)
    table, details = [], []
    for count in counts:
        left, right = groups['before', count], groups['after', count]
        cells = [f'{value(left,key,divisor):.2f} → {value(right,key,divisor):.2f}' for key, divisor in [('tick_duration_us',1000), ('tick_delay_us',1000), ('cpu_percent',1), ('hz_per_match',1)]]
        table.append(f'<tr><td>{count//4}개 / {count}명<br><small>성공 {len(pairs[count])}쌍</small></td>' + ''.join(f'<td>{cell}</td>' for cell in cells) + f'<td>{left["passed"]}/{left["runs"]} · {right["passed"]}/{right["runs"]}</td></tr>')
        for mode, group in [('전', left), ('후', right)]:
            details.append(f'| {count//4} | {mode} | {value(group,"tick_duration_us",1000):.4f} [{group["tick_duration_us"]["min"]/1000:.4f}–{group["tick_duration_us"]["max"]/1000:.4f}] | '
                           f'{value(group,"tick_delay_us",1000):.3f} | {value(group,"cpu_percent"):.4f} [{group["cpu_percent"]["min"]:.4f}–{group["cpu_percent"]["max"]:.4f}] | {value(group,"hz_per_match"):.3f} | '
                           f'{group["stages"]["height_ray"]["wall_ms_per_tick"]["median"]:.4f} | {group["stages"]["nav_ray"]["wall_ms_per_tick"]["median"]:.4f} | {group["passed"]}/{group["runs"]} |')
    memory = sum(mesh['bytes'] for mesh in terrain) / 1024 / 1024
    global_height = next(mesh['global'] for mesh in terrain if mesh['name'] == 'height')
    tick_before, tick_after = value(before, 'tick_duration_us', 1000), value(after, 'tick_duration_us', 1000)
    cpu_before, cpu_after = value(before, 'cpu_percent'), value(after, 'cpu_percent')
    cards = [('tick 실행', f'{tick_before:.2f} → {tick_after:.2f}ms', f'{(1-tick_after/tick_before)*100:.1f}% 감소'),
             ('게임 CPU', f'{cpu_before:.2f} → {cpu_after:.2f}%', '논리 코어 전체로 정규화'),
             ('매치별 갱신', f'{value(before,"hz_per_match"):.2f} → {value(after,"hz_per_match"):.2f}Hz', '실제 완료 tick / 매치 / 초'),
             ('실제 시나리오', f'{data["passed"]}/{data["runs"]} PASS', f'최대 {counts[-1]//4}매치 · {counts[-1]}명')]
    failed = data['failed']
    failures = '; '.join(f'{row.get("mode", "?")}/{row["clients"]}명/seed{row["seed"]}: {row.get("error") or row.get("infrastructure_error") or row.get("profile_error")}' for row in failed)
    failure_row = f'<tr><td>실행 실패 {len(failed)}회</td><td>준비/측정이 완료되지 않음</td><td>{html.escape(failures)}</td><td>실패율 유지·직접 원인 별도 추적</td></tr>' if failed else ''
    followup = failure_row + f'''<tr><td>예약 지연 {value(after, 'tick_delay_us', 1000):.2f}ms</td><td>대표 조건 갱신 {value(after, 'hz_per_match'):.2f}Hz</td><td>실행 시간 외 대기 유지·원인 미확정</td><td>타이머/worker·송신·관측 비용 분리</td></tr>
<tr><td>전역 후보 {global_height:,}개</td><td>호환성 때문에 후보 목록에 상시 포함</td><td>정상 전역 후보도 포함</td><td>데이터·수치 안정성 조사 후 범위 축소</td></tr>
<tr><td>첫 hit·높이 보정 규칙</td><td>가까운 면 선택과 다를 수 있음</td><td>이번에는 기존 동작 보존</td><td>계단·경사·중첩 지형의 품질 기준 확정</td></tr>
<tr><td>OBJ 로더 입력 검증</td><td>기존 인덱스 범위 검사 부족</td><td>코드 검토상 후속 대상</td><td>잘못된 면·재로드·수명 검사 추가</td></tr>
<tr><td>ContainsEdge 비교 대상</td><td>삼각형 정점을 검사하지 않음</td><td>기존 코드 결함 확인·경로 영향 미검증</td><td>인접 관계·Astar 회귀와 함께 수정</td></tr>
<tr><td>동일 이동 패킷 중복 송신</td><td>tick 밖 송신량에도 영향</td><td>기존 분석에서 확인·유지</td><td>독립 수정 후 전후 검증</td></tr>
<tr><td>근거리 전투·장시간</td><td>Astar/추가 웨이브 비용 미확정</td><td>현재 검증 범위 밖</td><td>전용 시나리오·잔여 비용 분석</td></tr>'''
    document = f'''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NavMesh·HeightMesh 공간 검색 개선</title><style>
*{{box-sizing:border-box}}body{{margin:0;background:#edf2f7;color:#172033;font:16px/1.6 "Malgun Gothic",sans-serif}}main{{max-width:1180px;margin:auto;padding:32px 24px 60px}}h1{{margin:0 0 12px}}h2{{font-size:24px;margin:0 0 15px}}section{{background:white;border:1px solid #d8e2ee;border-radius:18px;padding:25px;margin:22px 0}}.cards{{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}}.card{{padding:17px;background:#eef6fc;border-radius:12px}}.card strong{{display:block;font-size:21px;margin:8px 0}}small,.note{{color:#526379}}img{{width:100%;height:auto}}pre{{white-space:pre-wrap;overflow-wrap:anywhere;background:#111c30;color:#e2e8f0;padding:20px;border-radius:12px}}table{{border-collapse:collapse;width:100%;font-size:14px}}th,td{{padding:10px;border-bottom:1px solid #d8e2ee;text-align:left}}th{{background:#eef4fa}}a{{color:#0369a1}}.cols{{display:grid;grid-template-columns:1fr 1fr;gap:18px}}@media(max-width:850px){{.cards,.cols{{grid-template-columns:1fr 1fr}}table{{font-size:12px}}th,td{{padding:5px}}}}@media(max-width:560px){{.cards,.cols{{grid-template-columns:1fr}}main{{padding:16px 10px}}section{{padding:15px}}}}</style><main>
<h1>NavMesh·HeightMesh 공간 검색 개선</h1><p>XZ 격자로 검사 대상을 줄였습니다. 아래 대표 수치는 <b>{counts[-1]//4}매치 성공 {len(pairs[counts[-1]])}쌍</b>의 실행별 평균 중앙값입니다.</p>
<section><h2>1. 결과</h2><div class="cards">{''.join(f'<div class="card">{name}<strong>{number}</strong><small>{note}</small></div>' for name,number,note in cards)}</div><p class="note">원본 순서·판정 수식 유지. 인덱스 저장 공간 {memory:.2f}MiB 추가. 최대 처리량 검사가 아닌 고정 입력 부하 비교입니다.</p></section>
<section><h2>2. 분석한 원인</h2><div class="cols"><div><p>플레이어와 미니언이 지면 높이를 구할 때마다 메시 처음부터 검색했습니다. 같은 위치라도 대부분의 삼각형을 다시 계산했습니다.</p></div><div><pre>// 개선 전 흐름을 줄인 의사 코드
for (triangle : mesh) {{
    if (RayHits(triangle))
        return firstHit;
}}</pre></div></div>{image(charts/'candidates.png', '대표 매치 수에서 Ray당 방문 삼각형 수 전후 비교')}</section>
<section><h2>3. 해결 방식</h2><p>로드·높이 정렬 후 4단위 XZ 격자를 만들고, 해당 셀과 전역 후보를 원본 ID 순으로 병합합니다. tick 조회는 할당·잠금 없이 읽기만 합니다.</p>
<pre>// 개선 후 흐름을 줄인 의사 코드
for (id : index.Query(x, z, mesh.size())) {{
    if (RayHits(mesh[id]))
        return firstHit;  // 기존 순서와 수식
}}</pre><p class="note">경계에는 여유를 두고 수치적으로 불안정한 면은 전역 후보로 유지합니다. 비수직 Ray·극단 Y·무효 인덱스는 전체 검색합니다. <a href="../diagrams/terrain-spatial-index/terrain.html">공간 검색 흐름도</a></p></section>
<section><h2>4. 실제 측정한 내용</h2><p>Release x64 · LOCAL_TEST · Ryzen 5 7500F / 12논리 코어. 각 조건 전후 5쌍 시도, seed별 AB/BA 순서 교차·매회 재시작. 5초 워밍업 / 20초 측정 / 250ms 이동·정지, 입장 소프트 제한 해제·양쪽 계측 켬.</p>
{image(charts/'overview.png','1·4·16매치의 tick 시간 CPU 갱신 빈도 전후 중앙값과 최소 최대 범위')}
<table><tr><th>매치 / 인원</th><th>tick 실행 ms</th><th>예약 지연 ms</th><th>CPU %</th><th>갱신 Hz/매치</th><th>전 · 후 PASS</th></tr>{''.join(table)}</table>
<p class="note">값은 개선 전 → 후. 성능은 같은 seed의 양쪽 성공 쌍만 사용하며 실패는 PASS 분모에 남깁니다. 막대는 실행 평균의 중앙값, 오차선은 최소–최대입니다. 매치 준비 순서에 따라 NPC 수·매치 나이가 달라져 완전한 동일 월드 상태 비교는 아닙니다.</p>
<p>실제 메시 등가 {checks['terrain_assets']['checks']:,}비교 · Core Debug/Release {checks['core_checks']}검사씩 · ABI110종/106상수 · Python {checks['python_checks']}검사. 네트워크·초기화 실패 회귀와 미니언 이동 통합 {checks['integration_cases']}종 PASS.</p>
<p><a href="TERRAIN_SPATIAL_INDEX.md">상세 수치·재현·hash 근거</a> · <a href="GAME_TICK_PROFILE.html">기준 병목 분석</a></p></section>
<section><h2>5. 추가 발견한 결함 또는 대응 필요 요소</h2><table><tr><th>항목</th><th>영향</th><th>상태/근거</th><th>다음 대응</th></tr>{followup}</table><p class="note">기존 서버 경고와 테스트 포함으로 드러난 미사용 경고는 남아 있습니다. 전체 GUI 플레이·운영 DB/블록체인·최대 수용량 미검증. 그래프 PNG만 직접 점검하며 HTML 실제 시각/containment 검증은 미수행입니다.</p></section></main></html>'''
    (ROOT / 'docs/development/TERRAIN_SPATIAL_INDEX.html').write_text(document, encoding='utf-8')
    evidence = f'''# NavMesh·HeightMesh 공간 검색 개선 근거

실제 비교 {data['passed']}/{data['runs']} PASS. 보고서는 [HTML](TERRAIN_SPATIAL_INDEX.html), 구조는 [흐름도](../diagrams/terrain-spatial-index/README.md)다. 기존 기준 분석은 덮어쓰지 않았다. 실패: {failures or '없음'}.

## 실제 측정

원시 자료: `{directory.relative_to(ROOT).as_posix()}/`. `metadata.json`에 전후 실행 파일·소스·자원 hash, `source-snapshot/`에 개선 후 소스/실행 파일, `binaries/`에 비교 실행 파일을 보존했다. 기준 실행 파일은 이전 `game-tick-profile-20261012/source-snapshot/`에서 가져오며 데이터·시나리오 hash를 대조한다. 각 실행 `report.json`·원시 `lab/runs/*.json`·종료 로그, `before/analysis.json`·`after/analysis.json`·전체 `analysis.json`과 검증 JSON을 함께 보존한다. 원시 실행 로그는 Git 제외다.

Release x64 LOCAL_TEST, Ryzen 5 7500F, Windows11, 12논리 코어. 1/4/16매치 × 4명, 조건별5쌍 시도·seed1~5·홀수 후/전·짝수 전/후 교차. 5초 워밍업/20초 측정/250ms 이동·정지, 매회 서버 재시작·입장 소프트 제한 해제·양쪽 TickProfile 켬. 고정 부하 처리 성능이며 최대 처리량이 아니다. 수치는 실행별 평균의 중앙값 [최소–최대], 예약 지연과 실행 시간을 분리한다.

| 매치 | 전후 | tick 실행 ms [범위] | tick 예약 지연 ms | CPU % [범위] | 갱신 Hz/매치 | HeightRay ms/tick | NavRay ms/tick | PASS |
|---|---|---|---|---|---|---|---|---|
{chr(10).join(details)}

CPU는 논리 코어12개로 정규화한 프로세스값이다. Hz는 같은 프로파일 표본 구간의 완료 tick / 경과 초 / 매치 수다. 상세 cycle·stage·NPC 호출·후보 방문량과 observer CPU·generator lag는 전체 analysis에 보존한다. stage는 중첩하므로 합산하지 않는다. 매치 나이/활성 NPC는 순차 준비에 따라 달라지며 차이를 순수 경합 효과로 해석하지 않는다. 실패도 분모에 유지한다.

성능 표·그래프는 동일 seed의 양쪽 성공 쌍만 사용한다. 짝과 수치는 `paired-analysis.json`, 전체 성공/실패와 비대응 실행은 `analysis.json`에 보존한다. 조건별 대응 seed: `{pairs}`. 실패 실행에 측정 구간이 없으면 성능을 0으로 채우지 않는다.

대표 조건 observer CPU 중앙값은 {value(before, 'observer_cpu_percent'):.3f}→{value(after, 'observer_cpu_percent'):.3f}%다. 개선 후 수신/관측 처리량도 증가하므로 게임 프로세스 CPU와 구분한다. 원시 generator lag와 입력/패킷 처리량을 함께 보존하며 외부 시스템 부하 전체를 통제했다고 주장하지 않는다. 대표 조건 미니언 Update는 {before['stages']['minion_update']['calls_per_tick']['median']:.3f}→{after['stages']['minion_update']['calls_per_tick']['median']:.3f}마리/tick이다.

## 구현 계약

- `TerrainSpatialIndex.h`: 두 메시 공통 XZ 격자·CSR 후보 ID·전역 후보의 순서 병합. 빌드 후 불변, 조회 시 메모리 할당/잠금 없음. 최대셀1,048,576개·후보 참조8,388,608개 상한을 넘으면 전체 검색한다.
- `GameUtil.cpp`: NavMesh 로드, HeightMesh 기존 높이 정렬 뒤 구축. 노드/삼각형은 기존 소유자에 있고 인덱스에는 ID만 있다. worker 시작 전에 구축하며 종료 후 메시 해제 전에 초기화한다.
- `MathUtil.h`: 기본 수직 하향 Ray와 finite/절댓값1024이하 Y만 가속한다. 기존 first-hit·높이1.5 필터·일반 높이0.8/Nav0.6 보정·스킬 overload 의미·miss 출력값 유지. 근접 hit 재정렬이나 정적 판정 수식 캐시는 추가하지 않았다.
- 경계 패딩은 `0.01 + 64*float_epsilon*scale*condition`. condition>256·거의 수직·큰 좌표 등의 면은 전역 후보다. 기존 필터로 거부되는 면과 정확한 수직면은 수직 Ray 후보에서 제외한다. 비수직/비정상 좌표는 전체 검색으로 돌아간다.
- 저장 공간 `{memory:.3f}MiB`는 vector capacity 기반 인덱스 배열 합이며 구축 임시 메모리·전체 프로세스 메모리는 아니다. 실제 프로세스 peak sampled private bytes도 analysis에 별도로 있다.

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

`TerrainReferenceRay.h`는 main `3e34e71a` Ray 수식을 보존한 테스트 oracle다. 실제 두 메시 정점/경계 nextafter/중심·임의 XZ/Y·4 overload의 hit·float bit·노드 ID 비교 {checks['terrain_assets']['checks']:,}개 PASS. Core에는 중첩 지형·첫 hit 순서·높이 제한·miss 출력 보존·빈/무효/범위 초과 인덱스·비수직/극단 Y·비정상 float·동시 읽기 검사를 추가했다. 유한 표본 검사이며 모든 float 입력의 수학적 동등성을 증명한 것은 아니다.

Debug/Release 서버 빌드·Core 각{checks['core_checks']}·ABI110종/106상수·Python{checks['python_checks']} PASS. Release 네트워크12종/보스4·5 매치/초기화 실패4종과 Server Lab 통합{checks['integration_cases']}종·첫 미니언4개 경로 이동 PASS. 정상 종료 pending/sockets/leased=0, 비교 종료 후 개선 실행 파일 복원 및 fingerprint 불변을 확인한다.

기존 C4065/C4101/C4244/C4305/C4309가 남는다. 테스트에서 기존 MathUtil/고정 참조 코드의 C4100/C4189/C4505도 드러나며 숨기지 않았다. Archify showcase9/9·오류/경고0. 기존 local-file 브라우저 보안 거절을 우회하지 않아 실제 HTML 시각/containment는 미검증이다. PNG 그래프는 직접 검토한다. 운영 DB/블록체인·전체 GUI 플레이·장시간/근거리 전투·최대 수용량 미검증. 커밋/푸시는 수행하지 않았다.

## 후속 품질 개선

HeightMesh 전역 후보 {global_height:,}개를 유지했다. 이는 모두 결함이라는 의미가 아니다. 데이터의 가늘거나 수직에 가까운 면과 barycentric 수치 안정성을 조사한 뒤 경계 판정·중첩/계단/경사 기준을 정하고 축소한다. `Triangle::ContainsEdge`는 `_v1/_v2` 인자끼리/자기 자신만 비교하고 메시 정점을 사용하지 않는 기존 결함이 있다. 이번에는 수정하지 않았으며 실제 Astar 영향은 미검증이다. OBJ 입력/재로드·수명 검증, 기존 중복 송신, 근거리 전투/Astar·복수 웨이브·tick 밖 잔여 CPU는 별도 작업으로 남긴다.
'''
    (ROOT / 'docs/development/TERRAIN_SPATIAL_INDEX.md').write_text(evidence, encoding='utf-8')
    print('공간 검색 HTML 보고서·근거 문서·그래프2개 생성')


if __name__ == '__main__':
    main()
