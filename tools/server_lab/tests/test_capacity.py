import asyncio
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from capacity import exercise, levels, analyze_result
from runner import Peer, RunState, SCENARIOS, load_scenario
from protocol import packet
from observation import WorldObservation


class CapacityContracts(unittest.TestCase):
    def test_levels_and_reject_ambiguous_load(self):
        self.assertEqual(levels(24, [4, 16, 32]), [4, 16, 24])
        self.assertEqual(levels(4, [4, 16]), [4])
        original = load_scenario(SCENARIOS / 'capacity-match.json')
        for steps in ([4, 4], [16, 4], [3], [], [True]):
            with self.subTest(steps=steps), tempfile.TemporaryDirectory() as tmp:
                bad = copy.deepcopy(original)
                bad['capacity']['steps'] = steps
                path = Path(tmp) / 'bad.json'
                path.write_text(json.dumps(bad), encoding='utf-8')
                with self.assertRaises(ValueError):
                    load_scenario(path)
        state = RunState(original)
        state.capacity_stages.append({'clients': 4, 'status': 'failed', 'error': 'login'})
        snapshot = state.snapshot()
        snapshot['capacity_stages'][0]['status'] = 'passed'
        self.assertEqual(state.capacity_stages[0]['status'], 'failed')
        result = state.snapshot(full=True)
        result['samples'] = []
        self.assertEqual(analyze_result(result)[0]['error'], 'login')
        self.assertNotIn('servers', analyze_result(result)[0])

    def test_schedule_beyond_eighth_group(self):
        world = WorldObservation('abcde1234')
        world.peer(36, port=8911, state='connected')
        sample = {'match_id': 99, 'timestamp_ms': 10000, 'game_ms': 2000, 'next_wave_ms': 20000,
                  'fence_at_game_ms': 180000, 'active_minions': 4, 'fence': True, 'finished': False,
                  'member_keys': [(world.run_id[:5]+f'{i:03x}').encode().hex() for i in range(36,40)], 'spawns': []}
        world.apply_schedule({'schema_version': 1, 'matches': [sample]}, 'server')
        self.assertEqual(world.timelines[9]['match_id'], 99)
        sample['member_keys'][0] = 'wrong'
        other = WorldObservation('abcde1234')
        other.peer(36, port=8911, state='connected')
        other.apply_schedule({'schema_version': 1, 'matches': [sample]}, 'server')
        self.assertFalse(other.timelines)

    def test_source_switch_stays_within_group(self):
        world = WorldObservation('abcde1234')
        for index in range(1024):
            world.peer(index, port=8911, state='connected')
        self.assertEqual(world.source(127), 508)
        world.peer(508, state='disconnected')
        self.assertEqual(world.source(127), 509)
        world.peer(509, port=8910)
        self.assertEqual(world.source(127), 510)
        self.assertEqual(world.source(128), 512)
        self.assertIsNone(world.source(300))

    def test_progress_has_no_series_and_preserves_failure(self):
        from monitor import Lab
        with tempfile.TemporaryDirectory() as tmp:
            lab = Lab(tmp)
            try:
                self.assertEqual(lab.progress(), {'busy': False, 'run': None})
                lab.state = RunState(load_scenario(SCENARIOS/'capacity-lobby.json'))
                lab.state.capacity_stages.append({'clients': 4, 'status': 'failed', 'error': 'injected'})
                response = lab.progress()
                self.assertNotIn('series', response)
                self.assertEqual(response['run']['capacity_stages'][0]['error'], 'injected')
                response['run']['capacity_stages'][0]['error'] = 'changed'
                self.assertEqual(lab.state.capacity_stages[0]['error'], 'injected')
            finally:
                lab.close()


class FakePeer:
    def __init__(self, index, state):
        self.index, self.state = index, state
        self.measure_moves = 0
        self.measure_distance = 0
        self.measure_position = None
        self.error = None
        self.port = 8911
        self.player_id = index % 4
        self.sequence = 0
        self.timeout = .1

    async def send(self, *args):
        self.state.count('sent_packets')
        self.sequence += 1
        if self.state.current_phase == 'measure':
            self.measure_moves += 1
            self.measure_distance += 1

    async def expect(self, *args, **kwargs):
        return b'ok'

    async def check(self, stage, operation):
        return await operation()


class CapacityLifecycle(unittest.IsolatedAsyncioTestCase):
    def state(self):
        scenario = load_scenario(SCENARIOS/'capacity-match.json')
        scenario['capacity'] = {'steps': [4, 8], 'warmup_s': .01, 'duration_s': .15, 'interval_s': .05}
        state = RunState(scenario, 8)
        return state, [FakePeer(i, state) for i in range(8)]

    async def test_previous_peers_remain_active_and_failure_is_preserved(self):
        state, peers = self.state()
        setup_sizes = []
        async def setup(group, scenario):
            setup_sizes.append(len(group))
            if len(setup_sizes) == 2:
                before = peers[0].sequence
                await asyncio.sleep(.08)
                self.assertGreater(peers[0].sequence, before)
                raise RuntimeError('injected login failure')
            state.counters['connected'] += len(group)
        with self.assertRaisesRegex(RuntimeError, 'injected login failure'):
            await exercise(state, peers, setup)
        self.assertEqual(setup_sizes, [4, 4])
        self.assertEqual(state.capacity_stages[0]['status'], 'passed')
        self.assertEqual(state.capacity_stages[1]['ready_clients'], 4)
        self.assertEqual(state.capacity_stages[1]['status'], 'failed')
        self.assertEqual(state.current_phase, 'cleanup')
        self.assertEqual(len([t for t in asyncio.all_tasks() if t is not asyncio.current_task() and not t.done()]), 0)

    async def test_cancellation_stops_driver(self):
        state, peers = self.state()
        async def setup(group, scenario):
            await asyncio.sleep(10)
        task = asyncio.create_task(exercise(state, peers, setup))
        await asyncio.sleep(.01)
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertEqual(state.capacity_stages[0]['status'], 'failed')
        self.assertEqual(state.capacity_stages[0]['error'], 'CancelledError')
        self.assertEqual(len([t for t in asyncio.all_tasks() if t is not asyncio.current_task() and not t.done()]), 0)

    async def test_buffered_receive_yields_to_control_and_headless_rejects_bad_length(self):
        with patch.dict('os.environ', {'WOD_LAB_HEADLESS': '1'}):
            state = RunState(load_scenario(SCENARIOS/'capacity-match.json'), 4)
        peer = Peer(state, 0, b'test', 1)
        peer.reader = asyncio.StreamReader()
        frame = packet('SC_MOVE_NPC', 7, 0, *([1.] * 9), False)
        peer.reader.feed_data(frame * 4096)
        task = asyncio.create_task(peer.receive())
        await asyncio.sleep(0)
        self.assertGreater(peer.sequence, 0)
        self.assertLess(peer.sequence, 4096)
        task.cancel()
        await asyncio.gather(task, return_exceptions=True)
        self.assertFalse(state.snapshot()['world_observation'])
        self.assertFalse(state.snapshot(full=True)['world']['enabled'])
        peer.reader = asyncio.StreamReader()
        peer.reader.feed_data(bytes((2, frame[1])))
        await peer.receive()
        self.assertIn('프레임 길이', peer.error)


if __name__ == '__main__':
    unittest.main()
