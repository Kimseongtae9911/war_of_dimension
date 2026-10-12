"""게임 tick 구간별 wall/CPU cycle 분석 및 실제 LOCAL_TEST 반복 측정."""
import argparse
import hashlib
import json
import os
import platform
import statistics
import subprocess
import sys
from pathlib import Path

from baseline import one_run, write
from protocol import ROOT

FIELDS = ('calls', 'wall_ns', 'cycles', 'cycle_samples', 'cycle_errors', 'work')
TOP = ('game_clock', 'jobs', 'player_update', 'player_send', 'minion_update', 'monster_update', 'periodic', 'observation')


def profile_delta(before, after):
    """미계측은 None, schema/재시작/누적 감소는 오류로 보존한다."""
    if before is None or after is None:
        return None
    if before.get('schema_version') != 1 or after.get('schema_version') != 1:
        raise ValueError('tick_profile schema 불일치')
    if type(before.get('enabled')) is not bool or type(after.get('enabled')) is not bool or before['enabled'] != after['enabled']:
        raise ValueError('tick_profile enabled 불일치')
    if not after['enabled']:
        return None
    left, right = before.get('stages'), after.get('stages')
    if not isinstance(left, dict) or not isinstance(right, dict) or left.keys() != right.keys() or 'total' not in right:
        raise ValueError('tick_profile stage 불일치')
    output = {}
    for stage in right:
        if any(type(row.get(key)) is not int or row[key] < 0 for row in (left[stage], right[stage]) for key in FIELDS):
            raise ValueError('tick_profile 누적값 형식 오류')
        delta = {key: right[stage][key] - left[stage][key] for key in FIELDS}
        if min(delta.values()) < 0:
            raise ValueError('tick_profile 누적값 감소')
        output[stage] = delta
    ticks = output['total']['calls']
    cycles = output['total']['cycles']
    if not ticks:
        return {'ticks': 0, 'stages': output}
    for delta in output.values():
        delta.update(wall_ms_per_tick=delta['wall_ns'] / ticks / 1_000_000,
                     cycles_per_tick=delta['cycles'] / ticks,
                     cycle_share_percent=delta['cycles'] / cycles * 100 if cycles else None,
                     calls_per_tick=delta['calls'] / ticks,
                     work_per_call=delta['work'] / delta['calls'] if delta['calls'] else None)
    # atomic snapshot은 근사값이다. 음수 잔차를 0으로 숨기지 않는다.
    residual_ns = output['total']['wall_ns'] - sum(output[key]['wall_ns'] for key in TOP)
    return {'ticks': ticks, 'stages': output, 'top_residual_ms_per_tick': residual_ns / ticks / 1_000_000}


def summarize_profile(result):
    phases = {row['phase']: row['timestamp_ms'] for row in result.get('phases', [])}
    begin, end = phases.get('measure'), phases.get('verify')
    if begin is None or end is None:
        raise ValueError('측정 시작/끝 구간 없음')
    rows = [next((row for row in sample['servers'] if row['role'] == 'GameServer'), {})
            for sample in result['samples'] if begin <= sample['timestamp_ms'] <= end]
    if len(rows) < 3 or any(row.get('status') != 'live' for row in rows) or len({row['instance'] for row in rows}) != 1:
        raise ValueError('GameServer 표본 부족/stale/재시작')
    rows = list({row['sequence']: row for row in rows if begin <= row['timestamp_ms'] <= end}.values())
    if len(rows) < 3:
        raise ValueError('측정 구간 내부 reporter 표본 부족')
    profile = profile_delta(rows[0].get('tick_profile'), rows[-1].get('tick_profile'))
    if profile is not None:
        profile['seconds'] = (rows[-1]['uptime_ms'] - rows[0]['uptime_ms']) / 1000
    return profile


def analyze(directory):
    records = []
    for report_path in sorted(directory.glob('*/report.json')):
        record = json.loads(report_path.read_text(encoding='utf-8'))
        raw_path = report_path.parent / 'lab/runs' / (record.get('run_id', '') + '.json')
        record['name'] = report_path.parent.name
        try:
            record['tick_profile'] = summarize_profile(json.loads(raw_path.read_text(encoding='utf-8')))
            if record.get('profile_enabled') is True and (record['tick_profile'] is None or not record['tick_profile'].get('ticks')):
                raise ValueError('계측 켬 실행의 tick 표본 누락')
        except (OSError, ValueError, KeyError, TypeError) as error:
            record['profile_error'] = str(error)
        records.append(record)
    groups = []
    for clients in sorted({row['clients'] for row in records}):
        for enabled in (False, True):
            rows = [row for row in records if row['clients'] == clients and row.get('profile_enabled') is enabled]
            if not rows:
                continue
            group = {'clients': clients, 'matches': clients // 4, 'enabled': enabled, 'runs': len(rows),
                     'passed': sum(row.get('ok', False) and 'profile_error' not in row for row in rows)}
            values = [row['servers']['GameServer'] for row in rows if 'GameServer' in row.get('servers', {})]
            for key in ('cpu_percent', 'packets_per_s'):
                numbers = [value[key] for value in values if key in value]
                group[key] = stats(numbers)
            for key in ('tick_duration_us', 'tick_delay_us'):
                group[key] = stats([value[key]['mean_us'] for value in values if value.get(key, {}).get('mean_us') is not None])
            profiles = [row['tick_profile'] for row in rows if (row.get('tick_profile') or {}).get('ticks')]
            if profiles:
                group['stages'] = {name: {field: stats([profile['stages'][name][field] for profile in profiles])
                                          for field in ('wall_ms_per_tick', 'cycle_share_percent', 'calls_per_tick', 'work_per_call')}
                                   for name in profiles[0]['stages']}
                group['cycle_errors'] = sum(profile['stages']['total']['cycle_errors'] for profile in profiles)
            groups.append(group)
    output = {'records': records, 'groups': groups}
    write(directory / 'analysis.json', output)
    return output


