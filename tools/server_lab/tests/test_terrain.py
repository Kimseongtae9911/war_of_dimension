import base64
import http.client
import json
import math
import struct
import sys
import tempfile
import threading
import unittest
import zlib
from http.server import ThreadingHTTPServer
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from monitor import Lab, handler
from terrain import ROOT, TerrainStore, height_grid, png_height, read_obj, lobby_npcs


class TerrainTests(unittest.TestCase):
    def test_lobby_npc_original_positions_and_invalid_contract(self):
        data = lobby_npcs(ROOT)
        self.assertEqual([npc['id'] for npc in data['npcs']], [5, 6, 7, 8])
        self.assertEqual([npc['npc_type'] for npc in data['npcs']], [100, 101, 102, 103])
        self.assertEqual((data['npcs'][0]['x'], data['npcs'][3]['z']), (2.552067, -57.537098))
        self.assertTrue(all(npc['static'] for npc in data['npcs']))
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / data['source']
            path.parent.mkdir(parents=True)
            for text in ('not positions', 'm_npcPositions[LOBBY_NPC] = { XMFLOAT3(1,2,3) };',
                         'm_npcPositions[LOBBY_NPC] = {' + 'XMFLOAT3(nan,2,3),' * 4 + '};'):
                path.write_text(text, encoding='utf-8')
                with self.assertRaises(ValueError):
                    lobby_npcs(folder)

    def test_obj_indices_and_invalid_assets(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'mesh.obj'
            prefix = 'v 0 2 0\nv 2 2 0\nv 0 2 2\n'
            path.write_text(prefix + 'f -3/1 -2/2 -1/3 # triangle\n', encoding='utf-8')
            vertices, faces, digest = read_obj(path)
            self.assertEqual(faces, [(0, 1, 2)])
            self.assertEqual(len(digest), 64)
            for text in ('version https://git-lfs.github.com/spec/v1', prefix + 'f 0 2 3', prefix + 'f 1 2 4',
                         prefix + 'f 1 2 3 1', 'v nan 0 0\nf 1 1 1'):
                path.write_text(text, encoding='utf-8')
                with self.subTest(text=text), self.assertRaises(ValueError):
                    read_obj(path)

    def test_projection_height_overlap_and_png(self):
        # +Z is the top image row; interpolation is Y = X + Z on the lower plane.
        vertices = [(0, 0, 0), (2, 2, 0), (0, 2, 2), (0, 9, 0), (2, 9, 0), (0, 9, 2)]
        bounds = {'min': [0, 0, 0], 'max': [2, 9, 2]}
        grid = height_grid(vertices, [(0, 1, 2)], bounds, 2)
        self.assertEqual(grid[0], 2)
        self.assertEqual(grid[2], 1)
        self.assertFalse(math.isfinite(grid[1]))
        grid = height_grid(vertices, [(0, 1, 2), (3, 4, 5)], bounds, 2)
        self.assertEqual(grid[0], 9)
        image = png_height(grid, 2, 0, 9)
        self.assertEqual(image[:8], b'\x89PNG\r\n\x1a\n')
        offset, payload = 8, b''
        while offset < len(image):
            length = struct.unpack('>I', image[offset:offset + 4])[0]
            kind, data = image[offset + 4:offset + 8], image[offset + 8:offset + 8 + length]
            self.assertEqual(zlib.crc32(kind + data), struct.unpack('>I', image[offset + 8 + length:offset + 12 + length])[0])
            if kind == b'IDAT':
                payload += data
            offset += length + 12
        pixels = zlib.decompress(payload)
        self.assertEqual(len(pixels), 2 * (1 + 2 * 4))
        self.assertEqual(pixels[8], 0)  # uncovered top-right alpha
        self.assertEqual(pixels[4], 255)

    def test_real_server_assets_and_cache(self):
        store = TerrainStore()
        for map_id, counts, port in [('lobby', (127699, 124834, 445), 8910), ('game', (309024, 127756, 5743), 8911)]:
            data = store.get(map_id)
            self.assertIs(data, store.get(map_id))
            self.assertEqual((data['height']['vertices'], data['height']['triangles'], data['navigation']['triangles']), counts)
            self.assertEqual(len(data['nav_triangles']), counts[2])
            self.assertEqual(data['port'], port)
            self.assertTrue((ROOT / data['height']['source']).is_file())
            self.assertEqual(base64.b64decode(data['image'].split(',')[1])[:8], b'\x89PNG\r\n\x1a\n')
            self.assertLess(len(json.dumps(data)), 500_000)
        with self.assertRaises(ValueError):
            store.get('../game')

    def test_http_map_allowlist_and_missing_resource(self):
        with tempfile.TemporaryDirectory() as folder:
            lab = Lab(folder)
            lab.terrain = TerrainStore(folder)
            lab.terrain.cache['lobby'] = {'id': 'lobby', 'test': True}
            server = ThreadingHTTPServer(('127.0.0.1', 0), handler(lab))
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            connection = http.client.HTTPConnection('127.0.0.1', server.server_port, timeout=3)
            try:
                for path, status in [('/api/terrain?map=lobby', 200), ('/api/terrain?map=game', 503),
                                     ('/api/terrain', 400), ('/api/terrain?map=../game', 400),
                                     ('/api/terrain?map=game&map=lobby', 400), ('/api/terrain?map=game&path=foo', 400),
                                     ('/terrain.js', 200)]:
                    connection.request('GET', path)
                    response = connection.getresponse()
                    self.assertEqual(response.status, status, path)
                    response.read()
            finally:
                connection.close()
                server.shutdown()
                server.server_close()
                thread.join()
                lab.close()


if __name__ == '__main__':
    unittest.main()
