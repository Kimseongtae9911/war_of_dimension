"""정원1·채널1 LOCAL_TEST 서버에서 등록 실패와 슬롯 반환을 검사한다."""
import argparse
import asyncio
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from runner import RunState, SCENARIOS, load_scenario
from protocol import CONSTANTS
from lobby_movement import ObservedPeer


async def exercise():
    state = RunState(load_scenario(SCENARIOS / 'lobby-movement.json'), 3)
    first, rejected, replacement = [ObservedPeer(state, index) for index in range(3)]
    try:
        await first.login(8910)
        await rejected.connect(8910)
        deadline = asyncio.get_running_loop().time() + 3
        while rejected.error is None:
            assert asyncio.get_running_loop().time() < deadline, '미배정 접속이 종료되지 않음'
            await asyncio.sleep(.005)
        assert not any(kind == CONSTANTS['SC_LOGIN_INFO'] for _, kind, _ in rejected.history)
        marker = first.sequence
        await first.send('CS_MOVE', 0, 0)
        await first.expect('SC_MOVE_PLAYER', marker)
        await first.close()
        await asyncio.sleep(.1)
        await replacement.login(8910)
        assert replacement.player_id == 0
        return {'unassigned_connection_closed': True, 'existing_user_alive': True, 'slot_reused': True}
    finally:
        for peer in (first, rejected, replacement):
            await peer.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = {'passed': False, 'requires': 'WOD_LOBBY_CHANNEL_CAPACITY=1; WOD_LOBBY_CHANNELS=1'}
    try:
        result['checks'] = asyncio.run(exercise())
        result['passed'] = True
    finally:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
