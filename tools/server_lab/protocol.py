"""Shared/Protocol의 x64 MSVC ABI에 맞춘 첫 시나리오용 명시적 codec."""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ABI_PATH = ROOT / 'Shared/Protocol/tests/abi-x64-msvc.json'
ABI = json.loads(ABI_PATH.read_text(encoding='utf-8-sig'))
TYPES = {item['name']: item for item in ABI['types']}
CONSTANTS = dict(ABI['constants'])
# SC_MATCH_PACKET의 실제 type 상수 이름은 SC_MATCH_PLAYER다.
CONSTANTS['SC_MATCH'] = CONSTANTS['SC_MATCH_PLAYER']

# chrono time_point는 MSVC system_clock의 100ns 정수 표현이다. 이동 시각을 서버가 사용하지
# 않는 현재 계약에 한해 0을 전송한다. SC 시각으로 두 프로세스의 지연을 빼서 계산하지 않는다.
FORMATS = {
    'CS_LOGIN': '10s10s', 'CS_MATCH': '?BI', 'CS_MOVE': 'Bq',
    'CS_JOB_SELECT': 'ih', 'CS_SKILL_SELECT': 'iii', 'CS_READY': 'i?',
    'CS_LOAD_COMPLETE': 'i', 'CS_SKILL': 'iB?', 'CS_SKILL_FINISH': '',
    'SC_LOGIN_INFO': 'ifff56s', 'SC_MATCH': 'i22sh', 'SC_MOVE_PLAYER': 'ifffBq',
    'SC_JOB_SELECT': 'ih', 'SC_SKILL_SELECT': 'iii', 'SC_GAME_START': '',
    'SC_COOLTIME': 'iiiii', 'SC_SKILL': 'iBh?',
    'SC_ADD_NPC': 'ii9f', 'SC_MOVE_NPC': 'ii9f?', 'SC_REMOVE_NPC': 'ii',
    'SC_NPC_STAT_CHANGE': 'iii', 'SC_NPC_ATTACK': 'h', 'SC_ROTATE_PLAYER': 'i6f', 'SC_SKILL_FINISH': 'h',
}


def validate_abi():
    for name, layout in FORMATS.items():
        if name not in CONSTANTS:
            raise ValueError(f'패킷 type 상수가 없습니다: {name}')
        expected = TYPES[name + '_PACKET']
        if expected['align'] != 1 or struct.calcsize('<' + layout) + 2 != expected['size']:
            raise ValueError(f'지원 codec과 ABI가 다릅니다: {name}')
    # 크기만 같고 필드 순서가 바뀐 경우도 거부한다.
    fields = {
        'CS_LOGIN': {'name': (2, 10), 'password': (12, 10)},
        'CS_MATCH': {'match': (2, 1), 'character': (3, 1), 'match_time': (4, 4)},
        'CS_MOVE': {'direction': (2, 1), 'move_time': (3, 8)},
        'CS_JOB_SELECT': {'id': (2, 4), 'job': (6, 2)},
        'CS_SKILL_SELECT': {'id': (2, 4), 'storage': (6, 4), 'skill': (10, 4)},
        'CS_READY': {'id': (2, 4), 'ready': (6, 1)},
        'CS_LOAD_COMPLETE': {'id': (2, 4)},
        'CS_SKILL': {'id': (2, 4), 'skillType': (6, 1), 'onOff': (7, 1)},
        'SC_LOGIN_INFO': {'id': (2, 4), 'z': (14, 4), 'model': (18, 56)},
        'SC_MATCH': {'id': (2, 4), 'gameip': (6, 22), 'gameport': (28, 2)},
        'SC_MOVE_PLAYER': {'id': (2, 4), 'z': (14, 4), 'direction': (18, 1), 'move_time': (19, 8)},
        'SC_JOB_SELECT': {'id': (2, 4), 'job': (6, 2)},
        'SC_SKILL_SELECT': {'id': (2, 4), 'storage': (6, 4), 'skill': (10, 4)},
        'SC_COOLTIME': {'skill5': (18, 4)},
        'SC_SKILL': {'id': (2, 4), 'skillType': (6, 1), 'skillNum': (7, 2), 'onOff': (9, 1)},
        'SC_ADD_NPC': {'npcType': (2, 4), 'id': (6, 4), 'z': (18, 4), 'lookZ': (30, 4), 'rightZ': (42, 4)},
        'SC_MOVE_NPC': {'id': (2, 4), 'npcType': (6, 4), 'z': (18, 4), 'lookZ': (30, 4), 'rightZ': (42, 4), 'idle': (46, 1)},
        'SC_REMOVE_NPC': {'id': (2, 4), 'npcType': (6, 4)},
        'SC_NPC_STAT_CHANGE': {'id': (2, 4), 'maxHp': (6, 4), 'curHp': (10, 4)},
        'SC_NPC_ATTACK': {'id': (2, 2)},
        'SC_ROTATE_PLAYER': {'id': (2, 4), 'lookZ': (14, 4), 'rightZ': (26, 4)},
        'SC_SKILL_FINISH': {'id': (2, 2)},
    }
    for name, expected in fields.items():
        actual = {field['name']: (field['offset'], field['size']) for field in TYPES[name + '_PACKET']['fields']}
        if actual != expected:
            raise ValueError(f'지원 codec과 ABI 필드가 다릅니다: {name}')


def packet(name, *values):
    payload = struct.pack('<' + FORMATS[name], *values)
    return bytes((len(payload) + 2, CONSTANTS[name])) + payload


def unpack(name, payload):
    return struct.unpack('<' + FORMATS[name], payload)


validate_abi()
