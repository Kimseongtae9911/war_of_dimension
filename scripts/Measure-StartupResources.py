"""게임 코드를 바꾸지 않고 포함된 FMOD의 PCM/stream 및 DDS payload를 조사한다."""
import argparse
import ctypes as c
from datetime import datetime, timezone
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / 'Client/WarOfDimension'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=ROOT / 'artifacts/logs/startup-resource-probe.json')
args = parser.parse_args()
dll_path = CLIENT / 'fmod.dll'
api = c.WinDLL(str(dll_path))
ptr = c.c_void_p
uint = c.c_uint
integer = c.c_int

def bind(name, args):
    f = getattr(api, name)
    f.argtypes = args
    f.restype = integer
    return f

create = bind('FMOD_System_Create', [c.POINTER(ptr), uint])
output = bind('FMOD_System_SetOutput', [ptr, integer])
init = bind('FMOD_System_Init', [ptr, integer, uint, ptr])
version = bind('FMOD_System_GetVersion', [ptr, c.POINTER(uint)])
sound_create = bind('FMOD_System_CreateSound', [ptr, c.c_char_p, uint, ptr, c.POINTER(ptr)])
length = bind('FMOD_Sound_GetLength', [ptr, c.POINTER(uint), uint])
sound_release = bind('FMOD_Sound_Release', [ptr])
close = bind('FMOD_System_Close', [ptr])
release = bind('FMOD_System_Release', [ptr])
stats = bind('FMOD_Memory_GetStats', [c.POINTER(integer), c.POINTER(integer), integer])

def check(code, operation):
    if code:
        raise RuntimeError(f'{operation}: FMOD_RESULT={code}')

def memory():
    current, maximum = integer(), integer()
    check(stats(c.byref(current), c.byref(maximum), 1), 'memory stats')
    return current.value

system = ptr()
rows = []
check(create(c.byref(system), 0x00020001), 'create')
try:
    check(output(system, 2), 'NOSOUND output')
    check(init(system, 32, 0, None), 'init')
    ver = uint()
    check(version(system, c.byref(ver)), 'version')
    for file in sorted((CLIENT / 'Sound').rglob('*')):
        if not file.is_file():
            continue
        sound = ptr()
        before = memory()
        code = sound_create(system, str(file).encode('utf-8'), 0, None, c.byref(sound))
        if code:
            rows.append({'path': file.relative_to(CLIENT).as_posix(), 'error': code})
            continue
        try:
            size = uint()
            check(length(sound, c.byref(size), 0x4), 'PCM bytes')
            row = {'path': file.relative_to(CLIENT).as_posix(),
                   'sha256': hashlib.sha256(file.read_bytes()).hexdigest(),
                   'fileBytes': file.stat().st_size, 'pcmBytes': size.value,
                   'sampleFmodDeltaBytes': memory() - before}
        finally:
            check(sound_release(sound), 'sample release')
        if file.parent.name == 'BGM':
            stream = ptr()
            before = memory()
            check(sound_create(system, str(file).encode('utf-8'), 0x80, None, c.byref(stream)), 'CREATESTREAM')
            try:
                row['streamFmodDeltaBytes'] = memory() - before
            finally:
                check(sound_release(stream), 'stream release')
        rows.append(row)
finally:
    check(close(system), 'close')
    check(release(system), 'system release')

textures = []
for name in ['Image/Title/WOD_Background_Title_Ver4.dds', 'Image/GUI/Button_SignIn.dds',
             'Image/GUI/Button_SignUp.dds', 'SkyBox/Space.dds', 'Image/GUI/Clear/Victory.dds',
             'Image/GUI/Clear/Defeat.dds', 'Image/GUI/Blood.dds', 'Image/GUI/Speed.dds',
             'Model/Textures/dissolve.dds']:
    file = CLIENT / name
    data = file.read_bytes()
    assert data[:4] == b'DDS '
    fourcc = data[84:88]
    header = 148 if fourcc == b'DX10' else 128
    textures.append({'path': name, 'width': struct.unpack_from('<I', data, 16)[0],
                     'height': struct.unpack_from('<I', data, 12)[0],
                     'mips': struct.unpack_from('<I', data, 28)[0],
                     'rgbBitCount': struct.unpack_from('<I', data, 88)[0],
                     'caps2': struct.unpack_from('<I', data, 112)[0],
                     'fourccHex': fourcc.hex(), 'payloadBytes': len(data)-header,
                     'sha256': hashlib.sha256(data).hexdigest()})
groups = {}
for row in rows:
    if 'error' in row:
        continue
    group = Path(row['path']).parent.name
    groups.setdefault(group, {'count': 0, 'pcmBytes': 0})
    groups[group]['count'] += 1
    groups[group]['pcmBytes'] += row['pcmBytes']
source_paths = ['Client/WarOfDimension/' + name for name in
                ['GameFramework.cpp', 'GameFramework.h', 'Scene.cpp', 'Shader.cpp', 'Shader.h',
                 'Object.cpp', 'Object.h', 'SoundManager.cpp', 'SoundManager.h', 'fmod_common.h',
                 'ShadowMap.cpp', 'ShadowMap.h', 'DeferredRender.hlsl', 'CBlurShader.cpp']]
report = {'schemaVersion': 1, 'kind': 'startup_resource_review', 'reviewOnly': True,
          'measuredAtUtc': datetime.now(timezone.utc).isoformat(),
          'method': 'FMOD 2.00.01, NOSOUND, no playback, each sample independently loaded/released; PCM payload and FMOD allocator deltas are not process Private Bytes',
          'sourceSha256': {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in source_paths},
          'fmodVersionHex': hex(ver.value), 'dllSha256': hashlib.sha256(dll_path.read_bytes()).hexdigest(),
          'audio': rows, 'groups': groups, 'textures': textures,
          'fmodBytesAfterSystemRelease': memory()}
destination = args.output
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print(json.dumps({'groups': groups, 'bgm': [r for r in rows if '/BGM/' in r['path']],
                  'errors': [r for r in rows if 'error' in r], 'textures': textures,
                  'fmodBytesAfterSystemRelease': report['fmodBytesAfterSystemRelease']}, ensure_ascii=False, indent=2))
