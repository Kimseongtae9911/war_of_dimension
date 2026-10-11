import asyncio
import copy
import http.client
import json
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path
from http.server import ThreadingHTTPServer

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import protocol
from monitor import Lab, ServerFeed, NUMERIC_FIELDS, derive_rates, handler
from runner import Peer, RunState, SCENARIOS, load_scenario, percentile, run


class Contracts(unittest.TestCase):
    def test_golden_login_move(self):
        self.assertEqual(protocol.packet('CS_LOGIN', b'a' + bytes(9), bytes(10)), bytes([22, 1, 97]) + bytes(19))
        self.assertEqual(protocol.packet('CS_MOVE', 9, 0), bytes([11, 2, 9]) + bytes(8))
        self.assertEqual(protocol.CONSTANTS['SC_MATCH'], 5)

    def test_changed_abi_refused(self):
        old = protocol.TYPES['CS_MOVE_PACKET']['fields'][1]['offset']
        try:
            protocol.TYPES['CS_MOVE_PACKET']['fields'][1]['offset'] = 4
            with self.assertRaises(ValueError):
                protocol.validate_abi()
        finally:
            protocol.TYPES['CS_MOVE_PACKET']['fields'][1]['offset'] = old

    def test_scenario_validation(self):
        original = load_scenario(SCENARIOS / 'match-actions.json')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'case.json'
            for field, value in [('clients', 5), ('clients', True), ('timeout_s', float('nan')), ('execution', 'bad'), ('actions', [{'op': []}]), ('actions', [{'op': 'move', 'direction': True}]), ('actions', [{'op': 'unknown'}]), ('actions', [{'op': 'skill', 'slot': 6}]), ('actions', [{'op': 'wait', 'duration_s': -1}]), ('extra', 1)]:
                with self.subTest(field=field, value=value):
                    data = copy.deepcopy(original)
                    data[field] = value
                    path.write_text(json.dumps(data), encoding='utf-8')
                    with self.assertRaises(ValueError):
                        load_scenario(path)

    def test_report_failure_and_percentiles(self):
        state = RunState(load_scenario(SCENARIOS / 'match-actions.json'))
        state.event(0, 'bad', 'fail', 'response timeout', 50)
        state.status = 'failed'
        with tempfile.TemporaryDirectory() as directory:
            path = state.save(directory)
            self.assertEqual(json.loads(path.read_text(encoding='utf-8'))['assertions'][0]['detail'], 'response timeout')
            self.assertIn('response timeout', path.with_suffix('.csv').read_text(encoding='utf-8-sig'))
        self.assertIsNone(percentile([], .95))
        self.assertEqual(percentile([1, 2, 3, 4], .95), 4)


