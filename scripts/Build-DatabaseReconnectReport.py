"""완료된 장애 복구/회귀 검사에서 공개용 근거와 독립 HTML 보고서를 생성한다."""
import base64
import hashlib
import io
import json
import os
import platform
import statistics
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / '.runtime/db-test-deps'))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def read(path):
    return json.loads((ROOT / path).read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def main():
    cost = read('artifacts/logs/database-request-cost/result.json')
    assert cost['ok'] and len(cost['rows']) == 10
    assert cost['sql_counts']['before']['probe_sql'] == 100 and cost['sql_counts']['after']['probe_sql'] == 0
    assert all(item['actual_selects'] == 100 and item['dropped_events'] == 0 and not item['truncated'] for item in cost['sql_counts'].values())
    assert cost['binary_sha256']['after'] == sha('artifacts/bin/Release/LobbyServer/Lobby_Server.exe')
    configs = {}
    for config in ('Debug', 'Release'):
        result = read(f'artifacts/logs/database-reconnect/{config}/result.json')
        baseline = read(f'artifacts/logs/database/integration-{config}.json')
        consistency = read(f'artifacts/logs/database-consistency/{config}/result.json')
        core = read(f'artifacts/logs/test-server-core-{config}/checks.json')
        smoke = read(f'artifacts/logs/smoke-{config}.json')
        assert all(item['ok'] for item in (result, baseline, consistency, core)) and smoke['ServersConnected']
        assert len(result['cases']) == 34 and len(result['samples']) == 5
        assert all(case['passed'] for item in (result, baseline, consistency) for case in item['cases'])
        assert result['binary_sha256'] == sha(f'artifacts/bin/{config}/LobbyServer/Lobby_Server.exe')
        summaries = {key: {'median': statistics.median(sample[key] for sample in result['samples']), 'min': min(sample[key] for sample in result['samples']), 'max': max(sample[key] for sample in result['samples'])} for key in ('detected_ms', 'restore_to_resumed_ms', 'independent_response_ms')}
        configs[config] = {'reconnect': result, 'summary': summaries, 'regression': {'database': len(baseline['cases']), 'consistency': len(consistency['cases']), 'core': core['checks'], 'abi_types': 110, 'abi_constants': 106, 'smoke': True}}
    diagram = read('artifacts/logs/db-reconnect-diagram-deliver.json')
    visual = read('artifacts/logs/db-reconnect-diagram-visual.json')
    assert diagram['ok'] and visual['ok'] and diagram['artifact']['sha256'] == sha('docs/diagrams/database-reconnect/recovery.html')
    inputs = ['Server/ServerCore/include/ServerCore/RecoveringWorkQueue.h', 'Server/ServerCore/tests/DatabaseQueueTests.cpp', *[f'Server/Lobby_Server/{file}' for file in ('CClient.cpp', 'CClient.h', 'CDataBase.cpp', 'CDataBase.h', 'CDataBaseThread.cpp', 'CDataBaseThread.h', 'CPacketMgr.cpp', 'CP2PNetwork.cpp')], 'scripts/Initialize-LocalDatabase.ps1', 'scripts/Test-DatabaseReconnect.ps1', 'scripts/Measure-DatabaseRequestCost.ps1', 'tools/server_lab/tests/database_reconnect.py', 'tools/server_lab/tests/database_request_cost.py']
    evidence = {'schema_version': 2, 'date': '2026-11-02', 'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'environment': {'os': platform.platform(), 'cpu': platform.processor(), 'logical_cores': os.cpu_count(), 'python': platform.python_version(), 'build': 'MSVC v145 x64 C++20', 'database': 'private SQL Server Express LocalDB / ODBC Driver 17'}, 'inputs_sha256': {p: sha(p) for p in inputs}, 'configurations': configs, 'normal_request_comparison': cost, 'diagram': diagram, 'visual': {'containment': True, 'manual_review': 'passed', 'note': '1440x900 light and 2048x1320 dark PNG inspected'}, 'previous_validation_not_rerun': {'date': '2026-11-01', 'startup_failure_cases_per_configuration': 3, 'release_network_groups': 7}, 'limits': ['No production DB, WAN, maximum load or full GUI validation', 'Read-only single-client latency comparison, 200 requests x 5 runs each; median of run medians, not p99', 'Extended Events counts are separate 100-request captures; latency timing has XE disabled', 'First failing general write may be outcome-unknown; no automatic replay or durable reconciliation', 'SQL counts refer to probe SELECT plus business SELECT, not all ODBC calls or protocol packets']}
    output = ROOT / 'docs/development'
    (output/'evidence/database-reconnect-20261102.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2),encoding='utf-8')
    before, after = cost['summary']['before'], cost['summary']['after']
    reduction = (1 - after['median_ms'] / before['median_ms']) * 100
    plt.rcParams.update({'font.family': 'Malgun Gothic', 'axes.unicode_minus': False, 'font.size': 11})
    fig, axes = plt.subplots(1, 2, figsize=(12, 3.8), layout='constrained')
    axes[0].bar(['이전','수정'], [100,100], color='#586be6', label='업무 SELECT')
    axes[0].bar(['이전','수정'], [100,0], bottom=[100,100], color='#ef9c40', label='연결 확인 SELECT 1')
    axes[0].set(title='SELECT 실행 수 / 동일 조회 100건 (XE ON)', ylabel='실행 수', ylim=(0,250))
    axes[0].text(0,205,'200',ha='center'); axes[0].text(1,105,'100',ha='center'); axes[0].legend(loc='upper right',fontsize=9)
    for variant, offset, color in [('before',-.18,'#aebaca'),('after',.18,'#17a8a0')]:
        values = [row['median_ms'] for row in cost['rows'] if row['variant']==variant]
        axes[1].bar([i+offset for i in range(1,6)],values,width=.34,color=color,label='이전' if variant=='before' else '수정')
    axes[1].set(title='정상 응답 중앙값 / 200건×5회 (XE OFF)', ylabel='ms (낮을수록 빠름)', xlabel='반복 번호', ylim=(0,max(before['max_run_median_ms'],after['max_run_median_ms'])*1.3))
    axes[1].set_xticks(range(1,6)); axes[1].legend()
    for axis in axes: axis.spines[['top','right']].set_visible(False)
    png=io.BytesIO(); fig.savefig(png,format='png',dpi=150,facecolor='white'); plt.close(fig)
    chart=base64.b64encode(png.getvalue()).decode()
    cost_rows=''.join(f"<tr><td>{i+1}</td><td>{next(r['median_ms'] for r in cost['rows'] if r['round']==i and r['variant']=='before'):.4f}</td><td>{next(r['median_ms'] for r in cost['rows'] if r['round']==i and r['variant']=='after'):.4f}</td></tr>" for i in range(5))
    tests=''.join(f"<tr><td>{c}</td><td>{len(v['reconnect']['cases'])}/34 PASS</td><td>{v['regression']['database']} / {v['regression']['consistency']}</td><td>{v['regression']['core']}</td><td>{v['reconnect']['database']['failed']} / {v['reconnect']['database']['outcome_unknown']}</td></tr>" for c,v in configs.items())
    release=configs['Release']['summary']
    style = """*{box-sizing:border-box}body{margin:0;background:#eef2f6;color:#192e41;font:16px/1.7 'Malgun Gothic',sans-serif}main{max-width:1160px;margin:auto;padding:36px 24px}h1{font-size:32px;line-height:1.3;margin:10px 0}h2{font-size:23px;margin:0 0 14px}p{margin:8px 0 14px}a{color:#176d9a}section{background:white;padding:26px;margin:22px 0;border:1px solid #dae4ec;border-radius:14px}.eyebrow{color:#47718b;font-size:13px;font-weight:700}.cards{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin:22px 0}.card{background:#102f43;color:white;border-radius:12px;padding:18px}.card b{display:block;font-size:27px;color:#80e7d4}.card small{display:block;color:#b7d1df}.hint{background:#f2f7fa;border-left:4px solid #169f99;padding:12px 16px}.compare{display:grid;grid-template-columns:1fr 1fr;gap:18px}pre{background:#122c3d;color:#d9edf4;padding:18px;font:13px/1.6 Consolas,monospace;border-radius:9px;white-space:pre-wrap;word-break:break-word}table{width:100%;border-collapse:collapse;font-size:14px;table-layout:fixed}th,td{padding:11px 8px;text-align:left;border-bottom:1px solid #e2e9f0;overflow-wrap:anywhere}th{background:#f2f6fa}img{max-width:100%;height:auto}.muted{font-size:13px;color:#597083}@media(max-width:700px){main{padding:20px 12px}h1{font-size:26px}section{padding:18px}.cards{grid-template-columns:1fr 1fr}.compare{grid-template-columns:1fr}table{font-size:12px}th,td{padding:8px 4px}}"""
    document=f"""<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>DB 연결 복구와 정상 요청 비용 개선</title><style>{style}</style><main>
<div class="eyebrow">2026-11-02 · Windows x64 · 실제 TCP → ODBC → 독립 LocalDB</div><h1>DB 연결 복구와 정상 요청 비용 개선</h1>
<p><strong>정상 요청의 SELECT 1 사전 확인을 제거하고, 실제 SQL 실패 후에만 연결을 진단한다.</strong> 재접속·대기 작업 재개와 불명확한 쓰기의 반복 금지는 유지했다.</p>
<div class="cards"><div class="card"><b>100 → 0건</b>연결 확인 SELECT<small>동일 조회100건, XE 실측</small></div><div class="card"><b>{after['median_ms']:.4f} ms</b>정상 요청 중앙값<small>이전 {before['median_ms']:.4f} ms, n=5</small></div><div class="card"><b>{reduction:.1f}%</b>응답 중앙값 감소<small>로컬 순차 읽기, 최대 처리량 아님</small></div><div class="card"><b>68/68</b>장애 복구 검사<small>Debug·Release 각34건</small></div></div>
<p class="hint">probe SELECT100→0, 업무 SELECT100→100. 해당 두 종류의 SELECT 합계는200→100건이다. ODBC prepare/fetch·모든 SQL·네트워크 패킷 수가 절반이 됐다는 뜻은 아니다.</p>
<section><h2>1. 결과</h2><p>수정 전 실행 파일을 보존해 같은 Release 조회를 비교했다. 지연은 전후 각각200건×5회, SQL 수는 별도100건의 Extended Events로 확인했다. 장애 복구는 구성별5회 반복과 실패/만료/종료 경계를 재검증했다.</p><table><tr><th>지표</th><th>이전</th><th>수정</th></tr><tr><td>연결 확인 SELECT / 100요청</td><td>100건</td><td>0건</td></tr><tr><td>업무 조회 SELECT / 100요청</td><td>100건</td><td>100건</td></tr><tr><td>정상 응답 중앙값 ms</td><td>{before['median_ms']:.4f}</td><td>{after['median_ms']:.4f}</td></tr><tr><td>일반 쓰기 결과 불명확</td><td colspan="2">자동 재실행·거짓 성공 응답 금지</td></tr></table></section>
<section><h2>2. 분석한 원인</h2><p>기존 구현은 DB 작업마다 SELECT 1을 실행했다. 실제 조회가 한 번인 요청도 별도의 SQL 왕복과 결과 처리를 거쳤다. 사전 확인 성공 직후에도 단절될 수 있으므로 실행 실패 정책을 대체하지 못했다.</p><div class="compare"><div><b>이전: 매 작업 사전 확인</b><pre>return database.Probe();
// Probe 내부
ExecuteDirect(L"SELECT 1");</pre></div><div><b>수정: 정상 상태 바로 실행</b><pre>try {{
    work.m_execute();
    Finish(work);
}} catch (...) {{
    // 이 경로에서만 _connectionLost() 호출
}}</pre></div></div></section>
<section><h2>3. 해결 방식</h2><p>큐의 probe 콜백과 CDataBase::Probe를 제거했다. 실제 SQL이 실패하면 오류와 연결 상태를 진단해 재접속 상태로 전환한다. 연결 생성·스키마·DB 식별값 검증을 통과한 뒤 FIFO를 재개한다.</p><table><tr><th>상황</th><th>처리</th></tr><tr><td>정상 상태</td><td>SQL과 결과 처리만 실행. 사전 SQL·연결 진단 없음</td></tr><tr><td>첫 실패가 안전한 조회/보상</td><td>같은 작업을 선두에 보존, 재접속 후 재시도</td></tr><tr><td>첫 실패가 일반 쓰기</td><td>결과 불명확으로 집계, 성공 응답·자동 반복 금지</td></tr><tr><td>알려진 장애 뒤 미실행 요청</td><td>4096건 상한과 일반30초 기한 안에서 대기 후 순서대로 실행</td></tr></table><p><a href="../diagrams/database-reconnect/recovery.html">현재 상태 전이 구조도</a> · <a href="../diagrams/database-reconnect/README.md">설계 선택과 비용</a></p></section>
<section><h2>4. 실제 측정한 내용</h2><p>Release·단일 클라이언트·순차 CS_OPEN_CUSTOMIZE·워밍업20건 후200건×5회다. 매 실행 서버를 재시작하고 전후 순서를 교대했다. 지연 비교는 XE OFF, SELECT 수 확인은 별도100건 XE ON이다. 소유 아이템의 실제 응답도 확인했다.</p><img alt="SQL 실행 수와 정상 요청 지연의 전후 비교" src="data:image/png;base64,{chart}"><table><tr><th>반복</th><th>이전 중앙값 ms</th><th>수정 중앙값 ms</th></tr>{cost_rows}</table><p class="muted">실행별 중앙값5개의 중앙값을 대표값으로 사용했다. 이전 범위 {before['min_run_median_ms']:.4f}–{before['max_run_median_ms']:.4f} ms, 수정 범위 {after['min_run_median_ms']:.4f}–{after['max_run_median_ms']:.4f} ms. 원격 DB 지연·p99·최대 처리량으로 일반화하지 않는다. XE 이벤트 누락0, ring buffer 잘림 없음.</p><table><tr><th>구성</th><th>장애 복구</th><th>기존 DB / 정합성</th><th>Core checks</th><th>주입 실패 / 결과 불명확</th></tr>{tests}</table><p>양 구성 서버 빌드·ABI110종/106상수·DB 없는 smoke PASS. 장애 주입에서 발생한 실패 카운터도 표에 보존했다. 첫 요청이 쓰기인 단절과 COMMIT 후 응답 유실 모두 자동 반복하지 않았다.</p><p>Release 장애5회: DB ONLINE 명령부터 구매/후속 응답까지 중앙값 {release['restore_to_resumed_ms']['median']:.2f} ms, 장애 중 다른 세션 응답 {release['independent_response_ms']['median']:.3f} ms. 재접속 타이머 대기를 포함한다.</p><p class="muted">설정 실패 각3경로와 Release 네트워크7그룹은11/1 검증 기록이며 이번에 재실행하지 않았다. 기존 전체 빌드 경고 해소를 주장하지 않는다.</p><p><a href="evidence/database-reconnect-20261102.json">11/2 표본·판정·소스/바이너리 SHA-256</a> · <a href="evidence/database-reconnect-20261101.json">11/1 이전 복구 검증 기록</a> · <a href="DATABASE_RECONNECT.md">재현 명령</a></p></section>
<section><h2>5. 추가 발견한 결함 또는 대응 필요 요소</h2><table><tr><th>항목</th><th>영향·현재 상태</th><th>다음 대응</th></tr><tr><td>첫 요청의 쓰기 단절</td><td>사전 probe가 없으므로 첫 쓰기가 실패할 수 있다. 결과 불명확 시 자동 반복하지 않음</td><td>요청 ID·일반 쓰기 원장으로 처리 여부 확인 지원</td></tr><tr><td>검증 fixture</td><td>예전 검사는 사전 probe가 첫 구매를 보호한다고 가정. 안전한 조회 실패 뒤 미실행 구매 대기와 첫 쓰기 실패를 나누어 재검증</td><td>보장 경계에 맞는 실패 주입 유지</td></tr><tr><td>원격 DB·병렬 부하</td><td>이번 수치는 로컬 순차 읽기. 최대 처리량과 WAN 비용 미측정</td><td>실사용 부하에서 큐/SQL/네트워크 시간을 분리 측정</td></tr><tr><td>내구성과 인증</td><td>보상만 디스크 보존. 일반 요청 원장·원본 보상 ACK·운영 전송 인증은 기존 후속 과제</td><td>업무 계약에 맞춰 별도 설계</td></tr></table><p class="muted">측정 근거: <a href="https://learn.microsoft.com/en-us/sql/relational-databases/extended-events/quick-start-extended-events-in-sql-server">Microsoft Extended Events</a>. 운영 DB·전원/디스크 장애·전체 GUI는 검증하지 않았다.</p></section></main></html>"""
    (output/'DATABASE_RECONNECT.html').write_text(document,encoding='utf-8')
    print(json.dumps({'ok':True,'recovery_checks':68,'sql_counts':cost['sql_counts'],'normal_request_summary':cost['summary'],'median_reduction_percent':reduction},ensure_ascii=False))


if __name__ == '__main__':
    main()
