"""실제 JobQueue 벤치마크 반복 실행·원시 결과·환경/소스 식별값·요약. 게임 서버 TPS와 구분한다."""
import argparse
import datetime
import hashlib
import json
import platform
import random
import statistics
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CASES = ('single', 'producers4', 'consumers4', 'queues32', 'burst', 'preallocated', 'mixed')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(*arguments):
    return subprocess.check_output(['git', '-C', str(ROOT), *arguments])


def summarize(records):
    groups = {}
    for item in records:
        key = (item['case'], item['budget'], item['metrics'], item['detail'])
        groups.setdefault(key, []).append(item)
    output = []
    for key, rows in sorted(groups.items()):
        passed = [row for row in rows if row['ok']]
        result = dict(zip(('case', 'budget', 'metrics', 'detail'), key))
        result.update(passed=len(passed), failed=len(rows) - len(passed))
        for field in ('jobs_per_s', 'seconds', 'cpu_seconds', 'cold_wait_ms', 'hot_completed_before_cold', 'private_begin', 'private_end', 'preload_ms'):
            values = [row[field] for row in passed if field in row]
            if values:
                result[field] = {'median': statistics.median(values), 'min': min(values), 'max': max(values)}
        for distribution in ('enqueue_to_start', 'enqueue_to_finish'):
            samples = [row[distribution] for row in passed if row.get(distribution)]
            if samples:
                # 실행별 percentile의 중앙값이다. 전체 표본을 합친 percentile로 표시하지 않는다.
                result[distribution] = {field: {'median': statistics.median(row[field] for row in samples),
                                               'min': min(row[field] for row in samples), 'max': max(row[field] for row in samples)}
                                        for field in ('samples', 'p50_us', 'p95_us', 'p99_us', 'max_us')}
        output.append(result)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=200000)
    parser.add_argument('--repeats', type=int, default=5)
    args = parser.parse_args()
    if not 1000 <= args.jobs <= 2000000 or not 1 <= args.repeats <= 20:
        parser.error('jobs는 1000~2000000, repeats는 1~20')
    args.output.mkdir(parents=True, exist_ok=True)
    if (args.output / 'runs.jsonl').exists():
        parser.error('기존 원시 결과 덮어쓰기 거부')
    source_paths = ['Server/ServerCore/include/ServerCore/Concurrency.h', 'Server/ServerCore/include/ServerCore/Metrics.h',
                    'Server/ServerCore/tests/JobQueueBenchmark.cpp', 'Server/ServerCore/tests/ServerCore.Tests.vcxproj',
                    'scripts/Measure-JobQueue.py', 'scripts/Measure-JobQueue.ps1']
    metadata = {'schema_version': 1, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'commit': git('rev-parse', 'HEAD').decode().strip(), 'branch': git('branch', '--show-current').decode().strip(),
                'tracked_diff_sha256': sha(git('diff', 'HEAD', '--binary')), 'binary_sha256': sha(args.exe.read_bytes()),
                'sources': {name: sha((ROOT / name).read_bytes()) for name in source_paths}, 'python': sys.version,
                'platform': platform.platform(), 'jobs_per_run': args.jobs, 'repeats': args.repeats, 'order_seed': 20261011,
                'warmup_jobs_in_same_process': min(args.jobs, 20000), 'warmup_scope': '큐 처리량/대기 측정만. fairness/idle은 별도 진단',
                'latency_sample_stride': 67, 'callback': '큐별 완료 수/id 합계. detail은 전체 작업별 atomic 중복 검사와 1/67 시각 기록',
                'mixed_callback': '997개당 한 작업이 50us busy wait', 'generator': '최대 속도 생산. 고정 요청 주기나 실제 서버 패킷 부하가 아님',
                'timing': '스레드 생성/사전 할당/초기 backlog 제외, barrier 해제 직전부터 worker join까지. burst preload 별도',
                'metrics': '실제 JobMetrics 켬/끔, 실행 중 전환 없음', 'omitted': ['할당 횟수/bytes', '잠금별 대기 시간', '서버 tick', '실제 서버 TPS']}
    (args.output / 'metadata.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    matrix = [(case, policy, metrics, False) for case in CASES for policy in ('Five', 'Snapshot') for metrics in (False, True)]
    matrix += [(case, policy, True, True) for case in ('producers4', 'queues32') for policy in ('Five', 'Snapshot')]
    matrix += [('fairness', policy, False, False) for policy in ('Five', 'Snapshot')]
    matrix += [('idle', 'Five', False, False)]
    randomizer = random.Random(metadata['order_seed'])
    records = []
    with (args.output / 'runs.jsonl').open('x', encoding='utf-8') as stream:
        for repeat in range(args.repeats):
            order = matrix.copy()
            randomizer.shuffle(order)
            for case, policy, metrics, detail in order:
                command = [str(args.exe.resolve()), '--jobqueue-benchmark', case, policy, str(int(metrics)), str(int(detail)), str(args.jobs)]
                try:
                    process = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, encoding='utf-8', timeout=45)
                    if process.returncode != 0:
                        raise RuntimeError(f'exit={process.returncode}: {process.stderr[:3000]}')
                    row = json.loads(process.stdout)
                    if not row.get('ok') or (row['case'], row['budget'], row['metrics'], row['detail']) != (case, policy, metrics, detail):
                        raise ValueError('결과 schema/실행 조건 불일치')
                    if case != 'idle' and row['jobs'] != args.jobs + (case == 'fairness'):
                        raise ValueError('작업 완료 수 불일치')
                except (subprocess.TimeoutExpired, RuntimeError, ValueError, KeyError, OSError) as error:
                    row = {'ok': False, 'case': case, 'budget': policy, 'metrics': metrics, 'detail': detail, 'error': str(error)}
                row['repeat'] = repeat + 1
                records.append(row)
                stream.write(json.dumps(row, ensure_ascii=False) + '\n')
                stream.flush()
            print(f'JobQueue 반복 {repeat + 1}/{args.repeats}: {sum(not item["ok"] for item in records)} 실패', flush=True)
    summary = {'schema_version': 1, 'ok': all(row['ok'] for row in records), 'runs': len(records), 'groups': summarize(records),
               'percentile_summary': '각 실행 percentile의 중앙값/최소/최대. 전체 표본의 percentile이 아님'}
    (args.output / 'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'ok': summary['ok'], 'runs': summary['runs'], 'summary': str(args.output / 'summary.json')}, ensure_ascii=False))
    return 0 if summary['ok'] else 1


if __name__ == '__main__':
    sys.exit(main())
