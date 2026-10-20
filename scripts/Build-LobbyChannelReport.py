"""로비 채널 병렬화의 기존 근거만 읽어 수정 전후 HTML 보고서를 재생성한다."""
import base64
import hashlib
import html
import io
import json
from pathlib import Path
import statistics

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib import font_manager

ROOT = Path(__file__).resolve().parents[1]
LOGS = ROOT / 'artifacts/logs'
OUTPUT = ROOT / 'docs/development/LOBBY_CHANNEL_PARALLELISM.html'
GROUPS = {
    'repeat': 'lobby-channel-final-comparison-20261018',
    'final': 'lobby-channel-final-verification-20261018',
    'off': 'lobby-channel-final-metrics-off-20261018',
}


def read(relative):
    return json.loads((LOGS / relative).read_text(encoding='utf-8-sig'))


def checked_runs(name, count):
    data = read(GROUPS[name] + '/summary.json')
    assert data['fingerprints_unchanged'] and len(data['runs']) == count, name
    assert all(row['ok'] and row['shutdown_ok'] and row['assertions_failed'] == 0 for row in data['runs']), name
    for row in data['runs']:
        if row['metrics']:
            assert row['dispatcher']['owner_violations'] == 0
            assert row['dispatcher']['max_parallel'] == row['workers']
            settings = row['servers']['LobbyServer']['admission']
            assert settings['channel_capacity'] == 4 and settings['dispatcher']['channel_count'] == 32
        assert row['clients'] == 128
    return data['runs']


def values(rows, workers, metric):
    data = [row['servers']['LobbyServer'] for row in rows if row['workers'] == workers]
    if metric.endswith('_p99_ms'):
        name = metric.removesuffix('_p99_ms') + '_us'
        return [row[name]['p99_us_upper'] / 1000 for row in data]
    return [row[metric] for row in data]


def median(rows, workers, metric):
    return statistics.median(values(rows, workers, metric))


def chart(rows):
    font = Path('C:/Windows/Fonts/malgun.ttf')
    if not font.exists():
        raise RuntimeError('한글 그래프 폰트 malgun.ttf가 필요합니다')
    plt.rcParams.update({'font.family': font_manager.FontProperties(fname=font).get_name(), 'font.size': 11, 'axes.unicode_minus': False})
    fig, axes = plt.subplots(1, 3, figsize=(12, 3.7), layout='constrained', facecolor='#f7f9fc')
    specs = [('packet_job_wait_p99_ms', '패킷 대기 p99 상한', 'ms · 낮을수록 짧은 대기', 3),
             ('cpu_percent', '서버 CPU', '% · 전체 논리 코어 12개 기준', 2),
             ('packets_per_s', '고정 부하의 실제 처리율', '패킷/s · 입력 약 512/s', 2)]
    for ax, (metric, title, unit, digits) in zip(axes, specs):
        ax.set_facecolor('#f7f9fc')
        data = [values(rows, workers, metric) for workers in (1, 4)]
        centers = [statistics.median(points) for points in data]
        bars = ax.bar([0, 1], centers, color=['#8b98ad', '#1579b8'], width=.52, zorder=2)
        for index, (bar, points, center) in enumerate(zip(bars, data, centers)):
            ax.scatter([index - .06, index, index + .06], points, color='#142a40', s=18, zorder=3)
            ax.annotate(f'{center:,.{digits}f}', (bar.get_x() + bar.get_width()/2, max(points)), xytext=(0, 10),
                        textcoords='offset points', ha='center', fontsize=12, fontweight='bold')
        if metric == 'packets_per_s':
            ax.axhline(512, color='#917b43', linestyle='--', linewidth=1, zorder=1)
        ax.set_title(title, fontsize=13, loc='left', pad=15)
        ax.set_ylabel(unit, fontsize=10)
        ax.set_xticks([0, 1], ['worker 1개', 'worker 4개'])
        ax.set_ylim(0, max(max(points) for points in data) * 1.3)
        ax.spines[['top', 'right']].set_visible(False)
        ax.spines[['left', 'bottom']].set_color('#b8c5d3')
        ax.grid(axis='y', color='#dfe6ee', zorder=0)
        ax.set_axisbelow(True)
    image = io.BytesIO()
    fig.savefig(image, format='png', dpi=155)
    plt.close(fig)
    data = image.getvalue()
    (LOGS / 'lobby-channel-report-comparison-20261018.png').write_bytes(data)
    return base64.b64encode(data).decode('ascii')