def stats(values):
    values = [value for value in values if value is not None]
    return {'median': statistics.median(values), 'min': min(values), 'max': max(values)} if values else None


def fingerprint():
    sources = [*ROOT.glob('Server/Game_Server/*.cpp'), *ROOT.glob('Server/Game_Server/*.h'),
               *ROOT.glob('Server/ServerCore/src/*.cpp'), *ROOT.glob('Server/ServerCore/include/ServerCore/*.h'),
               *ROOT.glob('tools/server_lab/*.py'), *ROOT.glob('tools/server_lab/scenarios/*.json')]
    resources = [path for path in (ROOT / 'Server/Game_Server/Resource').glob('*') if path.is_file()]
    resources += [*ROOT.glob('Server/Game_Server/DataFile/CSV/*.csv')]
    binaries = [ROOT / 'artifacts/bin/Release' / role / exe for role, exe in
                [('GameServer', 'Game_Server.exe'), ('LobbyServer', 'Lobby_Server.exe')]]
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sources + resources + binaries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--clients', nargs='+', type=int, default=[4, 16, 64])
    parser.add_argument('--repeats', type=int, default=5)
    parser.add_argument('--profile-only', action='store_true', help='계측 끔 대조 생략')
    parser.add_argument('--analyze', action='store_true')
    args = parser.parse_args()
    args.output = args.output.resolve()
    if args.analyze:
        analyze(args.output)
        return 0
    if args.repeats < 1 or any(count < 4 or count % 4 for count in args.clients):
        parser.error('repeats는 양수, clients는 4의 양수 배수')
    if any((ROOT / '.runtime' / name).exists() for name in ('processes.json', 'server-lab.json')):
        parser.error('기존 서버 실행 기록 존재')
    args.output.mkdir(parents=True, exist_ok=False)
    hashes = fingerprint()
    write(args.output / 'metadata.json', {'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
          'diff_sha256': hashlib.sha256(subprocess.check_output(['git', 'diff', '--binary'], cwd=ROOT)).hexdigest(),
          'platform': platform.platform(), 'python': sys.version, 'logical_processors': os.cpu_count(), 'fingerprints': hashes,
          'clients': args.clients, 'repeats': args.repeats, 'off_control_clients': None if args.profile_only else min(args.clients),
          'scenario': 'perf-match', 'warmup_s': 5, 'duration_s': 20, 'interval_s': .25, 'uncap_admission': True})
    previous = os.environ.get('WOD_TICK_PROFILE')
    records = []
    try:
        for seed in range(1, args.repeats + 1):
            for clients in args.clients:
                modes = [True] if args.profile_only or clients != min(args.clients) else ([False, True] if seed % 2 else [True, False])
                for enabled in modes:
                    name = f'{seed:02d}-match-{clients}-profile-{"on" if enabled else "off"}'
                    os.environ['WOD_TICK_PROFILE'] = '1' if enabled else '0'
                    print(f'시작 {name}', flush=True)
                    record = one_run(args.output / name, 'perf-match', clients, seed, True)
                    record.update(profile_enabled=enabled, seed=seed)
                    write(args.output / name / 'report.json', record)
                    records.append(record)
                    with (args.output / 'runs.jsonl').open('a', encoding='utf-8') as stream:
                        stream.write(json.dumps(record, ensure_ascii=False) + '\n')
                    print(f'완료 {name}: ok={record.get("ok")}', flush=True)
                    if any((ROOT / '.runtime' / item).exists() for item in ('processes.json', 'server-lab.json')):
                        raise RuntimeError('종료 후 실행 기록 잔존')
    finally:
        if previous is None:
            os.environ.pop('WOD_TICK_PROFILE', None)
        else:
            os.environ['WOD_TICK_PROFILE'] = previous
        write(args.output / 'verification.json', {'fingerprints_unchanged': fingerprint() == hashes})
        analyze(args.output)
    return 0 if all(row.get('ok') for row in records) else 1


if __name__ == '__main__':
    sys.exit(main())
