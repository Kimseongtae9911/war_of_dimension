"""2026-10-13 로비 이동 예약 개선의 실제 비교 결과로 HTML/근거 문서를 만든다."""
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


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def runs(name):
    result = read(LOGS / name / 'summary.json')
    assert result['fingerprints_unchanged'], name
    assert all(row['ok'] and row['shutdown_ok'] and row['connected_end'] == 0 for row in result['runs']), name
    return result['runs']


def metric(rows, name):
    if name == 'p99_ms':
        return [row['servers']['LobbyServer']['packet_job_wait_us']['p99_us_upper'] / 1000 for row in rows]
    return [row['servers']['LobbyServer'][name] for row in rows]


def received_motion(rows, directory):
    output = []
    for row in rows:
        raw = read(LOGS / directory / f"{row['seed']:02d}-perf-lobby-128/lab/runs/{row['run_id']}.json")
        phases = {entry['phase']: entry['timestamp_ms'] for entry in raw['phases']}
        seconds = (phases['verify'] - phases['measure']) / 1000
        output.append(statistics.mean(peer['measure_moves'] for peer in raw['peers']) / seconds)
    return output


def main():
    before = runs('lobby-throughput-short-after-20261013')
    after_all = runs('lobby-movement-short-after-20261013')
    after = [row for row in after_all if row['clients'] == 128]
    assert len(before) == len(after) == 5
    soak_before = [row for row in runs('lobby-throughput-soak-after-20261013') if row['clients'] in (64, 128)]
    soak_after = runs('lobby-movement-soak-after-20261013')
    assert len(soak_before) == len(soak_after) == 2
    assert {row['clients'] for row in soak_after} == {64, 128}
    assert all(row['settlement']['status'] == 'passed' for row in soak_before + soak_after)
    names = ['packets_per_s', 'p99_ms', 'cpu_percent', 'send_frames_per_s']
    values = {name: {variant: {'median': statistics.median(metric(rows, name)), 'min': min(metric(rows, name)), 'max': max(metric(rows, name))}
                     for variant, rows in [('before', before), ('after', after)]} for name in names}
    observer = {variant: statistics.median(row['observer_cpu_core_percent'] for row in rows) / 100 for variant, rows in [('before', before), ('after', after)]}
    motion = {variant: statistics.median(received_motion(rows, directory)) for variant, rows, directory in [('before', before, 'lobby-throughput-short-after-20261013'), ('after', after, 'lobby-movement-short-after-20261013')]}
    idle = read(LOGS / 'lobby-movement-idle-20261013.json')
    idle_before, idle_after = [next(row['CpuPercent'] for row in idle if row['Variant'] == variant) for variant in ('before', 'after')]
    font = {'font.family': 'DejaVu Sans', 'font.size': 11}
    plt.rcParams.update(font)
    fig, axes = plt.subplots(1, 3, figsize=(12, 3.5), layout='constrained')
    for ax, name, title in zip(axes, names[:3], ['Packets / s (higher is better)', 'Packet wait p99 upper (ms)', 'Server CPU (% of 12 cores)']):
        data = [values[name][variant]['median'] for variant in ('before', 'after')]
        ax.bar(['Terrain baseline', 'Movement'], data, color=['#8593a6', '#0b968d'], width=.6)
        ax.set_title(title, fontsize=11)
        ax.set_ylim(0, max(data) * 1.25)
        ax.spines[['top', 'right']].set_visible(False)
        for index, value in enumerate(data):
            ax.text(index, value + max(data) * .035, f'{value:,.2f}', ha='center', fontsize=11)
    image = io.BytesIO()
    fig.savefig(image, format='png', dpi=150)
    plt.close(fig)
    encoded = base64.b64encode(image.getvalue()).decode()
    (LOGS / 'lobby-movement-comparison.png').write_bytes(image.getvalue())
    cpu_before, cpu_after = [values['cpu_percent'][variant]['median'] for variant in ('before', 'after')]
    p99_before, p99_after = [values['p99_ms'][variant]['median'] for variant in ('before', 'after')]
    details = ''.join(f'<tr><td>{name}</td><td>{values[name]["before"]["median"]:,.2f}<br><small>{values[name]["before"]["min"]:,.2f}–{values[name]["before"]["max"]:,.2f}</small></td><td>{values[name]["after"]["median"]:,.2f}<br><small>{values[name]["after"]["min"]:,.2f}–{values[name]["after"]["max"]:,.2f}</small></td></tr>' for name in names)
    soak_table, soak_md = '', []
    for count in (64, 128):
        old = next(row for row in soak_before if row['clients'] == count)
        new = next(row for row in soak_after if row['clients'] == count)
        a, b = old['servers']['LobbyServer'], new['servers']['LobbyServer']
        row = (f'{count}명', f'{a["packets_per_s"]:.2f} → {b["packets_per_s"]:.2f}',
               f'{a["packet_job_wait_us"]["p99_us_upper"]/1000:.2f} → {b["packet_job_wait_us"]["p99_us_upper"]/1000:.2f}',
               f'{a["cpu_percent"]:.2f} → {b["cpu_percent"]:.2f}',
               f'{a["pending_packets_end"]} → {b["pending_packets_end"]}',
               f'{old["settlement"]["elapsed_ms"]/1000:.3f} → {new["settlement"]["elapsed_ms"]/1000:.3f}')
        soak_table += '<tr>' + ''.join(f'<td>{cell}</td>' for cell in row) + '</tr>'
        soak_md.append('| ' + ' | '.join(row) + ' |')
    source_names = ['lobby-throughput-short-after-20261013', 'lobby-movement-short-after-20261013', 'lobby-throughput-soak-after-20261013', 'lobby-movement-soak-after-20261013']
    (LOGS / 'lobby-movement-analysis-20261013.json').write_text(json.dumps({'short_128': values, 'observer_cores': observer, 'own_nonzero_frames_mean_per_s': motion, 'short_before': before, 'short_after': after_all, 'soak_before': soak_before, 'soak_after': soak_after}, ensure_ascii=False, indent=2), encoding='utf-8')
    html = f'''<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>로비 이동 예약 개선</title>
<style>body{{margin:0;background:#edf2f6;color:#1b2938;font:16px/1.65 'Segoe UI','Malgun Gothic',sans-serif}}main{{max-width:1120px;margin:auto;padding:40px 24px}}section{{background:white;border:1px solid #d9e2e9;border-radius:16px;padding:24px;margin:22px 0}}h1{{font-size:32px;margin:0}}h2{{font-size:23px;margin:0 0 14px}}p{{margin:10px 0}}.cards{{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin-top:20px}}.card{{background:#ecf7f5;padding:18px;border-radius:12px}}strong.value{{font-size:26px;display:block}}img{{width:100%;height:auto}}pre{{white-space:pre-wrap;background:#172536;color:#dcf1ef;padding:18px;border-radius:10px;font-size:14px}}table{{width:100%;border-collapse:collapse;font-size:14px}}td,th{{text-align:left;border-bottom:1px solid #e0e7ed;padding:10px}}small,.muted{{color:#566a7c}}a{{color:#087e80}}details{{margin-top:16px}}@media(max-width:700px){{.cards{{grid-template-columns:1fr}}main{{padding:20px 12px}}section{{padding:16px}}table{{font-size:12px}}td,th{{padding:6px}}}}</style><main>
<h1>로비 이동 예약 개선</h1>
<section id="result"><h2>1. 결과</h2><p>128명의 입력 처리율을 유지하면서 CPU와 패킷 대기 p99 상한을 줄였다. CPU 감소에는 빈 타이머 반복 제거의 영향이 크다.</p>
<div class="cards"><div class="card">CPU · 20초 5회 중앙값<strong class="value">{cpu_before:.2f}% → {cpu_after:.2f}%</strong>{cpu_before-cpu_after:.2f}%p 감소</div><div class="card">패킷 대기 p99 bucket 상한<strong class="value">{p99_before/1000:.3f}s → {p99_after/1000:.3f}s</strong>실제 RTT와 구분</div><div class="card">128명 처리율<strong class="value">{values['packets_per_s']['after']['median']:.2f}패킷/s</strong>기준 {values['packets_per_s']['before']['median']:.2f}패킷/s</div></div>
<p class="muted">128명 유지 검사 끝 대기는106→131개로 늘었고, 이후 정리를 확인했다. 본인 이동 응답은 유저당 평균 {motion['before']:.2f}→{motion['after']:.2f}개/s다. 입력 즉시 응답도 포함하므로 실제 좌표 변화율은 아니다. 최대 동접·66.7Hz 유지·화면 품질 개선을 입증한 결과는 아니다.</p></section>
<section id="cause"><h2>2. 원인</h2><p>이동 갱신이 끝나자마자 자신을 다시 큐에 넣었다. 1ms 미만 작업도 반복됐고, 빠른 이동→정지→이동 입력은 갱신 체인을 두 개 만들 수 있었다.</p>
<pre>// 이전 흐름 · 개념 코드
MovePacket: 이전 방향 == 0 → 갱신 U1 추가
move → stop → move가 먼저 처리됨 → 갱신 U2도 추가
U1 실행 → U3 / U2 실행 → U4 ...
타이머 빈 큐 → yield 반복</pre><p>체인 중복은 코드 흐름으로 확인한 경로이며 기존 체인 수를 직접 계측한 결과는 아니다. 초기 이동 시각과 동일 shared_mutex 중첩 잠금도 함께 수정했다.</p></section>
<section id="solution"><h2>3. 해결 방식</h2><p>유저당 예약/실행 대기 하나만 유지한다. 타이머는 위치를 수정하지 않고 기존 FIFO에 generation 보호 작업을 넣는다.</p>
<pre>// 현재 흐름 · 개념 코드
입력: 지난 방향 이동 적분 → 새 방향 → 본인 응답 → dirty
예약: pending이면 추가하지 않음 / 15ms 뒤 FIFO에 한 번
갱신: 최신 이동·정지 상태를 주변에 전달
완료: 이동 중이면 다음 한 번 / 정지면 종료</pre>
<p>속도는 정규화한 방향 × 최대 속도로 계산해 갱신 횟수 의존을 제거했다. 게임 물리·wire 형식·공통 JobQueue는 유지했다. <a href="../diagrams/lobby-movement/movement.html">예약·정지 흐름도</a></p></section>
<section id="measurement"><h2>4. 실제 측정</h2><p>Release LOCAL_TEST·DB 없음·입장 제한 해제·단일 채널. 같은 seed 1–5, 5초 워밍업·20초 이동/정지(250ms) 반복. 기준은 1차 지형 개선 적용 상태다.</p>
<img src="data:image/png;base64,{encoded}" alt="128명 5회 중앙값 처리율·패킷 대기 p99 상한·CPU 비교">
<details><summary>128명 중앙값과 최소–최대 · 전후 각5회</summary><table><tr><th>지표</th><th>지형 기준</th><th>이동 개선</th></tr>{details}</table></details>
<p>idle·계측 OFF·연결0·전후 각5초1회: CPU {idle_before:.2f}→{idle_after:.2f}%(표본 해상도 내). CPU 감소의 큰 부분은 빈 타이머의 busy loop 제거와 일치한다.</p>
<p>더미/관측 CPU 중앙값은 {observer['before']:.3f}→{observer['after']:.3f}코어다. 128명120초 개선 측정도 {next(row['observer_cpu_core_percent'] for row in soak_after if row['clients']==128)/100:.3f}코어로0.9코어 여유 기준을 넘었으므로 서버 최대 동접을 확정할 수 없다.</p>
<p>120초 유지·최종 입력 정리: 전후 각 인원1회. 측정 끝 대기는128명에서106→131개로 늘었으며, 이후 마지막 입력 모두 처리·패킷/송신 대기0인 서로 다른 표본3개를 확인했다.</p><table><tr><th>동접</th><th>패킷/s</th><th>p99 상한 ms</th><th>CPU %</th><th>측정 끝 대기</th><th>정리 s</th></tr>{soak_table}</table>
<p>실제 소켓: 분할 이동·본인/주변 최종 정지·200개 입력 순서·갱신 중단·재시작·이동 중 재접속10회 통과. 측정 이후 예약 콜백을 함수로 분리한 코드도 빌드·기능 회귀를 통과했으며, 함수 분리 이후 성능은 재측정하지 않았다. 빌드·Core/ABI·통합 검증 상세는 <a href="LOBBY_MOVEMENT.md">검증 근거</a>에 기록한다.</p></section>
<section id="followup"><h2>5. 남은 대응</h2><table><tr><th>항목</th><th>영향·상태</th><th>다음 대응</th></tr>
<tr><td>주변 전송·단일 worker</td><td>과부하에서 15ms 주기 미유지</td><td>시야/송신 비용 분해·프레임 묶음</td></tr>
<tr><td>클라이언트 보간·외삽</td><td>timestamp 단위·정지 보간 및 전체 GUI 미검증</td><td>입력 sequence·서버 보정과 함께 개편</td></tr>
<tr><td>충돌 품질</td><td>지연 시 긴 이동의 끝점 판정은 기존 방식</td><td>연속 충돌·최대 적분 간격 검토</td></tr>
<tr><td>측정 경계</td><td>더미 CPU 포화 가능·120초 반복1회</td><td>분산 부하·교차 반복·SLO 확정</td></tr>
<tr><td>문서 시각 검증</td><td>로컬 파일 접근 제한 이력·브라우저 미검증</td><td>제한 해소 후 전체 레이아웃 확인</td></tr></table></section></main></html>'''
    (ROOT / 'docs/development/LOBBY_MOVEMENT.html').write_text(html, encoding='utf-8')
    md = f'''# 로비 이동 예약 개선 (2026-10-13)

[HTML 보고서](LOBBY_MOVEMENT.html) · [흐름도](../diagrams/lobby-movement/README.md)

## 결과·비교 조건

기준은 1차 지형 개선 커밋 `90a9e79f`의 로비 실행 파일이며, 그때 확보한 동일 시나리오 실측을 재사용한다. 이전 지형 선형 탐색 버전과 섞지 않는다. 새 개선은 같은 브랜치의 미커밋 구현이다.
Release x64 LOCAL_TEST·DB 없음·입장 제한 해제·단일 채널·원본 OBJ·headless·계측 ON/상세 프로파일 OFF. 12논리 코어 CPU 정규화 비율이다.
20초 검사 전후128명 각5회(seed1–5), 새64명5회는 새 구현 제어 검사이며 같은 시간의 이전64명 값은 없어 직접 비교하지 않는다.
120초 유지 전후64/128명 각각1회(seed3). 워밍업5초, 이동/정지250ms. 최대 동접 검사로 해석하지 않는다.

128명20초 중앙값: 처리율 {values['packets_per_s']['before']['median']:.2f}→{values['packets_per_s']['after']['median']:.2f}패킷/s, 패킷 대기 p99 bucket 상한 {p99_before:.3f}→{p99_after:.3f}ms, CPU {cpu_before:.2f}→{cpu_after:.2f}%.
세부 중앙값/범위와 원본 행은 `artifacts/logs/lobby-movement-analysis-20261013.json`에 보존한다. histogram bucket 상한은 네트워크 응답 지연이 아니다.
더미/관측 CPU 중앙값은 {observer['before']:.3f}→{observer['after']:.3f}코어다. 기준은0.9코어 여유 기준을 넘었다. 같은 PC의 생성기/관측 한계를 서버 최대 동접으로 해석하지 않는다.
유저당 방향0이 아닌 본인 응답 평균의 실행별 중앙값은 {motion['before']:.3f}→{motion['after']:.3f}개/s다. 개선 후에는 입력 즉시 응답도 포함하므로 실제 위치 변경/화면 갱신 빈도와 다르다. 66.7Hz 유지나 시각 품질 개선을 주장하지 않는다.

| 동접 | 처리율 패킷/s | 패킷 대기 p99 상한 ms | CPU % | 측정 끝 패킷 대기 | 최종 정리 s |
|---|---|---|---|---|---|
{chr(10).join(soak_md)}

## 구현과 계약

`CClient::ApplyMovementInput`은 입력 이전 방향의 지난 이동을 적분하고 새 방향/속도를 적용한 뒤 본인에게 좌표를 전송한다. 본인 응답은 입력 순서를 유지하며 고유 입력 sequence ACK는 아니다.
`ScheduleMovementUpdate`의 플래그는 단일 content worker만 변경한다. 예약 시각은 steady_clock이다. 타이머는 위치/방향을 읽지 않고 기존 유저 FIFO에만 작업을 추가한다.
`RunMovementUpdate`는 세션 generation이 일치할 때만 실행한다. pending 해제→최신 상태 broadcast→다음 한 번 예약 순서이며 밀린 횟수의 작업을 재생하지 않는다.
빠른 입력의 중간 상태는 주변에 병합될 수 있다. 최종 방향0은 한 번 전달하고 예약을 종료한다. 타이머 대기/작업 큐 체류 시간만큼 주변 반영이 늦을 수 있다.
`CNetworkMgr::PrepareStop`은 mutex 아래 중단 플래그와 CV wake를 처리한다. producer join 이후 CClient를 해제하며 옛 generation 콜백은 새 사용자 상태를 수정하지 않는다.
`CPhysic`은 로비 방향을 정규화한 목표 속도(현재10 units/s)로 해석한다. 이전 갱신당 속도 누적·단위 벡터 감속은 제거했다. 게임 이동·전투 물리는 변경하지 않았다.
`CViewList`의 동일 shared_mutex 재잠금을 제거했다. 나머지 채널 순회/시야/전송 구조와 공통 큐는 유지한다.

## 제외한 후보와 기능 검사

입력마다 주변 전체 전송한 첫 후보는128명322.38패킷/s·p99상한16.78초로 악화되어 제외했다(`lobby-movement-pilot-20261013`).
dirty 상태 병합 뒤 예비값513.22패킷/s·p99상한0.524초(`lobby-movement-pilot2-20261013`)는 최종 반복 통계에 포함하지 않는다.
주기 변경 시 이전 속도 누적 때문에 이동 거리가 감소하는 현상을 확인해 목표 속도로 수정했다. 초기 forward 검사는 스폰 근처 NavMesh 경계에 막혀1.18units였으므로 같은 위치의 backward로 전후를 비교했다.
실제 소켓 검사는 `tools/server_lab/tests/lobby_movement.py`다. 기존 실행 파일의 `--legacy` 검사는 새 정지 응답을 요구하지 않아 전후 동일 기능 검증으로 주장하지 않는다.
분할 입력·실제 유한 좌표/거리·본인/주변 방향0·200개 입력 응답 순서·정지 뒤 갱신 중단·다시 이동·이동 중 재접속10회를 검사한다. 이전 세션 객체의 재사용 자체를 강제로 유도한 검사는 아니며 generation 보호는 기존 Core 검사와 코드 경계를 함께 근거로 한다.

## 원시 근거·재현

{chr(10).join('- `artifacts/logs/' + name + '`' for name in source_names)}
- 기능 검사: `artifacts/logs/lobby-movement-functional-*.json`
- 기준 실행 파일: `artifacts/logs/lobby-movement-before-binaries-20261013/Lobby_Server.exe` (SHA-256 `e7852fa56a06f4fe4f09e6b9582c58462b5d779f0a2ba0e50e47b56bb0cd897e`)

```powershell
python scripts/Measure-CapacityBoundary.py --scenario perf-lobby --clients 64 128 --repeats 5 --headless --output artifacts/logs/lobby-movement-short-new
python scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/lobby-movement-soak-new
```

各 실행 metadata/source snapshot/hash와 shutdown 로그를 보존한다. 최종 입력 정리는 격리 서버의 총 처리량/3개 빈 표본을 사용하며 wire별 ACK가 아니다.
계측의 `movement_interval_us`·`movement_single_pending` 및 타이머/로직 지연 계약은 [Server Lab](SERVER_LAB.md#로비-이동-예약-2026-10-13)을 따른다. 이전1ms 목표와 현재15ms 목표의 tick_delay는 직접 비교하지 않는다.

## 검증 한계·후속

Release 서버 빌드, Core1,479검사·ABI110종/106상수, 네트워크12종·보스4/5 매칭·초기화 실패4종(통합7그룹), Python49검사 통과.
최종 실제 소켓3항목(재접속10회 포함)·로비4명 lobby-cycle20 assertion 통과. 마지막 기능 검사에서 약0.6초 backward 이동5.922units·방향0이 아닌 본인 응답28개·본인 정지 수신0.361ms를 관측했다(각1회, 로컬 환경).
기존 실행 파일은5.909units·590응답이었고 새 정지 응답은 없어 비교하지 않았다. 실제 유지 시간/타이머 오차가 있으므로 정확한 속도 등가 실험으로 해석하지 않는다.
실제 GET /api/state의 admission에서 이동 정책15000us/단일 대기true를 확인했다. 최종 종료 로그 양쪽pending=0 sockets=0 leased=0 및 소유 실행 기록 제거를 확인했다.
측정 완료 뒤 CPhysic/CServer의 빈 줄·줄바꿈을 정리하고 재빌드/기능 검증했다. 이후 ScheduleMovementUpdate의 중첩 람다를 EnqueueMovementUpdate/ExecuteScheduledMovementUpdate로 추출했으며 예약·세션 보유·generation 잠금·계측 경계는 유지했다. 함수 추출 뒤 Release 로비 빌드와 실제 소켓3항목(200개 입력 순서·이동 중 재접속10회 포함)을 통과했다. 근거는 `artifacts/logs/lobby-movement-lambda-refactor-20261013.json`이며 함수 추출 이후 성능은 재측정하지 않았다. 각 실측은 해당 metadata의 정확한 source snapshot/실행 파일 hash를 근거로 한다.
기존 빈 switch의C4065는 타이머 switch 제거로 사라졌고 최종 증분 서버 빌드에는 경고가 보고되지 않았다. 전체 클라이언트/clean rebuild의 기존 경고까지 해소했다는 뜻은 아니다.

idle·계측OFF·연결0·전후 각5초1회 CPU는 {idle_before:.2f}→{idle_after:.2f}%였다(`artifacts/logs/lobby-movement-idle-20261013.json`). 0%는 해당5초의 CPU 시간 표본 해상도 내 값이며 영구적으로 비용이 없다는 뜻이 아니다. 부하 CPU 감소의 큰 부분은 빈 타이머 busy loop 제거와 일치한다.
128명120초 개선의 더미/관측 CPU는 {next(row['observer_cpu_core_percent'] for row in soak_after if row['clients']==128)/100:.3f}코어로0.9코어 기준을 넘었다. 측정 끝 패킷 대기106→131개 증가는 숨기지 않으며 이후3개 빈 표본으로 정리를 확인했다.

DB/블록체인·전체 GUI 플레이·클라이언트의 시각적 이동 품질·다중 채널 병렬화·연속 충돌·운영 장시간 안정성·최대 동접은 미검증이다.
Archify showcase9/9 검증과 브라우저 시각 검증은 구분한다. 이 대화의 로컬 파일 접근 차단 이력으로 전체 보고서/다이어그램 브라우저 확인은 수행하지 않았다.
실제 기능/통합 검사 결과와 더미 CPU·갱신률의 세부 한계는 완료 검증 기록을 추가해 관리한다.
'''
    (ROOT / 'docs/development/LOBBY_MOVEMENT.md').write_text(md.replace('各 실행', '각 실행'), encoding='utf-8')


if __name__ == '__main__':
    main()
