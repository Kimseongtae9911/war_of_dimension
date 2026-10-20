"""실행 중인 LOCAL_TEST 로비의 기본 정원4·다중 채널 수명/시야/입력/매칭 회귀.

시작/종료와 worker 설정은 호출자가 소유한다. 제품 프로토콜을 바꾸지 않는다.
"""
import argparse
import asyncio
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from protocol import CONSTANTS, packet, unpack
from runner import RunState, SCENARIOS, load_scenario
from lobby_movement import ObservedPeer


def removals(peer, after):
    return [struct.unpack_from('<i', body)[0] for sequence, kind, body in peer.history
            if sequence > after and kind == CONSTANTS['SC_REMOVE_PLAYER']]


async def channel(peer, destination, success=True):
    marker = peer.sequence
    peer.writer.write(struct.pack('<BBh', 4, CONSTANTS['CS_CHANGE_CHANNEL'], destination))
    await peer.writer.drain()
    body = await peer.expect('SC_CHANGE_CHANNEL', marker)
    failed, actual = struct.unpack('<?i', body)
    assert failed != success, (destination, failed, actual)
    if success:
        assert actual == destination, (actual, destination)
        peer.player_id = unpack('SC_LOGIN_INFO', await peer.expect('SC_LOGIN_INFO', marker))[0]
    return marker


