"""동접 경계의 독립 재시작 반복 검사. 실패와 원시 결과를 그대로 보존한다."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/server_lab'))
from baseline import one_run, write
from tick_profile import fingerprint, summarize_profile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scenario', choices=['perf-match', 'perf-lobby', 'soak-lobby'], required=True)
    parser.add_argument('--clients', nargs='+', type=int, required=True)
    parser.add_argument('--repeats', type=int, default=5)
    parser.add_argument('--seed-start', type=int, default=1)
    parser.add_argument('--profile', action='store_true')
    parser.add_argument('--headless', action='store_true')
    parser.add_argument('--disable-metrics', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.scenario == 'soak-lobby' and args.disable_metrics:
        parser.error('soak-lobby 최종 입력 검증에는 계측이 필요합니다')
    if not 1 <= args.repeats <= 20 or any(count < 1 or (args.scenario == 'perf-match' and count % 4) for count in args.clients):
        parser.error('인원/반복 수 범위 오류')
    if any((ROOT/'.runtime'/name).exists() for name in ('server-lab.json','processes.json')):
        parser.error('이미 실행 중인 서버 기록 존재')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    hashes = fingerprint()
    hashes.update({str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in (ROOT/'Server/Lobby_Server').glob('*') if path.suffix in ('.cpp', '.h')})
    hashes.update({str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in (ROOT/'Server/Lobby_Server/Resource').glob('*') if path.is_file()})
    scenario = json.loads((ROOT/'tools/server_lab/scenarios'/f'{args.scenario}.json').read_text(encoding='utf-8'))
    shutil.copy2(Path(__file__),args.output/'driver.py')
    (args.output/'source.patch').write_bytes(subprocess.check_output(['git','diff','HEAD','--binary'],cwd=ROOT))
    for relative in hashes:
        if Path(relative).suffix in ('.py','.cpp','.h','.json','.exe'):
            target=args.output/'source-snapshot'/relative
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(ROOT/relative,target)
    write(args.output/'metadata.json', {'arguments': vars(args) | {'output':str(args.output)}, 'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                                      'fingerprints':hashes, 'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                                      'conditions':'Release LOCAL_TEST; uncapped; restart per run; 4 players per game match', 'performance':scenario['performance']})
    os.environ['WOD_TICK_PROFILE']='1' if args.profile else '0'
    os.environ['WOD_LAB_HEADLESS']='1' if args.headless else '0'
    records=[]
    for seed in range(args.seed_start, args.seed_start+args.repeats):
        for count in (args.clients if seed % 2 else list(reversed(args.clients))):
            name=f'{seed:02d}-{args.scenario}-{count}'
            print('START '+name, flush=True)
            row=one_run(args.output/name,args.scenario,count,seed,not args.disable_metrics)
            row.update(seed=seed,profile_enabled=args.profile)
            if row.get('run_id'):
                result=json.loads((args.output/name/'lab/runs'/f"{row['run_id']}.json").read_text(encoding='utf-8'))
                if args.scenario=='perf-match' and not args.disable_metrics:
                    try:
                        if args.profile:
                            row['profile']=summarize_profile(result)
                        game=row['servers']['GameServer']
                        row['hz_per_match']=game['tick_duration_us']['samples']/game['seconds']/(count/4)
                    except (ValueError,KeyError,TypeError) as error:
                        row['profile_error']=str(error)
                row['connected_end']=result['counters']['connected']
                row['settlement']=result.get('settlement')
                row['peer_errors']=[{key:peer.get(key) for key in ('id','receive_error','receive_error_phase')} for peer in result['peers'] if peer.get('receive_error') and peer.get('receive_error_phase') not in ('cleanup',)]
            write(args.output/name/'report.json',row)
            records.append(row)
            unchanged=all((ROOT/path).exists() and hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==value for path,value in hashes.items())
            write(args.output/'summary.json',{'runs':records,'fingerprints_unchanged':unchanged})
            print(json.dumps({'name':name,'ok':row['ok'],'error':row.get('error'),'hz':row.get('hz_per_match')}),flush=True)
            if any((ROOT/'.runtime'/p).exists() for p in ('server-lab.json','processes.json')):
                raise RuntimeError('종료 기록 잔존; 다음 실행 중단')
    return 0 if all(row['ok'] for row in records) else 1


if __name__=='__main__':
    raise SystemExit(main())
