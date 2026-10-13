"""OBJ 지형을 수정하지 않고 크기·퇴화·중복·원본 그룹을 진단한다."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import statistics


def distribution(values):
    if not values:
        return None
    values = sorted(values)
    return dict(min=values[0], median=statistics.median(values),
                p95=values[math.ceil(len(values) * .95) - 1], max=values[-1])


def inspect(path):
    data = path.read_bytes()
    vertices, faces = [], []
    object_name = group_name = ''
    for line_number, line in enumerate(data.decode('utf-8-sig').splitlines(), 1):
        fields = line.split('#', 1)[0].split()
        if not fields:
            continue
        try:
            if fields[0] == 'v':
                if len(fields) != 4:
                    raise ValueError('정점은 x y z 세 값 필요')
                vertex = tuple(float(value) for value in fields[1:])
                if not all(math.isfinite(value) for value in vertex):
                    raise ValueError('비정상 정점')
                vertices.append(vertex)
            elif fields[0] in ('o', 'g'):
                name = ' '.join(fields[1:])
                if fields[0] == 'o':
                    object_name = name
                else:
                    group_name = name
            elif fields[0] == 'f':
                if len(fields) != 4:
                    raise ValueError('삼각형 face만 지원')
                # The current server loader accepts plain positive vertex IDs only.
                indices = [int(value) - 1 for value in fields[1:]]
                if any(index < 0 or index >= len(vertices) for index in indices):
                    raise ValueError('face 인덱스 범위 오류')
                faces.append((indices, object_name, group_name, line_number))
        except ValueError as error:
            raise ValueError(f'{path}:{line_number}: {error}') from error
    if not vertices or not faces:
        raise ValueError(f'{path}: 비어 있는 메시')

    lengths, areas, projected = [], [], []
    counts, groups = Counter(), {}
    seen = set()
    examples = []
    for face_id, (indices, obj, group, line) in enumerate(faces):
        triangle = [vertices[index] for index in indices]
        a, b, c = triangle
        e1 = tuple(y - x for x, y in zip(a, b))
        e2 = tuple(y - x for x, y in zip(a, c))
        cross = (e1[1]*e2[2] - e1[2]*e2[1], e1[2]*e2[0] - e1[0]*e2[2], e1[0]*e2[1] - e1[1]*e2[0])
        cross_length = math.sqrt(sum(value*value for value in cross))
        dot11 = sum(value*value for value in e1)
        dot22 = sum(value*value for value in e2)
        condition = dot11*dot22 / (cross_length*cross_length) if cross_length else math.inf
        longest = max(math.dist(a, b), math.dist(a, c), math.dist(b, c))
        lengths.append(longest)
        areas.append(cross_length / 2)
        projected.append(abs(cross[1]) / 2)
        flags = []
        if any(vertex[1] < 1.5 for vertex in triangle):
            flags.append('below_current_height_filter')
        if cross_length == 0:
            flags.append('zero_area')
        elif abs(cross[1]) == 0:
            flags.append('vertical')
        elif abs(cross[1]) / cross_length < .0001:
            flags.append('near_vertical')
        if condition > 256:
            flags.append('slender')
        key = tuple(sorted(triangle))
        if key in seen:
            flags.append('duplicate_geometry')
        seen.add(key)
        counts.update(flags)
        source = (obj, group)
        entry = groups.setdefault(source, {'object': obj, 'group': group, 'triangles': 0, 'flags': Counter()})
        entry['triangles'] += 1
        entry['flags'].update(flags)
        if flags and len(examples) < 64:
            examples.append({'face_id': face_id, 'line': line, 'object': obj, 'group': group, 'flags': flags})
    return {'schema_version': 1, 'path': str(path), 'sha256': hashlib.sha256(data).hexdigest(),
            'vertices': len(vertices), 'triangles': len(faces),
            'has_source_groups': any(obj or group for obj, group in groups),
            'longest_edge_units': distribution(lengths), 'area_units_squared': distribution(areas),
            'projected_xz_area_units_squared': distribution(projected), 'flags': dict(counts),
            'groups': list(groups.values()), 'examples_limit': 64, 'examples': examples,
            'interpretation': '분류는 중복될 수 있다. double 기하 진단이며 서버 float 전역 후보 분류와 동일하지 않다. 제거 권고가 아니며 원본 오브젝트와 지면 용도를 확인해야 한다.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('meshes', type=Path, nargs='+')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve() in [path.resolve() for path in args.meshes]:
        parser.error('입력 메시를 출력으로 덮어쓸 수 없음')
    result = {'schema_version': 1, 'meshes': [inspect(path) for path in args.meshes]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False), encoding='utf-8')
    print(json.dumps([{key: mesh[key] for key in ('path', 'triangles', 'has_source_groups', 'flags')} for mesh in result['meshes']], ensure_ascii=False))


if __name__ == '__main__':
    main()
