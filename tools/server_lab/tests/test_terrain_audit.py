import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from terrain_audit import inspect


class TerrainAudit(unittest.TestCase):
    def test_groups_duplicates_and_degenerate_are_diagnostic_only(self):
        data = 'v 0 2 0\nv 4 2 0\nv 0 2 4\no Ground\ng top\nf 1 2 3\nf 3 2 1\ng broken\nf 1 1 2\n'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'mesh.obj'
            path.write_text(data, encoding='utf-8')
            result = inspect(path)
            self.assertEqual(path.read_text(), data)
            self.assertTrue(result['has_source_groups'])
            self.assertEqual(result['triangles'], 3)
            self.assertEqual(result['flags']['duplicate_geometry'], 1)
            self.assertEqual(result['flags']['zero_area'], 1)
            self.assertEqual([group['triangles'] for group in result['groups']], [2, 1])

    def test_bad_input_is_not_silently_discarded(self):
        for data in ('', 'v nan 2 0\nf 1 1 1\n', 'v 0 2 0\nf 1 2 3\n', 'v 0 2 0\nf 1/1 1/1 1/1\n', 'v 0 2 0\nf 1 1 1 1\n'):
            with self.subTest(data=data), tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'mesh.obj'
                path.write_text(data, encoding='utf-8')
                with self.assertRaises(ValueError):
                    inspect(path)


if __name__ == '__main__':
    unittest.main()
