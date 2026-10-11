"""웹 더미가 수신한 월드 관측. 서버 FSM과 구분하고 저장량을 제한한다."""
import math
import threading
import time
from collections import deque

from protocol import CONSTANTS, unpack

NPC_PACKETS = {CONSTANTS[name]: name for name in ('SC_ADD_NPC', 'SC_MOVE_NPC', 'SC_REMOVE_NPC', 'SC_NPC_STAT_CHANGE', 'SC_NPC_ATTACK')}
MAX_NPCS_PER_GROUP = 128
TRAIL_POINTS = 120
TRAIL_INTERVAL_MS = 100


def now_ms():
    return time.time_ns() // 1_000_000


def position(entity, x, y, z, stamp):
    if not all(math.isfinite(value) for value in (x, y, z)):
        raise ValueError('유효하지 않은 관측 좌표')
    previous = entity.get('sample')
    entity.update(x=x, y=y, z=z, position_ms=stamp)
    if previous and stamp - previous[0] < TRAIL_INTERVAL_MS:
        return
    distance = math.hypot(x - previous[1], z - previous[2]) if previous else 0
    entity['speed'] = distance * 1000 / (stamp - previous[0]) if previous and stamp > previous[0] else 0
    entity['sample'] = (stamp, x, z)
    trail = entity.setdefault('trail', deque(maxlen=TRAIL_POINTS))
    if not trail or distance > .001:
        trail.append([stamp, x, z])


