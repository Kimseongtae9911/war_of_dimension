"""2026-10-13 로비 단일 채널 전후 측정 보고서 재생성."""
import base64
import io
import json
from pathlib import Path
import statistics

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
LOGS = ROOT / 'artifacts/logs'
COUNTS = (64, 96, 112, 128)


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def runs(kind, variant):
    directory = LOGS / f'lobby-throughput-{kind}-{variant}-20261013'
    summary = read(directory / 'summary.json')
    if not summary['fingerprints_unchanged']:
        raise ValueError(f'실행 중 fingerprint 변경: {directory}')
    records = summary['runs']
    if any(not row['shutdown_ok'] or row.get('connected_end') != 0 for row in records):
        raise ValueError(f'종료 검증 실패: {directory}')
    return directory, records


def profile(variant):
    directory, records = runs('profile', variant)
    if len(records) != 1:
        raise ValueError('프로파일 실행 수 불일치')
    row = records[0]
    raw = read(directory / f"01-perf-lobby-128/lab/runs/{row['run_id']}.json")
    phases = {value['phase']: value['timestamp_ms'] for value in raw['phases']}
    rows = [next(server for server in sample['servers'] if server['role'] == 'LobbyServer')
            for sample in raw['samples'] if phases['measure'] <= sample['timestamp_ms'] <= phases['verify']]
    rows = list({server['sequence']: server for server in rows if phases['measure'] <= server['timestamp_ms'] <= phases['verify']}.values())
    if len(rows) < 3 or len({server['instance'] for server in rows}) != 1 or any(server.get('status') != 'live' for server in rows):
        raise ValueError('로비 프로파일 표본 부족/재시작')
    left, right = rows[0]['tick_profile']['stages'], rows[-1]['tick_profile']['stages']
    delta = {name: {key: right[name][key] - left[name][key] for key in left[name]} for name in right}
    if any(value < 0 for stage in delta.values() for value in stage.values()):
        raise ValueError('누적 프로파일 감소')
    total = delta['total']
    if not total['calls'] or total['cycle_errors']:
        raise ValueError('유효 cycle 표본 부족')
    for stage in delta.values():
        stage['cycle_share_percent'] = stage['cycles'] / total['cycles'] * 100
        stage['wall_ms_per_move'] = stage['wall_ns'] / total['calls'] / 1e6
    return {'ok': row['ok'], 'seconds': (rows[-1]['uptime_ms'] - rows[0]['uptime_ms']) / 1000, 'stages': delta}


def figure():
    buffer = io.BytesIO()
    plt.savefig(buffer, format='png', dpi=150, bbox_inches='tight', facecolor='white')
    plt.close()
    return 'data:image/png;base64,' + base64.b64encode(buffer.getvalue()).decode()


def fmt(value, digits=2):
    return '미수집' if value is None else f'{value:,.{digits}f}'


