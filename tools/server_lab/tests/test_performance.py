import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from baseline import summarize_run
from performance import HISTOGRAMS, histogram_delta
from runner import SCENARIOS, RunState, load_scenario, validate_clients


def histogram(bucket=0, count=0):
    values = [0] * 33
    values[bucket] = count
    return {'buckets': values, 'sum_us': count * (2 ** max(0, bucket - 1)), 'max_us': 0}


class PerformanceContracts(unittest.TestCase):
    def test_histogram_empty_bounds_overflow_and_restart(self):
        empty = histogram()
        self.assertIsNone(histogram_delta(empty, empty)['p99_us_upper'])
        self.assertEqual(histogram_delta(empty, histogram(3, 100))['p99_us_upper'], 4)
        overflow = histogram_delta(empty, histogram(32, 10))
        self.assertEqual(overflow['overflow'], 10)
        self.assertIsNone(overflow['p99_us_upper'])
        with self.assertRaises(ValueError):
            histogram_delta(histogram(1, 10), empty)
        with self.assertRaises(ValueError):
            histogram_delta(empty, {'buckets': [0], 'sum_us': 0})

    def test_expanded_clients_and_phases(self):
        validate_clients(64, 'lobby')
        validate_clients(128, 'match')
        for count in (True, 0, -1, 65):
            with self.assertRaises(ValueError):
                validate_clients(count, 'match')
        for name in ('perf-lobby', 'perf-match'):
            scenario = load_scenario(SCENARIOS / (name + '.json'))
            state = RunState(scenario, 64, 1)
            state.phase('measure')
            self.assertEqual(state.snapshot()['phases'][0]['phase'], 'measure')
            self.assertEqual(scenario['performance']['duration_s'], 20)

    def test_failed_run_keeps_interior_measurements_and_pending_wait(self):
        samples = []
        for index in range(5):
            rows = []
            for role in ('LobbyServer', 'GameServer'):
                performance = {name: histogram(3, index * 10) for name in HISTOGRAMS}
                performance.update(packets_processed=index * 20, move_requests=index * 10, stop_requests=index * 10,
                                   pending_packets={'count': 12, 'oldest_wait_us': index * 1000})
                rows.append({'role': role, 'status': 'live', 'instance': 'instance', 'sequence': index, 'timestamp_ms': index * 1000,
                             'uptime_ms': index * 1000, 'performance': performance, 'jobs_completed': index * 100,
                             'cpu_ticks': index * 10_000_000, 'logical_processors': 4, 'jobs_queued': index,
                             'private_bytes': 1000, 'jobs_failed': 0, 'io_errors': 0})
            samples.append({'timestamp_ms': index * 1000, 'servers': rows, 'observer_cpu_seconds': index * .1, 'logical_processors': 4})
        result = {'status': 'failed', 'error': 'response missing', 'clients': 4, 'counters': {'passed': 4, 'failed': 1},
                  'phases': [{'phase': 'measure', 'timestamp_ms': 1000}, {'phase': 'verify', 'timestamp_ms': 3000}], 'samples': samples}
        report = summarize_run(result)
        self.assertEqual(report['status'], 'failed')
        self.assertEqual(report['servers']['LobbyServer']['jobs_per_s'], 100)
        self.assertEqual(report['servers']['LobbyServer']['cpu_percent'], 25)
        self.assertEqual(report['servers']['LobbyServer']['job_wait_us']['samples'], 20)
        self.assertEqual(report['servers']['LobbyServer']['pending_packets_end'], 12)
        self.assertNotIn('send_queue_peak', report['servers']['LobbyServer'])
        observed = copy.deepcopy(result)
        for sample in observed['samples']:
            for row in sample['servers']:
                row.update(pending_io=40, send_queue={'count': 32, 'max_socket_count': 8, 'native_calls': row['sequence'] * 2,
                                                    'batches': row['sequence'], 'frames': row['sequence'] * 5})
        network = summarize_run(observed)['servers']['GameServer']
        self.assertEqual(network['pending_io_peak'], 40)
        self.assertEqual(network['send_queue_peak'], 32)
        self.assertEqual(network['max_socket_send_queue_peak'], 8)
        self.assertEqual(network['send_native_calls_per_s'], 2)
        self.assertEqual(network['send_frames_per_s'], 5)
        stale = copy.deepcopy(result)
        stale['samples'][2]['servers'][0]['status'] = 'stale'
        self.assertIn('stale', summarize_run(stale)['measurement_error'])
        restarted = copy.deepcopy(result)
        restarted['samples'][2]['servers'][0]['instance'] = 'new'
        self.assertIn('재시작', summarize_run(restarted)['measurement_error'])


if __name__ == '__main__':
    unittest.main()
