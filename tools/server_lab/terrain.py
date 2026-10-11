"""서버 원본 OBJ의 X/Z 높이 미리보기. 외부 이미지/메시 패키지는 사용하지 않는다."""
import base64
import hashlib
import math
import re
import struct
import threading
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAPS = {'lobby': ('Lobby_Server', 'HeightMesh2.obj', 'NavMeshData2.obj', 8910),
        'game': ('Game_Server', 'HeightMesh.obj', 'NavMeshData3.obj', 8911)}


def lobby_npcs(root):
    source = 'Client/WarOfDimension/Scene.h'
    data = (Path(root) / source).read_bytes()
    text = data.decode('utf-8-sig')
    block = re.search(r'm_npcPositions\[LOBBY_NPC\]\s*=\s*\{(.*?)\};', text, re.S)
    if not block:
        raise ValueError('로비 NPC 배치 원본을 찾지 못했습니다')
    points = re.findall(r'XMFLOAT3\(([^)]+)\)', block[1])
    if len(points) != 4:
        raise ValueError('로비 NPC 배치 개수 변경 · 배치 계약 확인 필요')
    result = []
    for index, point in enumerate(points):
        values = [float(value.strip().rstrip('fF')) for value in point.split(',')]
        if len(values) != 3 or not all(math.isfinite(value) for value in values):
            raise ValueError('잘못된 로비 NPC 좌표')
        result.append({'id': index + 5, 'group': -1, 'npc_type': 100 + index,
                       'x': values[0], 'y': values[1], 'z': values[2], 'static': True, 'removed': False,
                       'source': source, 'attacks': 0, 'trail': []})
    return {'source': source, 'sha256': hashlib.sha256(data).hexdigest(), 'npcs': result}


def read_obj(path):
    data = path.read_bytes()
    if len(data) > 20_000_000:
        raise ValueError('지형 파일 크기 초과')
    vertices, faces = [], []
    for line in data.decode('utf-8-sig').splitlines():
        fields = line.split('#', 1)[0].split()
        if not fields:
            continue
        if fields[0] == 'v':
            if len(fields) != 4:
                raise ValueError('OBJ 정점은 X/Y/Z여야 합니다')
            vertex = tuple(float(value) for value in fields[1:])
            if not all(math.isfinite(value) and abs(value) <= 1_000_000 for value in vertex):
                raise ValueError('유효하지 않은 OBJ 좌표')
            vertices.append(vertex)
        elif fields[0] == 'f':
            if len(fields) != 4:
                raise ValueError('OBJ 면은 삼각형이어야 합니다')
            indices = [int(value.split('/')[0]) for value in fields[1:]]
            indices = [index - 1 if index > 0 else len(vertices) + index if index < 0 else -1 for index in indices]
            if not all(0 <= index < len(vertices) for index in indices):
                raise ValueError('OBJ 면 인덱스 범위 초과')
            faces.append(tuple(indices))
        if len(vertices) > 500_000 or len(faces) > 500_000:
            raise ValueError('OBJ 정점/면 수 초과')
    if not vertices or not faces:
        raise ValueError('지형 정점/면이 없습니다 · LFS 원본을 확인하세요')
    return vertices, faces, hashlib.sha256(data).hexdigest()


def height_grid(vertices, faces, bounds, size):
    """픽셀 중심의 barycentric 보간. 겹친 면은 가장 높은 표면을 표시한다."""
    low, high = bounds['min'], bounds['max']
    dx, dz = high[0] - low[0], high[2] - low[2]
    if dx <= 0 or dz <= 0:
        raise ValueError('지형의 X/Z 범위가 없습니다')
    grid = [-math.inf] * (size * size)
    for face in faces:
        a, b, c = [( (vertices[i][0] - low[0]) / dx * size,
                     (high[2] - vertices[i][2]) / dz * size, vertices[i][1]) for i in face]
        det = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(det) < 1e-12:
            continue
        for row in range(max(0, math.ceil(min(a[1], b[1], c[1]) - .5)), min(size - 1, math.floor(max(a[1], b[1], c[1]) - .5)) + 1):
            for col in range(max(0, math.ceil(min(a[0], b[0], c[0]) - .5)), min(size - 1, math.floor(max(a[0], b[0], c[0]) - .5)) + 1):
                x, y = col + .5, row + .5
                u = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / det
                v = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / det
                if min(u, v, 1 - u - v) >= -1e-9:
                    height = u * a[2] + v * b[2] + (1 - u - v) * c[2]
                    index = row * size + col
                    grid[index] = max(grid[index], height)
    return grid


def png_height(grid, size, low, high):
    raw = bytearray()
    for row in range(size):
        raw.append(0)
        for col in range(size):
            index = row * size + col
            value = grid[index]
            if not math.isfinite(value):
                raw.extend((0, 0, 0, 0))
                continue
            t = (value - low) / max(1e-9, high - low)
            neighbor = grid[index - 1] if col else value
            shade = max(.65, min(1.15, 1 + (value - neighbor) * .08)) if math.isfinite(neighbor) else 1
            raw.extend((min(255, round((30 + 151 * t) * shade)), min(255, round((76 + 112 * t) * shade)),
                        min(255, round((88 + 50 * t) * shade)), 255))
    def chunk(kind, payload):
        return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')


class TerrainStore:
    def __init__(self, root=ROOT, size=512):
        self.root, self.size = Path(root), size
        self.cache = {}
        self.lock = threading.Lock()

    def get(self, map_id):
        if map_id not in MAPS:
            raise ValueError('등록되지 않은 지형 · lobby/game만 지원합니다')
        with self.lock:
            if map_id in self.cache:
                return self.cache[map_id]
            project, height_name, nav_name, port = MAPS[map_id]
            folder = self.root / 'Server' / project / 'Resource'
            vertices, faces, height_hash = read_obj(folder / height_name)
            nav_vertices, nav_faces, nav_hash = read_obj(folder / nav_name)
            bounds = {'min': [min(vertex[axis] for vertex in vertices) for axis in range(3)],
                      'max': [max(vertex[axis] for vertex in vertices) for axis in range(3)]}
            grid = height_grid(vertices, faces, bounds, self.size)
            image = png_height(grid, self.size, bounds['min'][1], bounds['max'][1])
            result = {'id': map_id, 'port': port, 'bounds': bounds, 'resolution': self.size,
                      'height': {'source': f'Server/{project}/Resource/{height_name}', 'sha256': height_hash,
                                 'vertices': len(vertices), 'triangles': len(faces)},
                      'navigation': {'source': f'Server/{project}/Resource/{nav_name}', 'sha256': nav_hash,
                                     'triangles': len(nav_faces)},
                      'nav_triangles': [[coordinate for index in face for coordinate in (nav_vertices[index][0], nav_vertices[index][2])] for face in nav_faces],
                      'image': 'data:image/png;base64,' + base64.b64encode(image).decode('ascii')}
            if map_id == 'lobby':
                result['lobby_npcs'] = lobby_npcs(self.root)
            self.cache[map_id] = result
            return result
