"""로컬 서버 관측/시나리오 제어. Python 표준 라이브러리만 사용한다."""
import argparse
import copy
import json
import re
import signal
import threading
import time
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit, parse_qs

from runner import ROOT, SCENARIOS, RunState, load_scenario, run_sync
from terrain import TerrainStore

ROLES = ('LobbyServer', 'GameServer')
WEB = Path(__file__).parent / 'web/index.html'
NUMERIC_FIELDS = ('pid', 'sequence', 'timestamp_ms', 'uptime_ms', 'cpu_ticks', 'logical_processors',
                  'private_bytes', 'working_set_bytes', 'pending_io', 'received_bytes', 'sent_bytes', 'io_errors',
                  'sockets', 'leased', 'jobs_submitted', 'jobs_started', 'jobs_cleared', 'jobs_queued', 'jobs_running',
                  'jobs_completed', 'jobs_failed', 'job_execute_us', 'job_max_execute_us', 'collect_us')


def derive_rates(previous, current):
    result = {key: None for key in ('cpu_percent', 'rx_bps', 'tx_bps', 'jobs_per_s', 'job_mean_us')}
    if not previous or previous['instance'] != current['instance'] or current['sequence'] <= previous['sequence']:
        return result
    elapsed = (current['uptime_ms'] - previous['uptime_ms']) / 1000
    if elapsed <= 0:
        return result
    fields = ('cpu_ticks', 'received_bytes', 'sent_bytes', 'jobs_completed', 'jobs_failed', 'job_execute_us')
    delta = {key: current[key] - previous[key] for key in fields}
    if min(delta.values()) < 0:
        return result
    executed = delta['jobs_completed'] + delta['jobs_failed']
    result.update(cpu_percent=min(100, delta['cpu_ticks'] / 10_000_000 / elapsed / current['logical_processors'] * 100),
                  rx_bps=delta['received_bytes'] / elapsed, tx_bps=delta['sent_bytes'] / elapsed,
                  jobs_per_s=delta['jobs_completed'] / elapsed, job_mean_us=delta['job_execute_us'] / executed if executed else None)
    return result


class ServerFeed:
    def __init__(self, directory):
        self.directory = Path(directory)
        self.previous = {}
        self.content = {}
        self.rates = {}

    def sample(self):
        output = []
        now = time.time_ns() // 1_000_000
        for role in ROLES:
            try:
                path = self.directory / (role + '.json')
                if path.stat().st_size > 2_000_000:
                    raise ValueError('snapshot 크기 초과')
                data = json.loads(path.read_text(encoding='utf-8'))
                content = data.pop('content', {})
                if data.get('schema_version') != 1 or data.get('role') != role or not isinstance(data.get('instance'), str) or type(data.get('stopped')) is not bool:
                    raise ValueError('snapshot schema/role 불일치')
                if any(type(data.get(key)) is not int or data[key] < 0 for key in NUMERIC_FIELDS) or data['logical_processors'] < 1:
                    raise ValueError('snapshot 지표 불일치')
                age = (now - data['timestamp_ms']) / 1000
                previous = self.previous.get(role)
                if not previous or previous['instance'] != data['instance'] or previous['sequence'] != data['sequence']:
                    self.rates[role] = derive_rates(previous, data)
                    self.previous[role] = data
                status = 'stopped' if data['stopped'] else 'stale' if age > 3.5 or age < -2 else 'live'
                if role == 'GameServer':
                    self.content = content if isinstance(content, dict) else {}
                output.append({**data, **self.rates.get(role, {}), 'status': status, 'age_s': max(0, age)})
            except FileNotFoundError:
                output.append({'role': role, 'status': 'missing', 'detail': '계측 파일 없음 · 계측을 켜고 서버를 시작하세요'})
            except (OSError, ValueError, KeyError, TypeError) as error:
                output.append({'role': role, 'status': 'invalid', 'detail': str(error)})
        return output


