"""Read existing model records and calculate allocation payloads without loading the game."""
import argparse
import hashlib
import json
import mmap
import struct
from collections import Counter
from pathlib import Path


def aligned_buffer(size):
    return ((size + 65535) // 65536) * 65536 if size else 0


class Reader:
    def __init__(self, data):
        self.data, self.position = data, 0

    def skip(self, size):
        if size < 0 or self.position + size > len(self.data):
            raise ValueError(f"File range exceeded at {self.position}, size {size}")
        begin = self.position
        self.position += size
        return begin

    def integer(self):
        return struct.unpack_from('<i', self.data, self.skip(4))[0]

    def count(self):
        value = self.integer()
        if value < 0:
            raise ValueError(f"Negative count: {value}")
        return value

    def token(self):
        length = self.data[self.skip(1)]
        begin = self.skip(length)
        return self.data[begin:begin + length].decode('utf-8')


def mesh(reader):
    begin = reader.position
    vertices = reader.count()
    name_begin = reader.position
    name = reader.token()
    name_end = reader.position
    cpu = gpu = 0
    buffers = []
    sizes = {'<Positions>:': 12, '<Normals>:': 12, '<Tangents>:': 12,
             '<BiTangents>:': 12, '<Colors>:': 16,
             '<TextureCoords0>:': 8, '<TextureCoords1>:': 8}
    while True:
        tag = reader.token()
        if tag == '</Mesh>':
            break
        if tag == '<Bounds>:':
            reader.skip(24)
        elif tag in sizes:
            count = reader.count()
            size = count * sizes[tag]
            if tag != '<Colors>:' and count not in (0, vertices):
                raise ValueError(f"Vertex count mismatch: {name} {tag}")
            reader.skip(size)
            cpu += size
            if size and tag != '<Colors>:':
                # Mesh.cpp creates a separate committed buffer for each attribute.
                gpu += size
                buffers.append(size)
        elif tag == '<SubMeshes>:':
            for index in range(reader.count()):
                if reader.token() != '<SubMesh>:' or reader.integer() != index:
                    raise ValueError('Invalid subset')
                size = reader.count() * 4
                reader.skip(size)
                cpu += size
                if size:
                    gpu += size
                    buffers.append(size)
        else:
            raise ValueError(f"Unsupported mesh tag: {tag}")
    digest = hashlib.sha256()
    digest.update(reader.data[begin:name_begin])
    digest.update(reader.data[name_end:reader.position])
    return {'name': name, 'digest': digest.hexdigest(), 'vertices': vertices,
            'cpuArrayBytes': cpu, 'gpuBufferPayloadBytes': gpu,
            'gpuBufferAllocationBytes': sum(map(aligned_buffer, buffers)),
            'gpuBufferCount': len(buffers)}


def skin(reader):
    reader.token()  # mesh name in skinning record
    bones = vertices = cpu = gpu = allocated = 0
    bind = 0
    while True:
        tag = reader.token()
        if tag == '</SkinningInfo>':
            break
        if tag == '<BonesPerVertex>:':
            reader.count()
        elif tag == '<Bounds>:':
            reader.skip(24)
        elif tag == '<BoneNames>:':
            bones = reader.count()
            for _ in range(bones):
                reader.token()
        elif tag == '<BoneOffsets>:':
            count = reader.count()
            reader.skip(count * 64)
            cpu += count * 64
            if count:
                bind = 256 * 64
        elif tag in ('<BoneIndices>:', '<BoneWeights>:'):
            vertices = reader.count()
            size = vertices * 16
            reader.skip(size)
            cpu += size
            gpu += size
            allocated += aligned_buffer(size)
        else:
            raise ValueError(f"Unsupported skin tag: {tag}")
    return {'bones': bones, 'vertices': vertices, 'cpuArrayBytes': cpu,
            'boneNameAndCacheBytes': bones * (64 + 8 + 8),
            'gpuBufferPayloadBytes': gpu, 'gpuBufferAllocationBytes': allocated,
            'bindPosePayloadBytes': bind, 'bindPoseAllocationBytes': aligned_buffer(bind)}


def parse_model(path):
    result = {'path': str(path), 'fileBytes': path.stat().st_size,
              'frames': 0, 'meshes': [], 'skins': [], 'animations': [], 'texturesCreated': []}
    with path.open('rb') as source, mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
        result['assetSha256'] = hashlib.sha256(data).hexdigest()
        reader = Reader(data)
        animated_bones = 0
        floats = {'<AlbedoColor>:': 4, '<EmissiveColor>:': 4, '<SpecularColor>:': 4,
                  '<Glossiness>:': 1, '<Smoothness>:': 1, '<Metallic>:': 1,
                  '<SpecularHighlight>:': 1, '<GlossyReflection>:': 1}
        while reader.position < len(data):
            tag = reader.token()
            if tag == '<Frame>:':
                reader.integer(); reader.integer(); reader.token()
                result['frames'] += 1
            elif tag == '<Transform>:':
                reader.skip(52)
            elif tag == '<TransformMatrix>:':
                reader.skip(64)
            elif tag == '<Mesh>:':
                result['meshes'].append(mesh(reader))
            elif tag == '<SkinningInfo>:':
                result['skins'].append(skin(reader))
            elif tag in ('<Children>:', '<Materials>:', '<Material>:', '<AnimationSets>:'):
                reader.count()
            elif tag in floats:
                reader.skip(floats[tag] * 4)
            elif tag.endswith('Map>:'):
                texture = reader.token()
                if texture != 'null' and not texture.startswith('@'):
                    result['texturesCreated'].append(texture)
            elif tag == '<FrameNames>:':
                animated_bones = reader.count()
                for _ in range(animated_bones):
                    reader.token()
            elif tag == '<AnimationSet>:':
                index, name = reader.count(), reader.token()
                reader.skip(4)  # animation length
                fps, keys = reader.count(), reader.count()
                for key in range(keys):
                    if reader.token() != '<Transforms>:' or reader.count() != key:
                        raise ValueError('Invalid animation key')
                    reader.skip(4 + animated_bones * 64)
                result['animations'].append({'index': index, 'name': name, 'fps': fps,
                    'keys': keys, 'animatedBones': animated_bones,
                    'matrixBytes': keys * animated_bones * 64,
                    'timeAndRowPointerBytes': keys * (4 + 8)})
            elif tag not in ('<Hierarchy>:', '</Hierarchy>', '<Animation>:', '</Animation>:', '</Animation>',
                             '</AnimationSets>', '</Frame>', '</Material>', '</Materials>'):
                raise ValueError(f"Unsupported model tag {tag!r} at {reader.position}")
        if reader.position != len(data):
            raise ValueError('Model parser did not consume the whole file')
    return result


def totals(records, fields):
    return {field: sum(record[field] for record in records) for field in fields}


def summarize(model):
    fields = ['cpuArrayBytes', 'gpuBufferPayloadBytes', 'gpuBufferAllocationBytes', 'gpuBufferCount']
    unique = {record['digest']: record for record in model['meshes']}
    return {'path': model['path'], 'assetSha256': model['assetSha256'], 'fileBytes': model['fileBytes'],
            'frames': model['frames'], 'meshRecords': len(model['meshes']), 'uniqueGeometry': len(unique),
            'legacyGeometry': totals(model['meshes'], fields), 'sharedGeometry': totals(unique.values(), fields),
            'skin': {'count': len(model['skins']), **totals(model['skins'], [
                'cpuArrayBytes', 'boneNameAndCacheBytes', 'gpuBufferPayloadBytes', 'gpuBufferAllocationBytes',
                'bindPosePayloadBytes', 'bindPoseAllocationBytes'])},
            'animation': {'sets': len(model['animations']),
                'animatedBones': max((item['animatedBones'] for item in model['animations']), default=0),
                'keys': sum(item['keys'] for item in model['animations']),
                **totals(model['animations'], ['matrixBytes', 'timeAndRowPointerBytes'])},
            'texturesCreated': dict(Counter(model['texturesCreated']))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--profile', type=Path, help='Use actual model load counts from a native profile')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    client = root / 'Client/WarOfDimension'
    if args.profile:
        profile = json.loads(args.profile.read_text(encoding='utf-8-sig'))
        if not profile['ok']:
            raise ValueError('Profile did not finish successfully')
        loads = Counter(sample['phase'].split(':', 1)[1] for sample in profile['snapshots']
                        if sample['phase'].startswith('model_loaded:'))
    else:
        loads = Counter({'Model/Plane1.bin': 1, 'Model/ModularModel.bin': 1})
    models = {path: parse_model(client / path) for path in loads}
    summaries = []
    all_legacy, all_shared = [], {}
    for path, model in models.items():
        summary = summarize(model)
        summary['path'], summary['loadCount'] = path, loads[path]
        summaries.append(summary)
        all_legacy.extend(model['meshes'] * loads[path])
        all_shared.update({record['digest']: record for record in model['meshes']})
    fields = ['cpuArrayBytes', 'gpuBufferPayloadBytes', 'gpuBufferAllocationBytes', 'gpuBufferCount']
    result = {'schemaVersion': 1, 'kind': 'calculated_model_payload_not_process_memory',
              'bufferAlignmentBytes': 65536, 'models': summaries,
              'retainedLoadSetGeometry': {'legacy': totals(all_legacy, fields),
                                        'shared': totals(all_shared.values(), fields)}}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Model payload audit: {len(models)} models, {sum(loads.values())} loads. {args.output}')


if __name__ == '__main__':
    main()