def density_report():
    cases = [('4채널 × 32명', 'lobby-density-c4-p32-20261018'),
             ('4채널 × 64명', 'lobby-density-c4-p64-20261018'),
             ('4채널 × 128명', 'lobby-density-c4-p128-20261018'),
             ('1채널 × 128명', 'lobby-density-c1-p128-20261018')]
    groups = []
    sources = []
    for label, directory in cases:
        if not (LOGS / directory / 'summary.json').exists():
            continue
        data = read(directory + '/summary.json')
        metadata = read(directory + '/metadata.json')
        assert data['fingerprints_unchanged']
        sources += [directory + '/summary.json', directory + '/metadata.json']
        groups.append((label, directory, data['runs'], metadata))
    if not groups:
        return {'result': '', 'measurement': '', 'sources': [], 'followup': '', 'cause': '', 'lead': ''}
    sources += ['lobby-density-machine-20261018.json', 'lobby-channel-density-python-tests-20261018.log', 'lobby-channel-density-build-20261018.log']
    fig, axes = plt.subplots(1, 3, figsize=(12, 4.6), layout='constrained', facecolor='#f7f9fc')
    specs = [('packet_job_wait_p99_ms', '패킷 대기 p99 상한', 'ms'),
             ('cpu_percent', '로비 CPU', '% · 논리 코어 12개 기준'),
             ('lobby_movement_gap_p99_ms', '이동 갱신 간격 p99 상한', 'ms')]
    table_rows = []
    detail_rows = []
    failed_rows = []
    analysis = []
    for index, (label, directory, rows, metadata) in enumerate(groups):
        for workers in (1, 4):
            selected = [row for row in rows if row['workers'] == workers]
            measured = [row for row in selected if 'LobbyServer' in row.get('servers', {})]
            if not selected:
                continue
            passed = sum(bool(row['ok']) for row in selected)
            stats = {metric: median(measured, workers, metric) for metric, _, _ in specs} if measured else {}
            for ax, (metric, _, _) in zip(axes, specs):
                if measured:
                    points = values(measured, workers, metric)
                    x = index + (-.18 if workers == 1 else .18)
                    color = '#8b98ad' if workers == 1 else '#1579b8'
                    if len(selected) == 3 and passed == 3:
                        ax.bar(x, stats[metric], width=.32, color=color,
                               label=f'worker {workers}개' if index == 0 else None, zorder=2)
                        ax.annotate(f'{stats[metric]:.1f}', (x, stats[metric]), xytext=(0, 6), textcoords='offset points', ha='center', fontsize=8)
                    ax.scatter([x + (j - (len(points)-1)/2)*.04 for j in range(len(points))], points, color=color, edgecolor='#172d43', s=16, zorder=3)
                    for j, row in enumerate(measured):
                        if not row['ok']:
                            ax.scatter(x + (j - (len(points)-1)/2)*.04, points[j], color='#b4492f', marker='x', s=48, zorder=4)
            def cell(metric, digits=2):
                if not measured:
                    return '미수집'
                if len(selected) != 3 or passed != 3:
                    points = values(measured, workers, metric)
                    low, high = min(points), max(points)
                    return f'{low:,.{digits}f}–{high:,.{digits}f}' if low != high else f'{low:,.{digits}f}'
                return f'{median(measured, workers, metric):,.{digits}f}'
            lag = [row['generator_lag_ms']['p99'] for row in measured]
            observer = [row['observer_cpu_core_percent'] for row in measured]
            limits = sum(bool(row.get('generator_limit')) for row in selected)
            errors = [row.get('measurement_error') or row.get('error') or row.get('infrastructure_error') for row in selected if not row['ok']]
            for row in selected:
                if not row['ok']:
                    value = row['servers'].get('LobbyServer', {})
                    run_directory = f'{row["repeat"]:02d}-workers-{workers}'
                    sources += [f'{directory}/{run_directory}/Stop-ServerLab.ps1.log', f'{directory}/{run_directory}/lab/runs/{row["run_id"]}.json']
                    failed_rows.append(f'<tr><th>{label} / worker{workers} / 반복{row["repeat"]}</th>'
                                       f'<td>{html.escape(row.get("error") or row.get("measurement_error") or row.get("infrastructure_error") or "검증 실패")}</td>'
                                       f'<td>{value.get("send_queue_end", 0):,}건</td><td>{value.get("private_bytes_peak_sampled", 0)/1e9:.2f}GB</td>'
                                       f'<td>{"정상" if row["shutdown_ok"] else "실패 · 중지 로그 참조"}</td></tr>')
            table_rows.append(f'<tr><th>{label}<br><small>총 {metadata["arguments"]["clients"]}명</small></th>'
                              f'<td>{workers}</td><td>{passed}/{len(selected)}' + (' · 생성기 한계' if limits else '') + '</td>'
                              f'<td>{cell("packet_job_wait_p99_ms", 3)}</td><td>{cell("cpu_percent")}</td>'
                              f'<td>{cell("packets_per_s")}</td><td>{cell("lobby_movement_gap_p99_ms", 3)}</td>'
                              f'<td>{max((row["servers"]["LobbyServer"]["send_queue_peak"] for row in measured), default=0):,}</td></tr>')
            if measured:
                detail_rows.append(f'<tr><th>{label} / w{workers}</th><td>{min(values(measured, workers, "cpu_percent")):.2f}–{max(values(measured, workers, "cpu_percent")):.2f}%</td>'
                                   f'<td>{min(lag):.1f}–{max(lag):.1f}ms</td><td>{min(observer)/100:.2f}–{max(observer)/100:.2f}코어</td>'
                                   f'<td>{min(row["servers"]["LobbyServer"]["send_queue_end"] for row in measured):,}–{max(row["servers"]["LobbyServer"]["send_queue_end"] for row in measured):,}</td>'
                                   f'<td>{max(row["dispatcher"]["max_parallel"] for row in measured)} / {sum(row["dispatcher"]["owner_violations"] for row in measured)}</td></tr>')
            analysis.append({'case': label, 'directory': directory, 'workers': workers, 'attempts': len(selected), 'passed': passed,
                             'complete_comparison': len(selected) == 3 and passed == 3,
                             'generator_limits': limits, 'medians': stats | {'packets_per_s': median(measured, workers, 'packets_per_s')} if measured else {},
                             'cpu_range': [min(values(measured, workers, 'cpu_percent')), max(values(measured, workers, 'cpu_percent'))] if measured else None,
                             'generator_lag_p99_ms_range': [min(lag), max(lag)] if lag else None,
                             'observer_core_percent_range': [min(observer), max(observer)] if observer else None,
                             'channel_population_ok': all(row.get('channel_population_ok') for row in selected),
                             'shutdown_ok': all(row.get('shutdown_ok') for row in selected), 'errors': errors,
                             'queue_peak': max((row['servers']['LobbyServer']['queued_peak'] for row in measured), default=None),
                             'send_queue_peak': max((row['servers']['LobbyServer']['send_queue_peak'] for row in measured), default=None),
                             'private_bytes_peak_sampled': max((row['servers']['LobbyServer']['private_bytes_peak_sampled'] for row in measured), default=None),
                             'send_queue_end_range': [min(row['servers']['LobbyServer']['send_queue_end'] for row in measured), max(row['servers']['LobbyServer']['send_queue_end'] for row in measured)] if measured else None,
                             'send_frames_per_s_median': median(measured, workers, 'send_frames_per_s') if measured else None,
                             'max_parallel': max((row['dispatcher']['max_parallel'] for row in measured), default=None),
                             'owner_violations': sum(row['dispatcher']['owner_violations'] for row in measured)})
    for ax, (_, title, unit) in zip(axes, specs):
        ax.set_facecolor('#f7f9fc')
        ax.set_title(title, fontsize=13, loc='left', pad=13)
        ax.set_ylabel(unit, fontsize=10)
        ax.set_xticks(range(len(groups)), [label.replace(' × ', '\n× ') for label, _, _, _ in groups], fontsize=10)
        ax.set_xlim(-.6, len(groups) - .4)
        ax.set_ylim(bottom=0)
        ax.margins(y=.15)
        ax.spines[['top', 'right']].set_visible(False)
        ax.grid(axis='y', color='#dfe6ee')
        ax.set_axisbelow(True)
    axes[0].legend(loc='upper left', fontsize=9)
    image = io.BytesIO()
    fig.savefig(image, format='png', dpi=155)
    plt.close(fig)
    png = image.getvalue()
    (LOGS / 'lobby-channel-density-report-20261018.png').write_bytes(png)
    (LOGS / 'lobby-channel-density-analysis-20261018.json').write_text(json.dumps(analysis, ensure_ascii=False, indent=2), encoding='utf-8')
    completed = sum(len(rows) for _, _, rows, _ in groups)
    passed = sum(bool(row['ok']) for _, _, rows, _ in groups for row in rows)
    comparison = next((rows for label, _, rows, _ in groups if label == '4채널 × 32명'), [])
    overview = ''
    if all(any(row['workers'] == count and row.get('servers', {}).get('LobbyServer') for row in comparison) for count in (1, 4)):
        overview = (f'4채널×32명에서 대기 p99 상한 {median(comparison, 1, "packet_job_wait_p99_ms"):.1f}→{median(comparison, 4, "packet_job_wait_p99_ms"):.1f}ms, '
                    f'CPU {median(comparison, 1, "cpu_percent"):.2f}→{median(comparison, 4, "cpu_percent"):.2f}%였다. ')
    peak = max(row['servers']['LobbyServer'].get('send_queue_peak', 0) for _, _, rows, _ in groups for row in rows if 'LobbyServer' in row.get('servers', {}))
    overview += f'밀집 실행의 송신 대기는 최고 {peak:,}건이었다. '
    failure_note = ''
    if failed_rows:
        failure_note = f'<p class="scope bad"><b>실제 실패:</b> 4채널×128명·worker4의 2번째 실행에서 120/512명 최종 정지 timeout. 송신 대기5,540,022건·private bytes 최고3.16GB, 로비 종료15초 제한 초과로 정상 종료에 실패했다. 해당 조건은3/6회만 수행하고 중단했다.</p>'
    links = ' · '.join(f'<a href="../../artifacts/logs/{directory}/summary.json">{label}</a>' for label, directory, _, _ in groups)
    measurement = f'''<h3 style="margin-top:26px">추가 측정 · 채널당 인원 증가</h3>
<p>동일한 개선 코드·새 Release 빌드에서 worker1/4를 번갈아 비교했다. 4채널의 정원을32→64→128명으로 늘리고, 단일 채널128명도 대조한다. 아래는 실제 확보한 실행만 표시한다.</p>
<figure><img src="data:image/png;base64,{base64.b64encode(png).decode('ascii')}" alt="채널당 인원 증가에 따른 worker1/4 대기·CPU·이동 갱신 간격 비교"><figcaption>막대: 각3회 완료 조건의 중앙값 · 점: 개별 실행 · 붉은×: 실패 실행. 512명은 중단된 불균등 표본이므로 점과 표의 범위로만 표시한다. 기존 4명/채널 결과와 합산하지 않는다.</figcaption></figure>
<table><thead><tr><th>실제 분포</th><th>worker</th><th>검증 성공/실행</th><th>대기 p99<br>ms 상한</th><th>CPU %</th><th>패킷/s</th><th>갱신 p99<br>ms 상한</th><th>송신 대기<br>표본 최고</th></tr></thead><tbody>{''.join(table_rows)}</tbody></table>
{failure_note}
<table><thead><tr><th>실패 실행</th><th>기능 실패</th><th>마지막 송신 대기</th><th>private bytes 최고</th><th>정상 종료</th></tr></thead><tbody>{''.join(failed_rows)}</tbody></table>
<p class="scope">기능 검사는 각 더미의 이동 수신·위치 변화·최종 정지를 확인한다. 모든 방송 프레임의 지연이나 송신 큐 해소를 보장하지 않는다. 송신 적체가 큰 실행을 안정적인 수용 인원으로 해석하지 않는다.</p>
<p class="muted">250ms 입력·5초 워밍업/20초 측정·계측ON·headless·DB없음. 각3회는 밀집 조건 탐색을 위한 기존 반복 수 유지이며 일반 목표5회의 예외다. Ryzen5 7500F·Windows11·균형 전원, CPU는 전체12논리 코어 기준이다. 최대 처리량/최대 동접이나 원본 main과의 성능 비교는 아니다. 추가 Release 서버 빌드·도구 Python50검사를 통과했다.</p>
<details><summary>밀집 조건의 원시 근거·실패·생성기/분포 검증</summary><p>{links}</p>
<table><thead><tr><th>조건</th><th>CPU 범위</th><th>발행 p99 범위</th><th>생성기 CPU 범위</th><th>마지막 표본 송신 대기 범위</th><th>최대 병렬/위반</th></tr></thead><tbody>{''.join(detail_rows)}</tbody></table>
<p>더미의 SC_CHANGE_CHANNEL 응답을 저장해 실제 채널 인원을 확인한다. 발행 p99&gt;250ms·생성기CPU≥0.9코어·기능 실패면 증원을 중단한다. 모든 준비/이동·정지 검사의 통과와 양쪽 서버 정상 종료를 별도로 확인한다. 원본 samples·실행 파일/도구/시나리오 hash·환경·working.diff는 각 묶음에 보존한다.</p></details>'''
    return {'result': failure_note + f'<p class="scope"><b>추가 밀집 측정:</b> {overview}검증 성공 {passed}/{completed}회. <a href="#density">밀집 비교와 생성기 한계</a>를 함께 확인해야 한다.</p>',
            'measurement': '<div id="density">' + measurement + '</div>', 'sources': sources,
            'lead': '밀집 채널에서는 worker4의 대기 개선이 확인되지 않았고 CPU가 증가했다. 512명에서는 정지·종료 실패를 관측했다.' if failed_rows else '채널당 인원을 늘린 실제 측정과 기존4명/채널 결과를 구분한다.',
            'cause': '<div class="panel" style="margin-top:16px"><h3>밀집 조건에서는 전송 적체가 추가로 관측됨</h3><p>입력 처리율이 목표에 근접해도 송신 대기는 누적됐다. 같은 채널은 직렬이므로 worker 증가가 채널 내부 연산을 분할하지 않는다. CPU 증가·전송 적체의 세부 원인 분해는 아직 미수행이다.</p><pre>// 관측 경계 · 개념 흐름\n이동 입력 → 채널 로직 → 방송 프레임 대기 → 비동기 전송 → 더미 수신\n처리 패킷/s           send_queue         실제 수신·정지 판정</pre></div>',
            'followup': f'<tr><th>밀집 채널의 송신 적체</th><td>입력 처리와 실제 수신이 벌어질 수 있음</td><td>표본 최고 {peak:,}건 · 기능 통과와 별도</td><td>전체 송신 drain 판정·대기/bytes 상한·중간 이동 상태 병합 검토</td></tr>' +
                        ('<tr><th>512명 정지·종료 실패</th><td>정지 관찰 지연·큰 메모리·정리 지연</td><td>120명 timeout, 로비 종료15초 초과. stop 잔여0 로그만으로 정상 종료가 아님</td><td>송신 적체 상한·대량 취소/반환 비용 분해·프로세스 종료까지 확인</td></tr>' if failed_rows else '')}


