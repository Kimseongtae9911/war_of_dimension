"""저장된 DB 측정/검증 JSON에서 HTML과 재현 가능한 수치 근거를 만든다."""
import hashlib
import html
import io
import json
import platform
import re
import statistics
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / '.runtime/db-test-deps'))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def read(path):
    return json.loads((ROOT / path).read_text(encoding='utf-8-sig'))


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def main():
    logs = 'artifacts/logs/database/'
    benchmark = read(logs + 'benchmark-Release.json')
    integrations = {config: read(logs + f'integration-{config}.json') for config in ['Debug', 'Release']}
    assert len(benchmark['runs']) == 10
    assert all(data['ok'] and all(case['passed'] for case in data['cases']) for data in integrations.values())
    controlled = {}
    for key in ['caller_block_ms', 'other_session_ms', 'total_ms']:
        controlled[key] = {}
        for flag, name in [(False, 'before'), (True, 'after')]:
            values = [row[key] for row in benchmark['runs'] if row['async'] == flag]
            controlled[key][name] = {'median': statistics.median(values), 'min': min(values), 'max': max(values), 'n': len(values)}
    real = integrations['Release']['real_sql_lock_runs']
    real_median = {key: statistics.median(row[key] for row in real) for key in ['sql_blocked_session_ms', 'unrelated_session_ms']}
    startup = {config: read(logs + f'startup-{config}.json') for config in ['Debug', 'Release']}
    cores = {config: read(f'artifacts/logs/test-server-core-{config}/checks.json') for config in ['Debug', 'Release']}
    regression = read('artifacts/logs/server-core-integration-Release/summary.json')
    sources = [
        'Server/ServerCore/include/ServerCore/OrderedWorkQueue.h', 'Server/ServerCore/tests/DatabaseQueueTests.cpp',
        'Server/Lobby_Server/LobbyDispatcher.h', 'Server/Lobby_Server/CClient.h', 'Server/Lobby_Server/CClient.cpp',
        'Server/Lobby_Server/CDataBase.h', 'Server/Lobby_Server/CDataBase.cpp',
        'Server/Lobby_Server/CDataBaseThread.h', 'Server/Lobby_Server/CDataBaseThread.cpp',
        'Server/Lobby_Server/CPacketMgr.h', 'Server/Lobby_Server/CPacketMgr.cpp',
        'Server/Lobby_Server/CNetworkMgr.cpp', 'Server/Lobby_Server/CServer.cpp', 'Server/Lobby_Server/CP2PNetwork.cpp',
        'database/local-schema.sql', 'scripts/Start-Local.ps1', 'scripts/Initialize-LocalDatabase.ps1',
        'scripts/Test-Database.ps1', 'scripts/Test-DatabaseStartup.ps1', 'scripts/Measure-Database.ps1',
        'tools/server_lab/tests/database.py',
    ]
    hashes = {path: digest(path) for path in sources}
    binaries = {path: digest(path) for path in [f'artifacts/bin/{config}/{module}/{executable}.exe' for config in ['Debug','Release'] for module,executable in [('LobbyServer','Lobby_Server'), ('GameServer','Game_Server'), ('ServerCore.Tests','ServerCore.Tests')]]}
    warning_codes = set()
    for config in ['Debug','Release']:
        text = (ROOT / f'artifacts/logs/build-Servers-{config}.log').read_bytes().decode('cp949', errors='replace')
        warning_codes.update(re.findall(r'warning (C\d+)', text))
    drains = {}
    for config in ['Debug','Release']:
        text = (ROOT / (logs + f'{config}-LobbyServer.err.log')).read_text(encoding='utf-8', errors='replace')
        match = re.search(r'Database drain (\{[^\n]+\})', text)
        assert match, 'drain evidence missing'
        drains[config] = json.loads(match[1])
        assert drains[config]['accepted'] == drains[config]['completed'] and drains[config]['rejected'] == 0
    evidence = {
        'schema_version': 1, 'generated_utc': datetime.now(timezone.utc).isoformat(),
        'base_commit': subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        'branch': subprocess.check_output(['git','branch','--show-current'],cwd=ROOT,text=True).strip(),
        'preserved_stash': '012034e978eb94333c4e84be57baae6b5d532194',
        'environment': {'os': platform.platform(), 'cpu': 'AMD Ryzen 5 7500F', 'logical_cpus': 12, 'python': platform.python_version(), 'compiler': 'MSVC v145 x64 C++20', 'database': 'SQL Server 2025 Express LocalDB; ODBC Driver 17'},
        'controlled_boundary': {'description': 'old locking boundary replay versus current production OrderedWorkQueue/LobbyDispatcher; requested sleep 10ms x8, observed full duration retained; one warmup pair then five alternating pairs; no SQL engine throughput claim', 'raw': benchmark, 'summary': controlled},
        'real_database': integrations, 'real_release_medians': real_median,
        'startup_failure_tests': startup, 'core_tests': cores, 'protocol_abi': {'types':110,'constants':106,'changed':False},
        'network_regression': regression, 'shutdown_drains': drains,
        'source_sha256': hashes, 'binary_sha256': binaries,
        'source_set_sha256': hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest(),
        'latest_incremental_build_warning_codes': sorted(warning_codes),
        'initial_validation_failures': ['test codec used SC_CUSTOMIZE instead of SC_MODEL_CUSTOMIZE; corrected', 'checked direct DELETE returned SQL_NO_DATA for zero rows; accepted as normal and regression added'],
        'limits': ['production DB and migrations not tested','blockchain and full GUI gameplay not tested','same-account multi-login cache synchronization not solved','one DB worker, no durable queue or automatic retries','expired listing item restoration still requires policy','client initial auction page 1 differs from zero-based paging buttons'],
    }
    target = ROOT / 'docs/development/evidence/database-improvement-20261029.json'
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    plt.rcParams.update({'font.family':'Malgun Gothic','font.size':11,'axes.spines.top':False,'axes.spines.right':False,'svg.fonttype':'none'})

    def svg(fig):
        buffer=io.StringIO()
        fig.savefig(buffer,format='svg',bbox_inches='tight',facecolor='white')
        plt.close(fig)
        return '\n'.join(line.rstrip() for line in buffer.getvalue()[buffer.getvalue().index('<svg'):].splitlines())

    fig, axes=plt.subplots(1,3,figsize=(13,3.0))
    for ax,key,title in zip(axes,['caller_block_ms','other_session_ms','total_ms'],['로비 작업의 점유 시간','다른 세션의 응답 대기','전체 작업 완료 시간']):
        values=[controlled[key]['before']['median'],controlled[key]['after']['median']]
        ax.barh(['이전 경계','개선 큐'],values,color=['#a7533d','#087e8b'],height=.52)
        ax.invert_yaxis()
        ax.set_xlim(0,160)
        ax.set_xlabel('ms / 작을수록 좋음')
        ax.set_title(title,pad=16)
        ax.grid(axis='x',alpha=.15)
        for i,v in enumerate(values):
            ax.text(v+2,i,f'{v:.4f}' if v<1 else f'{v:.2f}',va='center',fontsize=10,fontweight='bold')
    fig.tight_layout(w_pad=2)
    comparison_chart=svg(fig)
    fig,ax=plt.subplots(figsize=(12,2.8))
    y=list(range(5))
    ax.barh([v-.17 for v in y],[row['sql_blocked_session_ms'] for row in real],height=.3,color='#a7533d',label='SQL 잠금을 기다린 세션')
    ax.barh([v+.17 for v in y],[row['unrelated_session_ms'] for row in real],height=.3,color='#087e8b',label='다른 세션의 채널 변경')
    for i,row in enumerate(real):
        ax.text(row['sql_blocked_session_ms']+2,i-.17,f"{row['sql_blocked_session_ms']:.2f}",va='center',fontsize=9)
        ax.text(row['unrelated_session_ms']+2,i+.17,f"{row['unrelated_session_ms']:.3f}",va='center',fontsize=9,color='#087e8b')
    ax.set_yticks(y,[f'{i+1}회' for i in y]); ax.invert_yaxis()
    ax.set_xlim(0,290); ax.set_xlabel('ms / 두 계열은 같은 축, 0에서 시작')
    ax.legend(loc='upper center',bbox_to_anchor=(.5,1.22),ncol=2,frameon=False)
    fig.tight_layout()
    real_chart=svg(fig)
    before=controlled['caller_block_ms']['before']['median']; after=controlled['caller_block_ms']['after']['median']
    improvement=(before-after)/before*100
    rows=''.join(f'<tr><td>{title}</td><td>{controlled[key]["before"]["median"]:.4f}</td><td>{controlled[key]["after"]["median"]:.4f}</td><td>{controlled[key]["after"]["min"]:.4f}–{controlled[key]["after"]["max"]:.4f}</td></tr>' for key,title in [('caller_block_ms','로비 작업 점유'),('other_session_ms','다른 세션 응답'),('total_ms','전체 작업 완료')])
    code_before='''// 이전 경계: CDataBaseThread::ThreadFunction
auto global = GLobbyDispatcher->LockGlobal();
m_dataBase->SavePlayerInfo(ev.m_id, ev.m_playerInfo);
// 조회는 Packet handler가 GetDataBase()로 직접 실행
// 쓰기는 wakeUpTime 기반 priority queue 사용'''
    code_after='''// 현재 경계: CClient::RunDatabase
GLobbyDispatcher->Suspend(m_workQueue);
database->Submit([snapshot](CDataBase& db) {
    auto result = query(db);     // 로비 잠금 밖 SQL
    ResumeWithGeneration(result); // 세션 맨 앞에서 완료 적용
});'''
    report=f'''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>DB 실행 준비와 비동기 처리 개선</title>
<style>
:root{{color-scheme:light;--ink:#142a3d;--muted:#536775;--line:#d8e3e9;--teal:#087e8b;--bad:#a7533d}}*{{box-sizing:border-box}}body{{margin:0;background:#edf3f6;color:var(--ink);font:16px/1.7 "Malgun Gothic",sans-serif}}main{{max-width:1180px;margin:auto;padding:42px 28px 70px}}h1{{font-size:clamp(28px,4vw,43px);line-height:1.25;margin:12px 0 18px}}h2{{font-size:26px;margin:0 0 12px}}h3{{font-size:18px;margin:0 0 8px}}p{{margin:8px 0 16px}}.eyebrow{{letter-spacing:.12em;font-size:12px;font-weight:bold;color:var(--teal)}}.lead{{max-width:900px;font-size:19px}}.cards,.columns{{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:16px;margin:24px 0}}.card,section{{background:#fff;border:1px solid var(--line);border-radius:16px;padding:24px}}.card strong{{font-size:29px;display:block;color:var(--teal);line-height:1.4}}.card span,.note{{font-size:14px;color:var(--muted)}}section{{margin:22px 0}}.columns{{grid-template-columns:repeat(2,minmax(0,1fr))}}.columns>div{{min-width:0}}pre{{white-space:pre-wrap;overflow-wrap:anywhere;background:#142a3d;color:#e5f6fa;padding:18px;border-radius:10px;font-size:13px;line-height:1.7}}code{{overflow-wrap:anywhere}}a{{color:#006f83;text-underline-offset:3px}}table{{width:100%;border-collapse:collapse;font-size:14px}}td,th{{text-align:left;padding:12px 10px;border-bottom:1px solid var(--line);vertical-align:top}}th{{background:#edf5f7}}figure{{margin:24px 0}}figure svg{{width:100%;height:auto}}figcaption{{color:var(--muted);font-size:13px}}.pill{{display:inline-block;border:1px solid #a4cbd2;padding:3px 10px;border-radius:99px;font-size:12px;margin-right:6px}}.warning{{border-left:4px solid #b97726;padding:10px 16px;background:#fff6e8}}footer{{font-size:13px;color:var(--muted)}}@media(max-width:700px){{main{{padding:24px 14px}}.cards,.columns{{grid-template-columns:1fr}}section{{padding:18px}}table{{font-size:12px}}td,th{{padding:9px 5px}}}}
</style><main><header><div class="eyebrow">WAR OF DIMENSION / 2026.10.29 / LOCAL_TEST</div><h1>DB 준비는 한 번에,<br>SQL 대기는 서버 작업 밖으로</h1><p class="lead">DSN 등록과 소스 매크로 수정을 없애고, 모든 DB 작업을 전용 FIFO로 이동했다. DB 결과를 적용한 뒤 같은 세션의 다음 요청을 실행한다.</p>
<div class="cards"><div class="card"><span>실행 준비 · 설치된 LocalDB/ODBC 기준</span><strong>DSN 1 → 0</strong><span>소스 DB 스위치 수정도 1곳 → 0곳<br>초기 준비 스크립트 1회, 실행 시 -Database</span></div><div class="card"><span>제어 실험 · 로비 작업 점유 중앙값</span><strong>{before:.2f} → {after:.4f} ms</strong><span>{improvement:.3f}% 감소 · 각 5회<br>SQL 엔진 속도 향상을 뜻하지 않음</span></div><div class="card"><span>실제 SQL 잠금 5회 · 다른 세션 중앙값</span><strong>{real_median['unrelated_session_ms']:.3f} ms</strong><span>DB 요청 세션은 {real_median['sql_blocked_session_ms']:.2f}ms 대기<br>DB 통합 20/20 · 예상 SQL 실패 주입 2건</span></div></div>
<p class="warning">전체 DB 작업 완료 시간은 약 125ms로 유지됐다. 단일 DB worker의 처리량 한계, 같은 계정의 복수 로그인 동기화, 운영 DB·블록체인·GUI 플레이 검증은 남아 있다.</p></header>
<section id="cause"><span class="pill">02 원인</span><h2>별도 DB 스레드가 있어도 로비 전체가 기다렸다</h2><p>로그인·조회는 패킷 처리 중 ODBC를 직접 호출했다. 쓰기 worker도 SQL 실행 동안 <code>LockGlobal()</code>을 잡았고, 조회와 쓰기는 같은 연결에 다른 경로로 접근했다. 이벤트 시각만 비교하는 priority queue에는 동일 시각의 안정된 순서 계약이 없었다.</p><div class="columns"><div><h3>이전 코드의 핵심 경계</h3><pre>{html.escape(code_before)}</pre></div><div><h3>실패가 생기는 예</h3><p>연속 구매 뒤 조회가 이전 쓰기를 기다리지 않거나, 느린 SQL이 채널 전체의 작업을 막을 수 있었다. 종료 시 전체 PlayerInfo snapshot을 저장하면 DB에서 확정된 최신 잔액도 오래된 값으로 덮어쓸 수 있었다.</p><p class="note">코드 근거: CDataBaseThread / CPacketMgr / CClient. 이전 상태는 현재 브랜치의 기반 커밋에서 추출했다.</p></div></div></section>
<section id="solution"><span class="pill">03 해결</span><h2>SQL 실행·세션 순서·결과 적용의 책임을 분리했다</h2><div class="columns"><div><pre>{html.escape(code_after)}</pre><p class="note">위 코드는 실행 경계를 요약한 의사 코드다. 실제 함수는 <code>CClient::RunDatabase</code>, <code>LobbyDispatcher::Suspend/Resume</code>, <code>CDataBaseThread::Submit</code>이다.</p></div><div><h3>유지하는 계약</h3><p>전용 worker가 ODBC 연결을 단독 소유한다. mutex 아래 수락된 순서대로 실행하며, 요청 값은 복사하고 완료는 세션 큐 맨 앞에서 generation을 확인한다. 현재 작업보다 빨리 완료가 도착하는 경합도 검사했다.</p><p>상한 4096건, 조건변수 대기, 종료 시 수락 작업 drain. 구매는 트랜잭션 확정 뒤 성공 응답을 보내며 실패는 rollback 후 연결을 종료한다. SQL 로그인·질의 timeout은 각각 5초다.</p></div></div><p><a href="../diagrams/database-async/implementation.html">확대 가능한 실행 구조도</a> · <a href="../diagrams/database-async/README.md">잠금·큐·소유권을 선택한 이유와 비용</a></p>
<h3>로컬 실행</h3><pre>./scripts/Initialize-LocalDatabase.ps1
./scripts/Start-Local.ps1 -ServersOnly -Database
./scripts/Stop-Local.ps1</pre><p class="note">처음 한 번 LocalDB·ODBC x64 설치가 필요하다. 스크립트는 전용 로컬 스키마와 저장 프로시저 16개를 준비하고 .runtime에 연결 설정을 저장한다. 기본 실행은 계속 DB disabled이며, 환경변수 WOD_DB_CONNECTION / WOD_DB_CONFIG로 별도 연결을 지정할 수 있다. 운영 스키마의 자동 마이그레이션은 수행하지 않는다.</p></section>
<section id="measurement"><span class="pill">04 실제 측정</span><h2>대기 격리 효과와 남은 처리 비용을 함께 측정했다</h2><p>Windows / Ryzen 5 7500F 12 threads / MSVC v145 x64 Release. 제어 실험은 <code>sleep_for(10ms)</code> 8번을 같은 입력으로 실행하고, 워밍업 1쌍 이후 전후 순서를 바꿔 5쌍 측정했다. 실제 Windows 대기 시간을 측정했으며 요청 지연 80ms를 실측값으로 대신하지 않았다.</p><figure>{comparison_chart}<figcaption>이전 잠금 경계를 재현한 코드와 현재 production 큐·dispatcher의 제어 실험. 과거 서버 바이너리·실제 SQL 엔진의 전후 비교는 아니다. 모든 축은 0부터 시작하고 단위는 ms다.</figcaption></figure>
<table><thead><tr><th>제어 실험 / ms</th><th>이전 중앙값</th><th>개선 중앙값</th><th>개선 5회 범위</th></tr></thead><tbody>{rows}</tbody></table><p class="note">표본이 적으므로 p99·최대 수용량을 추정하지 않았다. 총 완료 시간의 작은 차이는 DB 처리량 향상으로 해석하지 않는다.</p>
<h3>실제 서버 + ODBC + SQL Server LocalDB</h3><p>전용 NewWodTest의 auction_data를 250ms 동안 잠근 상태에서 A 세션은 DB 조회 → 채널 변경을 전송했다. B 세션은 별도 채널 변경을 전송했다. 5회 모두 A의 조회 응답이 A의 후속 응답보다 먼저 도착했고 B는 SQL 대기 중 응답했다.</p><figure>{real_chart}<figcaption>개선 후 실제 TCP 응답 시간. 이전 서버의 실측값과 합치지 않았다. 로컬 DB 잠금을 고의로 만든 기능·대기 격리 검사이며 전체 게임 부하 시험은 아니다.</figcaption></figure>
<table><thead><tr><th>검증</th><th>실제 결과</th><th>판정 범위</th></tr></thead><tbody>
<tr><td>서버 빌드·Core</td><td>Debug / Release PASS, 각각 {cores['Release']['checks']} checks</td><td>FIFO·포화 거부·drain·예외 후 진행·빠른/느린 완료 포함</td></tr>
<tr><td>TCP→ODBC→LocalDB</td><td>Debug / Release 각각 20/20</td><td>로그인/중복/오류 인증, 연속 구매, 외형, 스테이킹, 경매, rollback, 재접속, timeout</td></tr>
<tr><td>SQL timeout / 오류</td><td>약 5초 종료, 예상 실패 2건</td><td>성공 응답 금지·rollback·다음 작업 진행을 확인</td></tr>
<tr><td>설정 실패</td><td>Debug / Release 각각 3/3</td><td>잘못된 mode·연결 누락·드라이버 누락에서 종료 코드 1, 자원 0</td></tr>
<tr><td>기존 네트워크 회귀</td><td>Release 7개 그룹 PASS</td><td>분할/잘못된 프레임 12검사, 4인 입장 2종, 초기화 실패 4종</td></tr>
<tr><td>프로토콜·종료</td><td>ABI 110종 / 106상수 동일</td><td>DB 수락/완료 {drains['Release']['accepted']}/{drains['Release']['completed']}, 예상 실패 {drains['Release']['failed']}건 포함; pending/sockets/leased=0</td></tr>
</tbody></table><p><a href="evidence/database-improvement-20261029.json">전체 표본·테스트·소스/실행 파일 SHA-256</a> · <a href="DATABASE_IMPROVEMENT.md">재현 명령·검증 근거·경고</a></p></section>
<section id="remaining"><span class="pill">05 추가 발견 / 대응</span><h2>남은 범위</h2><table><thead><tr><th>항목</th><th>영향</th><th>상태 / 다음 대응</th></tr></thead><tbody>
<tr><td>복수 로그인·다중 서버</td><td>같은 계정의 메모리 잔액 동기화와 프로세스 간 순서는 별도 문제</td><td>미구현 / 계정 소유권 또는 버전 기반 동기화 필요. DB 트랜잭션이 잔액/재고의 실패를 막음</td></tr>
<tr><td>추가 검토: 실제 재현</td><td>만료 경매 재고 1→0→0, 중복 로그인 구매 응답 1900 / 실제 DB 1800</td><td>격리 LocalDB·TCP에서 확인 / 원자적 만료 정산·계정 권위부터 개선. <a href="DATABASE_IMPROVEMENT.md">상세 검토</a></td></tr>
<tr><td>보상 내구성·세션 적체</td><td>보상 SQL 실패의 영속 재처리 없음. DB 대기 세션 inbox는 별도 상한 없음</td><td>코드 확인 / 거래 ID·중복 방지·완료 기록, 세션별 건수/바이트 제한과 backpressure 필요</td></tr>
<tr><td>단일 DB worker·종료</td><td>느린 SQL 뒤의 DB 요청은 기다림. backlog가 크면 종료 drain이 15초를 넘을 수 있음</td><td>처리량 확대 미측정 / 강제 종료 복구·내구성 큐·idempotency는 후속 설계</td></tr>
<tr><td>기존 비밀번호·경매 정책</td><td>로컬 스키마는 기존 평문 인증 계약을 재현. 만료 항목의 재고 반환 정책과 클라이언트 최초 page=1도 남음</td><td>운영 적용 전 암호 저장/인증 마이그레이션, 만료 정산과 첫 페이지 계약 정리 필요</td></tr>
<tr><td>빌드 경고</td><td>기존 변환·미사용 변수 경고를 숨기지 않음</td><td>최신 증분 로그: {html.escape(', '.join(sorted(warning_codes)) or '경고가 없는 증분 빌드; 전체 경고 해소를 의미하지 않음')} / 전체 경고 제거는 이번 범위 밖</td></tr>
<tr><td>운영·화면·장시간</td><td>운영 DB·블록체인·GUI 전투·최대 처리량·장시간 장애는 별도 확인 필요</td><td>미검증 / 로컬 기능 검사와 제어 실험 결과로 대체하지 않음</td></tr></tbody></table></section>
<footer>브랜치 codex/db-setup-async. 기존 작업은 stash 012034e978eb94333c4e84be57baae6b5d532194에 보관했다. 커밋·push는 수행하지 않았다.<br>생성: scripts/Build-DatabaseReport.py · 그래프: Matplotlib · 모든 수치는 저장된 원시 JSON에서 계산.</footer></main></html>'''
    output=ROOT/'docs/development/DATABASE_IMPROVEMENT.html'
    output.write_text('\n'.join(line.rstrip() for line in report.splitlines())+'\n',encoding='utf-8')
    print(json.dumps({'html':str(output),'evidence':str(target),'caller_reduction_percent':improvement,'source_set_sha256':evidence['source_set_sha256']},ensure_ascii=False))


if __name__=='__main__':
    main()
