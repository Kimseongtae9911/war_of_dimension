"""상세 실측 Markdown에서 결과 우선 HTML 요약 보고서를 생성한다. matplotlib 필요."""

import argparse
import hashlib
import html
import io
import json
import re
from pathlib import Path


def table(text, prefix):
    start = text.index(prefix)
    rows = []
    for line in text[start:].splitlines()[2:]:
        if not line.startswith('|'):
            break
        rows.append([cell.strip() for cell in line.strip('|').split('|')])
    return rows


def number(value):
    return float(value.strip().replace(',', ''))


def pair(value):
    return [number(part) for part in value.split(' → ')]


def formatted(value, digits=3):
    return f'{value:,.{digits}f}'.rstrip('0').rstrip('.') if digits else f'{value:,.0f}'


def cells_table(headers, rows, label):
    header = ''.join(f'<th scope="col">{html.escape(cell)}</th>' for cell in headers)
    body = ''.join('<tr>' + ''.join(f'<td>{html.escape(cell)}</td>' for cell in row) + '</tr>' for row in rows)
    return f'<div class="table-wrap" tabindex="0" role="region" aria-label="{label}"><table><thead><tr>{header}</tr></thead><tbody>{body}</tbody></table></div>'


def metric_svg(metric, index, font_path):
    import matplotlib
    matplotlib.use('Agg')
    from matplotlib import pyplot as plt, font_manager
    from matplotlib.ticker import FuncFormatter, MaxNLocator

    if font_path:
        font_manager.fontManager.addfont(str(font_path))
        family = font_manager.FontProperties(fname=str(font_path)).get_name()
    else:
        family = 'sans-serif'
    with matplotlib.rc_context({'font.family': family, 'svg.fonttype': 'none', 'svg.hashsalt': f'jobqueue-{index}', 'axes.unicode_minus': False}):
        fig, ax = plt.subplots(figsize=(10, 2.65), facecolor='#151f2f')
        ax.set_facecolor('#151f2f')
        colors = ['#aab9cc', '#64dbc8']
        values = metric['values']
        ax.barh([1, 0], values, height=.43, color=colors, zorder=3)
        ax.set_yticks([1, 0], ['개선 전', '개선 후'], color='#eaf0f8', fontsize=12)
        maximum = max(max(values), 1)
        ax.set_xlim(0, maximum * 1.26)
        ax.set_ylim(-.6, 1.6)
        ax.xaxis.set_major_locator(MaxNLocator(nbins=4))
        ax.xaxis.set_major_formatter(FuncFormatter(lambda value, _: formatted(value, 1)))
        ax.tick_params(axis='x', colors='#a9b8cb', labelsize=10, length=0, pad=10)
        ax.tick_params(axis='y', length=0, pad=15)
        ax.grid(axis='x', color='#2c3a4e', linewidth=.7, zorder=0)
        for side in ax.spines.values():
            side.set_visible(False)
        for y, value in zip([1, 0], values):
            ax.text(value + maximum * .018, y, formatted(value), color='#eaf0f8', va='center', fontsize=12, fontweight='bold')
        ax.set_xlabel(metric['unit'] + ' · 0부터 시작하는 공통 선형 축', color='#a9b8cb', fontsize=10, labelpad=12)
        fig.subplots_adjust(left=.115, right=.985, top=.95, bottom=.25)
        output = io.StringIO()
        fig.savefig(output, format='svg', metadata={'Date': None, 'Creator': 'Build-JobQueueReport.py / matplotlib'})
        plt.close(fig)
    svg = re.sub(r'^.*?(?=<svg)', '', output.getvalue(), flags=re.S)
    svg = re.sub(r'(?m)^\s+$', '', svg)
    svg = re.sub(r'(?m)[\t ]+$', '', svg)
    for identifier in re.findall(r'\bid="([^"]+)"', svg):
        unique = f'chart-{index}-{identifier}'
        svg = svg.replace(f'id="{identifier}"', f'id="{unique}"').replace(f'url(#{identifier})', f'url(#{unique})').replace(f'href="#{identifier}"', f'href="#{unique}"')
    description = f"{metric['label']}: 개선 전 {formatted(values[0])}, 개선 후 {formatted(values[1])} {metric['unit']}"
    return svg.replace('<svg ', f'<svg role="img" aria-label="{html.escape(description, quote=True)}" ', 1)


