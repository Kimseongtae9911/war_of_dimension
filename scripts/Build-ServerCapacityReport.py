"""동접 탐색과 경계 반복의 원시 보고서에서 한국어 HTML/Markdown을 생성한다."""
import base64
import html
import json
from pathlib import Path
import statistics

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
LOGS = ROOT/'artifacts/logs'
OUT = ROOT/'docs/development'
CHARTS = LOGS/'capacity-report-20261013'


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def number(value, digits=2):
    return '미수집' if value is None else f'{value:,.{digits}f}'


def hist(row, role, key, field='mean_us', divisor=1000):
    value = (row.get('servers',{}).get(role,{}).get(key) or {}).get(field)
    return value / divisor if value is not None else None


def quality(row):
    server=row.get('servers',{}).get('GameServer',{})
    lag=(row.get('generator_lag_ms') or {}).get('p99')
    wait=(server.get('packet_job_wait_us') or {}).get('p99_us_upper')
    observer=row.get('observer_cpu_core_percent')
    return bool(row.get('ok') and row.get('connected_end')==0 and not row.get('peer_errors') and not row.get('profile_error')
                and (row.get('hz_per_match') or 0)>=60 and lag is not None and lag<=25 and wait is not None and wait<=50000
                and observer is not None and observer<90)


def throughput_ok(row):
    rate=row.get('servers',{}).get('LobbyServer',{}).get('packets_per_s')
    lag=(row.get('generator_lag_ms') or {}).get('p99')
    cpu=row.get('observer_cpu_core_percent')
    return bool(row.get('ok') and (row.get('settlement') or {}).get('status')=='passed'
                and rate is not None and rate>=row['clients']*4*.99 and lag is not None and lag<=250 and cpu is not None and cpu<90)


def table(headers,rows):
    return '<table><thead><tr>'+''.join(f'<th>{html.escape(x)}</th>' for x in headers)+'</tr></thead><tbody>'+''.join('<tr>'+''.join(f'<td>{html.escape(str(v))}</td>' for v in row)+'</tr>' for row in rows)+'</tbody></table>'


def markdown_table(headers, rows):
    return '\n'.join(['| '+' | '.join(headers)+' |','|'+'---|'*len(headers),*['| '+' | '.join(str(v).replace('|','/').replace('\n',' ') for v in row)+' |' for row in rows]])+'\n\n'


def image(path,alt):
    return '<img alt="'+html.escape(alt)+'" src="data:image/png;base64,'+base64.b64encode(path.read_bytes()).decode()+'">'


