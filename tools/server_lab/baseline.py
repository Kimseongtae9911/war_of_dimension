"""LOCAL_TEST 실제 서버를 실행마다 재시작하는 기준 측정. 기능 실패도 원시 결과와 요약에 남긴다."""
import argparse
import collections
import hashlib
import json
import random
import shutil
import statistics
import subprocess
import sys
import time
import urllib.request
from pathlib import Path

from performance import HISTOGRAMS, OPTIONAL_HISTOGRAMS, histogram_delta
from protocol import ROOT

BASE_URL = 'http://127.0.0.1:8790'


def api(path, body=None):
    data = None if body is None else json.dumps(body).encode('utf-8')
    request = urllib.request.Request(BASE_URL + path, data=data, headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(request, timeout=10) as response:
        return json.load(response)


def write(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def script(name, output, *args):
    # 파일로 연결해 숨긴 자식 서버의 상속 handle 때문에 PIPE EOF를 기다리지 않는다.
    with (output / (name + '.log')).open('wb') as log:
        process = subprocess.run(['pwsh', '-NoProfile', '-File', str(ROOT / 'scripts' / name), *map(str, args)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=90)
    if process.returncode:
        raise RuntimeError(f'{name}: exit {process.returncode}')


def summarize_run(result, metrics=True):
    phases = {item['phase']: item['timestamp_ms'] for item in result.get('phases', [])}
    begin, end = phases.get('measure'), phases.get('verify')
    summary = {'status': result['status'], 'error': result.get('error'), 'clients': result['clients'],
               'assertions_passed': result['counters']['passed'], 'assertions_failed': result['counters']['failed'],
               'generator_lag_ms': result.get('generator_lag_ms', {}).get('measure'), 'servers': {}}
    if begin is None or end is None:
        summary['measurement_error'] = '측정 시작/끝 구간 없음'
        return summary
    summary['requested_measurement_ms'] = end - begin
    samples = [item for item in result['samples'] if begin <= item['timestamp_ms'] <= end]
    if len(samples) < 3:
        summary['measurement_error'] = '측정 구간 표본 부족'
        return summary
    elapsed = (samples[-1]['timestamp_ms'] - samples[0]['timestamp_ms']) / 1000
    observer_cpu = samples[-1]['observer_cpu_seconds'] - samples[0]['observer_cpu_seconds']
    summary.update(sample_count=len(samples), sampled_seconds=elapsed, observer_cpu_core_percent=observer_cpu / elapsed * 100,
                   observer_cpu_percent=observer_cpu / elapsed / samples[-1]['logical_processors'] * 100)
    for role in ('LobbyServer', 'GameServer'):
        if not metrics:
            first = samples[0].get('process_cpu', {}).get(role)
            last = samples[-1].get('process_cpu', {}).get(role)
            if not first or not last or first['creation_ticks'] != last['creation_ticks']:
                summary['measurement_error'] = f'{role} 외부 CPU 표본 누락/재시작'
                continue
            core = (last['cpu_ticks'] - first['cpu_ticks']) / 10_000_000 / elapsed * 100
            summary['servers'][role] = {'cpu_core_percent': core, 'cpu_percent': core / samples[-1]['logical_processors']}
            continue
        rows = [next((row for row in sample['servers'] if row['role'] == role), {}) for sample in samples]
        if any(row.get('status') != 'live' for row in rows) or len({row['instance'] for row in rows}) != 1:
            summary['measurement_error'] = f'{role} 누락/stale/재시작'
            continue
        # 중복 snapshot은 제외하고 실제 reporter uptime 차이로 나눈다.
        rows = list({row['sequence']: row for row in rows if begin <= row['timestamp_ms'] <= end}.values())
        if len(rows) < 3:
            summary['measurement_error'] = f'{role} 구간 내부 reporter 표본 부족'
            continue
        first, last = rows[0], rows[-1]
        seconds = (last['uptime_ms'] - first['uptime_ms']) / 1000
        if seconds <= 0:
            summary['measurement_error'] = f'{role} reporter 구간 없음'
            continue
        left, right = first['performance'], last['performance']
        value = {'seconds': seconds, 'jobs_per_s': (last['jobs_completed'] - first['jobs_completed']) / seconds,
                 'packets_per_s': (right['packets_processed'] - left['packets_processed']) / seconds,
                 'move_requests': right['move_requests'] - left['move_requests'], 'stop_requests': right['stop_requests'] - left['stop_requests'],
                 'cpu_percent': (last['cpu_ticks'] - first['cpu_ticks']) / 10_000_000 / seconds / last['logical_processors'] * 100,
                 'cpu_core_percent': (last['cpu_ticks'] - first['cpu_ticks']) / 10_000_000 / seconds * 100,
                 'queued_end': last['jobs_queued'], 'queued_peak': max(row['jobs_queued'] for row in rows),
                 'private_bytes_peak_sampled': max(row['private_bytes'] for row in rows), 'admission': last.get('admission'),
                 'job_failures': last['jobs_failed'] - first['jobs_failed'], 'io_errors': last['io_errors'] - first['io_errors']}
        value.update(pending_packets_end=right['pending_packets']['count'], oldest_pending_packet_us_end=right['pending_packets']['oldest_wait_us'],
                     oldest_pending_packet_us_peak=max(row['performance']['pending_packets']['oldest_wait_us'] for row in rows))
        if all('pending_io' in row for row in rows):
            value.update(pending_io_end=last['pending_io'], pending_io_peak=max(row['pending_io'] for row in rows))
        if all('send_queue' in row for row in rows):
            value.update(send_queue_end=last['send_queue']['count'], send_queue_peak=max(row['send_queue']['count'] for row in rows),
                         max_socket_send_queue_peak=max(row['send_queue']['max_socket_count'] for row in rows))
            for field in ('native_calls', 'batches', 'frames'):
                if field in first['send_queue'] and field in last['send_queue']:
                    value['send_' + field + '_per_s'] = (last['send_queue'][field] - first['send_queue'][field]) / seconds
        for name in HISTOGRAMS:
            value[name] = histogram_delta(left[name], right[name])
        for name in OPTIONAL_HISTOGRAMS:
            value[name] = histogram_delta(left.get(name), right.get(name))
        summary['servers'][role] = value
    return summary


def one_run(output, scenario, clients, seed, metrics=True):
    output.mkdir()
    info = {'scenario': scenario, 'clients': clients, 'seed': seed, 'metrics': metrics, 'status': 'infrastructure-failed', 'shutdown_ok': False}
    started = False
    try:
        started = True
        script('Start-ServerLab.ps1', output, '-Configuration', 'Release', '-UncapAdmission', '-Directory', output / 'lab', *([] if metrics else ['-DisableMetrics']))
        # 3초 이상 idle 표본을 남긴다. 상태도 수집해 설정과 instance를 보존한다.
        time.sleep(4)
        write(output / 'idle.json', api('/api/state'))
        state = api('/api/run', {'scenario': scenario, 'clients': clients, 'seed': seed})
        deadline = time.monotonic() + 180
        while True:
            snapshot = api('/api/state')
            if not snapshot['busy']:
                break
            if time.monotonic() > deadline:
                api('/api/stop', {})
                raise TimeoutError('실제 서버 시나리오 180초 제한')
            time.sleep(1)
        path = output / 'lab/runs' / (state['run_id'] + '.json')
        result = json.loads(path.read_text(encoding='utf-8'))
        info.update(summarize_run(result, metrics), run_id=state['run_id'])
    except Exception as error:
        info['infrastructure_error'] = str(error) or type(error).__name__
    finally:
        record_path = ROOT / '.runtime/server-lab.json'
        owned = record_path.exists() and json.loads(record_path.read_text(encoding='utf-8-sig')).get('Directory') == str(output / 'lab')
        if started and owned:
            try:
                script('Stop-ServerLab.ps1', output)
                info['shutdown_ok'] = True
            except Exception as error:
                info['shutdown_error'] = str(error)
        for role in ('LobbyServer', 'GameServer'):
            for suffix in ('out.log', 'err.log'):
                source = ROOT / 'artifacts/logs' / f'run-{role}.{suffix}'
                if source.exists():
                    shutil.copy2(source, output / source.name)
            log = output / f'run-{role}.err.log'
            if not log.exists() or 'ServerCore stop pending=0 sockets=0 leased=0' not in log.read_text(encoding='utf-8', errors='replace'):
                info['shutdown_ok'] = False
        info['ok'] = info['status'] == 'passed' and info['shutdown_ok'] and 'measurement_error' not in info and 'infrastructure_error' not in info
        write(output / 'report.json', info)
    return info


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clients', type=int, nargs='+', default=[4, 16, 64])
    parser.add_argument('--repeats', type=int, default=5)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--disable-metrics', action='store_true')
    parser.add_argument('--stage', choices=['before', 'after', 'diagnostic'], default='before')
    parser.add_argument('--scenarios', nargs='+', choices=['perf-lobby', 'perf-match'], default=['perf-lobby', 'perf-match'])
    args = parser.parse_args()
    if any(count < 4 or count % 4 for count in args.clients) or not 1 <= args.repeats <= 20:
        parser.error('인원은 4 이상의 4배수, repeats는 1~20')
    if args.output.exists() or (ROOT / '.runtime/server-lab.json').exists() or (ROOT / '.runtime/processes.json').exists():
        parser.error('기존 출력/서버 실행 기록이 있습니다')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True)
    files = [*sorted((ROOT / 'tools/server_lab').glob('*.py')), *sorted((ROOT / 'tools/server_lab/scenarios').glob('perf-*.json')),
             ROOT / 'scripts/Start-ServerLab.ps1', ROOT / 'scripts/Measure-ServerPerformance.ps1', ROOT / 'tools/server_lab/web/index.html']
    files += [ROOT / f'artifacts/bin/Release/{role}/{name}.exe' for role, name in [('LobbyServer', 'Lobby_Server'), ('GameServer', 'Game_Server')]]
    files += [ROOT / name for name in subprocess.check_output(['git', 'diff', '--name-only'], cwd=ROOT, text=True).splitlines() if name.startswith('Server/')]
    machine = json.loads(subprocess.check_output(['pwsh', '-NoProfile', '-Command', "@{Cpu=@(Get-CimInstance Win32_Processor | ForEach-Object {$_.Name}); System=(Get-CimInstance Win32_ComputerSystem | Select-Object NumberOfLogicalProcessors,TotalPhysicalMemory); OS=(Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version); PowerPlan=((& powercfg /getactivescheme)-join ' ')} | ConvertTo-Json -Depth 5"], encoding='utf-8'))
    metadata = {'schema_version': 1, 'machine': machine, 'started_ms': time.time_ns() // 1_000_000, 'branch': subprocess.check_output(['git', 'branch', '--show-current'], cwd=ROOT, text=True).strip(),
                'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                'tracked_diff_sha256': hashlib.sha256(subprocess.check_output(['git', 'diff', 'HEAD', '--binary'], cwd=ROOT)).hexdigest(),
                'hashes': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in files},
                'clients': args.clients, 'repeats': args.repeats, 'python': sys.version, 'configuration': 'Release x64 LOCAL_TEST',
                'admission': 'WOD_PERF_UNCAPPED=1; lobby channel=3000, storage lobby=3000/game=1000, match=4 players',
                'sampling': '1s reporter/monitor; measure interior snapshots; cumulative histogram delta; percentile bucket upper bounds',
                'metrics': not args.disable_metrics, 'optimized': args.stage == 'after', 'stage': args.stage, 'scenarios': args.scenarios}
    write(args.output / 'metadata.json', metadata)
    records = []
    rng = random.Random(20261011)
    for repeat in range(1, args.repeats + 1):
        matrix = [(scenario, count) for scenario in args.scenarios for count in args.clients]
        rng.shuffle(matrix)
        for scenario, count in matrix:
            name = f'{repeat:02d}-{scenario}-{count}'
            print(f'시작 {name}', flush=True)
            record = one_run(args.output / name, scenario, count, repeat, not args.disable_metrics)
            record['repeat'] = repeat
            records.append(record)
            with (args.output / 'runs.jsonl').open('a', encoding='utf-8') as stream:
                stream.write(json.dumps(record, ensure_ascii=False) + '\n')
            print(f'완료 {name}: {record["status"]}, shutdown={record["shutdown_ok"]}', flush=True)
            # 소유한 서버를 정리하지 못했으면 다음 실행의 부하/로그를 오염시키지 않는다.
            if (ROOT / '.runtime/server-lab.json').exists() or (ROOT / '.runtime/processes.json').exists():
                write(args.output / 'summary.json', {'ok': False, 'runs': records, 'error': '종료 후 실행 기록 잔존'})
                return 1
    groups = []
    for scenario in args.scenarios:
        for count in args.clients:
            rows = [row for row in records if row['scenario'] == scenario and row['clients'] == count]
            group = {'scenario': scenario, 'clients': count, 'passed': sum(row['ok'] for row in rows), 'failed': sum(not row['ok'] for row in rows), 'servers': {}}
            for role in ('LobbyServer', 'GameServer'):
                values = [row['servers'][role] for row in rows if role in row.get('servers', {})]
                fields = ('jobs_per_s', 'packets_per_s', 'cpu_percent', 'cpu_core_percent', 'queued_peak', 'pending_packets_end', 'oldest_pending_packet_us_peak',
                          'pending_io_end', 'pending_io_peak', 'send_queue_end', 'send_queue_peak', 'max_socket_send_queue_peak', 'send_native_calls_per_s', 'send_batches_per_s', 'send_frames_per_s')
                group['servers'][role] = {field: {'median': statistics.median(value[field] for value in values), 'min': min(value[field] for value in values), 'max': max(value[field] for value in values)} for field in fields if values and all(field in value for value in values)}
                for hist in HISTOGRAMS:
                    numbers = [value[hist]['p99_us_upper'] for value in values if value.get(hist) and value[hist]['p99_us_upper'] is not None]
                    group['servers'][role][hist + '_p99_upper'] = {'median': statistics.median(numbers), 'min': min(numbers), 'max': max(numbers)} if numbers else None
            groups.append(group)
    write(args.output / 'summary.json', {'ok': all(row['ok'] for row in records), 'groups': groups, 'runs': len(records), 'failed': sum(not row['ok'] for row in records)})
    print(f'기준 측정 결과: {args.output}', flush=True)
    return 0 if all(row['ok'] for row in records) else 1


if __name__ == '__main__':
    sys.exit(main())
