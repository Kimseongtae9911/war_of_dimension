import copy
import sys
import json
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tick_profile import TOP, analyze, profile_delta, summarize_profile


def snapshot():
    return {'schema_version': 1, 'enabled': True,
            'stages': {name: dict(calls=0, wall_ns=0, cycles=0, cycle_samples=0, cycle_errors=0, work=0)
                       for name in ('total', *TOP, 'height_ray')}}


class TickProfileContracts(unittest.TestCase):
    def test_enabled_run_without_profile_is_preserved_as_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'run'
            (output / 'lab/runs').mkdir(parents=True)
            (output / 'report.json').write_text(json.dumps({'clients': 4, 'ok': True, 'profile_enabled': True, 'run_id': 'missing', 'servers': {}}), encoding='utf-8')
            def sample(time):
                return {'timestamp_ms': time, 'servers': [{'role': 'GameServer', 'status': 'live', 'instance': 'same', 'timestamp_ms': time, 'uptime_ms': time, 'sequence': time}]}
            raw = {'phases': [{'phase': 'measure', 'timestamp_ms': 1000}, {'phase': 'verify', 'timestamp_ms': 4000}], 'samples': [sample(1000), sample(2000), sample(3000)]}
            (output / 'lab/runs/missing.json').write_text(json.dumps(raw), encoding='utf-8')
            result = analyze(Path(directory))
            self.assertEqual(result['groups'][0]['passed'], 0)
            self.assertIn('profile_error', result['records'][0])

    def test_missing_disabled_and_empty_are_distinct(self):
        self.assertIsNone(profile_delta(None, None))
        disabled = {**snapshot(), 'enabled': False}
        self.assertIsNone(profile_delta(disabled, disabled))
        self.assertEqual(profile_delta(snapshot(), snapshot())['ticks'], 0)

    def test_nested_work_and_cycles_are_not_added_to_top(self):
        before, after = snapshot(), snapshot()
        after['stages']['total'].update(calls=2, wall_ns=40_000_000, cycles=1000, cycle_samples=2)
        after['stages']['player_update'].update(calls=8, wall_ns=30_000_000, cycles=800, cycle_samples=8)
        after['stages']['height_ray'].update(calls=8, wall_ns=25_000_000, cycles=700, cycle_samples=8, work=80)
        result = profile_delta(before, after)
        self.assertEqual(result['stages']['height_ray']['wall_ms_per_tick'], 12.5)
        self.assertEqual(result['stages']['height_ray']['cycle_share_percent'], 70)
        self.assertEqual(result['stages']['height_ray']['work_per_call'], 10)
        self.assertEqual(result['top_residual_ms_per_tick'], 5)

    def test_failure_and_bad_counters_are_not_zero_filled(self):
        before, after = snapshot(), snapshot()
        after['stages']['total'].update(calls=1, wall_ns=1, cycle_errors=1)
        self.assertIsNone(profile_delta(before, after)['stages']['total']['cycle_share_percent'])
        for key, value in [('cycles', -1), ('calls', True)]:
            broken = copy.deepcopy(after)
            broken['stages']['total'][key] = value
            with self.assertRaises(ValueError):
                profile_delta(before, broken)
        after['enabled'] = False
        with self.assertRaises(ValueError):
            profile_delta(before, after)

    def test_measurement_excludes_warmup_and_rejects_restart(self):
        before, after = snapshot(), snapshot()
        after['stages']['total'].update(calls=2, wall_ns=40, cycles=20, cycle_samples=2)
        def sample(time, profile, instance='same'):
            return {'timestamp_ms': time, 'servers': [{'role': 'GameServer', 'status': 'live', 'instance': instance,
                    'timestamp_ms': time, 'uptime_ms': time, 'sequence': time, 'tick_profile': profile}]}
        result = {'phases': [{'phase': 'measure', 'timestamp_ms': 1000}, {'phase': 'verify', 'timestamp_ms': 4000}],
                  'samples': [sample(0, snapshot()), sample(1000, before), sample(2000, before), sample(3000, after), sample(5000, snapshot())]}
        self.assertEqual(summarize_profile(result)['ticks'], 2)
        result['samples'][3]['servers'][0]['instance'] = 'restarted'
        with self.assertRaises(ValueError):
            summarize_profile(result)


if __name__ == '__main__':
    unittest.main()