class Lab:
    def __init__(self, directory):
        self.directory = Path(directory)
        self.runs = self.directory / 'runs'
        self.feed = ServerFeed(self.directory / 'metrics')
        self.terrain = TerrainStore()
        self.lock = threading.RLock()
        self.stop = threading.Event()
        self.state = self.worker = None
        self.series = deque(maxlen=600)
        self.run_samples = deque(maxlen=3600)
        self.thread = threading.Thread(target=self.collect, daemon=True)
        self.thread.start()

    def scenarios(self):
        return [{'id': path.stem, **load_scenario(path)} for path in sorted(SCENARIOS.glob('*.json'))]

    def collect(self):
        while not self.stop.is_set():
            servers = self.feed.sample()
            with self.lock:
                run = self.state.snapshot() if self.state else None
                if self.state:
                    game = next((item for item in servers if item['role'] == 'GameServer'), {})
                    if game.get('status') in ('live', 'stopped'):
                        self.state.world.apply_schedule(self.feed.content, game['instance'], game['stopped'])
                sample = {'timestamp_ms': time.time_ns() // 1_000_000, 'servers': servers,
                          'run_id': run['run_id'] if run else None, 'dummy': run['counters'] if run else None}
                self.series.append(sample)
                if self.worker and self.worker.is_alive():
                    self.run_samples.append(sample)
            self.stop.wait(1)

    def start(self, request):
        if set(request) - {'scenario', 'clients', 'seed'}:
            raise ValueError('알 수 없는 실행 필드')
        scenarios = {item['id']: item for item in self.scenarios()}
        scenario_id = request.get('scenario')
        if not isinstance(scenario_id, str) or scenario_id not in scenarios:
            raise ValueError('등록되지 않은 시나리오')
        scenario = load_scenario(SCENARIOS / (scenario_id + '.json'))
        state = RunState(scenario, request.get('clients'), request.get('seed', 1))
        with self.lock:
            if self.worker and self.worker.is_alive():
                raise RuntimeError('실행 중인 시나리오가 있습니다')
            self.state = state
            self.run_samples.clear()
            self.worker = threading.Thread(target=self.execute, args=(state,), daemon=False)
            self.worker.start()
        return state.snapshot()

    def execute(self, state):
        try:
            run_sync(state)
            with self.lock:
                samples = copy.deepcopy(list(self.run_samples))
            state.save(self.runs, samples)
        except Exception as error:
            with state.lock:
                state.status, state.error = 'failed', f'결과 저장/실행 실패: {error}'

    def snapshot(self):
        with self.lock:
            history = sorted(self.runs.glob('*.json'), key=lambda path: path.stat().st_mtime, reverse=True)[:20] if self.runs.exists() else []
            return {'timestamp_ms': time.time_ns() // 1_000_000, 'servers': self.series[-1]['servers'] if self.series else [],
                    'series': list(self.series)[-120:], 'run': self.state.snapshot() if self.state else None,
                    'busy': bool(self.worker and self.worker.is_alive()), 'history': [path.stem for path in history]}

    def cancel(self):
        with self.lock:
            if self.state and self.worker and self.worker.is_alive():
                self.state.cancel()

    def world_snapshot(self):
        with self.lock:
            return self.state.world.snapshot(self.state.status) if self.state else {'run_id': None, 'timestamp_ms': time.time_ns() // 1_000_000, 'status': 'idle', 'players': [], 'npcs': [], 'events': [], 'sources': [], 'dropped': 0, 'timelines': []}

    def close(self):
        self.cancel()
        if self.worker:
            self.worker.join(timeout=10)
            if self.worker.is_alive():
                raise RuntimeError('시나리오 중지 시간 초과')
        self.stop.set()
        self.thread.join(timeout=3)


def handler(lab):
    class Handler(BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'

        def log_message(self, *_args):
            pass

        def respond(self, status, value, content_type='application/json; charset=utf-8', filename=None):
            data = value if isinstance(value, bytes) else json.dumps(value, ensure_ascii=False).encode('utf-8')
            self.send_response(status)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', str(len(data)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            if self.close_connection:
                self.send_header('Connection', 'close')
            if filename:
                self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
            self.end_headers()
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass

        def local_request(self):
            expected = f'127.0.0.1:{self.server.server_port}'
            host = self.headers.get('Host')
            origin = self.headers.get('Origin')
            return host in (expected, f'localhost:{self.server.server_port}') and (origin is None or origin in (f'http://{expected}', f'http://localhost:{self.server.server_port}'))

        def do_GET(self):
            if not self.local_request():
                return self.respond(403, {'error': '로컬 요청만 지원합니다'})
            parsed = urlsplit(self.path)
            if parsed.path == '/':
                return self.respond(200, WEB.read_bytes(), 'text/html; charset=utf-8')
            if parsed.path == '/terrain.js':
                return self.respond(200, (WEB.parent / 'terrain.js').read_bytes(), 'text/javascript; charset=utf-8')
            if parsed.path == '/api/state':
                return self.respond(200, lab.snapshot())
            if parsed.path == '/api/world':
                return self.respond(200, lab.world_snapshot())
            if parsed.path == '/api/scenarios':
                return self.respond(200, lab.scenarios())
            if parsed.path == '/api/terrain':
                try:
                    query = parse_qs(parsed.query, keep_blank_values=True)
                    if set(query) != {'map'} or len(query['map']) != 1:
                        raise ValueError('map 인자 하나가 필요합니다')
                    return self.respond(200, lab.terrain.get(query['map'][0]))
                except ValueError as error:
                    return self.respond(400, {'error': str(error)})
                except OSError:
                    return self.respond(503, {'error': '지형 원본을 읽을 수 없습니다 · Resource/LFS 파일을 확인하세요'})
            if parsed.path == '/api/result':
                query = parse_qs(parsed.query)
                run_id = query.get('id', [''])[0]
                extension = query.get('format', ['json'])[0]
                if not re.fullmatch('[0-9a-f]{32}', run_id) or extension not in ('json', 'csv'):
                    return self.respond(400, {'error': '잘못된 결과 ID/형식'})
                path = lab.runs / (run_id + '.' + extension)
                if not path.is_file():
                    return self.respond(404, {'error': '결과가 없습니다'})
                return self.respond(200, path.read_bytes(), 'application/json; charset=utf-8' if extension == 'json' else 'text/csv; charset=utf-8', path.name)
            return self.respond(404, {'error': '경로가 없습니다'})

        def do_POST(self):
            if not self.local_request():
                self.close_connection = True
                return self.respond(403, {'error': '로컬 요청만 지원합니다'})
            try:
                length = int(self.headers.get('Content-Length', '0'))
                if not 0 < length <= 8192:
                    raise ValueError('요청 크기 범위: 1~8192')
                request = json.loads(self.rfile.read(length))
                if not isinstance(request, dict):
                    raise ValueError('요청은 JSON 객체여야 합니다')
                if self.path == '/api/run':
                    return self.respond(202, lab.start(request))
                if self.path == '/api/stop':
                    lab.cancel()
                    return self.respond(202, {'status': 'stopping'})
                if self.path == '/api/shutdown':
                    self.respond(202, {'status': 'shutting-down'})
                    threading.Thread(target=self.server.shutdown, daemon=True).start()
                    return
                return self.respond(404, {'error': '경로가 없습니다'})
            except (ValueError, OSError) as error:
                self.close_connection = True
                return self.respond(400, {'error': str(error)})
            except RuntimeError as error:
                return self.respond(409, {'error': str(error)})
    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8790)
    parser.add_argument('--directory', type=Path, default=ROOT / 'artifacts/logs/server-lab')
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error('port 범위: 1024~65535')
    lab = Lab(args.directory)
    try:
        with ThreadingHTTPServer(('127.0.0.1', args.port), handler(lab)) as server:
            server.daemon_threads = True
            def stop(_signal, _frame):
                threading.Thread(target=server.shutdown, daemon=True).start()
            signal.signal(signal.SIGINT, stop)
            signal.signal(signal.SIGTERM, stop)
            print(f'Server Lab: http://127.0.0.1:{args.port}', flush=True)
            server.serve_forever(poll_interval=.25)
    finally:
        lab.close()


if __name__ == '__main__':
    main()