def make_report(source, font_path=None):
    text = source.read_text(encoding='utf-8-sig')
    main_rows = table(text, '| 조건 | packets/s 전→후')
    lobby = next(row for row in main_rows if row[0] == '로비 64명')
    game = next(row for row in main_rows if row[0] == '게임 64명')
    tick = next(row for row in table(text, '| 조건 | tick 지연 p99') if row[0] == '게임 64명')
    outputs = table(text, '| 게임 64명 출력/메모리')
    io_row = next(row for row in outputs if row[0] == '측정 구간 pending I/O peak')
    memory = next(row for row in outputs if row[0] == 'private bytes 표본 peak MiB')
    passed = re.search(r'개선 전 기능 통과는 (\d+)/(\d+)회, 개선 후는 (\d+)/(\d+)회', text)
    if not passed:
        raise ValueError('상세 근거에서 기능 통과/전체 횟수를 찾지 못했습니다.')
    before_pass, before_total, after_pass, after_total = map(int, passed.groups())
    metrics = [
        dict(label='로비 처리량', unit='packets/s', values=pair(lobby[1]), note='높을수록 좋음 · 목표 256 packets/s. 고정 부하 처리율이며 최대 TPS가 아닙니다.'),
        dict(label='로비 패킷 대기 p99', unit='ms', values=pair(lobby[2]), note='낮을수록 좋음 · 실행별 histogram bucket 상한의 중앙값. 종단 간 응답 지연과 구분합니다.'),
        dict(label='게임 pending I/O peak', unit='contexts', values=[number(cell.split(' [')[0]) for cell in io_row[1:]], note='낮을수록 좋음 · 1초 표본 peak의 중앙값. 전송 대기 전용 개수와는 다릅니다.'),
        dict(label='게임 private bytes peak', unit='MiB', values=[number(cell.split(' [')[0]) for cell in memory[1:]], note='낮을수록 좋음 · 1초 표본 peak의 중앙값. 풀 보유량을 포함하며 전부 누수는 아닙니다.'),
        dict(label='게임 CPU', unit='%', values=pair(game[3]), note='낮을수록 좋음 · 논리 CPU 12개 기준. 이번 변경에서 CPU는 낮아지지 않았습니다.'),
        dict(label='게임 tick 지연 p99', unit='ms', values=pair(tick[1]), note='낮을수록 좋음 · 목표 tick 15ms 초과 지연의 bucket 상한. 개선 전후 같습니다.'),
    ]
    charts = ''.join(f'<figure class="metric-chart" data-metric="{i}"><figcaption>{metric["label"]}</figcaption>{metric_svg(metric, i, font_path)}<p class="note">{metric["note"]}</p></figure>' for i, metric in enumerate(metrics))
    options = ''.join(f'<option value="{i}">{metric["label"]}</option>' for i, metric in enumerate(metrics))
    cards = [
        ('기능 검사 통과', f'{before_pass}/{before_total} → {after_pass}/{after_total}', '계측 켬·끔 합계 · 준비 EOF 5→0회', ''),
        ('로비 64명 처리량', ' → '.join(formatted(v, 2) for v in metrics[0]['values']), 'packets/s · 목표 256/s', ''),
        ('게임 64명 pending I/O peak', ' → '.join(formatted(v, 1) for v in metrics[2]['values']), '1초 표본 peak의 중앙값', ''),
        ('게임 64명 CPU', ' → '.join(formatted(v, 2) for v in metrics[4]['values']) + '%', 'CPU 감소는 확인되지 않음', 'caution'),
    ]
    card_html = ''.join(f'<div class="card"><small>{label}</small><strong>{value}</strong><span class="{kind}">{note}</span></div>' for label, value, note, kind in cards)
    rows = [[row[0], row[1], row[2], row[3]] for row in main_rows]
    measured = cells_table(['조건', '처리량 전→후 (packets/s)', '패킷 대기 p99 전→후 (ms)', 'CPU 전→후 (%)'], rows, '4 16 64명 실제 서버 전후 비교')
    obligations = [
        ['로비 64명 잔여 대기', '지연·처리율 부족', '확인 · 종료 대기 228개, p99 상한 2.097초', '재등록 주기·단일 worker·view/지형 비용 분리 측정'],
        ['게임 64명 CPU·tick', 'tick 목표 초과 지속', '확인 · CPU 49.375%, tick 지연 p99 131.072ms', 'AI·충돌·전송 실행 비용과 IOCP 경합 측정'],
        ['다중 스레드 큐 경합', '일부 단위 처리량 저하', '확인 · producers4/Five 계측 끔 4.745→4.594M jobs/s', '잠금 대기·게시/소비 경합 분리 측정'],
        ['SC_MATCH 전달 취소', '매치 준비 중 EOF', '수정 · 즉시 Disconnect 제거, 준비 실패 5→0회', '연결 유지 timeout 정책은 별도 검토'],
        ['장시간·출력 대기 상한', '장기 적체 가능성', '미측정 · backpressure 추가 안 됨', '장시간·포화 부하와 출력 대기량 제한 검토'],
    ]
    needs = cells_table(['항목', '영향', '상태 / 근거', '다음 대응'], obligations, '추가 결함 및 대응 필요 요소')
    cause_queue = html.escape('concurrent_priority_queue<JobRef> m_jobs;\nstd::priority_queue<JobTarget *> m_queue;\n// 포인터 주소로 우선순위가 정해짐')
    cause_send = html.escape('프레임 1개 전송 → 완료를 worker가 처리\n                 ↓\n            다음 프레임 발행\n// 같은 6개 worker가 매치 tick도 실행')
    solution_queue = html.escape('std::queue<JobRef> m_jobs;\nstd::queue<JobTarget *> m_queue;\n// 가장 먼저 게시한 대상부터 실행\ntarget = m_queue.front();\nm_queue.pop();\n// 남은 작업은 ready 목록 뒤에 재등록')
    solution_send = html.escape('for (auto *queued : _state.m_sends)\n{\n    _state.m_sendBatch.push_back(queued);\n    // 프레임 bytes를 소유 버퍼에 복사…\n    if (_state.m_sendBatch.size() == 64)\n        break;\n}')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    payload = json.dumps({'metrics': metrics, 'passed': [before_pass, before_total, after_pass, after_total]}, ensure_ascii=False).replace('</', r'<\/')
    template = '''<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>JobQueue 개선 보고서 · War Of Dimension</title>
<style>
:root{color-scheme:dark;--bg:#0d1420;--panel:#151f2f;--line:#2c3a4e;--ink:#eaf0f8;--muted:#a9b8cb;--accent:#64dbc8;--warn:#f0c37b}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.7 system-ui,'Malgun Gothic',sans-serif}main{max-width:1240px;padding:38px 32px 55px;margin:auto}h1,h2,h3,p{margin-top:0}h1{font-size:clamp(28px,3.7vw,44px);line-height:1.25;letter-spacing:-.04em;margin-bottom:24px}h2{font-size:24px;letter-spacing:-.025em;line-height:1.4;margin-bottom:17px}h2 .number{color:var(--accent);font-size:14px;letter-spacing:.05em;margin-right:12px}h3{font-size:19px;line-height:1.45;margin-bottom:12px}a{color:var(--accent);text-underline-offset:4px}.lead{font-size:18px;max-width:1000px;color:#d4e0ed;margin-bottom:18px}.cards{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px}.card,.panel{background:var(--panel);border:1px solid var(--line);border-radius:13px;padding:20px}.card small{font-size:12px;color:var(--muted);display:block}.card strong{display:block;font-size:clamp(20px,2.15vw,27px);letter-spacing:-.045em;line-height:1.4;margin:12px 0 8px;white-space:nowrap}.card span{color:var(--accent);font-size:12px}.card .caution{color:var(--warn)}.verdict{color:var(--warn);font-size:14px;margin:17px 0 0}.navigation{display:flex;gap:24px;flex-wrap:wrap;border-bottom:1px solid var(--line);padding:20px 0;margin-top:12px}.navigation a{text-decoration:none;font-size:13px}.section{margin-top:32px;scroll-margin-top:22px}.two{display:grid;grid-template-columns:1fr 1fr;gap:16px}.panel p{margin-bottom:12px;font-size:14px;color:#c6d3e3}.panel p:last-child{margin-bottom:0}.label{color:var(--accent);font-size:11px;letter-spacing:.1em;display:block;margin-bottom:9px}.note{font-size:12px;color:var(--muted)}.summary{border-left:3px solid var(--accent);padding:8px 16px;font-size:14px;color:#c6d3e3;margin:15px 0 0}pre{background:#0a111c;border:1px solid #27374a;padding:15px;margin:15px 0 0;border-radius:8px;font:13px/1.7 Consolas,'Malgun Gothic',monospace;overflow-x:auto}code{color:#c4ede5;font-size:.9em}.code-note{display:block;color:var(--muted);font-size:11px;margin-top:8px}.chart-head{display:flex;justify-content:space-between;align-items:center;gap:20px;flex-wrap:wrap;margin-bottom:10px}.chart-head h3{margin:0}.chart-head label{font-size:13px;color:var(--muted)}select,button{font:inherit;color:var(--ink);background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:8px 11px}button{cursor:pointer}a:focus-visible,select:focus-visible,button:focus-visible,.table-wrap:focus-visible{outline:2px solid var(--accent);outline-offset:3px}.metric-chart{margin:0}.metric-chart figcaption{color:var(--muted);font-size:13px;margin:12px 0 0}.metric-chart svg{display:block;width:100%;height:auto}.metric-chart .note{margin:0 0 5px}.js .metric-chart[hidden]{display:none}.facts{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:15px}.facts span{border:1px solid var(--line);border-radius:6px;padding:5px 9px;color:var(--muted);font-size:12px}.table-wrap{overflow-x:auto;border:1px solid var(--line);border-radius:10px;margin-top:18px}table{border-collapse:collapse;width:100%;font-size:12px;line-height:1.7}th,td{padding:11px 13px;text-align:left;border-bottom:1px solid var(--line);vertical-align:top}th{background:var(--panel);white-space:nowrap;font-weight:650}td{font-variant-numeric:tabular-nums}tr:last-child td{border-bottom:0}.checks{font-size:13px;color:#b9c9dc;margin-top:14px}footer{border-top:1px solid var(--line);padding-top:17px;margin-top:32px;color:var(--muted);font-size:12px}footer p{margin-bottom:8px}.hash{font:10px/1.6 Consolas,monospace;overflow-wrap:anywhere}
@media(max-width:850px){main{padding:25px 18px 40px}.cards{grid-template-columns:repeat(2,minmax(0,1fr))}.two{grid-template-columns:1fr}.card strong{font-size:26px}.navigation{gap:15px}}@media(max-width:450px){.cards{grid-template-columns:1fr}.card{padding:16px}.card strong{font-size:27px}.panel{padding:15px}.chart-head select{max-width:230px}table{min-width:650px}}@media print{body{font-size:11px}main{padding:0;max-width:none}.navigation,button,select{display:none}.panel,.card,figure{break-inside:avoid}.js .metric-chart[hidden]{display:block}.metric-chart svg{max-height:160px}.section{margin-top:20px}table{font-size:9px}th,td{padding:7px}.table-wrap{overflow:visible}h1{font-size:27px}h2{font-size:20px}pre{font-size:10px}.card strong{font-size:21px}.hash{font-size:8px}}
</style></head><body><main>
<h1>JobQueue 공정성·게임 전송 적체 개선</h1>
<section id="results" aria-labelledby="results-heading"><h2 id="results-heading"><span class="number">01</span>결과</h2><p class="lead">주소 우선순위 기아와 연결별 출력 적체를 수정해, 같은 부하의 이동·정지 검사가 <strong>__BEFORE_PASS__/__BEFORE_TOTAL__ → __AFTER_PASS__/__AFTER_TOTAL__회</strong> 통과했습니다.</p>
<div class="cards">__CARDS__</div><p class="verdict">남은 문제: 로비 64명 대기 228개 · 게임 CPU 감소 없음 · 게임 tick 지연 p99 131.072ms 유지</p></section>
<nav class="navigation" aria-label="보고서 목차"><a href="#results">결과</a><a href="#causes">분석한 원인</a><a href="#solutions">해결 방식</a><a href="#measurements">실제 측정</a><a href="#followups">추가 대응</a></nav>
<section class="section" id="causes"><h2><span class="number">02</span>분석한 원인</h2><div class="two">
<div class="panel"><span class="label">LOBBY / 주소 우선순위</span><h3>이동 작업이 정지 패킷을 굶겼습니다</h3><p>작업·ready 목록이 포인터 주소로 우선순위를 정했습니다. 즉시 자기 재등록하는 높은 주소의 이동 작업이 계속 선택됐습니다.</p><pre><code>__CAUSE_QUEUE__</code></pre><span class="code-note">기존 선언 발췌 · JobQueue / JobScheduler</span></div>
<div class="panel"><span class="label">GAME / 직렬 전송</span><h3>좌표는 변했지만 과거 상태만 도착했습니다</h3><p>16개 매치 모두 입력·좌표가 갱신됐습니다. 프레임마다 완료 처리를 기다리는 전송에 약 49만 context가 쌓였습니다.</p><pre><code>__CAUSE_SEND__</code></pre><span class="code-note">설명용 흐름 · 실제 구현 코드가 아님</span></div></div><p class="summary">분리 실험: FIFO만 적용하거나 TCP_NODELAY만 적용해도 게임 64명 실패는 남았습니다.</p></section>
<section class="section" id="solutions"><h2><span class="number">03</span>해결 방식</h2><div class="two">
<div class="panel"><span class="label">LOBBY / FIFO + 대상 순환</span><h3>먼저 게시한 작업과 대상부터 처리합니다</h3><p>작업·ready를 FIFO로 바꾸고 예산 뒤 남은 대상을 꼬리에 넣습니다. 실행 직렬화와 Five / Snapshot 예산은 유지합니다.</p><pre><code>__SOLUTION_QUEUE__</code></pre><span class="code-note">실제 코드 발췌 · 서로 다른 선언/처리 구간, 중간 생략</span></div>
<div class="panel"><span class="label">GAME / 대기 프레임 묶음 전송</span><h3>이미 기다리는 프레임을 최대 64개씩 보냅니다</h3><p>첫 프레임은 즉시 보냅니다. 이후 대기 프레임만 묶어 완료 대기 횟수를 줄이며 전송 순서·부분 전송·개별 회수는 유지합니다.</p><pre><code>__SOLUTION_SEND__</code></pre><span class="code-note">실제 IssueSend 발췌 · 버퍼 복사 부분 생략</span></div></div><p class="summary">별도 수정: SC_MATCH 직후 Disconnect를 제거해 목적지 패킷의 전달 취소를 막았습니다. 새 연결 유지 timeout 정책은 추가하지 않았습니다.</p></section>
<section class="section" id="measurements"><h2><span class="number">04</span>실제 측정한 내용</h2>
<div class="facts"><span>Release x64 · LOCAL_TEST · DB 없음</span><span>Ryzen 5 7500F · 논리 CPU 12개</span><span>4/16/64명 × seed 1~5 × 계측 켬·끔</span><span>워밍업 5초 · 측정 20초 · 입력 250ms</span><span>입장 소프트 제한 해제 · 매회 재시작</span><span>게임 64명 = 4명 × 16매치</span></div>
<div class="panel"><div class="chart-head"><h3>64명 지표 비교</h3><label for="metric">지표 선택 <select id="metric">__OPTIONS__</select></label></div>__CHARTS__</div>
__MEASURED__
<p class="note">표·그래프는 계측 켬 실행별 중앙값입니다. p99는 bucket 상한, CPU는 논리 CPU 12개 기준입니다. 전체 범위·계측 끔·큐 단위 대조는 <a href="JOBQUEUE_IMPROVEMENT.md">상세 근거</a>에 있습니다.</p>
<p class="checks">전후 각 60회 중 기능 통과 18→60회 · 준비 EOF 5→0회 · 유효 성능 구간 55→60회 · 전후 120회 종료 자원/더미 연결 0<br>검증 완료: Debug/Release Core 199검사·ABI, 실제 네트워크 회귀, Python 29검사. 이번 보고서 개편에서 부하 검사를 다시 실행한 것은 아닙니다.</p></section>
<section class="section" id="followups"><h2><span class="number">05</span>추가 발견한 결함·대응 필요 요소</h2>__NEEDS__<p class="note">로비 tick 지연은 이동 계산 간격−1ms로 정지/재시작도 포함하며 게임의 고정 tick 지연과 다릅니다. 요청별 ACK·장시간·전체 전투·운영 DB는 미검증입니다.</p></section>
<footer><p><a href="JOBQUEUE_IMPROVEMENT.md">전체 실측·분포·검증 근거</a> · <a href="SERVER_BASELINE.md">개선 전 실제 서버 기준</a> · <a href="JOBQUEUE_BASELINE.md">큐 단위 기준</a> · <a href="../diagrams/jobqueue-improvement/jobqueue-improvement.architecture.html">개선 흐름도</a> · <button id="print" type="button">인쇄 / PDF 저장</button></p><p>측정: 2026-10-11 · 원시 결과: artifacts/logs/jobqueue-improvement/final-on, final-off · 재생성: python scripts/Build-JobQueueReport.py (matplotlib 필요)</p><p class="hash">상세 근거 SHA-256: __HASH__</p></footer>
</main><script id="report-data" type="application/json">__DATA__</script><script>
document.documentElement.classList.add('js');const metricSelect=document.getElementById('metric'),charts=[...document.querySelectorAll('.metric-chart')];function selectMetric(){charts.forEach((chart,i)=>chart.hidden=i!==Number(metricSelect.value));}metricSelect.addEventListener('change',selectMetric);document.getElementById('print').addEventListener('click',()=>window.print());selectMetric();
</script></body></html>
'''
    substitutions = {
        '__CARDS__': card_html, '__OPTIONS__': options, '__CHARTS__': charts,
        '__MEASURED__': measured, '__NEEDS__': needs, '__CAUSE_QUEUE__': cause_queue,
        '__CAUSE_SEND__': cause_send, '__SOLUTION_QUEUE__': solution_queue,
        '__SOLUTION_SEND__': solution_send, '__HASH__': digest, '__DATA__': payload,
        '__BEFORE_PASS__': str(before_pass), '__BEFORE_TOTAL__': str(before_total),
        '__AFTER_PASS__': str(after_pass), '__AFTER_TOTAL__': str(after_total),
    }
    for key, value in substitutions.items():
        template = template.replace(key, value)
    return template


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=root / 'docs/development/JOBQUEUE_IMPROVEMENT.md')
    parser.add_argument('--output', type=Path, default=root / 'docs/development/JOBQUEUE_IMPROVEMENT.html')
    default_font = Path('C:/Windows/Fonts/malgun.ttf')
    parser.add_argument('--font', type=Path, default=default_font if default_font.is_file() else None, help='한글 글꼴 경로')
    args = parser.parse_args()
    result = make_report(args.source, args.font)
    args.output.write_text(result, encoding='utf-8', newline='\n')
    print(f'{args.output}: {len(result.encode("utf-8")):,} bytes')


if __name__ == '__main__':
    main()