def main():
    CHARTS.mkdir(exist_ok=True)
    roots={'game':LOGS/'capacity-ramp-game-headless-20261013','lobby':LOGS/'capacity-ramp-lobby-20261013'}
    ramps={name:load(root/'report.json') for name,root in roots.items()}
    boundaries={}
    for name in ('game','lobby'):
        path=LOGS/f'capacity-boundary-{name}-20261013/summary.json'
        if path.exists(): boundaries[name]=load(path)['runs']
    controls={}
    for name in ('profileoff','metricsoff'):
        path=LOGS/f'capacity-control-{name}-20261013/summary.json'
        if path.exists(): controls[name]=load(path)['runs']
    soak=[]
    for name in ('capacity-soak-lobby-20261013','capacity-soak-refine-20261013'):
        path=LOGS/name/'summary.json'
        if path.exists(): soak.extend(load(path)['runs'])
    soak.sort(key=lambda row:row['clients'])
    sustained=max((row['clients'] for row in soak if throughput_ok(row)),default=0)
    plt.rcParams.update({'font.family':'Malgun Gothic','axes.unicode_minus':False,'font.size':10})
    fig,axes=plt.subplots(1,3,figsize=(13,3.8),layout='constrained')
    game=[row for row in ramps['game']['stages'] if row.get('hz_per_match') is not None]
    for axis,values,title in [(axes[0],[r['hz_per_match'] for r in game],'게임 갱신 (Hz/매치)'),
                              (axes[1],[r['servers']['GameServer']['cpu_percent'] for r in game],'게임 CPU (12논리 스레드 정규화 %)'),
                              (axes[2],[hist(r,'GameServer','tick_delay_us') for r in game],'게임 tick 예약 지연 (ms)')]:
        axis.plot([r['clients'] for r in game],values,'o-',color='#0284c7')
        axis.set_xscale('log',base=2)
        axis.set_xticks([r['clients'] for r in game],[str(r['clients']) for r in game])
        axis.set_ylim(bottom=0)
        axis.set_title(title)
        axis.set_xlabel('실제 준비 완료 동접')
        axis.grid(alpha=.2)
    axes[0].axhline(60,linestyle='--',color='#b45309',label='임시 품질 기준60Hz')
    axes[0].legend(fontsize=8)
    fig.savefig(CHARTS/'capacity.png',dpi=150)
    plt.close(fig)
    fig,axis=plt.subplots(figsize=(10,3.6),layout='constrained')
    timing=[r for r in game if hist(r,'GameServer','tick_timer_lateness_us') is not None and hist(r,'GameServer','tick_dispatch_wait_us') is not None]
    x=list(range(len(timing)))
    timer=[hist(r,'GameServer','tick_timer_lateness_us') for r in timing]
    dispatch=[hist(r,'GameServer','tick_dispatch_wait_us') for r in timing]
    axis.bar(x,timer,label='예정 → Post 직전',color='#0284c7')
    axis.bar(x,dispatch,bottom=timer,label='Post 직전 → MatchUpdate',color='#f59e0b')
    axis.set_xticks(x,[f"{r['clients']}명" for r in timing])
    axis.set_ylabel('실행된 tick의 평균 지연 (ms)')
    axis.legend()
    axis.set_title('타이머 전달 전과 IOCP 처리 대기 분리')
    fig.savefig(CHARTS/'delay.png',dpi=150)
    plt.close(fig)
    lobby=[r for r in ramps['lobby']['stages'] if r.get('servers',{}).get('LobbyServer')]
    fig,axes=plt.subplots(1,2,figsize=(11,3.6),layout='constrained')
    axes[0].plot([r['clients'] for r in lobby],[hist(r,'LobbyServer','packet_job_wait_us','p99_us_upper') for r in lobby],'o-',color='#dc2626')
    axes[0].set_ylabel('패킷 대기 p99 버킷 상한 (ms)')
    axes[0].set_yscale('log',base=2)
    ticks=sorted({hist(r,'LobbyServer','packet_job_wait_us','p99_us_upper') for r in lobby})
    axes[0].set_yticks(ticks,[f'{value:,.0f}' for value in ticks])
    axes[0].set_xscale('log',base=2)
    axes[0].set_xticks([r['clients'] for r in lobby],[str(r['clients']) for r in lobby])
    axes[1].bar([str(r['clients']) for r in lobby],[r['servers']['LobbyServer']['pending_packets_end'] for r in lobby],color='#dc2626')
    axes[1].set_ylabel('측정 끝 미처리 패킷 수')
    for axis in axes:
        axis.set_xlabel('동접 (128명 단계 기능 실패)')
        axis.grid(axis='y',alpha=.2)
    fig.savefig(CHARTS/'lobby.png',dpi=150)
    plt.close(fig)
    tables={}
    maxima={}
    for name,role in [('game','GameServer'),('lobby','LobbyServer')]:
        rows=[]
        passed=[]
        for row in ramps[name]['stages']:
            values=row.get('servers',{}).get(role,{})
            expected_matches=[row['clients']//4]*2
            if row['status']=='passed' and not any(key in row for key in ('measurement_error','profile_error')) and (name!='game' or values.get('fresh_matches_min_max')==expected_matches):
                passed.append(row['clients'])
            rows.append([row['clients'],row.get('ready_clients','미수집'),row['status'],number(row.get('hz_per_match')) if name=='game' else '—',
                         number(values.get('cpu_percent')),number(hist(row,role,'packet_job_wait_us','p99_us_upper')),number(row.get('observer_cpu_core_percent')),
                         number((row.get('generator_lag_ms') or {}).get('p99'))])
        maxima[name]=max(passed,default=0)
        tables[name]=table(['요청','준비','판정','Hz/매치','서버 CPU %','패킷 대기 p99 ≤ ms','더미 core %','발행 p99 ms'],rows)
    grouped=[]
    safe=[]
    for count in sorted({row['clients'] for row in boundaries.get('game',[])}):
        rows=[r for r in boundaries['game'] if r['clients']==count]
        valid=[r for r in rows if r.get('hz_per_match') is not None]
        passed=sum(quality(r) for r in rows)
        if len(rows)>=5 and passed==len(rows): safe.append(count)
        grouped.append([count,f"{sum(bool(r.get('ok')) for r in rows)}/{len(rows)}",f'{passed}/{len(rows)}',
                        number(statistics.median(r['hz_per_match'] for r in valid)) if valid else '미수집',
                        f"{number(min(r['hz_per_match'] for r in valid))}–{number(max(r['hz_per_match'] for r in valid))}" if valid else '미수집'])
    boundary_table=table(['동접','기능/종료 성공','임시 품질 충족','Hz 중앙값','Hz 범위'],grouped)
    lobby_grouped=[]
    for count in sorted({row['clients'] for row in boundaries.get('lobby',[])}):
        rows=[r for r in boundaries['lobby'] if r['clients']==count]
        waits=[hist(r,'LobbyServer','packet_job_wait_us','p99_us_upper') for r in rows]
        waits=[v for v in waits if v is not None]
        pending=[r['servers']['LobbyServer']['pending_packets_end'] for r in rows if r.get('servers',{}).get('LobbyServer',{}).get('pending_packets_end') is not None]
        lobby_grouped.append([count,f"{sum(bool(r.get('ok')) for r in rows)}/{len(rows)}",
                              number(statistics.median(waits)) if waits else '미수집',
                              number(statistics.median(pending),0) if pending else '미수집',
                              '; '.join(f"seed{r['seed']}: {r.get('error') or r.get('infrastructure_error') or r.get('measurement_error')}" for r in rows if not r.get('ok')) or '없음'])
    lobby_boundary_table=table(['동접','기능/종료 성공','패킷 대기 p99 상한 중앙값 ms','끝 미처리 중앙값','실패'],lobby_grouped)
    control_rows=[]
    for label,rows in [('계측/구간 프로파일 켬',[r for r in boundaries.get('game',[]) if r['clients']==16]),
                       ('계측 켬/구간 프로파일 끔',controls.get('profileoff',[])),('계측/구간 프로파일 끔',controls.get('metricsoff',[]))]:
        cpus=[r.get('servers',{}).get('GameServer',{}).get('cpu_percent') for r in rows]
        cpus=[v for v in cpus if v is not None]
        hz=[r['hz_per_match'] for r in rows if r.get('hz_per_match') is not None]
        control_rows.append([label,f"{sum(bool(r.get('ok')) for r in rows)}/{len(rows)}",number(statistics.median(cpus)) if cpus else '미수집',
                             f'{number(min(cpus))}–{number(max(cpus))}' if cpus else '미수집',number(statistics.median(hz)) if hz else '미계측'])
    control_table=table(['16명·매회 재시작','기능/종료 성공','게임 CPU 중앙값 %','CPU 범위 %','Hz 중앙값'],control_rows)
    soak_rows=[]
    soak_display=[]
    for row in soak:
        drain=row.get('settlement') or {}
        lobby=row.get('servers',{}).get('LobbyServer',{})
        soak_rows.append([row['clients'],row.get('ok'),number(lobby.get('packets_per_s')),throughput_ok(row),
                          lobby.get('pending_packets_end','미수집'),number(hist(row,'LobbyServer','packet_job_wait_us','p99_us_upper')),
                          drain.get('status','미수집'),number(drain.get('elapsed_ms')),row.get('error') or row.get('infrastructure_error') or '없음'])
        elapsed=drain.get('elapsed_ms')
        soak_display.append([row['clients'],f"{number(lobby.get('packets_per_s'))} / {row['clients']*4}",
                             '통과' if throughput_ok(row) else '미달',lobby.get('pending_packets_end','미수집'),
                             number(hist(row,'LobbyServer','packet_job_wait_us','p99_us_upper')),
                             f"{ {'passed':'통과','failed':'실패'}.get(drain.get('status'),'미수집')} / {number(elapsed/1000 if elapsed is not None else None)}초"])
    soak_table=table(['동접','처리/입력 패킷/s','처리량 유지','측정 끝 대기','p99 상한 ms','최종 정리/시간'],soak_display)
    profile_rows=[]
    for row in game:
        stages=row.get('profile',{}).get('stages',{})
        profile_rows.append([row['clients'],*[number(stages.get(key,{}).get('cycle_share_percent')) for key in ('jobs','minion_update','height_ray','nav_ray','player_send')]])
    profile_table=table(['동접','Jobs %','미니언 %','HeightRay %','NavRay %','플레이어 송신 %'],profile_rows)
    safe_text=str(max(safe))+'명' if safe else '5회 모두 충족한 인원 없음'
    findings=[]
    for name in ramps:
        tail=ramps[name]['stages'][-1]
        findings.append(f"{name}: {tail['clients']}명 목표 중 실제 준비{tail.get('ready_clients','미수집')}명, {tail['status']}; {tail.get('error','탐색 상한 도달')}; 서버 종료 {ramps[name]['shutdown_ok']}")
    evidence={'ramps':ramps,'boundaries':boundaries,'controls':controls,'soak':soak,'sustained_lobby_observed':sustained,'max_passed':maxima,'game_quality_all_repeats':safe,'findings':findings}
    (CHARTS/'summary.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2),encoding='utf-8')
    body=f'''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>실제 서버 동접 한계와 병목 조사</title><style>
body{{font:16px/1.65 'Malgun Gothic',sans-serif;color:#172b4d;background:#f5f7fb;max-width:1150px;margin:auto;padding:28px}}h1,h2{{line-height:1.3}}section{{background:white;border:1px solid #dce3ec;border-radius:12px;padding:22px;margin:18px 0}}.cards{{display:flex;gap:12px;flex-wrap:wrap}}.cards div{{background:#e0f2fe;padding:14px 20px;border-radius:8px}}strong{{font-size:23px}}table{{border-collapse:collapse;width:100%;font-size:13px}}td,th{{border-bottom:1px solid #dce3ec;text-align:left;padding:8px}}pre{{white-space:pre-wrap;background:#eef2f7;padding:14px;font-size:14px}}img{{max-width:100%;height:auto}}.scroll{{overflow-x:auto}}a{{color:#0369a1}}.warning{{color:#9a3412}}
</style><h1>실제 서버 동접 한계 · 병목 조사</h1>
<section><h2>1. 결과</h2><p>게임512명에서11.9Hz, 로비128명에서는 입력 중단20초 뒤에도2,742개 패킷이 남았습니다. 아래 값은 이 PC·시나리오의 관측값이며 운영 최대 동접 보장이 아닙니다.</p>
<div class="cards"><div>게임 누적 이동 확인<br><strong>{maxima['game']}명</strong></div><div>로비120초 입력 처리량 유지<br><strong>{sustained}명</strong></div><div>게임16/20/24명 · 60Hz<br><strong>{safe_text}</strong></div></div><p>게임 누적 탐색·로비120초 유지는 각1회입니다. 더미 제한에 먼저 도달하면 서버의 절대 최대 동접을 확정하지 않습니다. 로비 처리량 유지도 낮은 지연을 보장하지 않습니다.</p>
<p class="warning">{html.escape(' / '.join(findings))}</p><p>최초 전체 월드 관측 실행은 더미 포화/제어 지연으로 중단했습니다. 이후 headless 결과는 NPC 계산을 계속 실행하면서 시각 관측 비용만 끈 별도 조건입니다.</p></section>
<section><h2>2. 분석한 원인</h2><p>실행 시간과 예약 대기를 분리했습니다. 타이머 전달 전 대기가 큰 구간과 IOCP 처리 대기가 커지는 구간을 아래 그래프로 확인할 수 있습니다. CPU 전체 비율만으로 여유가 있다고 판단할 수 없습니다.</p>
{image(CHARTS/'delay.png','동접별 타이머와 IOCP 대기 분해')}
<pre>// 현재 게임 타이머 경로 (핵심 흐름)
이벤트 pop → 아직 미래이면 다시 push → sleep_for(1ms)
기한 도착 → Post → IOCP 작업 시작 → 게임 tick
다음 tick = 현재 시각 + 15ms</pre>
<p>로비는 IOCP 4스레드와 별개로 JobScheduler를 단 한 스레드에서 실행합니다. 이동은 자신을 재등록하며 지형 충돌·채널 전체 가시성 검사·근처 플레이어 송신을 수행합니다. 전체 CPU가 낮아도 이 처리 경로에는 대기가 쌓일 수 있습니다.</p>
{image(CHARTS/'lobby.png','로비 동접별 패킷 대기와 미처리 패킷 누적')}
<pre>// Lobby CServer::Run / CClient::ProcessUpdate
producers.Launch([] {{ GPacketJobQueue-&gt;ProcessJob(); }}); // 1개
Move(); // 충돌 + 채널 전체 순회 + 근처 대상 송신
if (이동 중) {{ PushJob(ProcessUpdate); AddSessionQueue(this); }}</pre>
<p>같은 누적 실행의 tick CPU cycle 비중입니다. 미니언 내부에 HeightRay/NavRay가 포함되므로 열을 합산하지 않습니다. JobQueue 내부 실행 비중만으로 모든 스케줄링 비용을 설명할 수는 없습니다.</p><div class="scroll">{profile_table}</div></section>
<section><h2>3. 적용한 대응과 다음 개선</h2><p>이번에는 측정 신뢰도를 보강했습니다. 게임 이동·지형·타이머 정책 최적화는 적용하지 않았습니다.</p><pre>// 관측 도구
NPC 대표 검색: 전체 참가자 순회 → 해당 매치의 4명 조회
수신 처리: 버퍼 소진까지 연속 처리 → 32프레임마다 제어에 양보
진행 조회: 대형 시계열 포함 → /api/progress
대규모 검사: --headless + 동일 플레이어 이동/정지 검증</pre><p>후속 서버 작업: 타이머 대기/깨움 방식 대조, 중복 SendMovePacket 제거 대조, 로비 첫 갱신 시각 초기화와 idle 스레드 점검. 각 변경은 같은 부하에서 독립 재측정해야 합니다. <a href="../diagrams/server-capacity/README.md">측정 흐름도</a></p></section>
<section><h2>4. 실제 측정</h2><p>Ryzen5 7500F·6코어/12스레드·Windows11·Release x64 LOCAL_TEST·DB 없음·입장 소프트 제한 해제. 기준 커밋012d1046에 측정 코드만 추가했습니다. 게임은 4인 매치, 이동/정지 250ms 입력. 누적 단계는 3초 워밍업/20초 측정이며 먼저 생성한 매치가 오래 실행됩니다.</p>
{image(CHARTS/'capacity.png','게임 동접별 갱신률 CPU 예약 지연')}
<h3>게임 누적 증가</h3><div class="scroll">{tables['game']}</div><h3>로비 누적 증가</h3><div class="scroll">{tables['lobby']}</div>
<h3>게임 경계 · 매회 재시작</h3><p>워밍업5초/측정20초·seed별 인원 순서 교차. 임시 품질 기준은60Hz 이상·패킷 대기 p99 bucket 상한50ms 이하·발행 p99 25ms 이하·기능/종료 성공입니다. 제품 SLO가 아닙니다.</p><div class="scroll">{boundary_table}</div>
<h3>로비 경계 · 매회 재시작</h3><div class="scroll">{lobby_boundary_table}</div><p>짧은 기능 통과와 대기열 안정성은 별개입니다. 기존 정지 판정은0.4초 동안 자기 이동 수신이 멎었는지를 보므로 적체 상태의 최종 명령 완료를 보장하지 않습니다.</p>
<h3>로비120초 유지 · 최종 입력 정리</h3><div class="scroll">{soak_table}</div><p>각1회·seed3·워밍업5초·측정120초. 최종 stop 이후20초 내 처리 입력 수 충족, 패킷/송신 대기0인 새 표본3개를 확인합니다. 처리량 유지는 입력 목표(동접×4/s)의99% 이상·정리/기능/종료 성공·더미 lag≤250ms/CPU&lt;0.9코어입니다. 64/128명 확인 후 세분화 전에 정한 진단 기준이며 낮은 지연·장시간 운영 보장은 아닙니다.</p>
<h3>계측 비용 대조</h3><div class="scroll">{control_table}</div><p>동일16명·seed1~5 대조입니다. 계측을 끈 실행은 외부 프로세스 CPU를 사용합니다. 세 묶음을 순차 실행했으므로 OS 스케줄링 변동과 계측 비용을 완전히 분리하지 못하며, 새 두 histogram만의 순수 비용도 아닙니다.</p>
<p>서버 CPU는 전체12논리 스레드 정규화, 더미 core %는1코어 기준입니다. 서버 p99는 histogram bucket 상한이며 정확한 네트워크 RTT가 아닙니다. 더미 lag는 각 발행 주기 시작의 지연입니다. 준비 실패 단계에는 정상 측정 수치를 채우지 않습니다. <a href="SERVER_CAPACITY.md">전체 수치·실패·재현 근거</a></p></section>
<section><h2>5. 추가 결함과 대응 필요 요소</h2>
{table(['항목','영향/근거','다음 대응'],[
['로비 첫 갱신 시각','m_updateTime={} 이후 첫 Login→ProcessUpdate에서 epoch 규모 경과 시간이 histogram에 들어감','Initialize에서 갱신 기준 시각 설정·신규/재접속 비교'],
['로비 idle CPU','게임 부하 중 로비 한 스레드 약0.98코어; 빈 TimerFunc의 yield 루프와 일치하는 가설','스레드 식별/프로파일 뒤 blocking wait 대조'],
['로비 가시성 검사','매 이동마다 채널 전체 순회·근처 대상별 송신; 같은 m_viewLock을 중첩 lock_shared하는 경로도 확인','밀집도별 검사/관심 영역 분할·잠금 책임 단일화; 실제 교착 재현은 미확인'],
['중복 플레이어 송신','동일 SendMovePacket이 연속2회 호출됨','중복 제거 후 수신/송신량·위치/정지 재검증'],
['관측기 한계','전체 월드 관측은 고동접에서 자체 포화 가능','headless 결과와 구분·필요 시 다중 프로세스 생성기'],
['NPC/전투 범위','NPC 애니메이션 완료 ACK·실제 근거리 전투 전체는 미지원','해당 시나리오 추가 후 별도 상한 측정'],
['관측 일정 제한','첫8그룹까지만 연결하던 도구 코드 수정·확장 검사 통과','큰 동접에서 일정·상태 API 계속 확인']])}
</section></html>'''
    (OUT/'SERVER_CAPACITY.html').write_text(body,encoding='utf-8')
    markdown='# 실제 서버 동접 한계 조사 근거\n\n[HTML 보고서](SERVER_CAPACITY.html) · [측정 흐름](../diagrams/server-capacity/README.md).\n\n'
    markdown+='## 결과\n\n'+'\n'.join('- '+x for x in findings)+f'\n\n게임 임시 품질5회 통과: {safe_text}.\n\n'
    markdown+=f'로비120초 입력 처리량 유지 기준의 최대 확인 인원: {sustained}명. 96명부터 입력보다 처리량이 낮아 적체가 관측됐다. 게임512명은128개의4인 매치이며, 596명 확장 중 더미 지연1.484초로 중단해 서버 절대 최대를 확정하지 못했다.\n\n'
    markdown+='## 실행 자료\n\n'
    markdown+='기준 커밋 `012d1046352a35c5edd255e1789800e3db996fed`, main 작업 트리의 추가 계측·더미 도구를 사용했다. Ryzen5 7500F(6코어/12논리 스레드), RAM33,933,996,032bytes, Windows11, 균형 전원 계획, Release x64 LOCAL_TEST, DB 없음. 게임250매치×4명=1000명/로비3000명의 고정 저장 공간은 유지하고 WOD_PERF_UNCAPPED=1로 입장 소프트 제한을 풀었다. 실행마다 metadata의 파일 SHA-256, 소스/실행 파일 snapshot 또는 기존 Git 기준과 source.patch, 종료 상태를 보존했다.\n\n'
    for path in [LOGS/'capacity-probe-game-20261013',LOGS/'capacity-ramp-game-20261013',LOGS/'capacity-ramp-game-v2-20261013',*roots.values(),*(LOGS/f'capacity-boundary-{name}-20261013' for name in boundaries),*(LOGS/f'capacity-control-{name}-20261013' for name in controls)]:
        markdown+=f'- `{path.relative_to(ROOT).as_posix()}`: metadata/fingerprint, report, lab/runs의 원시 표본·판정, 종료 로그.\n'
    markdown+='\n원시 결과는 Git 제외이며 실패/취소도 보존했다. 두 전체 월드 누적 실험은 도구 비용 조사 후 수동 중단했으며 서버 한계 실패로 집계하지 않는다. headless는 NPC 위치 의미 검증을 생략하지만 서버 NPC 처리와 수신/플레이어 검사는 유지한다.\n\n'
    markdown+='## 실행별 수치\n\n'
    for name in ramps:
        markdown+=f'### {name}\n\n| 요청 인원 | 준비 인원 | 상태 | Hz/매치 | 서버 CPU % | tick 실행 ms | 타이머 초과 ms | Post→handler ms | 오류 |\n|---|---|---|---|---|---|---|---|---|\n'
        role='GameServer' if name=='game' else 'LobbyServer'
        for r in ramps[name]['stages']:
            markdown+='| '+' | '.join(map(str,[r['clients'],r.get('ready_clients'),r['status'],number(r.get('hz_per_match')),number(r.get('servers',{}).get(role,{}).get('cpu_percent')),number(hist(r,role,'tick_duration_us')),number(hist(r,role,'tick_timer_lateness_us')),number(hist(r,role,'tick_dispatch_wait_us')),r.get('error','')]))+' |\n'
        markdown+='\n'
    markdown+='## 경계 반복과 계측 비용\n\n'+markdown_table(['게임 동접','기능/종료','임시 품질','Hz 중앙값','Hz 범위'],grouped)
    markdown+=markdown_table(['로비 동접','기능/종료','패킷 대기 p99 상한 중앙값 ms','끝 미처리 중앙값','실패'],lobby_grouped)
    markdown+=markdown_table(['16명 조건','기능/종료','CPU 중앙값 %','CPU 범위 %','Hz 중앙값'],control_rows)
    markdown+='20초 경계 실행은 매번 서버를 재시작하고 seed1~5, 워밍업5초/입력250ms다. 누적 탐색과 달리 준비 중 기존 참가자에게 이동 입력을 주지 않는다. 순차 실행한 계측 켬/끔은 OS 변동을 완전히 통제하지 못하며 두 신규 histogram만의 오버헤드가 아니다. 누락 지표는 0으로 대체하지 않는다.\n\n'
    markdown+='## 로비120초 유지와 최종 입력 정리\n\n'+markdown_table(['동접','성공','처리 패킷/s','처리량 유지','측정 끝 대기','p99 상한 ms','정리 판정','정리 ms','오류'],soak_rows)
    markdown+='`artifacts/logs/capacity-soak-lobby-20261013`(64/128명)·`capacity-soak-refine-20261013`(96/112명)의 각1회 seed3 진단이다. 처리량 유지 기준은 완료 패킷/s가 동접×4/s의99% 이상, 정리/기능/종료 성공, 발행 p99<=250ms/더미 CPU<0.9코어다. 64/128명 결과 이후 세분화 전에 정했으며 낮은 지연을 보장하는 SLO가 아니다. 5초 워밍업/120초 유지 후20초 내 최종 입력 수 이상 처리·패킷/송신 대기0을 새 reporter 표본3개로 확인한다. 기존20초/누적 검사는 이 조건이 없어 성공을 최종 명령 완료 보장으로 해석하지 않는다. 신규 settlement 코드/표본 전달이 포함된 별도 소스 hash를 보존했으며 서버 실행 파일은 동일하다. 운영 장시간·다른 참가자가 있는 서버의 개별 ACK 검증은 아니다.\n\n'
    markdown+='## 원인과 코드 근거\n\n'
    markdown+='- 게임 `CNetworkMgr.cpp` TimerFunc/ProcessTimerEvent/MatchUpdate: 미래 이벤트 pop/push+1ms sleep, 전달 전 초과와 전달 후 대기를 분리. 512명 tick 본문2.44ms에 비해 IOCP 진입 이후 대기61.37ms. 단순 CPU 전체 사용률만으로 판단하지 않는다.\n'
    markdown+='- 게임 `CMatch.cpp` InGameUpdate: 같은 SendMovePacket을 연속 두 번 호출. 제거 효과는 아직 측정하지 않았다.\n'
    markdown+='- 로비 `CServer.cpp` Run: IOCP4개와 별개로 JobScheduler 소비자1개. `CClient.cpp` ProcessUpdate는 이동 중 자신을 다시 등록하고 Move에서 충돌/가시성 검사를 수행한다. `CViewList.cpp`는 채널 전체를 순회한다. 밀집 참가자 증가에 따라 이 단일 경로의 처리량·공정성이 제한되는 구조이며 세부 함수별 CPU 비율은 미계측이다.\n'
    markdown+='- 로비128명 누적 단계: 미처리3802개, 입력 목표512/s 대비 완료418.995/s, 11명의 이동 응답/위치 변화 검증 실패. 20초 동안4명은 자기 이동 응답0회, 7명은1회로 두 위치 간 변화 검증 불가였다. 더미 CPU31.02% of one core·발행 주기 시작 p99 지연0ms로 생성기 포화 판정에 해당하지 않았다.\n'
    markdown+='- 로비 `CClient.h` m_updateTime={} → Initialize에서 미설정 → LoginPacket의 ProcessUpdate → Move: 첫 경과 시간이 epoch 규모로 기록된다. 전체 누적 histogram을 오염시키며 신규 참가자가 없는 측정 구간 차분과 구분해야 한다.\n'
    markdown+='- 로비 `CViewList.cpp` CheckViewList: 호출부가 nearClient의 m_viewLock을 shared 획득하고 내부 CheckViewList(int)도 같은 lock_shared를 수행한다. 중첩 잠금은 코드에서 확인했지만 이번 실패를 교착 때문이라고 확정하지 않는다.\n'
    markdown+='- `capacity-ramp-game-20261013/thread-cpu.jsonl`: 58.56초 동안 로비 최대 스레드 약0.98코어. TimerFunc 빈 큐 yield 루프와 일치하는 가설이며 함수 주소를 식별하지 않아 원인 확정이 아니다.\n\n'
    markdown+=markdown_table(['게임 동접','Jobs CPU cycle %','미니언 %','HeightRay %','NavRay %','플레이어 송신 %'],profile_rows)
    markdown+='## 재현\n\n```powershell\npython tools/server_lab/capacity.py --scenario capacity-match --clients 1000 --seed 3 --profile --headless --output artifacts/logs/capacity-game-new\npython tools/server_lab/capacity.py --scenario capacity-lobby --clients 3000 --seed 3 --headless --output artifacts/logs/capacity-lobby-new\npython scripts/Measure-CapacityBoundary.py --scenario perf-match --clients 16 20 24 --repeats 5 --profile --headless --output artifacts/logs/capacity-boundary-new\npython scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 64 128 --repeats 5 --headless --output artifacts/logs/capacity-lobby-boundary-new\npython scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 96 112 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/capacity-soak-new\n```\n\n'
    markdown+='## 검증 범위\n\nRelease 서버 빌드·Core1479검사/ABI110종·106상수/실제 지형27904비교, Python49검사 통과. 기존 컴파일 경고 유지. 정상 종료는 각 실행 report의 shutdown_ok와 pending=0/sockets=0/leased=0 로그로 판정한다. 실패 후 정상 종료는 미처리 업무 완료를 뜻하지 않으며 settlement 실패를 유지한다. 전체 GUI·운영DB·전투와 HTML 실브라우저 시각 검사는 수행하지 않았다. 보고서 PNG와 HTML 구조/링크는 확인했다. 기본15ms 예약의 이론66.7Hz와 실측 갱신률을 구분한다. 구간 프로파일은 중첩하므로 합산하지 않는다.\n'
    (OUT/'SERVER_CAPACITY.md').write_text(markdown,encoding='utf-8')
    print(json.dumps({'max_passed':maxima,'safe':safe,'findings':findings},ensure_ascii=False))


if __name__=='__main__':
    main()