def main():
    before = read('lobby-channel-before-fixes-20261018.json')['cases']
    after = read('lobby-channel-after-fixes-20261018.json')['cases']
    assert before[0]['observer_remove_ids'] == [-1] and after[0]['observer_remove_ids'] == [1]
    assert before[1]['old_channel_observer_remove_ids'] == [] and before[1]['moving_client_remove_ids'] == [1]
    assert after[1]['old_channel_observer_remove_ids'] == [1] and after[1]['moving_client_remove_ids'] == []
    repeat = checked_runs('repeat', 6)
    final = checked_runs('final', 2)
    off = checked_runs('off', 2)
    for build in ('release', 'debug'):
        result = read(f'lobby-channels-{build}-20261018.json')
        assert result['passed'] and len(result['cases']) == 9
        assert read(f'lobby-channels-movement-{build}-20261018.json')['passed']
    assert read('lobby-admission-release-20261018.json')['passed']
    assert read('lobby-channel-implementation-verification-20261018.json')['core_checks_each'] == 1686
    graph = chart(repeat)
    density = density_report()
    p99a, p99b = [median(final, workers, 'packet_job_wait_p99_ms') for workers in (1, 4)]
    cpua, cpub = [median(final, workers, 'cpu_percent') for workers in (1, 4)]
    metrics = [('CPU (%)', 'cpu_percent', 2), ('처리 패킷/s', 'packets_per_s', 2),
               ('패킷 대기 p99 상한 (ms)', 'packet_job_wait_p99_ms', 3),
               ('타이머 dispatch 대기 p99 상한 (ms)', 'tick_dispatch_wait_p99_ms', 3),
               ('이동 갱신 간격 p99 상한 (ms)', 'lobby_movement_gap_p99_ms', 3)]
    repeated_table = ''.join('<tr><th>' + label + '</th>' + ''.join(
        f'<td>{median(repeat, workers, metric):,.{digits}f}</td>' for workers in (1, 4)) + '</tr>'
        for label, metric, digits in metrics)
    final_table = ''.join('<tr><th>' + label + '</th>' + ''.join(
        f'<td>{median(final, workers, metric):,.{digits}f}</td>' for workers in (1, 4)) + '</tr>'
        for label, metric, digits in metrics[:3])
    ranges = {workers: (min(values(repeat, workers, 'cpu_percent')), max(values(repeat, workers, 'cpu_percent'))) for workers in (1, 4)}
    final_hashes = read(GROUPS['final'] + '/metadata.json')['hashes']
    exe_hash = next(value for name, value in final_hashes.items() if name.replace('\\', '/').endswith('LobbyServer/Lobby_Server.exe'))
    source_files = [f"{directory}/{filename}" for directory in GROUPS.values() for filename in ('summary.json', 'metadata.json')]
    source_files += ['lobby-channel-before-fixes-20261018.json', 'lobby-channel-after-fixes-20261018.json', 'lobby-channel-implementation-verification-20261018.json']
    source_files += density['sources']
    manifest = {name: hashlib.sha256((LOGS / name).read_bytes()).hexdigest() for name in source_files}
    (LOGS / 'lobby-channel-report-analysis-20261018.json').write_text(json.dumps({
        'scope': 'original-main vs fixed wire defects; performance is worker1/4 within improved implementation, not original-main baseline',
        'repeat_medians': {workers: {metric: median(repeat, workers, metric) for _, metric, _ in metrics} for workers in (1, 4)},
        'final_control': {workers: {metric: median(final, workers, metric) for _, metric, _ in metrics[:3]} for workers in (1, 4)},
        'metrics_off_cpu': {workers: median(off, workers, 'cpu_percent') for workers in (1, 4)}, 'source_sha256': manifest,
        'final_lobby_exe_sha256': exe_hash, 'density_measurements_included': bool(density['sources']), 'generator_starts_servers': False}, ensure_ascii=False, indent=2), encoding='utf-8')
    report = f'''<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>로비 채널 병렬화 · 수정 전후 보고서</title>
<style>
:root{{--ink:#172d43;--muted:#536a80;--line:#dbe4ed;--blue:#126eaa;--amber:#8b4d00}}
*{{box-sizing:border-box}}body{{margin:0;background:#eef3f8;color:var(--ink);font:16px/1.65 'Segoe UI','Malgun Gothic',sans-serif}}
main{{max-width:1160px;margin:auto;padding:38px 24px 52px}}header .eyebrow{{font-size:13px;color:var(--muted);letter-spacing:.07em}}h1{{font-size:34px;line-height:1.25;margin:9px 0 20px;letter-spacing:-1px}}
section{{background:#fff;border:1px solid var(--line);border-radius:17px;padding:26px;margin:20px 0}}h2{{font-size:23px;margin:0 0 12px}}h3{{font-size:17px;margin:0 0 10px}}p{{margin:9px 0}}a{{color:var(--blue);text-underline-offset:3px}}code{{font-family:Consolas,monospace;font-size:.9em}}
.lead{{font-size:19px;font-weight:600}}.cards{{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:13px;margin:20px 0}}.card{{background:#eef5fc;border:1px solid #d8e7f7;border-radius:12px;padding:17px}}.card.warn{{background:#fff4e4;border-color:#efd4aa}}.card .value{{display:block;font-size:27px;font-weight:750;line-height:1.4;margin:5px 0}}.card .label{{font-size:14px;font-weight:600}}.card small,.muted,small{{color:var(--muted)}}.card.warn .value{{color:var(--amber)}}
.scope{{border-left:4px solid #8a9eb3;background:#f4f7fa;padding:12px 15px;font-size:14px}}.split{{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:20px}}.panel{{background:#f7f9fc;padding:18px;border:1px solid var(--line);border-radius:11px;min-width:0}}
pre{{margin:12px 0 0;background:#172d43;color:#e9f1fa;border-radius:9px;padding:15px;white-space:pre-wrap;overflow-wrap:anywhere;font:14px/1.65 Consolas,'Malgun Gothic',monospace}}
table{{width:100%;border-collapse:collapse;font-size:14px;margin:13px 0}}th,td{{padding:11px 10px;border-bottom:1px solid var(--line);text-align:left;vertical-align:top;overflow-wrap:anywhere}}thead{{background:#f1f5f9}}tbody th{{font-weight:550}}.bad{{color:#a34023}}.good{{color:#116847}}.chips{{display:flex;flex-wrap:wrap;gap:7px;margin:13px 0}}.chip{{background:#edf3fa;border:1px solid #dce6f0;border-radius:20px;padding:4px 11px;font-size:13px}}figure{{margin:18px 0}}img{{display:block;width:100%;height:auto;border-radius:9px}}figcaption{{font-size:13px;color:var(--muted);margin-top:8px}}
details{{border-top:1px solid var(--line);margin-top:17px;padding-top:13px}}summary{{cursor:pointer;font-weight:600}}.checks{{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px;margin-top:17px}}.check{{background:#f0f6f5;border-radius:9px;padding:13px;font-size:14px}}.check b{{display:block;font-size:19px}}footer{{font-size:13px;color:var(--muted);overflow-wrap:anywhere}}.nowrap{{white-space:nowrap}}
@media(max-width:760px){{main{{padding:23px 12px}}section{{padding:18px 14px}}h1{{font-size:27px}}.cards,.split{{grid-template-columns:1fr}}.checks{{grid-template-columns:repeat(2,minmax(0,1fr))}}th,td{{padding:8px 6px;font-size:12px}}.card .value{{font-size:25px}}.lead{{font-size:17px}}}}
@media print{{body{{background:white}}main{{max-width:none;padding:0}}section{{break-inside:avoid;border-radius:0}}details{{display:block}}summary{{list-style:none}}}}
</style></head><body><main>
<header><div class="eyebrow">2026-10-18 · codex/lobby-channel-parallelism</div><h1>로비 채널 병렬화 · 수정 전후</h1></header>
<section id="result"><h2>1. 결과</h2><p class="lead">{density['lead'] or '기존 제거 패킷 결함을 해결하고 채널 간 병렬 실행을 적용했다. 4명/채널 대조에서는 대기가 줄었다.'}</p>{density['result']}
<div class="cards"><div class="card"><span class="label">실제 재현한 기존 결함</span><span class="value">2건 → 해결</span><small>종료 ID · 채널 이동 수신자</small></div>
<div class="card"><span class="label">기존 4명/채널 · 패킷 대기 p99 상한</span><span class="value">{p99a:.1f} → {p99b:.1f}ms</span><small>개선 코드 worker 1개 → 4개</small></div>
<div class="card warn"><span class="label">기존 4명/채널 · 프로세스 CPU</span><span class="value">{cpua:.2f} → {cpub:.2f}%</span><small>+{cpub-cpua:.2f}%p · 최종 각1회 대조</small></div></div>
<p class="scope"><b>비교 범위:</b> 기능은 원본 <code>main@54af931c</code> 대비 수정 전후다. 성능은 <b>개선 코드 안에서 worker 수만 바꾼 대조</b>이며, 원본 main의 같은 32채널 조건 성능은 미측정이다.</p></section>

<section id="cause"><h2>2. 분석한 원인</h2><div class="split"><article class="panel"><h3>모든 채널이 로직 소비자 하나를 공유</h3><p>채널이 나뉘어도 로직 실행은 한 줄이었다. IOCP 4개와 로직 worker 1개는 서로 다른 역할이다.</p>
<pre>// 수정 전 · CServer::Run 발췌
producers.Launch([] {{
    GPacketJobQueue-&gt;ProcessJob();
}});</pre></article>
<article class="panel"><h3>정리 순서와 제거 수신자가 잘못됨</h3><p>종료 유저의 ID를 초기화한 뒤 상대 시야에서 재조회했다. 채널 이동은 관찰자의 시야를 지우면서 본인에게 패킷을 보냈다.</p>
<pre>// 수정 전 경로 · 개념 코드
client.Reset();                 // ID = -1
observer.Remove(client.ID());   // REMOVE(-1)
observer.Delete(..., mover.Sender()); // 수신자 오류</pre></article></div>
<p class="muted">유저별 큐의 잠금은 이웃 유저의 좌표·시야까지 보호하지 않는다. 소비자 수를 늘리기 전에 채널 소유권을 정리해야 했다.</p>{density['cause']}</section>

<section id="solution"><h2>3. 해결 방식</h2><p>같은 채널은 직렬 큐로 묶고, 서로 다른 채널을 공통 worker pool에서 처리한다. 유저 입력함은 선두 한 작업만 발행해 이동 전후 순서를 유지한다.</p>
<div class="split"><article class="panel"><h3>채널 이동 후 다음 작업을 새 소유 큐로</h3><pre>// 현재 실행 경계 · 개념 코드
SessionWorkQueue에 입력 순서대로 보관
선두 1작업만 채널/전역 큐에 발행
작업 완료 → 현재 소유 채널 확인
다음 입력을 그 채널에 발행</pre></article>
<article class="panel"><h3>초기화 전에 이전 ID로 제거 통지</h3><pre>// 현재 정리 순서 · 개념 코드
이전 ID와 관찰자 확인
관찰자의 sender로 제거 통지
출발 시야·슬롯 정리 → 상태 초기화</pre></article></div>
<table><thead><tr><th>경계</th><th>수정 전</th><th>수정 후</th></tr></thead><tbody>
<tr><th>로직 실행</th><td>전역 소비자 1개</td><td>채널별 직렬 큐 + 공통 worker 기본 4개</td></tr>
<tr><th>채널 이동·전역 매칭</th><td>단일 worker에 의존</td><td>배타 실행 구간에서 원자적으로 처리</td></tr>
<tr><th>재접속·타이머</th><td>연결 generation 보호</td><td>generation 유지 + SessionWorkQueue를 통한 소유 큐 전달</td></tr>
</tbody></table><p><a href="../diagrams/lobby-channel-parallelism/implementation.html">실제 구현 구조도</a> · <a href="LOBBY_CHANNEL_PARALLELISM.md">설정·계측·코드 근거</a></p></section>

<section id="measurement"><h2>4. 실제 측정</h2><div class="chips"><span class="chip">Release LOCAL_TEST · DB 없음</span><span class="chip">128명 = 32채널 × 4명</span><span class="chip">250ms 입력</span><span class="chip">워밍업 5초 · 측정 20초</span><span class="chip">worker 1/4 각3회</span></div>
<figure><img src="data:image/png;base64,{graph}" alt="개선 코드 worker1개와 4개의 각3회 대조: 패킷 대기 p99 상한 감소, CPU 증가, 고정 입력 처리율 비교"><figcaption>막대는 각3회 중앙값, 점은 개별 실행 값이다. 모든 막대 축은 0에서 시작한다. p99는 실제 RTT가 아닌 작업 대기 histogram 버킷 상한이다.</figcaption></figure>
<table><thead><tr><th>각3회 중앙값</th><th>worker 1개</th><th>worker 4개</th></tr></thead><tbody>{repeated_table}</tbody></table>
<p class="scope">입력이 약512패킷/s로 고정돼 있어 최대 처리량 증가는 입증하지 않았다. 이동 갱신 간격 p99 상한도 동일하다. CPU 증가의 비용 분해는 후속이며, worker 깨움·공용 송신 잠금은 아직 가설이다.</p>
<details><summary>최종 실행 파일 대조 · 계측 OFF · 반복 범위</summary>
<p>3회 반복 뒤 채널 이동의 legacy dummy 정책을 유지하는 1행을 수정했다. 이동/큐 알고리즘은 유지했고, 그 최종 실행 파일로 각1회 대조를 추가했다. 서로 다른 실행 파일의 결과는 합산하지 않는다.</p>
<table><thead><tr><th>최종 각1회</th><th>worker 1개</th><th>worker 4개</th></tr></thead><tbody>{final_table}</tbody></table>
<p>3회 CPU 범위: worker1 {ranges[1][0]:.2f}–{ranges[1][1]:.2f}%, worker4 {ranges[4][0]:.2f}–{ranges[4][1]:.2f}%. 계측 OFF 각1회도 {median(off,1,'cpu_percent'):.2f}→{median(off,4,'cpu_percent'):.2f}%였다. OFF 반복이 적어 정확한 계측 오버헤드는 확정하지 않는다.</p>
<p class="muted">위 기존4명/채널 측정은 공백 정리 전 실행 파일이다. 이후 분기 빈 줄·검사 파일 포맷을 정리하고 공백/토큰 동일성을 확인했다. 아래 밀집 추가 측정은 그 현재 소스를 새로 빌드한 별도 실행 파일이다.</p></details>
<h3 style="margin-top:22px">기존 결함의 실제 수정 전후 · 단일 worker에서 먼저 검증</h3>
<table><thead><tr><th>시나리오</th><th>수정 전 수신</th><th>수정 후 수신</th></tr></thead><tbody>
<tr><th>ID 1 유저 종료</th><td class="bad">관찰자: REMOVE(-1)</td><td class="good">관찰자: REMOVE(1)</td></tr>
<tr><th>ID 1 유저 채널 이동</th><td class="bad">관찰자: 없음<br>이동자 본인: REMOVE(1)</td><td class="good">관찰자: REMOVE(1)<br>이동자 본인: 없음</td></tr></tbody></table>
<div class="checks"><div class="check">Core · Release/Debug<b>각 1,686 통과</b></div><div class="check">채널 소켓 회귀<b>각 9항목 통과</b></div><div class="check">기존 Python 검사<b>49개 통과</b></div><div class="check">성능/정상 종료<b>10 / 10 통과</b></div></div>
<p class="muted">ABI 110종/106상수 유지. 기존 네트워크12종·보스4/5 전환·초기화 실패4종, 이동/정지·재접속10회·입장 실패/슬롯 반환도 확인했다. 계측 ON 8회에서 소유권 위반0건, 최대 동시 채널 callback은1→4였다. 기존10회 양쪽 종료 pending/sockets/leased=0이다.</p>{density['measurement']}</section>

<section id="followup"><h2>5. 추가 발견·대응 필요</h2><table><thead><tr><th>항목</th><th>영향</th><th>상태·근거</th><th>다음 대응</th></tr></thead><tbody>
{density['followup']}
<tr><th>CPU 증가</th><td>처리 여유 비용 증가</td><td>반복/최종/OFF에서 관측</td><td>worker2 대조·송신 잠금/깨움 비용 분해</td></tr>
<tr><th>전역 배타 구간</th><td>긴 DB/I/O가 다른 채널도 지연</td><td>현재 보호 설계 · 운영 부하 미검증</td><td>전역 기능별 서비스 메시지로 분리</td></tr>
<tr><th>단일 밀집 채널</th><td>해당 채널 내부는 직렬</td><td>현재 실행 계약</td><td>시야·전송·공간 occupancy 개선</td></tr>
<tr><th>미측정 범위</th><td>최대 동접·운영 안정성 미확정</td><td>운영 DB/블록체인·전체 GUI·동적 race 미검증</td><td>집중/장시간 부하·입력 상한·전용 검증</td></tr>
</tbody></table><p class="muted">기존 형변환·미사용 변수 경고는 남아 있다. 빌드 성공과 위 회귀를 전체 플레이 검증으로 확대하지 않는다.</p></section>
<footer><p><a href="LOBBY_CHANNEL_PARALLELISM.md">상세 근거·재현 명령</a> · <a href="LOBBY_CHANNEL_CONCURRENCY.md">수정 전 공유 데이터 검토</a> · <a href="../../scripts/Build-LobbyChannelReport.py">보고서 재생성 스크립트</a></p>
<details><summary>원시 근거와 검증 범위</summary><p>반복 <a href="../../artifacts/logs/{GROUPS['repeat']}/summary.json">ON6회</a> · <a href="../../artifacts/logs/{GROUPS['off']}/summary.json">OFF2회</a> · <a href="../../artifacts/logs/{GROUPS['final']}/summary.json">최종2회</a> · <a href="../../artifacts/logs/lobby-channel-before-fixes-20261018.json">결함 수정 전</a> · <a href="../../artifacts/logs/lobby-channel-after-fixes-20261018.json">결함 수정 후</a>. 원시 로그는 Git 제외 로컬 파일이다.</p>
<p>기존4명/채널 최종 로비 EXE SHA-256: <code>{html.escape(exe_hash)}</code>. 밀집 추가 측정 실행 파일은 각 묶음 metadata의 hash로 구분한다.</p><p>기존 자료와 별도 밀집 재측정을 구분했다. 추가 작업은 Release 서버 빌드·부하 실행·관측 도구 보완이며 서버 로직 변경·커밋/push는 수행하지 않았다. PNG 실제 이미지와 HTML 구조/수치/링크를 확인했다. 전체 브라우저 레이아웃은 기존 로컬 파일 접근 제한 때문에 미검증이다.</p></details></footer>
</main></body></html>'''
    OUTPUT.write_text(report, encoding='utf-8')
    print(str(OUTPUT))


if __name__ == '__main__':
    main()
