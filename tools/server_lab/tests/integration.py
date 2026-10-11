"""실제 로컬 서버와 monitor의 공개 API를 검증한다. 서버 기동/종료는 PowerShell이 소유한다."""
import argparse
import json
import math
import time
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import Request, urlopen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8790)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeline', action='store_true', help='190초 실제 미니언/게이트 일정 관측 추가')
    parser.add_argument('--minions', action='store_true', help='실제 첫 웨이브 미니언 4개의 경로 이동/수신 검증 추가')
    args = parser.parse_args()
    base = f'http://127.0.0.1:{args.port}'
    results = []

    def request(path, body=None, expected=200, raw=False):
        query = Request(base + path, data=json.dumps(body).encode() if body is not None else None,
                        headers={'Content-Type': 'application/json'})
        try:
            response = urlopen(query, timeout=5)
        except HTTPError as error:
            response = error
        with response:
            assert response.status == expected, (path, response.status, response.read())
            data = response.read()
            return data if raw else json.loads(data)

    def settled():
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            status = request('/api/state')
            if not status['busy']:
                assert status['run']['counters']['connected'] == 0, status['run']
                return status['run']
            time.sleep(.2)
        raise AssertionError('시나리오 종료 시간 초과')

    def completed(scenario, clients):
        request('/api/run', {'scenario': scenario, 'clients': clients, 'seed': 7}, 202)
        result = settled()
        assert result['status'] == 'passed', result
        report = request('/api/result?id=' + result['run_id'])
        csv = request('/api/result?id=' + result['run_id'] + '&format=csv', raw=True)
        assert report['assertions'] and report['samples'] and report['abi_sha256']
        if scenario == 'match-actions':
            world = request('/api/world')
            assert world['run_id'] == result['run_id'] and len(world['players']) == clients
            assert world['npcs'] and world['dropped'] == 0
            assert len({(entity['group'], entity['id']) for entity in world['npcs']}) == len(world['npcs'])
            assert {entity['group'] for entity in world['npcs']} == set(range(clients // 4))
            assert all(source['observer'] is None for source in world['sources'])
            assert any(len(entity['trail']) > 1 for entity in world['players'])
            # NPC 이동/공격 발생은 시나리오 입력과 서버 AI에 달려 있다.
            # 정지한 NPC도 위치와 HP를 그대로 관측해야 한다.
            assert all(entity['trail'] for entity in world['npcs'])
            assert {entity['npc_type'] for entity in world['npcs']} == set(range(8))
            assert len(world['timelines']) == clients // 4
            assert all(sample['fence'] and sample['game_ms'] > 0 for sample in world['timelines'])
            assert report['world']['run_id'] == result['run_id']
        assert b'timestamp_ms,client,stage,status' in csv
        results.append({'case': scenario, 'clients': clients, 'status': result['status'],
                        'run_id': result['run_id'], 'assertions': result['counters']['passed']})

    for map_id, port in [('lobby', 8910), ('game', 8911)]:
        terrain = request('/api/terrain?map=' + map_id)
        assert terrain['id'] == map_id and terrain['port'] == port
        assert terrain['height']['triangles'] > 100_000 and terrain['nav_triangles']
        assert terrain['resolution'] == 512 and terrain['image'].startswith('data:image/png;base64,iVBOR')
        assert request('/api/terrain?map=' + map_id) == terrain
        results.append({'case': 'terrain-' + map_id, 'status': 'passed', 'triangles': terrain['height']['triangles']})
    observation = request('/api/state')
    assert len(observation['servers']) == 2 and all(item['status'] == 'live' for item in observation['servers'])
    request('/api/run', {'scenario': 'match-actions', 'clients': 5}, 400)
    completed('lobby-cycle', 4)
    if args.timeline:
        request('/api/run', {'scenario': 'timeline-watch', 'clients': 4, 'seed': 11}, 202)
        deadline = time.monotonic() + 225
        records, signatures = [], set()
        while time.monotonic() < deadline:
            status = request('/api/state')
            world = request('/api/world')
            for sample in world['timelines']:
                signature = (sample['active_minions'], sample['fence'], tuple(spawn['id'] for spawn in sample['spawns']), sample['game_ms'] // 30000)
                if signature not in signatures:
                    signatures.add(signature)
                    records.append(sample)
                    print(json.dumps({'game_ms': sample['game_ms'], 'active_minions': sample['active_minions'], 'reserved': len(sample['spawns']), 'fence': sample['fence']}), flush=True)
            if not status['busy']:
                assert status['run']['status'] == 'passed', status['run']
                break
            time.sleep(.25)
        else:
            raise AssertionError('일정 관측 종료 시간 초과')
        assert any(sample['live'] and sample['fence'] and sample['spawns'] for sample in records), records
        assert max(sample['active_minions'] for sample in records) >= 8
        assert any(sample['live'] and not sample['fence'] and sample['game_ms'] >= 180000 for sample in records)
        assert not request('/api/world')['timelines'][0]['live']
        results.append({'case': 'timeline-watch', 'clients': 4, 'status': status['run']['status'], 'run_id': status['run']['run_id'], 'observations': records})
    completed('match-actions', 4)
    completed('match-actions', 8)
    request('/api/run', {'scenario': 'match-actions', 'clients': 8}, 202)
    request('/api/run', {'scenario': 'lobby-cycle', 'clients': 1}, 409)
    deadline = time.monotonic() + 5
    while request('/api/state')['run']['counters']['connected'] == 0 and time.monotonic() < deadline:
        time.sleep(.05)
    request('/api/stop', {}, 202)
    result = settled()
    assert result['status'] == 'cancelled', result
    results.append({'case': 'cancel-active-and-reject-duplicate', 'status': result['status'], 'run_id': result['run_id']})
    completed('lobby-cycle', 4)
    if args.minions:
        request('/api/run', {'scenario': 'world-watch', 'clients': 4, 'seed': 11}, 202)
        deadline = time.monotonic() + 75
        positions, distances = {}, {}
        while time.monotonic() < deadline:
            world = request('/api/world')
            for npc in world['npcs']:
                if npc['group'] != 0 or not 0 <= npc['id'] < 4 or npc['removed']:
                    continue
                assert npc['npc_type'] == 0
                assert all(math.isfinite(npc[field]) for field in ('x', 'y', 'z'))
                # 첫 ADD가 남은 trail의 시작점과 이후 수신값을 비교한다.
                origin = positions.setdefault(npc['id'], npc['trail'][0][1:])
                distance = math.hypot(npc['x'] - origin[0], npc['z'] - origin[1])
                distances[npc['id']] = max(distances.get(npc['id'], 0), distance)
            status = request('/api/state')
            if not status['busy']:
                assert status['run']['status'] == 'passed', status['run']
                assert status['run']['counters']['connected'] == 0
                break
            time.sleep(.2)
        else:
            raise AssertionError('미니언 이동 관측 종료 시간 초과')
        assert set(distances) == set(range(4)) and all(value > 4 for value in distances.values()), distances
        report = request('/api/result?id=' + status['run']['run_id'])
        assert report['world']['npcs']
        results.append({'case': 'minion-movement', 'status': 'passed', 'run_id': status['run']['run_id'],
                        'max_displacement': distances})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({'ok': True, 'cases': results}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'ok': True, 'cases': len(results), 'output': str(args.output)}))


if __name__ == '__main__':
    main()
