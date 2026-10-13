"""공간 검색 전후의 실제 게임 서버를 같은 seed로 AB/BA 비교한다."""
import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

from baseline import one_run, write
from protocol import ROOT
from tick_profile import analyze, fingerprint, stats

GAME_EXE = Path('artifacts/bin/Release/GameServer/Game_Server.exe')


def hashes():
    result = {name.replace('\\', '/'): value for name, value in fingerprint().items()}
    evidence = [*ROOT.glob('Server/ServerCore/tests/*.cpp'), *ROOT.glob('Server/ServerCore/tests/*.h'),
                ROOT / 'Server/ServerCore/tests/ServerCore.Tests.vcxproj', ROOT / 'Server/Game_Server/Game_Server.vcxproj',
                ROOT / 'scripts/Build.ps1', ROOT / 'scripts/Start-ServerLab.ps1', ROOT / 'scripts/Test-ServerTerrain.ps1']
    result.update({path.relative_to(ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest() for path in evidence})
    return result


def summarize(directory):
    modes = {mode: analyze(directory / mode) for mode in ('before', 'after')}
    groups = []
    for mode, result in modes.items():
        for group in result['groups']:
            group['mode'] = mode
            rows = [row for row in result['records'] if row['clients'] == group['clients']]
            for field in ('jobs_per_s', 'packets_per_s', 'private_bytes_peak_sampled', 'send_native_calls_per_s'):
                group[field] = stats([row['servers']['GameServer'][field] for row in rows if field in row.get('servers', {}).get('GameServer', {})])
            group['hz_per_match'] = stats([row['tick_profile']['ticks'] / row['tick_profile']['seconds'] / group['matches']
                                          for row in rows if (row.get('tick_profile') or {}).get('seconds')])
            group['observer_cpu_percent'] = stats([row['observer_cpu_percent'] for row in rows if 'observer_cpu_percent' in row])
            group['generator_lag_ms'] = [row.get('generator_lag_ms') for row in rows]
            groups.append(group)
    records = [row for result in modes.values() for row in result['records']]
    summary = {'groups': groups, 'runs': len(records), 'passed': sum(row.get('ok', False) and 'profile_error' not in row for row in records),
               'failed': [row for row in records if not row.get('ok') or 'profile_error' in row]}
    write(directory / 'analysis.json', summary)
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, help='검증된 metadata.json과 source-snapshot이 있는 기준 측정 경로')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--clients', nargs='+', type=int, default=[4, 16, 64])
    parser.add_argument('--repeats', type=int, default=5)
    parser.add_argument('--analyze', action='store_true')
    args = parser.parse_args()
    directory = args.output.resolve()
    if args.analyze:
        summarize(directory)
        return 0
    if not args.baseline or not 1 <= args.repeats <= 20 or any(count < 4 or count % 4 for count in args.clients):
        parser.error('baseline 필수, repeats 1~20, clients 4의 양수 배수')
    if directory.exists() or any((ROOT / '.runtime' / name).exists() for name in ('processes.json', 'server-lab.json')):
        parser.error('기존 출력 또는 서버 실행 기록 존재')
    baseline = args.baseline.resolve()
    original = baseline / 'source-snapshot' / GAME_EXE
    expected = {name.replace('\\', '/'): value for name, value in json.loads((baseline / 'metadata.json').read_text(encoding='utf-8'))['fingerprints'].items()}
    current = hashes()
    if hashlib.sha256(original.read_bytes()).hexdigest() != expected[GAME_EXE.as_posix()]:
        parser.error('기준 실행 파일 hash 불일치')
    resources = {name: value for name, value in current.items() if '/Resource/' in name or '/DataFile/' in name or '/scenarios/' in name}
    if any(expected.get(name) != value for name, value in resources.items()):
        parser.error('기준과 현재 데이터/시나리오가 다름')
    directory.mkdir(parents=True)
    binaries = directory / 'binaries'
    binaries.mkdir()
    shutil.copy2(ROOT / GAME_EXE, binaries / 'after.exe')
    shutil.copy2(original, binaries / 'before.exe')
    for mode in ('before', 'after'):
        (directory / mode).mkdir()
    write(directory / 'metadata.json', {'schema_version': 1, 'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
          'platform': platform.platform(), 'logical_processors': os.cpu_count(), 'fingerprints': current,
          'before_exe_sha256': expected[GAME_EXE.as_posix()], 'baseline': str(baseline), 'resources': resources,
          'clients': args.clients, 'repeats': args.repeats, 'profile_enabled': True, 'order': 'odd BA, even AB',
          'scenario': 'perf-match', 'warmup_s': 5, 'duration_s': 20, 'interval_s': .25, 'uncap_admission': True})
    # Keep source and executable evidence even after later builds/edits.
    for name in current:
        destination = directory / 'source-snapshot' / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / name, destination)
    previous = os.environ.get('WOD_TICK_PROFILE')
    records = []
    try:
        os.environ['WOD_TICK_PROFILE'] = '1'
        for seed in range(1, args.repeats + 1):
            for clients in args.clients:
                for mode in (('after', 'before') if seed % 2 else ('before', 'after')):
                    shutil.copy2(binaries / f'{mode}.exe', ROOT / GAME_EXE)
                    name = f'{seed:02d}-match-{clients}'
                    print(f'시작 {mode}/{name}', flush=True)
                    record = one_run(directory / mode / name, 'perf-match', clients, seed, True)
                    record.update(profile_enabled=True, seed=seed, mode=mode)
                    write(directory / mode / name / 'report.json', record)
                    records.append(record)
                    print(f'완료 {mode}/{name}: ok={record.get("ok")}', flush=True)
                    if any((ROOT / '.runtime' / item).exists() for item in ('processes.json', 'server-lab.json')):
                        raise RuntimeError('종료 후 실행 기록 잔존')
    finally:
        shutil.copy2(binaries / 'after.exe', ROOT / GAME_EXE)
        if previous is None:
            os.environ.pop('WOD_TICK_PROFILE', None)
        else:
            os.environ['WOD_TICK_PROFILE'] = previous
        write(directory / 'verification.json', {'fingerprints_unchanged_and_after_exe_restored': hashes() == current})
        summarize(directory)
    return 0 if len(records) == len(args.clients) * args.repeats * 2 and all(row.get('ok') for row in records) else 1


if __name__ == '__main__':
    sys.exit(main())
