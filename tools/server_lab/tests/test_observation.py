import asyncio
import http.client
import json
import math
import struct
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch
from http.server import ThreadingHTTPServer
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import protocol
from monitor import Lab, handler
from observation import MAX_NPCS_PER_GROUP, TRAIL_POINTS, WorldObservation, position
from runner import Peer, RunState, SCENARIOS, load_scenario


def npc(world, observer, name, *values, stamp=1000):
    world.npc_packet(observer, protocol.CONSTANTS[name], protocol.packet(name, *values)[2:], stamp)


def add(world, observer=0, npc_id=7, npc_type=1, x=0, stamp=1000):
    npc(world, observer, 'SC_ADD_NPC', npc_type, npc_id, x, 2, 3, 0, 0, 1, 1, 0, 0, stamp=stamp)


def move(world, observer=0, npc_id=7, x=1, stamp=1200, idle=False):
    npc(world, observer, 'SC_MOVE_NPC', npc_id, 0, x, 2, 3, 1, 0, 0, 0, 0, -1, idle, stamp=stamp)


class ObservationTests(unittest.TestCase):
    def schedule(self, run_id='abcde0000000', group=0, **fields):
        sample = {'match_id': 17, 'timestamp_ms': 10000, 'game_ms': 2000, 'next_wave_ms': 48000,
                  'fence_at_game_ms': 180000, 'active_minions': 0, 'fence': True, 'finished': False,
                  'member_keys': [(run_id[:5] + f'{index:03x}').encode().hex() for index in range(group * 4, group * 4 + 4)],
                  'spawns': [{'id': 0, 'due_ms': 12000}, {'id': 1, 'due_ms': 14000}]}
        sample.update(fields)
        return sample

    def test_authoritative_schedule_group_countdown_stale_end_and_restart(self):
        world = WorldObservation('abcde0000000')
        world.peer(0, port=8911, state='connected')
        world.peer(4, port=8911, state='connected')
        samples = [self.schedule(), self.schedule(group=1, match_id=32, game_ms=1000), self.schedule(run_id='other')]
        world.apply_schedule({'schema_version': 1, 'matches': samples}, 'pid-one')
        with patch('observation.now_ms', return_value=11000):
            data = world.snapshot()['timelines']
        self.assertEqual(len(data), 2)
        self.assertEqual((data[0]['group'], data[0]['match_id'], data[0]['game_display_ms']), (0, 17, 3000))
        self.assertEqual((data[0]['wave_remaining_ms'], data[0]['fence_remaining_ms'], data[0]['spawns'][0]['remaining_ms']), (37000, 177000, 1000))
        self.assertNotIn('member_keys', data[0])
        with patch('observation.now_ms', return_value=15000):
            stale = world.snapshot()['timelines'][0]
        self.assertTrue(stale['stale'])
        self.assertFalse(stale['live'])
        self.assertEqual(stale['game_display_ms'], 2000)
        with patch('observation.now_ms', return_value=12000):
            ended = world.snapshot('passed')['timelines'][0]
        self.assertEqual(ended['spawns'][0]['remaining_ms'], 2000)
        for stopped, finished in ((True, False), (False, True)):
            world.apply_schedule({'schema_version': 1, 'matches': [self.schedule(finished=finished)]}, 'pid-one', stopped)
            with patch('observation.now_ms', return_value=11000):
                frozen = world.snapshot()['timelines'][0]
            self.assertFalse(frozen['live'])
            self.assertEqual(frozen['game_display_ms'], 2000)
        world.peer(0, state='disconnected')
        world.apply_schedule({'schema_version': 1, 'matches': [self.schedule()]}, 'pid-one')
        with patch('observation.now_ms', return_value=11000):
            self.assertFalse(world.snapshot()['timelines'][0]['live'])
        world.apply_schedule({'schema_version': 1, 'matches': []}, 'pid-two')
        self.assertEqual(world.snapshot()['timelines'], [])

    def test_schedule_validation_capacity_gate_confirmation_and_freeze(self):
        world = WorldObservation('abcde0000000')
        world.peer(0, port=8911, state='connected')
        for fields in ({'spawns': [{'id': 99, 'due_ms': 1}]}, {'active_minions': 13}, {'game_ms': -1},
                       {'fence': 1}, {'spawns': [{'id': 1, 'due_ms': 1}] * 2}, {'member_keys': ['bad'] * 4}):
            world.apply_schedule({'schema_version': 1, 'matches': [self.schedule(**fields)]}, 'pid')
        self.assertEqual(world.snapshot()['timelines'], [])
        world.apply_schedule({'schema_version': 1, 'matches': [self.schedule(game_ms=181000, fence=False, active_minions=12, spawns=[])]}, 'pid')
        data = world.snapshot()['timelines'][0]
        self.assertFalse(data['fence'])
        self.assertEqual((data['fence_remaining_ms'], data['active_minions'], data['spawns']), (0, 12, []))
        world.freeze()
        world.apply_schedule({'schema_version': 1, 'matches': [self.schedule()]}, 'pid')
        self.assertFalse(world.snapshot()['timelines'][0]['fence'])

    def world(self):
        world = WorldObservation('run-one')
        for index in (0, 1, 4):
            world.peer(index, port=8911, state='connected', player_id=index % 4)
        return world

    def test_npc_codec_order_and_abi_fields(self):
        data = protocol.packet('SC_ADD_NPC', 1, 7, *range(9))
        self.assertEqual(data[:10], bytes((46, protocol.CONSTANTS['SC_ADD_NPC'])) + struct.pack('<ii', 1, 7))
        for name in ('SC_ADD_NPC', 'SC_MOVE_NPC', 'SC_REMOVE_NPC', 'SC_NPC_STAT_CHANGE', 'SC_NPC_ATTACK', 'SC_ROTATE_PLAYER', 'SC_SKILL_FINISH'):
            field = protocol.TYPES[name + '_PACKET']['fields'][0]
            old = field['offset']
            try:
                field['offset'] += 1
                with self.subTest(name=name), self.assertRaises(ValueError):
                    protocol.validate_abi()
            finally:
                field['offset'] = old

    def test_group_dedup_failover_remove_respawn_and_type(self):
        world = self.world()
        for observer in (0, 1, 4):
            add(world, observer)
            npc(world, observer, 'SC_NPC_ATTACK', 7)
        snapshot = world.snapshot()
        self.assertEqual(len(snapshot['npcs']), 2)
        self.assertEqual([entity['attacks'] for entity in snapshot['npcs']], [1, 1])
        self.assertEqual(len(snapshot['events']), 4)
        world.peer(0, state='disconnected')
        move(world, 1, x=2)
        entity = world.snapshot()['npcs'][0]
        self.assertEqual((entity['observer'], entity['npc_type'], entity['x']), (1, 1, 2))
        npc(world, 1, 'SC_NPC_STAT_CHANGE', 7, 100, 25)
        npc(world, 1, 'SC_REMOVE_NPC', 7, 0)
        move(world, 1, x=99)
        entity = world.snapshot()['npcs'][0]
        self.assertTrue(entity['removed'])
        self.assertEqual((entity['x'], entity['hp']), (2, 25))
        add(world, 1, x=4, stamp=2000)
        entity = world.snapshot()['npcs'][0]
        self.assertFalse(entity['removed'])
        self.assertEqual((len(entity['trail']), entity['attacks']), (1, 0))
        self.assertNotIn('hp', entity)

    def test_bounded_trails_objects_events_and_detached_snapshot(self):
        world = self.world()
        add(world)
        for index in range(1000):
            move(world, x=index, stamp=1200 + index * 100)
        entity = world.snapshot()['npcs'][0]
        self.assertEqual(len(entity['trail']), TRAIL_POINTS)
        self.assertEqual(entity['speed'], 10)
        entity['trail'][0][1] = -999
        self.assertNotEqual(world.snapshot()['npcs'][0]['trail'][0][1], -999)
        for index in range(MAX_NPCS_PER_GROUP + 2):
            add(world, npc_id=index)
        data = world.snapshot()
        self.assertEqual(len(data['npcs']), MAX_NPCS_PER_GROUP)
        self.assertEqual(len(data['events']), 100)
        self.assertEqual(data['dropped'], 2)
        stationary = {}
        for stamp in range(0, 1000, 10):
            position(stationary, 1, 2, 3, stamp)
        self.assertEqual(len(stationary['trail']), 1)

    def test_bad_frame_finite_coordinates_and_port_reset(self):
        world = self.world()
        with self.assertRaises(struct.error):
            world.npc_packet(0, protocol.CONSTANTS['SC_ADD_NPC'], b'bad')
        for observer in (0, 1):
            with self.assertRaises(ValueError):
                add(world, observer, x=float('nan'))
        with self.assertRaises(ValueError):
            npc(world, 0, 'SC_NPC_ATTACK', -1)
        world.peer(0, x=1, y=2, z=3)
        self.assertEqual(len(world.snapshot()['players']), 1)
        world.peer(0, port=8910, state='connected')
        self.assertEqual(world.snapshot()['players'], [])
        self.assertEqual(world.source(0), 1)
        world.peer(1, state='disconnected')
        self.assertIsNone(world.source(0))

    def test_player_rotation_skill_and_saved_report(self):
        state = RunState(load_scenario(SCENARIOS / 'match-actions.json'))
        state.peer(0, port=8911, state='connected', player_id=0, x=1, y=2, z=3)
        state.world.player_packet(0, 'SC_ROTATE_PLAYER', protocol.packet('SC_ROTATE_PLAYER', 0, 1, 0, 0, 0, 0, -1)[2:])
        state.world.player_packet(0, 'SC_SKILL', protocol.packet('SC_SKILL', 0, 1, 0, False)[2:])
        player = state.world.snapshot()['players'][0]
        self.assertEqual((player['look_x'], player['look_z'], player['action']), (1, 0, 'skill'))
        with tempfile.TemporaryDirectory() as folder:
            saved = json.loads(state.save(folder).read_text(encoding='utf-8'))
            self.assertEqual(saved['world']['run_id'], state.run_id)
            self.assertEqual(saved['world']['players'][0]['x'], 1)
        fresh = RunState(state.scenario)
        self.assertEqual(fresh.world.snapshot()['players'], [])

    def test_world_http_idle_and_observed_state(self):
        with tempfile.TemporaryDirectory() as folder:
            lab = Lab(folder)
            server = ThreadingHTTPServer(('127.0.0.1', 0), handler(lab))
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            connection = http.client.HTTPConnection('127.0.0.1', server.server_port, timeout=3)
            try:
                connection.request('GET', '/api/world')
                data = json.loads(connection.getresponse().read())
                self.assertIsNone(data['run_id'])
                lab.state = RunState(load_scenario(SCENARIOS / 'match-actions.json'))
                lab.state.peer(0, port=8911, state='connected', player_id=0, x=1, y=2, z=3)
                add(lab.state.world)
                connection.request('GET', '/api/world')
                data = json.loads(connection.getresponse().read())
                self.assertEqual((len(data['players']), len(data['npcs'])), (1, 1))
                self.assertEqual(data['sources'][0], {'group': 0, 'observer': 0})
            finally:
                connection.close()
                server.shutdown()
                server.server_close()
                thread.join()
                lab.close()