class WorldObservation:
    def __init__(self, run_id):
        self.run_id = run_id
        self.lock = threading.RLock()
        self.players = {}
        self.npcs = {}
        self.events = deque(maxlen=100)
        self.sequence = 0
        self.dropped = 0
        self.timelines = {}
        self.ended_ms = None

    def freeze(self):
        with self.lock:
            self.ended_ms = now_ms()

    def apply_schedule(self, content, instance, stopped=False):
        if not isinstance(content, dict) or content.get('schema_version') != 1 or not isinstance(content.get('matches'), list):
            return
        updates = {}
        for sample in content['matches']:
            if not isinstance(sample, dict):
                continue
            if any(type(sample.get(field)) is not int or sample[field] < 0 for field in ('match_id', 'timestamp_ms', 'game_ms', 'next_wave_ms', 'fence_at_game_ms', 'active_minions')):
                continue
            if type(sample.get('fence')) is not bool or type(sample.get('finished')) is not bool or not 0 <= sample['active_minions'] <= 12:
                continue
            members, spawns = sample.get('member_keys'), sample.get('spawns')
            if not isinstance(members, list) or len(members) != 4 or not all(isinstance(key, str) for key in members):
                continue
            if not isinstance(spawns, list) or len(spawns) > 12 or any(not isinstance(spawn, dict) or type(spawn.get('id')) is not int or not 0 <= spawn['id'] < 12
                    or type(spawn.get('due_ms')) is not int or spawn['due_ms'] < 0 for spawn in spawns):
                continue
            if len({spawn['id'] for spawn in spawns}) != len(spawns):
                continue
            # 게임 match ID와 runner 그룹 번호는 다르다. 로그인 이름 byte 4개의 정확한 집합으로 연결한다.
            for group in range(8):
                expected = {(self.run_id[:5] + f'{index:03x}').encode().hex() for index in range(group * 4, group * 4 + 4)}
                if set(members) == expected and len(set(members)) == 4:
                    updates[group] = {**{field: sample[field] for field in ('match_id', 'timestamp_ms', 'game_ms', 'next_wave_ms', 'fence_at_game_ms', 'active_minions', 'fence', 'finished')},
                                      'group': group, 'spawns': [dict(spawn) for spawn in spawns], 'instance': instance, 'stopped': stopped}
                    break
        with self.lock:
            if self.ended_ms is None:
                self.timelines = {group: sample for group, sample in self.timelines.items() if sample['instance'] == instance}
                self.timelines.update(updates)

    def schedules(self, status, stamp):
        result = []
        for group, sample in self.timelines.items():
            age = stamp - sample['timestamp_ms']
            live = status == 'running' and self.source(group) is not None and not sample['stopped'] and not sample['finished'] and -2000 <= age <= 3500
            # 수집 중단/종료 때 마지막 수집값을 고정한다. 오래된 값에서 카운트다운을 계속하지 않는다.
            point = stamp if live else sample['timestamp_ms']
            game_ms = sample['game_ms'] + (max(0, age) if live else 0)
            result.append({**sample, 'live': live, 'stale': age > 3500 or age < -2000, 'game_display_ms': game_ms,
                           'wave_remaining_ms': max(0, sample['next_wave_ms'] - point),
                           'fence_remaining_ms': max(0, sample['fence_at_game_ms'] - game_ms) if sample['fence'] else 0,
                           'spawns': [{**spawn, 'remaining_ms': max(0, spawn['due_ms'] - point)} for spawn in sample['spawns']]})
        return result

    def peer(self, index, **fields):
        with self.lock:
            entity = self.players.setdefault(index, {'id': index, 'group': index // 4, 'connected': False})
            if 'port' in fields and fields['port'] != entity.get('port'):
                for name in ('x', 'y', 'z', 'trail', 'sample', 'speed', 'direction', 'look_x', 'look_z', 'action', 'action_ms', 'player_id'):
                    entity.pop(name, None)
            if fields.get('state') in ('connected', 'disconnected'):
                entity['connected'] = fields['state'] == 'connected'
            entity.update({key: value for key, value in fields.items() if key not in ('x', 'y', 'z')})
            stamp = now_ms()
            if all(name in fields for name in ('x', 'y', 'z')):
                position(entity, fields['x'], fields['y'], fields['z'], stamp)
            entity['updated_ms'] = stamp

    def source(self, group):
        candidates = [index for index, entity in self.players.items() if index // 4 == group and entity.get('port') == 8911 and entity['connected']]
        return min(candidates) if candidates else None

    def event(self, group, npc_id, action, stamp):
        self.sequence += 1
        self.events.append({'sequence': self.sequence, 'timestamp_ms': stamp, 'group': group, 'npc_id': npc_id, 'action': action})

    def npc_packet(self, observer, kind, body, stamp=None):
        name = NPC_PACKETS[kind]
        values = unpack(name, body)  # 대표가 아닌 수신도 잘못된 프레임/좌표는 거부한다.
        if name in ('SC_ADD_NPC', 'SC_MOVE_NPC') and not all(math.isfinite(value) for value in values[2:11]):
            raise ValueError('유효하지 않은 NPC 위치/방향')
        npc_id = values[1] if name == 'SC_ADD_NPC' else values[0]
        if not 0 <= npc_id <= 65535:
            raise ValueError('유효하지 않은 NPC ID')
        stamp = now_ms() if stamp is None else stamp
        group = observer // 4
        with self.lock:
            if self.source(group) != observer:
                return
            key = (group, npc_id)
            entity = self.npcs.get(key)
            if entity is None:
                if sum(key[0] == group for key in self.npcs) >= MAX_NPCS_PER_GROUP:
                    self.dropped += 1
                    return
                entity = self.npcs[key] = {'id': npc_id, 'group': group, 'removed': False, 'spawn_known': False, 'attacks': 0}
            entity.update(observer=observer, updated_ms=stamp)
            if name == 'SC_ADD_NPC':
                if entity['removed']:
                    entity.clear()
                    entity.update(id=npc_id, group=group, removed=False, attacks=0, observer=observer, updated_ms=stamp)
                entity.update(npc_type=values[0], spawn_known=True, action='spawn', action_ms=stamp)
                position(entity, *values[2:5], stamp)
                entity.update(look_x=values[5], look_z=values[7])
                self.event(group, npc_id, 'spawn', stamp)
            elif name == 'SC_MOVE_NPC':
                if entity['removed']:
                    return  # 제거 뒤 남은 업데이트가 다시 객체를 살리지 않는다.
                if not entity['spawn_known']:
                    entity['npc_type'] = values[1]
                position(entity, *values[2:5], stamp)
                entity.update(look_x=values[5], look_z=values[7], idle=values[-1])
            elif name == 'SC_NPC_STAT_CHANGE':
                entity.update(max_hp=values[1], hp=values[2])
            elif name == 'SC_NPC_ATTACK':
                if not entity['removed']:
                    entity.update(action='attack', action_ms=stamp, attacks=entity['attacks'] + 1)
                    self.event(group, npc_id, 'attack', stamp)
            elif name == 'SC_REMOVE_NPC':
                entity.update(removed=True, action='remove', action_ms=stamp)
                self.event(group, npc_id, 'remove', stamp)

    def player_packet(self, observer, name, body):
        values = unpack(name, body)
        if name == 'SC_ROTATE_PLAYER' and not all(math.isfinite(value) for value in values[1:]):
            raise ValueError('유효하지 않은 플레이어 방향')
        with self.lock:
            entity = self.players.get(observer)
            if not entity or values[0] != entity.get('player_id'):
                return
            if name == 'SC_ROTATE_PLAYER':
                entity.update(look_x=values[1], look_z=values[3])
            else:
                entity.update(action='skill' if name == 'SC_SKILL' else 'skill-finish', action_ms=now_ms())

    def snapshot(self, status='running'):
        with self.lock:
            def copy(entity):
                return {**{key: value for key, value in entity.items() if key not in ('trail', 'sample')}, 'trail': [list(point) for point in entity.get('trail', ())]}
            stamp = now_ms()
            return {'run_id': self.run_id, 'timestamp_ms': stamp, 'status': status,
                    'players': [copy(entity) for entity in self.players.values() if 'x' in entity],
                    'npcs': [copy(entity) for entity in self.npcs.values() if 'x' in entity],
                    'sources': [{'group': group, 'observer': self.source(group)} for group in sorted({index // 4 for index in self.players})],
                    'events': list(self.events), 'dropped': self.dropped, 'timelines': self.schedules(status, stamp)}