async def exercise():
    state = RunState(load_scenario(SCENARIOS / 'lobby-movement.json'), 8)
    peers = []
    rows = []

    async def login():
        peer = ObservedPeer(state, len(peers))
        peers.append(peer)
        await peer.login(8910)
        return peer

    try:
        for _ in range(8):
            await login()
        for index, peer in enumerate(peers):
            assigned = await peer.expect('SC_CHANGE_CHANNEL')
            assert struct.unpack('<?i', assigned) == (False, index // 4)

        await channel(peers[0], -1, False)
        await channel(peers[0], 32767, False)
        await channel(peers[0], 1, False)
        await channel(peers[0], 0)
        rows.append({'case': 'negative-upper-full-same-channel', 'passed': True})

        observer, departed = peers[0], peers[3]
        await observer.expect('SC_ADD_PLAYER')
        marker, old = observer.sequence, departed.player_id
        await departed.close()
        await observer.expect('SC_REMOVE_PLAYER', marker, lambda data: struct.unpack_from('<i', data)[0] == old)
        assert all(0 <= value < 4 for value in removals(observer, marker))
        rows.append({'case': 'disconnect-removes-frozen-id', 'id': old})

        mover, old_observer = peers[4], peers[5]
        marker, own_marker, old = old_observer.sequence, mover.sequence, mover.player_id
        await channel(mover, 0)
        await old_observer.expect('SC_REMOVE_PLAYER', marker, lambda data: struct.unpack_from('<i', data)[0] == old)
        assert old not in removals(mover, own_marker), '이동한 본인에게 출발 제거 전송'
        await mover.send('CS_MOVE', 1, 0)
        for target in [2, 0] * 6:
            await channel(mover, target)
            marker = mover.sequence
            await mover.send('CS_MOVE', 2, 0)
            await mover.expect('SC_MOVE_PLAYER', marker, lambda data: unpack('SC_MOVE_PLAYER', data)[4] == 2)
        marker = mover.sequence
        await mover.send('CS_MOVE', 0, 0)
        await mover.expect('SC_MOVE_PLAYER', marker, lambda data: unpack('SC_MOVE_PLAYER', data)[4] == 0)
        await asyncio.sleep(.08)
        assert unpack('SC_MOVE_PLAYER', mover.moves[mover.player_id][1])[4] == 0
        rows.append({'case': 'moving-transfer-reroutes-timer-and-stop', 'transfers': 13})

        for peer in (peers[0], peers[4], peers[5]):
            marker = len(peer.observations)
            peer.writer.write(b''.join(packet('CS_MOVE', value, 0) for value in [1, 0] * 100))
            await peer.writer.drain()
            deadline = asyncio.get_running_loop().time() + 4
            while sum(data[0] == peer.player_id for _, data in peer.observations[marker:]) < 200:
                assert asyncio.get_running_loop().time() < deadline, 'FIFO 응답 누락'
                await asyncio.sleep(.005)
            values = [data[4] for _, data in peer.observations[marker:] if data[0] == peer.player_id]
            assert values[:200] == [1, 0] * 100, ('입력 FIFO 손상', peer.index, len(values), values[:20])
        rows.append({'case': 'three-peers-two-channels-200-inputs', 'passed': True})

        active = [peer for peer in peers if peer is not departed]
        markers = {peer.index: peer.sequence for peer in active}
        message = 'global-check'.encode('utf-16-le').ljust(60, b'\0')
        payload = struct.pack('<i10s60sB', mover.player_id, mover.name, message, 0)
        mover.writer.write(bytes([len(payload) + 2, CONSTANTS['CS_CHAT']]) + payload)
        await mover.writer.drain()
        for peer in active:
            await peer.expect('SC_CHAT', markers[peer.index])
        rows.append({'case': 'global-chat-reaches-active-channels', 'recipients': len(active)})

        # DB 없는 선택 기능 요청 뒤에도 정상 이동 응답이 이어져야 한다.
        mover.writer.write(struct.pack('<BBi', 6, CONSTANTS['CS_SHOP'], 0))
        mover.writer.write(bytes([2, CONSTANTS['CS_OPEN_BLOCKCHAIN']]))
        await mover.writer.drain()
        marker = mover.sequence
        await mover.send('CS_MOVE', 0, 0)
        await mover.expect('SC_MOVE_PLAYER', marker)
        rows.append({'case': 'database-disabled-feature-does-not-crash', 'passed': True})

        for peer in peers:
            await peer.close()
        await asyncio.sleep(.15)
        abandoned = await login()
        await abandoned.send('CS_MATCH', True, 0, 0)
        marker = abandoned.sequence
        await abandoned.send('CS_MOVE', 0, 0)
        await abandoned.expect('SC_MOVE_PLAYER', marker)
        await abandoned.close()
        await asyncio.sleep(.05)
        group = [await login() for _ in range(4)]
        for index, peer in enumerate(group):
            await channel(peer, index + 2)
        first = group[0]
        for _ in range(6):
            await first.send('CS_MATCH', True, 0, 0)
            await first.send('CS_MATCH', False, 0, 0)
        await first.send('CS_MATCH', True, 0, 0)
        await first.send('CS_MATCH', True, 0, 0)
        marker = first.sequence
        await first.send('CS_MOVE', 0, 0)
        await first.expect('SC_MOVE_PLAYER', marker)
        await channel(first, 7, False)
        for index, peer in enumerate(group[1:], 1):
            await peer.send('CS_MATCH', True, int(index == 3), index)
        slots = [unpack('SC_MATCH', await peer.expect('SC_MATCH'))[0] for peer in group]
        assert sorted(slots) == [0, 1, 2, 3], slots
        assert all(peer.error is None for peer in group)
        rows.append({'case': 'cross-channel-match-cancel-reregister-duplicate-disconnect', 'slots': slots})

        bad = await login()
        await bad.send('CS_MATCH', True, 255, 0)
        deadline = asyncio.get_running_loop().time() + 3
        while bad.error is None:
            assert asyncio.get_running_loop().time() < deadline, '잘못된 매칭 역할 거부 누락'
            await asyncio.sleep(.005)
        await bad.close()
        rows.append({'case': 'invalid-match-role-rejected', 'passed': True})

        legacy = await login()
        marker = legacy.sequence
        legacy.writer.write(struct.pack('<BBi', 6, CONSTANTS['CS_DUMMY_CLIENT'], 0))
        await legacy.writer.drain()
        await legacy.expect('SC_DUMMY_LOGIN_INFO', marker)
        marker = await channel(legacy, 2)
        await legacy.expect('SC_MOVE_PLAYER', marker, lambda data: unpack('SC_MOVE_PLAYER', data)[0] == 8 + legacy.player_id)
        rows.append({'case': 'channel-transfer-preserves-legacy-dummy-policy', 'passed': True})
        return rows
    finally:
        for peer in peers:
            await peer.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = {'passed': False, 'requires': 'LOCAL_TEST; capacity=4; at least 6 channels; DB off'}
    try:
        result['cases'] = asyncio.run(exercise())
        result['passed'] = True
    except Exception as error:
        result['error'] = str(error) or type(error).__name__
        raise
    finally:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