class Wire(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.state = RunState(load_scenario(SCENARIOS / 'lobby-cycle.json'), clients=1)

    async def exchange(self, responder, wanted='SC_LOGIN_INFO'):
        async with await asyncio.start_server(responder, '127.0.0.1', 0) as server:
            peer = Peer(self.state, 0, bytes(10), .15)
            await peer.connect(server.sockets[0].getsockname()[1])
            try:
                return await peer.expect(wanted)
            finally:
                await peer.close()
                self.assertEqual(self.state.counters['connected'], 0)

    async def test_split_and_batch(self):
        async def responder(_reader, writer):
            data = protocol.packet('SC_LOGIN_INFO', 7, 1, 2, 3, bytes(56))
            writer.write(data[:1])
            await writer.drain()
            await asyncio.sleep(.01)
            writer.write(data[1:] + data)
            await writer.drain()
            writer.close()
            await writer.wait_closed()
        body = await self.exchange(responder)
        self.assertEqual(protocol.unpack('SC_LOGIN_INFO', body)[0], 7)
        self.assertEqual(self.state.counters['received_packets'], 2)

    async def test_bad_frame_and_eof(self):
        for data in (b'\1\1', b'\x4a\1bad'):
            async def responder(_reader, writer):
                writer.write(data)
                await writer.drain()
                writer.close()
                await writer.wait_closed()
            with self.assertRaises(ConnectionError):
                await self.exchange(responder)

    async def test_movement_flood_keeps_control_response(self):
        async def responder(reader, writer):
            move = protocol.packet('SC_MOVE_PLAYER', 0, 1, 2, 3, 1, 0)
            login = protocol.packet('SC_LOGIN_INFO', 7, 1, 2, 3, bytes(56))
            writer.write(login + move * 5000)
            await writer.drain()
            await reader.read()
            writer.close()
            await writer.wait_closed()
        async with await asyncio.start_server(responder, '127.0.0.1', 0) as server:
            peer = Peer(self.state, 0, bytes(10), 2)
            await peer.connect(server.sockets[0].getsockname()[1])
            try:
                deadline = asyncio.get_running_loop().time() + 2
                while self.state.counters['received_packets'] < 5001 and asyncio.get_running_loop().time() < deadline:
                    await asyncio.sleep(.01)
                body = await peer.expect('SC_LOGIN_INFO')
                self.assertEqual(protocol.unpack('SC_LOGIN_INFO', body)[0], 7)
                self.assertEqual(len(peer.moves), 1)
                self.assertEqual(len(peer.history), 1)
            finally:
                await peer.close()

    async def test_timeout_has_deadline(self):
        async def responder(reader, writer):
            await reader.read()
            writer.close()
            await writer.wait_closed()
        with self.assertRaises(TimeoutError):
            await self.exchange(responder)

    async def test_cancel_before_start(self):
        self.state.cancel()
        await run(self.state)
        self.assertEqual(self.state.status, 'cancelled')
        self.assertEqual(self.state.counters['connected'], 0)


class Observation(unittest.TestCase):
    def snapshot(self):
        return {**{key: 0 for key in NUMERIC_FIELDS}, 'schema_version': 1, 'role': 'LobbyServer', 'instance': 'process-one', 'stopped': False,
                'sequence': 1, 'logical_processors': 2, 'timestamp_ms': time.time_ns() // 1_000_000}

    def test_rates_and_restart(self):
        previous = self.snapshot()
        current = {**previous, 'sequence': 2, 'uptime_ms': 1000, 'cpu_ticks': 10_000_000, 'received_bytes': 100,
                   'sent_bytes': 200, 'jobs_completed': 2, 'jobs_failed': 1, 'job_execute_us': 300}
        rates = derive_rates(previous, current)
        self.assertEqual(rates['cpu_percent'], 50)
        self.assertEqual(rates['rx_bps'], 100)
        self.assertEqual(rates['job_mean_us'], 100)
        for changed in ({'instance': 'new'}, {'sequence': 1}, {'uptime_ms': 0}, {'sent_bytes': -1}):
            self.assertIsNone(derive_rates(previous, {**current, **changed})['cpu_percent'])

    def test_missing_stale_invalid(self):
        with tempfile.TemporaryDirectory() as directory:
            feed = ServerFeed(directory)
            self.assertEqual(feed.sample()[0]['status'], 'missing')
            path = Path(directory) / 'LobbyServer.json'
            data = self.snapshot()
            path.write_text(json.dumps(data), encoding='utf-8')
            self.assertEqual(feed.sample()[0]['status'], 'live')
            data['timestamp_ms'] -= 5000
            path.write_text(json.dumps(data), encoding='utf-8')
            self.assertEqual(feed.sample()[0]['status'], 'stale')
            path.write_text('{bad', encoding='utf-8')
            self.assertEqual(feed.sample()[0]['status'], 'invalid')

    def test_http_scope_and_input(self):
        with tempfile.TemporaryDirectory() as directory:
            lab = Lab(directory)
            server = ThreadingHTTPServer(('127.0.0.1', 0), handler(lab))
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            connection = http.client.HTTPConnection('127.0.0.1', server.server_port, timeout=2)
            try:
                connection.request('GET', '/api/scenarios')
                response = connection.getresponse()
                self.assertEqual(response.status, 200)
                self.assertEqual(len(json.loads(response.read())), 5)
                connection.request('POST', '/api/run', json.dumps({'scenario': 'match-actions', 'clients': 5}))
                response = connection.getresponse()
                self.assertEqual(response.status, 400)
                response.read()
                connection.request('POST', '/api/stop', '{}', headers={'Origin': 'https://example.invalid'})
                response = connection.getresponse()
                self.assertEqual(response.status, 403)
                response.read()
                connection.request('GET', '/api/result?id=../bad')
                response = connection.getresponse()
                self.assertEqual(response.status, 400)
                response.read()
            finally:
                connection.close()
                server.shutdown()
                server.server_close()
                thread.join()
                lab.close()


if __name__ == '__main__':
    unittest.main()
