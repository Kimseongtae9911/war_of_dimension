"""연결을 유지한 동접 계단 부하와 단계별 성능 분석. 게임 규칙·입장 정책은 변경하지 않는다."""
import argparse
import asyncio
import copy
import json
import math
import os
import random
import time
from pathlib import Path


def levels(maximum, steps):
    return [value for value in steps if value < maximum] + [maximum]


async def exercise(state, peers, setup_match):
    from protocol import unpack
    from runner import percentile
    config = state.scenario['capacity']
    active = []
    rng = random.Random(state.seed)
    directions = {peer.index: rng.choice((1, 2, 4, 8)) for peer in peers}
    opposite = {1: 2, 2: 1, 4: 8, 8: 4}
    enabled = True
    driver = None

    async def drive():
        loop = asyncio.get_running_loop()
        due, pulse = loop.time(), 0
        while True:
            await asyncio.sleep(max(0, due - loop.time()))
            lag = max(0, loop.time() - due) * 1000
            if enabled and active:
                if state.current_phase == 'setup' and state.capacity_stages:
                    with state.lock:
                        stage = state.capacity_stages[-1]
                        stage['setup_lag_max_ms'] = max(stage.get('setup_lag_max_ms', 0), lag)
                        if lag > 1000:
                            stage['status'] = 'generator-limited'
                            raise RuntimeError('추가 로그인 중 더미 발행 지연 1초 초과')
                if state.current_phase == 'measure':
                    with state.lock:
                        state.load_lags['measure'].append(lag)
                on = pulse % 2 == 0
                await asyncio.gather(*(peer.send('CS_MOVE', (directions[peer.index] if (pulse // 2) % 2 == 0 else opposite[directions[peer.index]]) if on else 0, 0) for peer in tuple(active)))
            pulse += 1
            due += config['interval_s']

    async def verify(peer, marker):
        async def operation():
            state.peer(peer.index, measure_moves=peer.measure_moves, measure_distance=peer.measure_distance)
            if peer.error:
                raise ConnectionError(peer.error)
            if not peer.measure_moves or peer.measure_distance <= .0001:
                raise ValueError('측정 구간 이동 응답/위치 변화 없음')
            if peer.port == 8911:
                await peer.expect('SC_MOVE_PLAYER', marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == peer.player_id and unpack('SC_MOVE_PLAYER', body)[4] == 0)
            else:
                deadline = asyncio.get_running_loop().time() + peer.timeout
                while True:
                    sequence = peer.sequence
                    await asyncio.sleep(.4)
                    if peer.error:
                        raise ConnectionError(peer.error)
                    if peer.moves.get(peer.player_id, (0, None))[0] <= sequence:
                        break
                    if asyncio.get_running_loop().time() >= deadline:
                        raise TimeoutError('로비 최종 정지 관찰 timeout')
        try:
            await peer.check('capacity-move-stop', operation)
            return True
        except Exception:
            return False

    try:
        driver = asyncio.create_task(drive())
        for count in levels(state.clients, config['steps']):
            stage = {'clients': count, 'status': 'setup', 'started_ms': time.time_ns() // 1_000_000}
            with state.lock:
                state.capacity_stages.append(stage)
            try:
                state.phase('setup')
                enabled = True
                size = 4 if state.scenario['setup'] == 'match' else 1
                for start in range(len(active), count, size):
                    group = peers[start:start + size]
                    if size == 4:
                        await setup_match(group, state.scenario)
                    else:
                        await group[0].login(8910)
                    active.extend(group)
                    if driver.done():
                        driver.result()
                stage.update(ready_clients=len(active), connected=state.counters['connected'])
                state.phase('warmup')
                await asyncio.sleep(config['warmup_s'])
                for peer in active:
                    peer.measure_moves, peer.measure_distance, peer.measure_position = 0, 0, None
                lag_start = len(state.load_lags['measure'])
                sent_start = state.counters['sent_packets']
                cpu_start, wall_start = time.process_time(), time.perf_counter()
                state.phase('measure')
                stage['measure_ms'] = state.phases[-1]['timestamp_ms']
                stage['status'] = 'measure'
                await asyncio.sleep(config['duration_s'])
                if driver.done():
                    driver.result()
                state.phase('verify')
                stage['verify_ms'] = state.phases[-1]['timestamp_ms']
                wall = time.perf_counter() - wall_start
                enabled = False
                # Stop emission before markers; queued TCP writes precede the final stop.
                driver.cancel()
                await asyncio.gather(driver, return_exceptions=True)
                lags = state.load_lags['measure'][lag_start:]
                stage.update(issued_packets=state.counters['sent_packets'] - sent_start,
                             observer_cpu_core_percent=(time.process_time() - cpu_start) / wall * 100,
                             generator_lag_ms={'samples': len(lags), 'p99': percentile(lags, .99), 'max': max(lags, default=None)})
                markers = [peer.sequence for peer in active]
                await asyncio.gather(*(peer.send('CS_MOVE', 0, 0) for peer in active))
                results = await asyncio.gather(*(verify(peer, marker) for peer, marker in zip(active, markers)))
                stage.update(verified_clients=sum(results), movement_clients=sum(peer.measure_moves > 0 and peer.measure_distance > .0001 for peer in active))
                if not all(results):
                    raise RuntimeError(f'이동/정지 실패 {results.count(False)}/{len(active)}명')
                if stage['generator_lag_ms']['p99'] is None or stage['generator_lag_ms']['p99'] > config['interval_s'] * 1000 or stage['observer_cpu_core_percent'] >= 90:
                    stage['status'] = 'generator-limited'
                    raise RuntimeError('부하 생성기 한계: 발행 p99가 입력 주기 초과 또는 관측/더미 CPU 0.9코어 이상')
                stage['status'] = 'passed'
                enabled = True
                driver = asyncio.create_task(drive())
            except BaseException as error:
                with state.lock:
                    stage.update(status=stage['status'] if stage['status'] == 'generator-limited' else 'failed', error=str(error) or type(error).__name__, ready_clients=len(active), connected=state.counters['connected'])
                raise
            finally:
                stage['finished_ms'] = time.time_ns() // 1_000_000
    finally:
        if driver:
            driver.cancel()
            await asyncio.gather(driver, return_exceptions=True)
        state.phase('cleanup')


def analyze_result(result):
    from baseline import summarize_run
    from tick_profile import summarize_profile
    output = []
    for stage in result.get('capacity_stages', []):
        row = copy.deepcopy(stage)
        if 'measure_ms' in row and 'verify_ms' in row:
            view = dict(result, clients=row['clients'], status=row['status'], error=row.get('error'),
                        phases=[{'phase': 'measure', 'timestamp_ms': row['measure_ms']}, {'phase': 'verify', 'timestamp_ms': row['verify_ms']}])
            summary = summarize_run(view)
            row.update(servers=summary['servers'], measured_observer_cpu_core_percent=summary.get('observer_cpu_core_percent'))
            measured = [sample for sample in result['samples'] if row['measure_ms'] <= sample['timestamp_ms'] <= row['verify_ms']]
            for role in row['servers']:
                samples = [server for sample in measured for server in sample['servers'] if server['role'] == role and row['measure_ms'] <= server.get('timestamp_ms', 0) <= row['verify_ms']]
                if len(samples) >= 2:
                    seconds = (samples[-1]['uptime_ms'] - samples[0]['uptime_ms']) / 1000
                    if seconds > 0:
                        row['servers'][role]['sent_bytes_per_s'] = (samples[-1]['sent_bytes'] - samples[0]['sent_bytes']) / seconds
                    matches = [server['match_observation']['fresh_active'] for server in samples if 'match_observation' in server]
                    if matches:
                        row['servers'][role]['fresh_matches_min_max'] = [min(matches), max(matches)]
            if 'measurement_error' in summary:
                row['measurement_error'] = summary['measurement_error']
            if result['scenario']['setup'] == 'match':
                try:
                    row['profile'] = summarize_profile(view)
                    game = row['servers'].get('GameServer', {})
                    hist = game.get('tick_duration_us') or {}
                    row['hz_per_match'] = hist.get('samples', 0) / game['seconds'] / (row['clients'] / 4) if 'seconds' in game else None
                except (KeyError, TypeError, ValueError) as error:
                    row['profile_error'] = str(error)
        output.append(row)
    return output


def main():
    from baseline import api, script, write
    from protocol import ROOT
    from tick_profile import fingerprint
    import hashlib
    import platform
    import shutil
    import subprocess
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scenario', choices=['capacity-match', 'capacity-lobby'], required=True)
    parser.add_argument('--clients', type=int, required=True)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--profile', action='store_true')
    parser.add_argument('--headless', action='store_true', help='NPC/월드 시각 관측을 끄고 이동/정지 검증과 프레임 수신 유지')
    args = parser.parse_args()
    if args.clients < 1 or (args.scenario == 'capacity-match' and args.clients % 4):
        parser.error('game 인원은 4의 양수 배수, lobby는 양수')
    if any((ROOT / '.runtime' / p).exists() for p in ('server-lab.json', 'processes.json')):
        parser.error('실행 중인 서버 기록 존재')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    hashes = fingerprint()
    hashes.update({str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'Server/Lobby_Server').glob('*') if p.suffix in ('.cpp', '.h')})
    hashes.update({str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'Server/Lobby_Server/Resource').glob('*') if p.is_file()})
    diff = subprocess.check_output(['git', 'diff', 'HEAD', '--binary'], cwd=ROOT)
    (output/'source.patch').write_bytes(diff)
    for relative in hashes:
        if Path(relative).suffix in ('.py', '.cpp', '.h', '.json', '.exe'):
            destination = output/'source-snapshot'/relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT/relative,destination)
    write(output/'metadata.json', {'commit': subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(), 'diff_sha256':hashlib.sha256(diff).hexdigest(), 'platform':platform.platform(), 'logical_processors':os.cpu_count(), 'python':sys.version, 'fingerprints':hashes, 'configuration':'Release x64 LOCAL_TEST', 'uncap_admission':True, 'profile':args.profile, 'world_observation':not args.headless, 'scenario':args.scenario, 'clients':args.clients, 'seed':args.seed})
    os.environ['WOD_TICK_PROFILE'] = '1' if args.profile else '0'
    os.environ['WOD_LAB_HEADLESS'] = '1' if args.headless else '0'
    report = {'ok': False, 'shutdown_ok': False}
    try:
        script('Start-ServerLab.ps1', output, '-Configuration','Release','-UncapAdmission','-Directory',output/'lab')
        time.sleep(4)
        write(output/'idle.json',api('/api/state'))
        run = api('/api/run',{'scenario':args.scenario,'clients':args.clients,'seed':args.seed})
        deadline = time.monotonic()+1800
        last = None
        while True:
            state = api('/api/progress')
            stages = (state.get('run') or {}).get('capacity_stages',[])
            progress = [(stage['clients'],stage['status'],stage.get('ready_clients')) for stage in stages]
            if progress != last:
                print(json.dumps(progress),flush=True)
                last = progress
            if not state['busy']:
                break
            if time.monotonic()>deadline:
                api('/api/stop',{})
                raise TimeoutError('동접 탐색 전체 1800초 제한')
            time.sleep(1)
        result = json.loads((output/'lab/runs'/f"{run['run_id']}.json").read_text(encoding='utf-8'))
        report.update(status=result['status'], error=result.get('error'), run_id=run['run_id'], stages=analyze_result(result), connected_end=result['counters']['connected'])
        report['ok']=result['status']=='passed' and report['connected_end']==0 and all('measurement_error' not in row and 'profile_error' not in row for row in report['stages'])
    except Exception as error:
        report['infrastructure_error']=str(error) or type(error).__name__
    finally:
        record=ROOT/'.runtime/server-lab.json'
        if record.exists() and json.loads(record.read_text(encoding='utf-8-sig')).get('Directory')==str(output/'lab'):
            try:
                script('Stop-ServerLab.ps1',output)
                report['shutdown_ok']=True
            except Exception as error:
                report['shutdown_error']=str(error)
        for role in ('LobbyServer','GameServer'):
            for suffix in ('out.log','err.log'):
                source=ROOT/'artifacts/logs'/f'run-{role}.{suffix}'
                if source.exists(): shutil.copy2(source,output/source.name)
            log=output/f'run-{role}.err.log'
            if not log.exists() or 'ServerCore stop pending=0 sockets=0 leased=0' not in log.read_text(encoding='utf-8',errors='replace'):
                report['shutdown_ok']=False
        report['fingerprints_unchanged']=all((ROOT/p).exists() and hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==value for p,value in hashes.items())
        report['ok'] &= report['shutdown_ok'] and report['fingerprints_unchanged']
        write(output/'report.json',report)
    print(json.dumps({k:v for k,v in report.items() if k!='stages'},ensure_ascii=False),flush=True)
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
