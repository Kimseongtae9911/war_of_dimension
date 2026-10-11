"""시나리오 상태 머신. GUI와 CLI가 같은 실행/판정/결과 모델을 사용한다."""
import asyncio
import csv
import hashlib
import json
import math
import random
import threading
import time
import uuid
from collections import deque
from pathlib import Path

from protocol import ABI_PATH, CONSTANTS, ROOT, packet, unpack
from observation import NPC_PACKETS, WorldObservation

SCENARIOS = Path(__file__).parent / 'scenarios'
VALID_DIRECTIONS = (0, 1, 2, 4, 5, 6, 8, 9, 10)
PLAYER_OBSERVATIONS = {CONSTANTS[name]: name for name in ('SC_ROTATE_PLAYER', 'SC_SKILL', 'SC_SKILL_FINISH')}
TRANSIENT_PACKETS = (set(NPC_PACKETS) - {CONSTANTS['SC_ADD_NPC']}) | {CONSTANTS['SC_ROTATE_PLAYER'], CONSTANTS['SC_UPDATE_SKILL_OBJECT']}


def load_scenario(path):
    data = json.loads(Path(path).read_text(encoding='utf-8-sig'))
    if not isinstance(data, dict):
        raise ValueError('시나리오는 JSON 객체여야 합니다')
    if set(data) - {'schema_version', 'name', 'description', 'setup', 'clients', 'timeout_s', 'boss_job', 'execution', 'actions'}:
        raise ValueError('알 수 없는 시나리오 필드')
    if data.get('schema_version') != 1 or data.get('setup') not in ('lobby', 'match'):
        raise ValueError('schema_version=1, setup=lobby|match가 필요합니다')
    if not isinstance(data.get('name'), str) or not data['name'] or len(data['name']) > 80:
        raise ValueError('시나리오 이름이 필요합니다')
    validate_clients(data.get('clients'), data['setup'])
    if data.get('execution', 'parallel') not in ('parallel', 'sequential'):
        raise ValueError('execution은 parallel|sequential')
    timeout = data.get('timeout_s', 15)
    if type(timeout) not in (int, float) or not math.isfinite(timeout) or not .1 <= timeout <= 60:
        raise ValueError('timeout_s 범위: 0.1~60')
    if data.get('boss_job', 4) not in (4, 5):
        raise ValueError('boss_job은 4 또는 5')
    actions = data.get('actions')
    if not isinstance(actions, list) or not 1 <= len(actions) <= 100:
        raise ValueError('actions는 1~100개')
    for action in actions:
        if not isinstance(action, dict):
            raise ValueError('action은 객체여야 합니다')
        op = action.get('op')
        allowed = {'move': {'direction', 'duration_s'}, 'stop': set(), 'skill': {'slot'}, 'wait': {'duration_s'}, 'reconnect': set()}
        if not isinstance(op, str) or op not in allowed or set(action) - (allowed[op] | {'op', 'actors'}):
            raise ValueError(f'지원하지 않는 행동/필드: {op}')
        if action.get('actors', 'all') not in ('all', 'heroes', 'boss'):
            raise ValueError('actors는 all|heroes|boss')
        if data['setup'] == 'lobby' and action.get('actors', 'all') != 'all':
            raise ValueError('로비에는 boss/heroes 역할이 없습니다')
        if op == 'move' and (type(action.get('direction')) not in (int, str) or action.get('direction') not in (*VALID_DIRECTIONS[1:], 'random')):
            raise ValueError('move direction이 유효하지 않습니다')
        if op in ('move', 'wait'):
            duration = action.get('duration_s', .5)
            if type(duration) not in (int, float) or not math.isfinite(duration) or not .05 <= duration <= 120:
                raise ValueError('duration_s 범위: 0.05~120')
        if op == 'skill' and (data['setup'] != 'match' or type(action.get('slot')) is not int or not 1 <= action['slot'] <= 5):
            raise ValueError('skill은 match에서 slot=1~5로 지정합니다')
        if op == 'reconnect' and data['setup'] != 'lobby':
            raise ValueError('첫 구현의 reconnect는 lobby에서 지원합니다')
    return data


def validate_clients(count, setup):
    if type(count) is not int or not 1 <= count <= 32 or (setup == 'match' and count % 4):
        raise ValueError('clients는 1~32, match는 4의 배수여야 합니다')


