import asyncio
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from settlement import DrainCheck, wait_for_drain
from runner import RunState, load_scenario, SCENARIOS


def sample(sequence=1, pending=0, sends=0, processed=10):
    return {'status': 'live', 'timestamp_ms': 1000 + sequence, 'sequence': sequence, 'instance': 'a',
            'performance': {'packets_processed': processed, 'pending_packets': {'count': pending}}, 'send_queue': {'count': sends}}


class DrainContracts(unittest.TestCase):
    def test_settlement_scenario_is_explicit_and_lobby_only(self):
        scenario = load_scenario(SCENARIOS/'soak-lobby.json')
        self.assertEqual(scenario['performance']['duration_s'], 120)
        for value in (True, 0, 61, float('nan')):
            scenario['performance']['settle_s'] = value
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory)/'bad.json'
                path.write_text(json.dumps(scenario), encoding='utf-8')
                with self.assertRaises(ValueError):
                    load_scenario(path)
        scenario['performance']['settle_s'] = 20
        scenario['setup'] = 'match'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'bad.json'
            path.write_text(json.dumps(scenario), encoding='utf-8')
            with self.assertRaises(ValueError):
                load_scenario(path)

    def test_needs_processed_inputs_and_three_distinct_clean_samples(self):
        check = DrainCheck(1000, 10)
        self.assertFalse(check.observe(sample(1, processed=9), 1010))
        self.assertFalse(check.observe(sample(2), 1010))
        self.assertFalse(check.observe(sample(2), 1010))
        self.assertFalse(check.observe(sample(3, sends=1), 1010))
        self.assertFalse(check.observe(sample(4), 1010))
        self.assertFalse(check.observe(sample(5), 1010))
        self.assertTrue(check.observe(sample(6), 1010))

    def test_stale_missing_and_pre_stop_samples_do_not_prove_drain(self):
        for row in ({}, sample(1), {**sample(2), 'status': 'stale'}, {**sample(3), 'send_queue': {}}, {**sample(4), 'timestamp_ms': 1}):
            check = DrainCheck(1000, 10)
            self.assertFalse(check.observe(row, 6000))
        check = DrainCheck(1000, 10)
        self.assertFalse(check.observe(sample(1), 1010))
        self.assertFalse(check.observe({**sample(2), 'send_queue': {}}, 1010))
        self.assertEqual(check.clean_samples, 0)

    def test_restart_and_sequence_reversal_are_errors(self):
        check = DrainCheck(1000, 10)
        check.observe(sample(2), 1010)
        with self.assertRaisesRegex(ValueError, '재시작'):
            check.observe({**sample(3), 'instance': 'b'}, 1010)
        with self.assertRaisesRegex(ValueError, '감소'):
            check.observe(sample(1), 1010)


class DrainLifecycle(unittest.IsolatedAsyncioTestCase):
    async def test_missing_monitor_is_failure_and_cancellation_is_preserved(self):
        state = RunState(load_scenario(SCENARIOS/'perf-lobby.json'))
        with self.assertRaises(TimeoutError):
            await wait_for_drain(state, .03, .01)
        self.assertEqual(state.settlement['status'], 'failed')
        self.assertEqual(state.counters['failed'], 1)
        task = asyncio.create_task(wait_for_drain(state, 10, .01))
        await asyncio.sleep(.01)
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertEqual(state.settlement['status'], 'cancelled')
        self.assertEqual(len([task for task in asyncio.all_tasks() if task is not asyncio.current_task() and not task.done()]), 0)


if __name__ == '__main__':
    unittest.main()
