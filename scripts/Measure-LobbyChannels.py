"""같은 채널 분포·입력·실행 파일에서 로비 worker 수만 바꾸는 반복 비교."""
import argparse
import collections
import hashlib
import json
import os
import platform
from pathlib import Path
import shutil
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/server_lab'))
from baseline import one_run, write


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workers', nargs='+', type=int, default=[1, 4])
    parser.add_argument('--channels', type=int, default=32)
    parser.add_argument('--capacity', type=int, default=4)
    parser.add_argument('--clients', type=int, default=128)
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--disable-metrics', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.repeats <= 10 or any(not 1 <= value <= 32 for value in args.workers):
        parser.error('worker=1..32, repeats=1..10')
    if min(args.channels, args.capacity, args.clients) < 1 or not args.clients <= args.channels * args.capacity <= 3000:
        parser.error('clients <= channels*capacity <= 3000')
    if any((ROOT / '.runtime' / name).exists() for name in ('processes.json', 'server-lab.json')):
        parser.error('서버/Server Lab을 먼저 중지하세요')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    files = sorted((ROOT / 'Server/Lobby_Server').glob('*.cpp')) + sorted((ROOT / 'Server/Lobby_Server').glob('*.h'))
    files += [ROOT / 'artifacts/bin/Release/LobbyServer/Lobby_Server.exe', ROOT / 'artifacts/bin/Release/GameServer/Game_Server.exe',
              ROOT / 'tools/server_lab/scenarios/perf-lobby.json', ROOT / 'Shared/Protocol/tests/abi-x64-msvc.json', Path(__file__)]
    files += sorted((ROOT / 'tools/server_lab').glob('*.py'))
    files += [ROOT / 'scripts' / name for name in ('Start-ServerLab.ps1', 'Stop-ServerLab.ps1', 'Start-Local.ps1', 'Stop-Local.ps1', 'Development.ps1')]
    hashes = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in files}
    for path in files:
        target = args.output / 'source-snapshot' / path.relative_to(ROOT)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    write(args.output / 'metadata.json', {'arguments': vars(args) | {'output': str(args.output)}, 'hashes': hashes,
          'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
          'tracked_diff_sha256': hashlib.sha256(subprocess.check_output(['git', 'diff', 'HEAD', '--binary'], cwd=ROOT)).hexdigest(),
          'machine': {'platform': platform.platform(), 'logical_processors': os.cpu_count(), 'python': sys.version,
                      'processor': platform.processor()},
          'conditions': 'Release LOCAL_TEST; same channel distribution and perf-lobby; restart per run; 5s warmup/20s measure/250ms input'})
    (args.output / 'working.diff').write_bytes(subprocess.check_output(['git', 'diff', 'HEAD', '--binary'], cwd=ROOT))
    names = ['WOD_LOBBY_WORKERS', 'WOD_LOBBY_CHANNELS', 'WOD_LOBBY_CHANNEL_CAPACITY', 'WOD_LAB_HEADLESS', 'WOD_TICK_PROFILE']
    previous = {name: os.environ.get(name) for name in names}
    records = []
    try:
        os.environ.update(WOD_LOBBY_CHANNELS=str(args.channels), WOD_LOBBY_CHANNEL_CAPACITY=str(args.capacity), WOD_LAB_HEADLESS='1', WOD_TICK_PROFILE='0')
        for repeat in range(1, args.repeats + 1):
            for workers in args.workers if repeat % 2 else list(reversed(args.workers)):
                os.environ['WOD_LOBBY_WORKERS'] = str(workers)
                path = args.output / f'{repeat:02d}-workers-{workers}'
                print(f'START {path.name}', flush=True)
                row = one_run(path, 'perf-lobby', args.clients, repeat, not args.disable_metrics)
                row.update(workers=workers, repeat=repeat)
                if row.get('run_id'):
                    result = json.loads((path / 'lab/runs' / f"{row['run_id']}.json").read_text(encoding='utf-8'))
                    counts = collections.Counter(peer.get('lobby_channel') for peer in result['peers'])
                    row['channel_population'] = {str(key): value for key, value in counts.items()}
                    expected = {str(index): min(args.capacity, args.clients - index * args.capacity)
                                for index in range((args.clients + args.capacity - 1) // args.capacity)}
                    row['channel_population_ok'] = row['channel_population'] == expected
                    if not row['channel_population_ok']:
                        row.update(ok=False, measurement_error='실제 입장 채널 분포 불일치')
                if row.get('run_id') and not args.disable_metrics:
                    samples = [server for sample in result['samples'] for server in sample['servers'] if server['role'] == 'LobbyServer']
                    settings = samples[-1]['admission']
                    assert settings['logic_workers'] == workers and settings['channel_capacity'] == args.capacity, settings
                    row['dispatcher'] = settings['dispatcher']
                    assert row['dispatcher']['channel_count'] == args.channels
                    assert row['dispatcher']['owner_violations'] == 0
                lag = row.get('generator_lag_ms') or {}
                observer = row.get('observer_cpu_core_percent')
                row['generator_limit'] = (lag.get('p99') is not None and lag['p99'] > 250) or (observer is not None and observer >= 90)
                records.append(row)
                write(path / 'report.json', row)
                unchanged = all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in hashes.items())
                assert unchanged, '측정 중 실행 파일/소스 변경'
                groups = []
                for count in args.workers:
                    values = [item['servers']['LobbyServer'] for item in records if item['workers'] == count and item.get('ok')]
                    if values:
                        groups.append({'workers': count, 'runs': len(values),
                                       'cpu_percent': statistics.median(value['cpu_percent'] for value in values),
                                       'packets_per_s': None if args.disable_metrics else statistics.median(value['packets_per_s'] for value in values),
                                       'packet_wait_p99_us': None if args.disable_metrics else statistics.median(value['packet_job_wait_us']['p99_us_upper'] for value in values)})
                write(args.output / 'summary.json', {'runs': records, 'medians': groups, 'fingerprints_unchanged': unchanged})
                print(json.dumps({'workers': workers, 'repeat': repeat, 'ok': row['ok'], 'error': row.get('error')}), flush=True)
                if not row['ok']:
                    return 1
                if row['generator_limit']:
                    print('STOP: 부하 생성기 한계 (발행 p99>250ms 또는 CPU>=0.9코어)', flush=True)
                    return 2
    finally:
        for name, value in previous.items():
            if value is None:
                os.environ.pop(name, None)
            else:
                os.environ[name] = value
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