def main():
    plt.rcParams.update({'font.family': 'Malgun Gothic', 'axes.unicode_minus': False, 'font.size': 10})
    checks = read(LOGS / 'lobby-throughput-checks-20261013.json')
    if not checks['core']['ok'] or not checks['terrain']['ok'] or not checks['integration']['Ok'] or not checks['reconnect']['ok']:
        raise ValueError('회귀 검사 실패')
    verification = f"Release 빌드, Core {checks['core']['checks']:,}건·ABI 110종/106상수, 실제 OBJ {checks['terrain']['checks']:,}건, 통합/실패 경로 {len(checks['integration']['Cases'])}그룹, Python {checks['python']['passed']}건, 로비 재접속 {checks['reconnect']['assertions']['passed']} assertion·지형 API 확인 통과."
    soak, short, profiles, metadata = {}, {}, {}, {}
    for variant in ('before', 'after'):
        directory, values = runs('soak', variant)
        soak[variant] = {row['clients']: row for row in values}
        if set(soak[variant]) != set(COUNTS):
            raise ValueError('유지 검사 인원 불일치')
        metadata[variant] = read(directory / 'metadata.json')
        metadata[variant]['fingerprints'] = {key.replace('\\', '/'): value for key, value in metadata[variant]['fingerprints'].items()}
        _, short[variant] = runs('short', variant)
        if len(short[variant]) != 5 or {row['seed'] for row in short[variant]} != set(range(1, 6)):
            raise ValueError('128명 반복 검사 seed/수 불일치')
        profiles[variant] = profile(variant)
    if metadata['before']['performance'] != metadata['after']['performance']:
        raise ValueError('전후 시나리오 조건 불일치')
    for key, value in metadata['before']['fingerprints'].items():
        if ('Resource/' in key or 'scenarios/' in key) and metadata['after']['fingerprints'].get(key) != value:
            raise ValueError(f'콘텐츠/시나리오 변경: {key}')
    x = list(range(len(COUNTS)))
    colors = ('#c86b4c', '#247a9c')
    fig, axes = plt.subplots(1, 2, figsize=(12, 3.5))
    for variant, offset, color, label in zip(('before', 'after'), (-.18, .18), colors, ('개선 전', '개선 후')):
        values = [soak[variant][count]['servers']['LobbyServer'] for count in COUNTS]
        for axis, data in zip(axes, ([value['packets_per_s'] for value in values], [value['pending_packets_end'] for value in values])):
            bars = axis.bar([v + offset for v in x], data, .36, label=label, color=color)
            axis.bar_label(bars, fmt='%.0f', fontsize=8, padding=3)
    axes[0].plot(x, [count * 4 for count in COUNTS], 'k--', label='입력 목표 4회/초/명')
    for axis, title, unit in zip(axes, ('입력 처리량', '측정 종료 시 미실행 패킷'), ('패킷/초', '개')):
        axis.set(title=title, ylabel=unit, xticks=x, xticklabels=[f'{count}명' for count in COUNTS], ylim=(0, None))
        axis.legend(fontsize=8)
        axis.grid(axis='y', alpha=.18)
        axis.set_axisbelow(True)
    plt.tight_layout()
    throughput_image = figure()
    fig, axes = plt.subplots(1, 2, figsize=(12, 3.5))
    for variant, marker, color, label in zip(('before', 'after'), ('o', 's'), colors, ('개선 전', '개선 후')):
        values = [soak[variant][count]['servers']['LobbyServer'] for count in COUNTS]
        axes[0].plot(x, [value['packet_job_wait_us']['p99_us_upper'] / 1000 for value in values], marker=marker, color=color, label=label)
        axes[1].plot(x, [value['cpu_percent'] for value in values], marker=marker, color=color, label=label)
    for axis, title, unit in zip(axes, ('패킷 작업 대기 p99 bucket 상한', '로비 프로세스 CPU'), ('ms · 낮을수록 좋음', '% · 12논리 코어 정규화')):
        axis.set(title=title, ylabel=unit, xticks=x, xticklabels=[f'{count}명' for count in COUNTS], ylim=(0, None))
        axis.legend(fontsize=8)
        axis.grid(alpha=.18)
    plt.tight_layout()
    latency_image = figure()
    stages = ('lobby_height', 'lobby_view')
    fig, axis = plt.subplots(figsize=(11, 2.9))
    for variant, offset, color, label, hatch in zip(('before', 'after'), (-.18, .18), colors, ('개선 전', '개선 후'), ('//', '')):
        values = [profiles[variant]['stages'][stage]['wall_ms_per_move'] for stage in stages]
        bars = axis.barh([index + offset for index in range(len(stages))], values, .34, color=color, label=label, hatch=hatch)
        axis.bar_label(bars, fmt='%.3f ms', padding=5, fontsize=10)
    axis.set(yticks=range(len(stages)), yticklabels=('HeightMesh 검사', '시야 탐색·주변 전송'), xlabel='경과 시간 / 실제 실행된 Move 1회 (ms)',
             xlim=(0, max(profiles[v]['stages'][stage]['wall_ms_per_move'] for v in profiles for stage in stages) * 1.2))
    axis.invert_yaxis()
    axis.grid(axis='x', alpha=.18)
    axis.set_axisbelow(True)
    axis.legend(loc='lower right')
    plt.tight_layout()
    profile_image = figure()
    table, detail = [], []
    for count in COUNTS:
        cells = []
        for variant in ('before', 'after'):
            row = soak[variant][count]
            server, drain = row['servers']['LobbyServer'], row['settlement']
            cells.append(f"{fmt(server['packets_per_s'])} / {server['pending_packets_end']:,} / {'통과' if row['ok'] else '실패'}")
            detail.append({'variant': variant, 'clients': count, 'target_packets_per_s': count * 4, 'ok': row['ok'], 'settlement': drain,
                           'server': server, 'generator_lag_ms': row['generator_lag_ms'], 'observer_cpu_core_percent': row.get('observer_cpu_core_percent'), 'peer_errors': row.get('peer_errors')})
        table.append(f'<tr><td>{count}명 · 목표 {count*4}/s</td><td>{cells[0]}</td><td>{cells[1]}</td></tr>')
    metrics_rows = ''.join(f"<tr><td>{count}명</td><td>{fmt(soak['before'][count]['servers']['LobbyServer']['packet_job_wait_us']['p99_us_upper']/1000)} → {fmt(soak['after'][count]['servers']['LobbyServer']['packet_job_wait_us']['p99_us_upper']/1000)}ms</td><td>{fmt(soak['before'][count]['servers']['LobbyServer']['cpu_percent'])} → {fmt(soak['after'][count]['servers']['LobbyServer']['cpu_percent'])}%</td></tr>" for count in COUNTS)
    repeated = []
    for variant, label in (('before', '개선 전'), ('after', '개선 후')):
        rows = short[variant]
        cells = []
        for key in ('packets_per_s', 'cpu_percent'):
            values = [row['servers']['LobbyServer'][key] for row in rows]
            cells.append(f'{fmt(statistics.median(values))} ({fmt(min(values))}~{fmt(max(values))})')
        waits = [row['servers']['LobbyServer']['packet_job_wait_us']['p99_us_upper'] / 1000 for row in rows]
        cells.append(f'{fmt(statistics.median(waits))} ({fmt(min(waits))}~{fmt(max(waits))})')
        repeated.append(f"<tr><td>{label}</td><td>{sum(row['ok'] for row in rows)}/5</td>" + ''.join(f'<td>{cell}</td>' for cell in cells) + '</tr>')
    b128, a128 = (soak[v][128]['servers']['LobbyServer'] for v in ('before', 'after'))
    bheight = profiles['before']['stages']['lobby_height']
    profile_rows = ''.join(f"<tr><td>{label}</td><td>{fmt(profiles['before']['stages'][name]['cycle_share_percent'])}% / {fmt(profiles['before']['stages'][name]['wall_ms_per_move'], 3)}ms</td>"
                           f"<td>{fmt(profiles['after']['stages'][name]['cycle_share_percent'])}% / {fmt(profiles['after']['stages'][name]['wall_ms_per_move'], 3)}ms</td></tr>"
                           for name, label in (('lobby_nav', 'NavMesh'), ('lobby_height', 'HeightMesh'), ('lobby_view', '시야 탐색·주변 전송')))
    outcome = f"128명 입력 처리량 {fmt(b128['packets_per_s'])} → {fmt(a128['packets_per_s'])}패킷/초. 로비 CPU는 {fmt(b128['cpu_percent'])}% → {fmt(a128['cpu_percent'])}%이며, 시야·전송 비용이 남습니다."
    headroom_rows = ''.join(f"<tr><td>{count}명</td><td>{fmt(soak['before'][count]['observer_cpu_core_percent'])}% → {fmt(soak['after'][count]['observer_cpu_core_percent'])}%</td>"
                            f"<td>{fmt(soak['before'][count]['generator_lag_ms']['p99'])} → {fmt(soak['after'][count]['generator_lag_ms']['p99'])}ms</td></tr>" for count in COUNTS)
    memory_before, memory_after = b128['private_bytes_peak_sampled'] / 1048576, a128['private_bytes_peak_sampled'] / 1048576
    gain = (a128['packets_per_s'] / b128['packets_per_s'] - 1) * 100
    cpu_increase = a128['cpu_percent'] - b128['cpu_percent']
    wait_before, wait_after = (server['packet_job_wait_us']['p99_us_upper'] / 1e6 for server in (b128, a128))
    view_share = profiles['after']['stages']['lobby_view']['cycle_share_percent']
    drain_before, drain_after = (soak[v][128]['settlement'] for v in ('before', 'after'))
    document = f'''<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>로비 단일 채널 처리량 개선 보고서</title>
<style>
*{{box-sizing:border-box}}body{{margin:0;background:#edf2f5;color:#172c3c;font:16px/1.65 "Malgun Gothic",sans-serif}}
main{{max-width:1180px;margin:auto;padding:32px 24px 56px}}h1{{font-size:32px;line-height:1.3;margin:6px 0 22px}}
h2{{font-size:23px;margin:0 0 12px}}h3{{font-size:18px;margin:22px 0 10px}}p{{margin:10px 0}}
section{{background:#fff;border:1px solid #d8e2e8;border-radius:16px;padding:24px;margin:20px 0}}
.kicker,.muted,figcaption{{color:#536979;font-size:14px}}.lead{{font-size:20px;font-weight:700}}
.stats{{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px;margin:18px 0}}
.card{{min-width:0;background:#eaf5f4;border:1px solid #cbe3df;border-radius:12px;padding:16px}}
.card strong{{display:block;color:#116b60;font-size:28px;line-height:1.4;margin:7px 0}}
.card small{{display:block;font-size:13px}}.cost{{background:#fff1e9;border-color:#f0d3c4}}.cost strong{{color:#9b4028}}
.notice{{padding:12px 16px;background:#fff7e7;border-left:4px solid #b57a19;border-radius:5px}}
.pair{{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}}.step{{padding:16px;background:#f1f6f9;border-radius:10px}}
pre{{margin:12px 0;background:#142e40;color:#edf7ff;border-radius:10px;padding:16px;white-space:pre-wrap;overflow-wrap:anywhere;font:14px/1.6 Consolas,monospace}}
figure{{margin:18px 0}}img{{display:block;width:100%;height:auto}}figcaption{{margin-top:8px}}
table{{width:100%;border-collapse:collapse;font-size:14px}}th,td{{border-bottom:1px solid #dce5eb;padding:10px;text-align:left;vertical-align:top}}
th{{background:#f0f5f8}}details{{margin:14px 0;border:1px solid #dce5eb;border-radius:10px;padding:12px 16px}}
summary{{cursor:pointer;font-weight:700}}a{{color:#12688b;text-underline-offset:3px}}code{{overflow-wrap:anywhere}}
@media(max-width:1000px){{.stats{{grid-template-columns:repeat(2,minmax(0,1fr))}}}}
@media(max-width:650px){{main{{padding:18px 10px}}h1{{font-size:27px}}section{{padding:16px}}.pair{{grid-template-columns:1fr}}.card{{padding:12px}}.card strong{{font-size:24px}}th,td{{padding:6px;font-size:12px}}}}
</style></head><body><main>
<div class="kicker">2026.10.13 · Lobby_Server · 단일 채널 / 로직 스레드 1개</div>
<h1>로비 단일 채널 처리량 개선 보고서</h1>
<section id="result"><h2>1. 결과</h2>
<p class="lead">지형 검색 비용을 줄여 128명 입력 부하를 처리했습니다. CPU와 응답 지연은 추가 개선이 필요합니다.</p>
<p class="muted">128명 · 120초 · 목표 512패킷/초 · 같은 이동·정지 입력 조건</p>
<div class="stats">
 <div class="card">처리량<strong>+{fmt(gain, 1)}%</strong><small>{fmt(b128['packets_per_s'])} → {fmt(a128['packets_per_s'])}패킷/초</small></div>
 <div class="card">측정 종료 미실행 패킷<strong>{a128['pending_packets_end']:,}개</strong><small>개선 전 {b128['pending_packets_end']:,}개</small></div>
 <div class="card">패킷 작업 대기 p99 상한<strong>{fmt(wait_after)}초</strong><small>개선 전 {fmt(wait_before)}초 · RTT 아님</small></div>
 <div class="card cost">서버 프로세스 CPU<strong>+{fmt(cpu_increase)}%p</strong><small>{fmt(b128['cpu_percent'])}% → {fmt(a128['cpu_percent'])}%</small></div>
</div>
<p><strong>이동·최종 입력 정리 통과:</strong> 64/96/112/128명 중 개선 전 {sum(r['ok'] for r in soak['before'].values())}/4 → 개선 후 {sum(r['ok'] for r in soak['after'].values())}/4.
128명은 정지 후 {fmt(drain_after['elapsed_ms']/1000)}초에 마지막 입력까지 처리했습니다.</p>
<p class="notice">낮은 응답 지연과 최대 동접은 미확정입니다. 개선 후 96명 이상에서 더미 수신 CPU가 코어 하나의 90%를 넘었습니다.</p>
</section>
<section id="cause"><h2>2. 분석한 원인</h2>
<p>로비 이동은 HeightMesh <strong>124,834면</strong>을 앞에서부터 검사했습니다. 이 검사가 개선 전 실제 Move cycle의 <strong>{fmt(bheight['cycle_share_percent'])}%</strong>를 사용해, 같은 로직 스레드의 새 패킷 처리가 밀렸습니다.</p>
<pre>// 개선 전 MapCollision의 핵심 호출
ray.RayCast(m_naviMesh, _height);
ray.RayCast(m_heightMesh, _height); // 원래 순서대로 전체 목록 탐색</pre>
<p class="muted">전체 지형 순회는 코드로, 비용 집중은 별도 128명 프로파일로 확인했습니다. 큐의 잠금·budget·FIFO 정책은 변경하지 않았습니다.</p>
</section>
<section id="solution"><h2>3. 해결 방식</h2>
<div class="pair"><div class="step"><strong>로드 시 한 번 준비</strong><br>4-unit XZ 셀 인덱스와 삼각형 정적 계산 캐시를 구성합니다. HeightMesh는 기존 높이 정렬 후 인덱스를 만듭니다.</div>
<div class="step"><strong>이동 시 후보만 검사</strong><br>해당 셀과 전역 후보를 원래 면 순서로 검사합니다. 첫 교차·높이 계산·원본 OBJ를 보존합니다.</div></div>
<pre>// 개선 후 MapCollision의 실제 호출
navigable = ray.RayCast(m_naviMesh, _height, &amp;m_naviIndex);
// navigable인 경우
ray.RayCast(m_heightMesh, _height, &amp;m_heightIndex);</pre>
<p class="muted">기존 TerrainSpatialIndex/Ray를 재사용했습니다. 무효 인덱스·비수직 ray·극단 높이는 전체 순회로 복귀합니다. <a href="../diagrams/lobby-throughput/lobby.html">검색 흐름도</a></p>
<figure><img alt="실행된 Move당 HeightMesh와 시야·전송 경과 시간 전후 비교" src="{profile_image}">
<figcaption>지형 검사 비용이 줄어든 뒤 시야 탐색·주변 전송이 주요 비용으로 남았습니다. 개선 후 Move cycle의 {fmt(view_share)}%입니다.</figcaption></figure>
<details><summary>프로파일 수치와 해석 범위</summary><table><tr><th>구간</th><th>개선 전 · cycle 비중 / ms/Move</th><th>개선 후 · cycle 비중 / ms/Move</th></tr>{profile_rows}</table>
<p class="muted">128명·20초 진단 전후 각 1회. 분모는 실제 실행된 Move이며 전체 프로세스 CPU가 아닙니다. wall은 대기를 포함합니다. 같은 입력 조건이지만 ray 질의 좌표까지 고정한 미세 벤치마크는 아닙니다.</p></details>
</section>
<section id="measurement"><h2>4. 실제 측정</h2>
<p class="muted">Release x64 LOCAL_TEST · DB 없음 · Ryzen5 7500F / 12논리 코어 · 5초 워밍업 · 250ms마다 이동·정지 교대 · 기본 계측 켬 / 상세 프로파일 끔 / headless</p>
<figure><img alt="64~128명 처리량 및 종료 대기 패킷 전후 비교" src="{throughput_image}">
<figcaption>120초 유지 검사: 인원별 전후 각 1회, seed 3. 실패한 개선 전 실행도 포함했습니다.</figcaption></figure>
<figure><img alt="64~128명 패킷 작업 대기 p99 상한과 CPU 전후 비교" src="{latency_image}">
<figcaption>CPU는 전체 12논리 코어로 정규화했습니다. p99는 실행을 시작한 패킷 작업의 대기 histogram bucket 상한이며 RTT가 아닙니다.</figcaption></figure>
<p><strong>128명 최종 정리:</strong> 개선 전 20초 제한 뒤 {drain_before['last']['pending_packets']:,}개 남음 → 개선 후 {fmt(drain_after['elapsed_ms']/1000)}초에 {drain_after['last']['processed_packets']:,}개 입력 처리·패킷/송신 대기 0.</p>
<details><summary>120초 유지 검사 전체 수치</summary><table><tr><th>부하</th><th>개선 전 · 처리/s / 미실행 / 판정</th><th>개선 후 · 처리/s / 미실행 / 판정</th></tr>{''.join(table)}</table>
<table><tr><th>부하</th><th>패킷 대기 p99 상한 · 전 → 후</th><th>서버 CPU · 전 → 후</th></tr>{metrics_rows}</table></details>
<details><summary>128명 · 20초 반복 5회 — 중앙값과 범위</summary><table><tr><th>구분</th><th>기능 통과</th><th>패킷/s</th><th>CPU %</th><th>대기 p99 상한 ms</th></tr>{''.join(repeated)}</table>
<p class="muted">괄호는 최소~최대, seed 1~5. 120초 검사 각 1회를 5회 반복 안정성으로 해석하지 않습니다.</p></details>
<details><summary>CPU 증가·더미 수신 한계·측정 조건</summary>
<p>128명 송신 프레임은 {fmt(b128['send_frames_per_s'],0)} → {fmt(a128['send_frames_per_s'],0)}개/초로 늘었습니다. CPU 증가도 함께 관측했으며 원인별 CPU 비중은 아직 분리하지 않았습니다.
측정 표본의 private bytes 최대는 {fmt(memory_before)} → {fmt(memory_after)}MiB입니다.</p>
<table><tr><th>부하</th><th>더미·관측 CPU · 코어 1개 기준 %</th><th>입력 발행 lag p99</th></tr>{headroom_rows}</table>
<p class="muted">최종 정리는 격리 서버의 누적 처리 수 충족·빈 패킷/송신 큐를 서로 다른 reporter 3표본에서 확인합니다. 패킷별 ACK는 아닙니다.
전후 순차 실행으로 OS 변동을 완전히 통제하지 못했습니다. 상세 계측 오버헤드의 반복 대조는 후속입니다.</p></details>
<p class="muted"><strong>이전 작업에서 실제 수행한 검증:</strong> {verification} 성능 실행 20개 모두 연결 0·정상 자원 회수를 확인했습니다.
<a href="LOBBY_THROUGHPUT.md">원시 결과·소스 식별값·재현 명령</a></p>
</section>
<section id="followup"><h2>5. 추가 발견한 결함 또는 대응 필요 요소</h2>
<table><tr><th>항목</th><th>영향·상태/근거</th><th>다음 대응</th></tr>
<tr><td>시야 탐색·주변 전송</td><td>Move cycle의 {fmt(view_share)}% · 비용 집중 확인</td><td>순회·거리·뷰 차이·전송을 분리 계측한 뒤 송신 묶음/이동 TODO 검토</td></tr>
<tr><td>더미 수신 한계</td><td>96명 이상 코어 하나의 90% 초과 · 최대 동접 미확정</td><td>수신을 여러 프로세스/호스트로 분산해 재측정</td></tr>
<tr><td>ViewList 중첩 shared lock</td><td>재진입 코드 확인 · 실제 교착은 미재현</td><td>잠금 범위 정리와 회귀 검사</td></tr>
<tr><td>최초 업데이트 시각·타이머 idle loop</td><td>초기 간격 왜곡·idle CPU 후보 · 이번 수정 범위 밖</td><td>시각 초기화 수정 및 타이머 CPU 별도 확인</td></tr>
</table><p class="muted">채널 병렬화, 운영 DB·블록체인, GUI 전체 플레이, 장시간 반복 안정성은 미검증입니다.
문서 수치·구조와 내장 차트는 확인하며 전체 HTML 브라우저 레이아웃 검증 여부는 <a href="LOBBY_THROUGHPUT.md#검증">검증 근거</a>에 구분해 기록합니다.</p>
</section></main></body></html>'''
    target = ROOT / 'docs/development/LOBBY_THROUGHPUT.html'
    target.write_text(document, encoding='utf-8')
    analysis = {'soak': detail, 'short': short, 'profiles': profiles}
    (LOGS / 'lobby-throughput-analysis-20261013.json').write_text(json.dumps(analysis, ensure_ascii=False, indent=2), encoding='utf-8')
    lines = ['# 로비 단일 채널 처리량 개선 근거', '', outcome, '', '## 조건과 식별값', '',
             '로직 소비자 1개·FIFO/budget·이동 주기·프로토콜 유지. 전후 동일 Release x64 LOCAL_TEST, 1채널, 입장 제한 해제, DB 없음.',
             'CPU: Ryzen5 7500F, 6코어/12논리 스레드. 120초 인원별 각 1회 및 128명 20초 seed 1~5 각 5회. 상세 프로파일은 별도 1회.',
             f"Python/monitor: {checks['python_environment']['version']}; 환경 원본은 artifacts/logs/lobby-throughput-machine-20261013.json·lobby-throughput-python-20261013.json.",
             '소스·실행 파일·로비 OBJ·시나리오 SHA-256 및 source snapshot은 아래 metadata에 보존한다. 전후 배치는 순차 실행한다.', '']
    for variant in ('before', 'after'):
        lines += [f"- {variant}: 기준 commit `{metadata[variant]['commit']}`, `artifacts/logs/lobby-throughput-soak-{variant}-20261013/metadata.json`",
                  f"  Lobby EXE SHA-256 `{metadata[variant]['fingerprints']['artifacts/bin/Release/LobbyServer/Lobby_Server.exe']}`"]
    lines += ['', '## 실제 측정', '', '| 인원 | 구분 | 처리 패킷/s | 대기 개수 | p99 상한 ms | CPU % | 최종 정리 | 기능 |', '|---|---|---:|---:|---:|---:|---|---|']
    for item in detail:
        v, s, d = item['variant'], item['server'], item['settlement']
        lines.append(f"| {item['clients']} | {v} | {fmt(s['packets_per_s'])} | {s['pending_packets_end']} | {fmt(s['packet_job_wait_us']['p99_us_upper']/1000)} | {fmt(s['cpu_percent'])} | {d.get('status')} ({fmt(d['elapsed_ms']/1000)}s) | {item['ok']} |")
    lines += ['', '## 프로파일 구간과 비용', '',
              '128명·20초 상세 계측 전후 각 1회. measure→verify 안의 monitor 표본 중 reporter 자체 timestamp도 측정 구간 내부인 서로 다른 sequence만 사용한다.',
              f"실제 차분 구간: 전 {fmt(profiles['before']['seconds'],3)}초/{profiles['before']['stages']['total']['calls']:,} Move, 후 {fmt(profiles['after']['seconds'],3)}초/{profiles['after']['stages']['total']['calls']:,} Move.",
              'wall/Move는 실제 실행된 Move 전체 수로 나눈 평균이다. cycle 비중도 Move가 분모이며 전체 프로세스 CPU가 아니다. 같은 입력 조건이지만 ray 질의 좌표까지 고정한 비교는 아니다.', '',
              '| 구간 | 전 wall ms/Move | 후 wall ms/Move | 전 cycle 비중 | 후 cycle 비중 |', '|---|---:|---:|---:|---:|']
    for name, label in (('lobby_nav', 'NavMesh'), ('lobby_height', 'HeightMesh'), ('lobby_view', '시야 탐색·주변 전송')):
        before_stage, after_stage = (profiles[v]['stages'][name] for v in ('before', 'after'))
        lines.append(f"| {label} | {fmt(before_stage['wall_ms_per_move'],3)} | {fmt(after_stage['wall_ms_per_move'],3)} | {fmt(before_stage['cycle_share_percent'])}% | {fmt(after_stage['cycle_share_percent'])}% |")
    lines += ['', '## 원시 근거·재현', '', '`artifacts/logs/lobby-throughput-{short,soak,profile}-{before,after}-20261013`의 summary.json과 실행별 report.json/lab/runs/*.json을 사용한다.',
              '`artifacts/logs/lobby-throughput-analysis-20261013.json`은 구간 차분·반복·더미 지연·정리 판정의 통합 근거다.',
              '```powershell',
              'python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 128 --repeats 5 --headless --output artifacts/logs/<새-짧은-측정-폴더>',
              'python scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 96 112 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/<새-유지-측정-폴더>',
              'python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 128 --repeats 1 --profile --headless --output artifacts/logs/<새-프로파일-폴더>',
              'python scripts/Build-LobbyThroughputReport.py', '```', '',
              '원시 JSON의 샘플은 측정 구간 안의 서로 다른 reporter sequence 차분이다. 없는 표본은 0으로 채우지 않는다. 실패 실행도 포함한다.',
              'p99 bucket 상한은 RTT·클라이언트 체감 지연의 정확한 값이 아니다. 전체 CPU와 Move 구간 cycle 비중도 구분한다.',
              '96명 이상 개선 후 더미 수신 CPU가 코어 하나의 90%를 넘었다. 이전 수용량 진단의 생성기 여유 기준을 통과한 결과로 해석하지 않는다. 처리량/최종 정리 기능 통과와 낮은 지연 SLO·최대 동접 판정을 구분한다.',
              '120초 검사 각 1회는 5회 반복의 대체가 아니다. 기본 5회 반복은 짧은 128명 비교에 수행했다. 최대 동접·장시간 안정성은 미확정이다.', '',
              '## 검증', '', verification, '검사 원본: artifacts/logs/lobby-throughput-checks-20261013.json. 모든 20개 성능 실행의 종료·fingerprint 확인: artifacts/logs/lobby-throughput-verification-20261013.json.',
              '보고서 편집은 기존 실측·회귀 결과를 사용한다. 서버 수정·새 부하 검사는 수행하지 않았다. 현재 보고서의 구조·내장 이미지·링크·원시 수치 대조 결과는 artifacts/logs/lobby-throughput-report-verification-20261013.json에 기록한다.',
              '전체 HTML 브라우저 레이아웃과 Archify visual-check는 로컬 파일 접근 제한 이력 때문에 미검증이다. 내장 PNG 3개는 실제 이미지로 확인하고 HTML 5개 섹션·상세 펼침·링크·수치/단위 구조를 검사한다.',
              '원본 OBJ와 첫 교차/float 높이 판정은 유지한다. 공간 인덱스와 캐시는 이미 공유된 Game_Server/MathUtil.h·TerrainSpatialIndex.h를 재사용한다.',
              '기존 ViewList 재귀 shared lock·최초 업데이트 시간·타이머 idle CPU 문제는 이번 수정과 분리한 후속 과제다.']
    (ROOT / 'docs/development/LOBBY_THROUGHPUT.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(target)


if __name__ == '__main__':
    main()
