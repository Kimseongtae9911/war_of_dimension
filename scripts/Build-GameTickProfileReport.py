"""실제 tick profile 결과에서 짧은 HTML 보고서와 수치 근거를 생성한다."""
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
TOP = ('game_clock', 'jobs', 'player_update', 'player_send', 'minion_update', 'monster_update', 'periodic', 'observation')
COLORS = ['#38bdf8', '#fbbf24', '#fb7185', '#a78bfa', '#34d399', '#94a3b8']


def number(value, digits=2):
    return f'{value:,.{digits}f}' if value is not None else '미계측'


def image(path, alt):
    data = base64.b64encode(path.read_bytes()).decode('ascii')
    return f'<img src="data:image/png;base64,{data}" alt="{html.escape(alt)}">'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    args = parser.parse_args()
    directory = args.input.resolve()
    data = json.loads((directory / 'analysis.json').read_text(encoding='utf-8'))
    enabled = sorted((group for group in data['groups'] if group['enabled']), key=lambda group: group['clients'])
    if not enabled or any('stages' not in group for group in enabled):
        raise ValueError('profile 켬의 구간별 실측이 필요함')
    off = next((group for group in data['groups'] if not group['enabled'] and group['clients'] == enabled[0]['clients']), None)
    first = enabled[0]
    passed, runs = sum(group['passed'] for group in data['groups']), sum(group['runs'] for group in data['groups'])
    charts = directory / 'report-charts'
    charts.mkdir(exist_ok=True)
    plt.rcParams.update({'font.family': 'Malgun Gothic', 'axes.unicode_minus': False, 'font.size': 11,
                         'figure.facecolor': '#f8fafc', 'axes.facecolor': '#f8fafc', 'savefig.facecolor': '#f8fafc',
                         'axes.spines.top': False, 'axes.spines.right': False})
    labels = [f'{group["matches"]}매치\n{group["clients"]}명' for group in enabled]
    # 같은 실행들의 평균을 사용해 큰 구간과 실제 잔차의 합이 total과 일치하게 한다.
    means = []
    for group in enabled:
        profiles = [row['tick_profile'] for row in data['records'] if row.get('profile_enabled') is True
                    and row['clients'] == group['clients'] and (row.get('tick_profile') or {}).get('ticks')]
        means.append({stage: statistics.mean(profile['stages'][stage]['wall_ms_per_tick'] for profile in profiles) for stage in ('total', *TOP)})
    components = [('player_update', '플레이어 Update'), ('minion_update', '미니언 Update'), ('player_send', '플레이어 송신'), ('monster_update', '일반 몬스터 Update')]
    fig, ax = plt.subplots(figsize=(10.2, 4.1), layout='constrained')
    bottom = [0.] * len(enabled)
    for index, (key, label) in enumerate(components):
        values = [row[key] for row in means]
        ax.bar(labels, values, bottom=bottom, label=label, color=COLORS[index])
        bottom = [left + right for left, right in zip(bottom, values)]
    residual = [row['total'] - value for row, value in zip(means, bottom)]
    if any(value < 0 for value in residual):
        raise ValueError('동시 snapshot 잔차가 음수여서 stack 그래프 생성 중단')
    ax.bar(labels, residual, bottom=bottom, label='그 외·계측 잔차', color=COLORS[5])
    ax.axhline(15, color='#b91c1c', linestyle='--', label='예약 간격 15ms')
    for index, row in enumerate(means):
        ax.text(index, row['total'] + .6, f'{row["total"]:.2f}ms', ha='center', fontweight='bold')
    ax.set(ylabel='tick당 경과 시간 (ms)', ylim=(0, max(row['total'] for row in means) * 1.24), title='tick의 큰 구간 · 실행별 평균을 다시 평균')
    ax.legend(loc='upper left', bbox_to_anchor=(1, 1), frameon=False)
    fig.savefig(charts / 'wall.png', dpi=165)
    plt.close(fig)
    fig, ax = plt.subplots(figsize=(10.2, 4.1), layout='constrained')
    bottom = [0.] * len(enabled)
    for index, (key, label) in enumerate([('height_ray_minion', '미니언 높이 Ray'), ('height_ray_player', '플레이어 높이 Ray'), ('nav_ray', 'NavMesh Ray')]):
        values = [group['stages'][key]['cycle_share_percent']['median'] for group in enabled]
        ax.bar(labels, values, bottom=bottom, label=label, color=[COLORS[1], COLORS[0], COLORS[2]][index])
        bottom = [left + right for left, right in zip(bottom, values)]
    other = [100 - value for value in bottom]
    if any(value < 0 for value in other):
        raise ValueError('cycle 중앙값의 합이 100% 초과, 원시 결과 확인 필요')
    ax.bar(labels, other, bottom=bottom, label='그 외·잔차', color=COLORS[5])
    ax.set(ylabel='계측한 tick CPU cycle 비중 (%)', ylim=(0, 105), title='중첩을 제외한 지형 검색 비중 · 실행별 비중의 중앙값')
    ax.legend(loc='upper left', bbox_to_anchor=(1, 1), frameon=False)
    fig.savefig(charts / 'cycles.png', dpi=165)
    plt.close(fig)
    table = []
    ray_table = []
    for group in enabled:
        stages = group['stages']
        table.append(f'<tr><td>{group["matches"]}개 / {group["clients"]}명</td><td>{number(group["tick_duration_us"]["median"] / 1000)}ms</td>'
                     f'<td>{number(group["tick_delay_us"]["median"] / 1000)}ms</td><td>{number(group["cpu_percent"]["median"])}%</td><td>{group["passed"]}/{group["runs"]}</td></tr>')
        ray_table.append(f'<tr><td>{group["matches"]}개</td><td>{number(stages["height_ray"]["wall_ms_per_tick"]["median"])}ms</td>'
                         f'<td>{number(stages["height_ray"]["cycle_share_percent"]["median"])}%</td><td>{number(stages["height_ray"]["calls_per_tick"]["median"])}회</td>'
                         f'<td>{number(stages["height_ray"]["work_per_call"]["median"], 0)}개</td><td>{number(stages["minion_update"]["calls_per_tick"]["median"])}마리</td></tr>')
    overhead = '<p>계측 끔 대조 없음.</p>'
    if off:
        overhead = f'<p>1매치 계측 끔 → 켬: tick <b>{number(off["tick_duration_us"]["median"] / 1000)} → {number(first["tick_duration_us"]["median"] / 1000)}ms</b>, CPU <b>{number(off["cpu_percent"]["median"])} → {number(first["cpu_percent"]["median"])}%</b>. 각 {off["runs"]}/{first["runs"]}회, 같은 seed 순서 교차.</p>'
    stages = first['stages']
    height_ms = stages['height_ray']['wall_ms_per_tick']['median']
    height_cpu = stages['height_ray']['cycle_share_percent']['median']
    minion_cpu = stages['minion_update']['cycle_share_percent']['median']
    detail_rows = []
    for key, label in [('minion_update', '미니언 Update'), ('player_update', '플레이어 Update'), ('height_ray', 'HeightMesh Ray (부모 내부)'),
                       ('height_ray_minion', '미니언 높이 Ray (내부)'), ('height_ray_player', '플레이어 높이 Ray (내부)'), ('nav_ray', 'NavMesh Ray (부모 내부)'), ('player_send', '플레이어 이동 broadcast'), ('send_lock', '송신 잠금 획득 (부모 내부)'), ('jobs', 'JobQueue 처리')]:
        detail_rows.append(f'<tr><td>{label}</td><td>{number(stages[key]["wall_ms_per_tick"]["median"], 3)}ms</td><td>{number(stages[key]["cycle_share_percent"]["median"])}%</td></tr>')
    output = ROOT / 'docs/development/GAME_TICK_PROFILE.html'
    report = '''<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>게임 tick 비용 분석</title><style>
:root{color-scheme:light}*{box-sizing:border-box}body{margin:0;background:#edf2f7;color:#172033;font:16px/1.65 "Malgun Gothic",sans-serif}main{max-width:1120px;margin:auto;padding:32px 24px 60px}h1{font-size:32px;margin:0 0 12px}h2{font-size:23px;margin:0 0 14px}h3{font-size:17px;margin:12px 0}p{margin:8px 0 14px}section{background:white;border:1px solid #dce4ef;border-radius:16px;padding:25px;margin-top:22px}.lead{font-size:20px;font-weight:700}.muted{color:#526178;font-size:14px}.cards{display:grid;grid-template-columns:repeat(3,1fr);gap:14px}.card{padding:18px;background:#f1f6fc;border-radius:12px}.card strong{display:block;font-size:27px;color:#164f82}.grid{display:grid;grid-template-columns:1fr 1fr;gap:20px}.flow{padding:16px;border-left:5px solid #f59e0b;background:#fffbeb;font-weight:600}table{border-collapse:collapse;width:100%;font-size:14px}th,td{text-align:left;padding:10px;border-bottom:1px solid #e2e8f0}th{background:#f1f5f9}pre{background:#172033;color:#e2e8f0;padding:18px;border-radius:10px;white-space:pre-wrap;overflow-wrap:anywhere;font-size:13px}img{width:100%;height:auto;display:block;margin:12px 0}a{color:#175da1}code{font-family:Consolas,monospace}.note{border-left:4px solid #38bdf8;padding:9px 14px;background:#f0f9ff}@media(max-width:720px){main{padding:20px 12px}.cards,.grid{grid-template-columns:1fr}section{padding:18px}table{font-size:12px}th,td{padding:6px}h1{font-size:27px}}
</style></head><body><main>'''
    report += f'''<h1>게임 tick 비용 분석</h1><p class="lead">HeightMesh 선형 검색이 tick의 지배 비용이다. 첫 개선 대상은 지형 검색이다.</p>
<p class="muted">2026-10-12 · main 기반 codex/game-tick-profiling · 최적화 전 분석</p>
<section id="result"><h2>1. 결과</h2><div class="cards"><div class="card">1매치 tick 실행<strong>{number(first['tick_duration_us']['median'] / 1000)}ms</strong>예약 간격 15ms 초과</div>
<div class="card">1매치 높이 Ray<strong>{number(height_ms)}ms / {number(height_cpu, 1)}%</strong>경과 시간 / tick CPU cycle</div>
<div class="card">실제 기능·종료 검사<strong>{passed}/{runs}</strong>실패도 분모에 보존</div></div><p>미니언 Update는 1매치 tick CPU cycle의 {number(minion_cpu, 1)}%. 아래 세부 구간은 부모 내부 비용이므로 서로 중복 합산하지 않는다.</p></section>
<section id="cause"><h2>2. 분석한 원인</h2><div class="flow">플레이어·미니언 이동 → NavMesh 검색 → HeightMesh 검색 → 다음 tick에서 반복</div>
<div class="grid"><div><p>HeightMesh 127,756개 삼각형을 앞에서부터 검색한다. 첫 교차점을 찾을 때까지 법선·평면 교차·삼각형 내부 판정을 매번 계산한다.</p>
<pre>// MathUtil.h의 실제 RayCast 핵심 (발췌)
for (const Triangle&amp; triangle : _triangles) {{
    // 높이 필터 이후
    Vector3 normal = Vector3::Normalize(...);
    float d = (...).Dot(normal) / m_direction.Dot(normal);
    // 교차점·내부 판정 → 첫 hit 반환
}}</pre></div><div><h3>1매치의 비용</h3><table><thead><tr><th>구간</th><th>wall/tick</th><th>CPU cycle 비중</th></tr></thead><tbody>{''.join(detail_rows)}</tbody></table></div></div>
<p class="muted">cycle 비중은 계측한 tick의 CPU cycle이 분모다. 전체 프로세스 CPU 비중이나 순수 CPU 실행 시간(ms)으로 해석하지 않는다. wall에는 OS 스케줄링·잠금 대기도 포함한다.</p></section>
<section id="solution"><h2>3. 해결 방향</h2><p>이번 변경은 계측과 분석이다. 지형 검색 최적화와 중복 송신 수정은 아직 적용하지 않았다.</p>
<div class="grid"><div><h3>우선: 지형 검색 후보 축소</h3><pre>// 후속 설계 의사 코드 · 미구현
candidates = xzGrid.Query(ray.origin);
SortByOriginalTriangleOrder(candidates);
for (triangle : candidates) {{
    if (ExistingRayTest(triangle, ray))
        return ExistingHitResult(triangle);
}}</pre></div><div><h3>유지할 판정 계약</h3><p>수직 Ray의 공간 후보를 줄이고 정적 삼각형 계산을 사전 저장한다. 기존 첫 hit 순서·높이 필터·일반/스킬 판정 차이를 보존해야 한다.</p>
<p>NavMesh는 현재 노드/주변 후보를 우선 활용하는 방식을 검토한다. 정지 상태 캐시는 위치·지형 세대·강제 이동의 무효화 조건부터 정해야 한다.</p></div></div>
<p class="note">다음 단계: 동일 좌표/경계/실패/스킬 Ray 판정 회귀 → 같은 시나리오 5회 전후 비교 → tick·CPU·이동/NPC 동작 확인.</p></section>
<section id="measurement"><h2>4. 실제 측정</h2><p>Release x64 LOCAL_TEST · Ryzen 5 7500F / 논리 CPU 12개 · 실제 4인 매치 · 입장 소프트 제한 해제 · 매회 서버 재시작. 워밍업5초 / 측정20초 / 입력250ms, 인원별 profile 켬5회.</p>
<table><thead><tr><th>매치 / 인원</th><th>tick 실행</th><th>예약 지연</th><th>프로세스 CPU</th><th>통과</th></tr></thead><tbody>{''.join(table)}</tbody></table>
<p class="muted">표는 실행별 평균의 중앙값. CPU는 논리 CPU 전체로 정규화. 그래프는 표기한 집계 방식이며 최대 처리량 측정이 아니다.</p>
{image(charts / 'wall.png', '매치 수별 tick 큰 구간의 경과 시간과 15ms 예약 간격')}
{image(charts / 'cycles.png', '매치 수별 미니언 높이 검색, 플레이어 높이 검색, NavMesh 검색의 tick CPU cycle 비중')}
<table><thead><tr><th>매치</th><th>HeightRay wall/tick</th><th>cycle 비중</th><th>Ray/tick</th><th>방문/Ray</th><th>미니언 Update/tick</th></tr></thead><tbody>{''.join(ray_table)}</tbody></table>
{overhead}<p class="muted">계측 끔도 같은 계측 코드가 포함된 실행 파일이다. 원본 main 바이너리 대비 비활성 분기/카운터 코드 비용은 별도 미측정. 다중 매치는 NPC 나이·활성 수와 방문 삼각형 수도 달라진다. tick 시간 증가를 worker 경합만으로 설명하거나 순수 매치 수 확장 비율로 해석하지 않는다.</p></section>
<section id="followup"><h2>5. 추가 결함·대응 필요</h2><table><thead><tr><th>항목</th><th>확인 내용 / 영향</th><th>다음 대응</th></tr></thead><tbody>
<tr><td>이동 패킷 중복 · 확인</td><td>CMatch가 같은 player→receiver 프레임을 연속2회 송신</td><td>독립 수정 후 프레임/tick·이동 기능 비교</td></tr>
<tr><td>정지 상태 검색 · 코드 확인</td><td>일반 Move 경로는 입력/변위가0이어도 지형 검색에 진입 가능</td><td>정확한 정지 조건·캐시 무효화 설계 및 별도 정지 시나리오</td></tr>
<tr><td>송신·worker 경합 · 부분 계측</td><td>tick 내부 Send 잠금 대기만 측정; 비동기 완료/타이머 경합 비용은 미분해</td><td>지형 개선 후 잔여 비용에 맞춰 ETW/handler 계측</td></tr>
<tr><td>전투·장시간 · 미측정</td><td>현재 이동/정지 시나리오의 NPC 상태·수만 포함</td><td>근거리 전투/복수 웨이브·동일 상태 재생 시나리오 추가</td></tr></tbody></table>
<p class="muted">Debug/Release 서버 빌드·Core 각218검사·ABI110종/106상수·Python33검사·Release 네트워크12종/보스4·5/초기화 실패4종 통과. 기존 C4065/C4101/C4244/C4305/C4309 경고는 유지한다. 전체 GUI 플레이·운영 DB/블록체인은 미검증이다. 보고서 HTML의 실제 브라우저 검토는 미수행이다.</p></section>
<p class="muted"><a href="GAME_TICK_PROFILE.md">수치·조건·원시 근거</a> · <a href="SERVER_LAB.md#게임-tick-구간별-비용-분석">도구/API 계약</a> · <a href="../diagrams/game-tick-profile/README.md">계측 흐름도</a></p></main></body></html>'''
    output.write_text(report, encoding='utf-8')
    md = ['# 게임 tick 로직 비용 분석 (2026-10-12)', '', '[HTML 요약](GAME_TICK_PROFILE.html) · [도구 계약](SERVER_LAB.md#게임-tick-구간별-비용-분석) · [계측 흐름도](../diagrams/game-tick-profile/README.md).', '',
          '## 1. 결과', '', f'HeightMesh 선형 검색이 지배 비용이다. 1매치 tick 실행 {number(first["tick_duration_us"]["median"] / 1000)}ms, HeightRay {number(height_ms)}ms/tick·tick CPU cycle {number(height_cpu)}%, 실제 기능/종료 검사 {passed}/{runs}. 최적화 전 분석이며 로직 최적화는 미적용이다.', '',
          '## 2. 분석한 원인', '', 'MathUtil.h의 RayCast는 매 호출마다 HeightMesh127,756개/NavMesh5,743개 중 첫 hit까지 선형 순회한다. 방문 수는 높이 필터로 건너뛴 항목도 포함한다. 삼각형 법선·교차·내부 판정을 반복하며, 미니언 이동과 플레이어 이동 양쪽이 GameUtil::MapCollision을 호출한다. HeightMesh는 로드 시 평균 높이 내림차순으로 정렬되며 현재 첫 hit 순서는 게임 판정의 일부다.', '',
          '## 3. 해결 방향', '', 'XZ 공간 후보 축소·정적 삼각형 계산 캐시를 우선 검토한다. 기존 first-hit 원본 순서·높이 필터·스킬/일반 Ray 차이를 유지하는 회귀가 먼저다. NavMesh의 현재 노드 활용과 정지 캐시는 위치 변경/강제 이동/지형 세대의 무효화 조건을 검증해야 한다. 모두 후속 제안이며 이번에는 미적용이다.', '',
          '## 4. 실제 측정', '', 'Release x64 LOCAL_TEST·Ryzen5 7500F/논리 CPU12·입장 소프트 제한 해제·기존 perf-match·4인 매치·매회 재시작·워밍업5초/측정20초/250ms 입력. 각 인원 profile 켬5회, 최소 인원 끔5회는 seed 홀짝 실행 순서를 교차한다.', '',
          '| 매치 / 인원 | tick 실행 평균 ms | 예약 지연 평균 ms | 프로세스 CPU % | 통과 |', '|---|---:|---:|---:|---:|']
    for group in enabled:
        md.append(f'| {group["matches"]} / {group["clients"]} | {number(group["tick_duration_us"]["median"] / 1000)} [{number(group["tick_duration_us"]["min"] / 1000)}–{number(group["tick_duration_us"]["max"] / 1000)}] | {number(group["tick_delay_us"]["median"] / 1000)} | {number(group["cpu_percent"]["median"])} | {group["passed"]}/{group["runs"]} |')
    md += ['', '실행별 평균의 중앙값 [최소–최대]. CPU는 논리 CPU 전체로 정규화한다. wall은 스케줄링/대기를 포함하며 cycle은 QueryThreadCycleTime의 해당 worker 누적 차분이다. cycle을 시간으로 변환하지 않고 전체 프로세스 CPU 비중으로 해석하지 않는다. 큰 구간/세부 구간/소유자별 height_ray는 포함 관계이므로 중복 합산하지 않는다.', '',
           '| 매치 | HeightRay ms/tick | tick cycle 비중 % | 방문 삼각형/Ray | 미니언 Update/tick |', '|---|---:|---:|---:|---:|']
    for group in enabled:
        s = group['stages']
        md.append(f'| {group["matches"]} | {number(s["height_ray"]["wall_ms_per_tick"]["median"])} | {number(s["height_ray"]["cycle_share_percent"]["median"])} | {number(s["height_ray"]["work_per_call"]["median"],0)} | {number(s["minion_update"]["calls_per_tick"]["median"])} |')
    if off:
        md += ['', f'1매치 계측 끔→켬: tick {number(off["tick_duration_us"]["median"] / 1000)}→{number(first["tick_duration_us"]["median"] / 1000)}ms, CPU {number(off["cpu_percent"]["median"])}→{number(first["cpu_percent"]["median"])}%. 각5회. 같은 실행 파일의 계측 플래그 대조이며, 원본 main 대비 비활성 경로 비용은 미측정이다. NPC 난수/매치 나이·활성 수가 완전히 고정되지 않으므로 차이를 순수 계측 오버헤드로 확정하지 않는다.']
    md += ['', f'원시 결과: `{directory.relative_to(ROOT).as_posix()}/`의 metadata/fingerprints·runs.jsonl·analysis.json·verification.json, 실행별 report.json·idle.json·lab/runs/*.json 및 서버 종료 로그. HTML 그래프는 report-charts의 Matplotlib PNG를 내장한다.', '',
           '```powershell', 'python tools/server_lab/tick_profile.py --output artifacts/logs/game-tick-profile-new --clients 4 16 64 --repeats 5', 'python tools/server_lab/tick_profile.py --output artifacts/logs/game-tick-profile-new --analyze', f'python scripts/Build-GameTickProfileReport.py --input {directory.relative_to(ROOT).as_posix()}', '```', '',
           '수집/API/저장 경로와 누락·오류·재시작·scope 포함 관계는 Server Lab 계약을 따른다. reporter snapshot은 동시 누적의 근사값이다. frame 합산 wall은 여러 worker의 합계이며 실제 elapsed와 구분한다. root에는 MatchUpdate 종료 경로도 포함할 수 있으므로 perf-match 측정 구간의 활성 매치만 해석한다.', '',
           '실제 검증: Debug/Release 서버 빌드·Core 각218검사·Protocol ABI110종/106상수·Python33검사 PASS. Release 네트워크12종·보스4/5의4인 매치·초기화 실패4종 PASS. 성능20/20 정상 종료·양쪽 pending/sockets/leased=0, 모든 구간 cycle_errors=0, API/저장 결과 필드 확인. 소스/실행 파일/데이터340개 hash 유지 및 source-snapshot 보존. 기존 C4065/C4101/C4244/C4305/C4309 경고 유지. Astar는 이번 이동/정지 측정에서 호출0이며 전투 비용 판정에 사용하지 않는다. 그래프 PNG 직접 점검·HTML 정적 검사와 실제 HTML 시각 검토 미수행을 구분한다.', '', '## 5. 추가 결함·대응 필요', '', '| 항목 | 영향/상태 | 후속 |', '|---|---|---|',
           '| 이동 프레임 중복 | CMatch.cpp 동일 SendMovePacket 연속2회, 확인 | 독립 수정·프레임/tick 비교 |',
           '| 정지 지형 검색 | CClient::Move 일반 경로에 변위0의 지형 검색 진입 가능, 코드 확인 | 정확한 정지 조건/캐시 무효화·정지 전용 측정 |',
           '| 비동기 완료/스킬·전투 비용 | tick 밖 IOCP handler 및 근거리 전투 시나리오 미분해 | 지형 개선 후 잔여 비용 계측·전투/장시간 시나리오 |',
           '| 다중 매치 상태 차이 | 순차 준비로 매치 나이·NPC 스폰 수가 다름 | 실제 미니언 Update/tick 함께 해석·상태 통제 후 확장 비교 |', '',
           '기존 JobQueue 추가 후보/전용 도구는 제외했고 과거 보고서는 보존했다. 새 브랜치는 codex/game-tick-profiling이며 큐는 main 상태다. 커밋/푸시는 수행하지 않았다. 운영 DB·블록체인·전체 GUI 플레이·최대 처리량·장시간 전투는 미검증이다.']
    text = '\n'.join(md) + '\n'
    output.with_suffix('.md').write_text(text, encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
