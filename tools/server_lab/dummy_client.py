"""새 더미 CLI. 웹 화면 없이 같은 시나리오를 실행하고 JSON/CSV 결과를 남긴다."""
import argparse
import json
import sys
from pathlib import Path

from runner import ROOT, SCENARIOS, RunState, load_scenario, run_sync


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scenario', type=Path, default=SCENARIOS / 'match-actions.json')
    parser.add_argument('--clients', type=int)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--output', type=Path, default=ROOT / 'artifacts/logs/server-lab/runs')
    args = parser.parse_args()
    try:
        state = RunState(load_scenario(args.scenario), args.clients, args.seed)
    except (ValueError, OSError) as error:
        parser.error(str(error))
    try:
        run_sync(state)
    except KeyboardInterrupt:
        state.cancel()
        state.status = 'cancelled'
    path = state.save(args.output)
    print(json.dumps({'status': state.status, 'result': str(path), 'error': state.error}, ensure_ascii=False))
    return 0 if state.status == 'passed' else 130 if state.status == 'cancelled' else 1


if __name__ == '__main__':
    sys.exit(main())