def percentile(values, fraction):
    if not values:
        return None
    ordered = sorted(values)
    return ordered[max(0, math.ceil(len(ordered) * fraction) - 1)]


class RunState:
    def __init__(self, scenario, clients=None, seed=1):
        self.scenario = scenario
        self.clients = clients if clients is not None else scenario['clients']
        validate_clients(self.clients, scenario['setup'])
        if type(seed) is not int or not 0 <= seed <= 2**32 - 1:
            raise ValueError('seed 범위: 0~4294967295')
        self.seed = seed
        self.run_id = uuid.uuid4().hex
        self.started_ms = time.time_ns() // 1_000_000
        self.lock = threading.RLock()
        self.status = 'running'
        self.events = deque(maxlen=2000)
        self.assertions = []
        self.latencies = deque(maxlen=10000)
        self.peers = {}
        self.world = WorldObservation(self.run_id)
        self.counters = {'sent_packets': 0, 'received_packets': 0, 'sent_bytes': 0, 'received_bytes': 0, 'connected': 0, 'passed': 0, 'failed': 0}
        self.error = None
        self.loop = self.task = None
        self.cancel_requested = False

    def event(self, client, stage, status, detail='', elapsed_ms=None):
        with self.lock:
            entry = {'timestamp_ms': time.time_ns() // 1_000_000, 'client': client, 'stage': stage, 'status': status, 'detail': detail, 'elapsed_ms': elapsed_ms}
            self.events.append(entry)
            if status in ('pass', 'fail'):
                self.assertions.append(entry)
                self.counters['passed' if status == 'pass' else 'failed'] += 1
            if status == 'pass' and elapsed_ms is not None:
                self.latencies.append(elapsed_ms)

    def count(self, key, amount=1):
        with self.lock:
            self.counters[key] += amount

    def peer(self, index, **fields):
        with self.lock:
            self.peers.setdefault(str(index), {'id': index}).update(fields)
            self.world.peer(index, **fields)

    def snapshot(self, full=False):
        with self.lock:
            result = {'run_id': self.run_id, 'name': self.scenario['name'], 'status': self.status, 'seed': self.seed,
                      'started_ms': self.started_ms, 'clients': self.clients, 'error': self.error, 'counters': dict(self.counters),
                      'latency_ms': {'p50': percentile(self.latencies, .5), 'p95': percentile(self.latencies, .95), 'p99': percentile(self.latencies, .99), 'samples': len(self.latencies)},
                      'peers': [dict(value) for value in self.peers.values()], 'events': list(self.events)[-100:]}
            if full:
                result.update(schema_version=1, scenario=self.scenario, abi_sha256=hashlib.sha256(ABI_PATH.read_bytes()).hexdigest(),
                              assertions=list(self.assertions), finished_ms=time.time_ns() // 1_000_000,
                              scope='LOCAL_TEST; 수신된 플레이어/NPC 상태; 운영 DB/전체 월드/전체 전투 검증 제외')
                result['world'] = self.world.snapshot(self.status)
            return result

    def cancel(self):
        with self.lock:
            self.cancel_requested = True
            if self.loop and self.task:
                self.loop.call_soon_threadsafe(self.task.cancel)

    def save(self, directory, samples=()):
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        result = self.snapshot(full=True)
        result['samples'] = list(samples)
        path = directory / (self.run_id + '.json')
        temporary = path.with_suffix('.tmp')
        temporary.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        temporary.replace(path)
        with path.with_suffix('.csv').open('w', newline='', encoding='utf-8-sig') as stream:
            writer = csv.DictWriter(stream, fieldnames=('timestamp_ms', 'client', 'stage', 'status', 'detail', 'elapsed_ms'))
            writer.writeheader()
            writer.writerows(result['assertions'])
        return path


class Peer:
    def __init__(self, state, index, name, timeout):
        self.state, self.index, self.name, self.timeout = state, index, name, timeout
        self.reader = self.writer = self.pump = None
        self.changed = asyncio.Event()
        self.history = deque(maxlen=4096)
        self.moves = {}
        self.evicted = {}
        self.sequence = 0
        self.error = None
        self.slot = None
        self.port = None
        self.player_id = None

    async def connect(self, port):
        self.reader, self.writer = await asyncio.wait_for(asyncio.open_connection('127.0.0.1', port), self.timeout)
        self.state.count('connected')
        self.state.peer(self.index, state='connected', port=port)
        self.port = port
        self.pump = asyncio.create_task(self.receive())

    async def receive(self):
        try:
            while True:
                header = await self.reader.readexactly(2)
                size, kind = header
                if size < 2:
                    raise ValueError('서버가 잘못된 프레임 길이를 보냈습니다')
                body = await self.reader.readexactly(size - 2)
                self.sequence += 1
                self.state.count('received_packets')
                self.state.count('received_bytes', size)
                if kind in NPC_PACKETS:
                    self.state.world.npc_packet(self.index, kind, body)
                if kind in PLAYER_OBSERVATIONS:
                    self.state.world.player_packet(self.index, PLAYER_OBSERVATIONS[kind], body)
                if kind == CONSTANTS['SC_MOVE_PLAYER']:
                    player, x, y, z, direction, _ = unpack('SC_MOVE_PLAYER', body)
                    if not all(math.isfinite(value) for value in (x, y, z)):
                        raise ValueError('유효하지 않은 서버 위치')
                    self.moves[player] = (self.sequence, body)
                    if len(self.moves) > 4096:
                        raise RuntimeError('플레이어 관측 한도 초과')
                    if player == self.player_id:
                        self.state.peer(self.index, x=x, y=y, z=z, direction=direction)
                elif kind not in TRANSIENT_PACKETS:
                    if len(self.history) == self.history.maxlen:
                        seq, evicted_kind, _ = self.history[0]
                        self.evicted[evicted_kind] = seq
                    self.history.append((self.sequence, kind, body))
                self.changed.set()
        except asyncio.CancelledError:
            raise
        except Exception as error:
            self.error = str(error) or type(error).__name__
            self.changed.set()

    async def send(self, name, *values, split=False):
        data = packet(name, *values)
        if split:
            self.writer.write(data[:1])
            await self.writer.drain()
            await asyncio.sleep(.01)
            self.writer.write(data[1:])
        else:
            self.writer.write(data)
        await self.writer.drain()
        self.state.count('sent_packets')
        self.state.count('sent_bytes', len(data))

    async def expect(self, name, after=0, predicate=lambda _: True):
        deadline = asyncio.get_running_loop().time() + self.timeout
        while True:
            self.changed.clear()
            if self.evicted.get(CONSTANTS[name], 0) > after:
                raise RuntimeError(f'{name}: 수신 보관 한도 초과')
            candidates = ((seq, CONSTANTS[name], body) for seq, body in self.moves.values()) if name == 'SC_MOVE_PLAYER' else self.history
            for seq, kind, body in candidates:
                if seq > after and kind == CONSTANTS[name] and predicate(body):
                    return body
            if self.error:
                raise ConnectionError(self.error)
            remaining = deadline - asyncio.get_running_loop().time()
            if remaining <= 0:
                raise TimeoutError(f'{name}: 기대 응답 없음 ({self.timeout}s)')
            try:
                await asyncio.wait_for(self.changed.wait(), remaining)
            except TimeoutError as error:
                raise TimeoutError(f'{name}: 기대 응답 없음 ({self.timeout}s)') from error

    async def check(self, stage, operation):
        started = time.perf_counter()
        self.state.peer(self.index, state=stage)
        self.state.event(self.index, stage, 'start')
        try:
            result = await operation()
        except asyncio.CancelledError:
            self.state.event(self.index, stage, 'cancelled')
            raise
        except Exception as error:
            self.state.event(self.index, stage, 'fail', str(error) or type(error).__name__, (time.perf_counter() - started) * 1000)
            raise RuntimeError(f'client {self.index} / {stage}: {str(error) or type(error).__name__}') from error
        self.state.event(self.index, stage, 'pass', elapsed_ms=(time.perf_counter() - started) * 1000)
        return result

    async def login(self, port):
        async def operation():
            await self.connect(port)
            await self.send('CS_LOGIN', self.name, bytes(10))
            body = await self.expect('SC_LOGIN_INFO')
            response_id, x, y, z, _ = unpack('SC_LOGIN_INFO', body)
            if not all(math.isfinite(value) for value in (x, y, z)):
                raise ValueError('유효하지 않은 로그인 위치')
            # 게임 로그인 응답의 id=0은 기존 ACK 관례다. 게임의 ID는 로비가 배정한 슬롯이다.
            self.player_id = response_id if port == 8910 else self.slot
            self.state.peer(self.index, player_id=self.player_id, x=x, y=y, z=z)
        await self.check('lobby-login' if port == 8910 else 'game-login', operation)

    async def close(self):
        if self.writer:
            self.writer.close()
            try:
                await asyncio.wait_for(self.writer.wait_closed(), 1)
            except (Exception, asyncio.CancelledError):
                pass
            self.writer = None
            self.state.count('connected', -1)
        if self.pump:
            self.pump.cancel()
            await asyncio.gather(self.pump, return_exceptions=True)
            self.pump = None
        self.state.peer(self.index, state='disconnected')

    async def reconnect(self, port):
        await self.close()
        self.history.clear()
        self.moves.clear()
        self.evicted.clear()
        self.sequence, self.error = 0, None
        # 서버의 disconnect 후 사용자 정리가 완료될 여유. 자동 재시도로 실패를 숨기지 않는다.
        await asyncio.sleep(.3)
        await self.login(port)


async def setup_match(peers, scenario):
    for peer in peers:
        await peer.login(8910)
    for index, peer in enumerate(peers):
        await peer.send('CS_MATCH', True, int(index == 3), index)
    slots = {}
    for peer in peers:
        async def matched(peer=peer):
            slot, address, port = unpack('SC_MATCH', await peer.expect('SC_MATCH'))
            if address.split(b'\0', 1)[0] != b'127.0.0.1' or port != 8911 or slot not in range(4) or slot in slots:
                raise ValueError('로컬 매치 목적지/슬롯 불일치')
            peer.slot = slot
            slots[slot] = peer
            await peer.reconnect(port)
        await peer.check('match-transfer', matched)
    jobs = [0, 1, 3, scenario.get('boss_job', 4)]
    for slot, peer in slots.items():
        await peer.send('CS_JOB_SELECT', slot, jobs[slot])
    for peer in peers:
        async def job_echo(peer=peer):
            for slot, job in enumerate(jobs):
                await peer.expect('SC_JOB_SELECT', predicate=lambda body, slot=slot, job=job: unpack('SC_JOB_SELECT', body) == (slot, job))
        await peer.check('job-selection', job_echo)
    boss_raw = 20 if jobs[3] == 4 else 30
    await slots[3].send('CS_SKILL_SELECT', 3, 0, boss_raw)
    for peer in peers:
        await peer.send('CS_READY', peer.slot, True, split=True)
    for peer in peers:
        async def started(peer=peer):
            await peer.expect('SC_GAME_START')
            skills = {}
            for _, kind, body in peer.history:
                if kind == CONSTANTS['SC_SKILL_SELECT']:
                    player, storage, skill = unpack('SC_SKILL_SELECT', body)
                    skills[player, storage] = skill
            if len(skills) != 16 or skills.get((3, 0)) != boss_raw + 1:
                raise ValueError('게임 시작 전 스킬 16개/수동 선택 불일치')
            peer.skills = skills
        await peer.check('ready-game-start', started)
    markers = {peer: peer.sequence for peer in peers}
    for peer in peers:
        await peer.send('CS_LOAD_COMPLETE', peer.slot)
    for peer in peers:
        async def loaded(peer=peer):
            await peer.expect('SC_COOLTIME', markers[peer])
            # 초기 NPC는 전체 로딩 및 매치 업데이트 시작 이후에만 전달된다.
            await peer.expect('SC_ADD_NPC', markers[peer])
        await peer.check('loading-complete', loaded)


async def execute_actions(peer, actions, rng):
    for step, action in enumerate(actions):
        actors = action.get('actors', 'all')
        if actors != 'all' and (actors == 'boss') != (peer.slot == 3):
            continue
        op = action['op']
        async def perform():
            marker = peer.sequence
            if op in ('move', 'stop'):
                direction = action.get('direction', 0)
                if direction == 'random':
                    direction = rng.choice(VALID_DIRECTIONS[1:])
                await peer.send('CS_MOVE', direction, 0)
                if op == 'stop' and peer.port == 8910:
                    await asyncio.sleep(.2)
                    deadline = asyncio.get_running_loop().time() + peer.timeout
                    while True:
                        quiet_after = peer.sequence
                        await asyncio.sleep(.4)
                        if peer.error:
                            raise ConnectionError(peer.error)
                        if peer.moves.get(peer.player_id, (0, None))[0] <= quiet_after:
                            break
                        if asyncio.get_running_loop().time() >= deadline:
                            raise ValueError('로비 정지 관찰 중 자신의 이동 응답이 계속됩니다')
                    peer.state.event(peer.index, 'lobby-stop-observation', 'note', '명시적 ACK 없음; 0.4초 이동 응답 중단 관찰')
                else:
                    await peer.expect('SC_MOVE_PLAYER', marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == peer.player_id and unpack('SC_MOVE_PLAYER', body)[4] == direction)
                if op == 'move':
                    first = unpack('SC_MOVE_PLAYER', await peer.expect('SC_MOVE_PLAYER', marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == peer.player_id))
                    next_marker = peer.sequence
                    await asyncio.sleep(action.get('duration_s', .5))
                    body = await peer.expect('SC_MOVE_PLAYER', next_marker, lambda body: unpack('SC_MOVE_PLAYER', body)[0] == peer.player_id and math.hypot(unpack('SC_MOVE_PLAYER', body)[1] - first[1], unpack('SC_MOVE_PLAYER', body)[3] - first[3]) > .0001)
                    moved = unpack('SC_MOVE_PLAYER', body)
                    if not all(math.isfinite(value) for value in moved[1:4]) or math.hypot(moved[1] - first[1], moved[3] - first[3]) <= .0001:
                        raise ValueError('방향 응답은 있으나 실제 위치 변화가 없습니다')
            elif op == 'skill':
                slot = action['slot']
                await peer.send('CS_SKILL', peer.player_id, slot, False)
                body = await peer.expect('SC_SKILL', marker, lambda body: unpack('SC_SKILL', body)[:2] == (peer.player_id, slot))
                player, skill_type, skill_num, on_off = unpack('SC_SKILL', body)
                if on_off or skill_num < 0:
                    raise ValueError('스킬 승인 응답 불일치')
                await peer.send('CS_SKILL_FINISH')
            elif op == 'wait':
                await asyncio.sleep(action.get('duration_s', .5))
            elif op == 'reconnect':
                await peer.reconnect(8910)
        await peer.check(f'{step + 1}:{op}', perform)
    peer.state.peer(peer.index, state='complete')


async def run(state):
    with state.lock:
        state.loop = asyncio.get_running_loop()
        state.task = asyncio.current_task()
        cancelled = state.cancel_requested
    all_peers = []
    children = []
    setup_lock = asyncio.Lock()
    action_lock = asyncio.Lock()
    try:
        if cancelled:
            raise asyncio.CancelledError()
        for index in range(state.clients):
            name = (state.run_id[:5] + f'{index:03x}').encode().ljust(10, b'\0')
            all_peers.append(Peer(state, index, name, state.scenario.get('timeout_s', 15)))
        async def group_run(group):
            async with setup_lock:
                if state.scenario['setup'] == 'match':
                    await setup_match(group, state.scenario)
                else:
                    for peer in group:
                        await peer.login(8910)
            async def actions():
                tasks = [asyncio.create_task(execute_actions(peer, state.scenario['actions'], random.Random(state.seed + peer.index))) for peer in group]
                try:
                    await asyncio.gather(*tasks)
                finally:
                    for task in tasks:
                        if not task.done():
                            task.cancel()
                    await asyncio.gather(*tasks, return_exceptions=True)
            if state.scenario.get('execution', 'parallel') == 'sequential':
                async with action_lock:
                    await actions()
            else:
                await actions()
        group_size = 4 if state.scenario['setup'] == 'match' else 1
        children = [asyncio.create_task(group_run(all_peers[start:start + group_size])) for start in range(0, len(all_peers), group_size)]
        await asyncio.gather(*children)
        with state.lock:
            state.status = 'passed'
    except asyncio.CancelledError:
        with state.lock:
            state.status = 'cancelled'
    except Exception as error:
        with state.lock:
            state.status, state.error = 'failed', str(error) or type(error).__name__
    finally:
        for task in children:
            if not task.done():
                task.cancel()
        await asyncio.gather(*children, return_exceptions=True)
        await asyncio.gather(*(peer.close() for peer in all_peers), return_exceptions=True)
        state.world.freeze()
        with state.lock:
            state.loop = state.task = None


def run_sync(state):
    asyncio.run(run(state))
