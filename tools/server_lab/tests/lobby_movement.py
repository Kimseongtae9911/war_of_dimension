"""실행 중인 LOCAL_TEST 로비의 이동 주기·최종 정지·빠른 입력·재접속 회귀.

로그인 응답만으로 통과하지 않고 실제 SC_MOVE_PLAYER의 좌표와 방향을 확인한다.
--legacy는 이전 실행 파일의 이동 거리/갱신 수 비교용이며 새 정지 ACK를 요구하지 않는다.
서버의 시작/종료는 PowerShell이 소유한다.
"""
import argparse
import asyncio
import json
import math
from pathlib import Path
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from protocol import packet, unpack
from runner import Peer, RunState, SCENARIOS, load_scenario


class ObservedPeer(Peer):
    def __init__(self, state, index):
        super().__init__(state, index, f'Motion{index}'.encode().ljust(10, b'\0'), 3)
        self.observations = []

    async def receive(self):
        # Peer가 처리한 최신 이동 상태를 별도 polling으로 추정하지 않는다.
        from protocol import CONSTANTS
        try:
            while True:
                header = await self.reader.readexactly(2)
                size, kind = header
                if size < 2:
                    raise ValueError('잘못된 길이')
                body = await self.reader.readexactly(size - 2)
                self.sequence += 1
                if kind == CONSTANTS['SC_MOVE_PLAYER']:
                    values = unpack('SC_MOVE_PLAYER', body)
                    assert all(math.isfinite(value) for value in values[1:4]), values
                    self.moves[values[0]] = (self.sequence, body)
                    self.observations.append((time.perf_counter(), values))
                    assert len(self.observations) < 20000, '수신 보존 한도 초과'
                else:
                    self.history.append((self.sequence, kind, body))
                self.changed.set()
        except asyncio.CancelledError:
            raise
        except Exception as error:
            self.error = str(error) or type(error).__name__
            self.changed.set()


async def exercise(legacy):
    state = RunState(load_scenario(SCENARIOS / 'lobby-movement.json'), 2)
    peers = [ObservedPeer(state, index) for index in range(2)]
    rows = []
    try:
        for peer in peers:
            await peer.login(8910)
        mover, observer = peers
        await asyncio.sleep(.15)
        start = mover.observations[-1][1][1:4]
        begin = time.perf_counter()
        marker = mover.sequence
        await mover.send('CS_MOVE', 2, 0, split=True)
        await mover.expect('SC_MOVE_PLAYER', marker, lambda body: unpack('SC_MOVE_PLAYER', body)[4] == 2)
        await asyncio.sleep(.6)
        stop_at = time.perf_counter()
        marker, near_marker = mover.sequence, observer.sequence
        await mover.send('CS_MOVE', 0, 0)
        if not legacy:
            await mover.expect('SC_MOVE_PLAYER', marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == mover.player_id and unpack('SC_MOVE_PLAYER', body)[4] == 0)
            ack_ms = (time.perf_counter() - stop_at) * 1000
            await observer.expect('SC_MOVE_PLAYER', near_marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == mover.player_id and unpack('SC_MOVE_PLAYER', body)[4] == 0)
        else:
            ack_ms = None
        await asyncio.sleep(.1)
        final = unpack('SC_MOVE_PLAYER', mover.moves[mover.player_id][1])
        if not legacy:
            near_final = unpack('SC_MOVE_PLAYER', observer.moves[mover.player_id][1])
            assert final[4] == near_final[4] == 0 and final[1:4] == near_final[1:4], '본인/주변 최종 정지 상태 불일치'
        distance = math.dist(start[::2], final[1:4:2])
        assert 4 < distance < 7, ('이동 거리', distance)
        own_count = sum(at >= begin and values[0] == mover.player_id and values[4] != 0 for at, values in mover.observations)
        stable_at = time.perf_counter()
        await asyncio.sleep(.15)
        assert not any(at >= stable_at and values[0] == mover.player_id for at, values in mover.observations), '정지 뒤 갱신 반복'
        rows.append(dict(case='continuous-split-stop', distance=distance, own_updates=own_count, own_stop_ms=ack_ms))

        if not legacy:
            marker = mover.sequence
            burst = b''.join(packet('CS_MOVE', direction, 0) for direction in [1, 0] * 100)
            burst_at = time.perf_counter()
            mover.writer.write(burst)
            await mover.writer.drain()
            await mover.expect('SC_MOVE_PLAYER', marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == mover.player_id and unpack('SC_MOVE_PLAYER', body)[4] == 0)
            await asyncio.sleep(.3)
            own_burst = [values[4] for at, values in mover.observations if at >= burst_at and values[0] == mover.player_id]
            assert own_burst[:200] == [1, 0] * 100, '유저 입력/본인 응답 순서 유실'
            settled = time.perf_counter()
            await asyncio.sleep(.15)
            assert not any(at >= settled and values[0] == mover.player_id for at, values in mover.observations), '빠른 입력 뒤 중복 갱신'
            await mover.send('CS_MOVE', 1, 0)
            await asyncio.sleep(.08)
            assert unpack('SC_MOVE_PLAYER', mover.moves[mover.player_id][1])[4] == 1, '정지 뒤 재시작 실패'
            await mover.send('CS_MOVE', 0, 0)
            rows.append(dict(case='200-coalesced-inputs-and-restart', passed=True))

            for _ in range(10):
                await mover.send('CS_MOVE', 1, 0)
                await mover.reconnect(8910)
                await asyncio.sleep(.05)
                assert mover.error is None, mover.error
                assert unpack('SC_MOVE_PLAYER', mover.moves[mover.player_id][1])[4] == 0, '재접속에 이전 이동 유입'
            rows.append(dict(case='moving-disconnect-reconnect', repeats=10, passed=True))
        assert all(peer.error is None for peer in peers), [peer.error for peer in peers]
        return rows
    finally:
        for peer in peers:
            await peer.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--legacy', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = {'legacy': args.legacy, 'passed': False}
    try:
        result['cases'] = asyncio.run(exercise(args.legacy))
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