class ObservationWireTests(unittest.IsolatedAsyncioTestCase):
    async def test_npc_split_flood_and_control_response(self):
        state = RunState(load_scenario(SCENARIOS / 'match-actions.json'))
        add_packet = protocol.packet('SC_ADD_NPC', 1, 7, 0, 2, 3, 0, 0, 1, 1, 0, 0)
        move_packet = protocol.packet('SC_MOVE_NPC', 7, 0, 1, 2, 3, 1, 0, 0, 0, 0, -1, False)
        async def responder(reader, writer):
            writer.write(add_packet[:1])
            await writer.drain()
            await asyncio.sleep(.01)
            writer.write(add_packet[1:] + move_packet * 5000 + protocol.packet('SC_NPC_ATTACK', 7) + protocol.packet('SC_LOGIN_INFO', 8, 1, 2, 3, bytes(56)))
            await writer.drain()
            await reader.read()
            writer.close()
            await writer.wait_closed()
        async with await asyncio.start_server(responder, '127.0.0.1', 0) as server:
            peer = Peer(state, 0, bytes(10), 3)
            await peer.connect(server.sockets[0].getsockname()[1])
            state.peer(0, port=8911, state='connected')
            try:
                body = await peer.expect('SC_LOGIN_INFO')
                self.assertEqual(protocol.unpack('SC_LOGIN_INFO', body)[0], 8)
                self.assertEqual(len(peer.history), 2)
                snapshot = state.world.snapshot()
                self.assertEqual(snapshot['npcs'][0]['attacks'], 1)
                self.assertLessEqual(len(snapshot['npcs'][0]['trail']), TRAIL_POINTS)
            finally:
                await peer.close()
            self.assertEqual(state.counters['connected'], 0)


if __name__ == '__main__':
    unittest.main()
