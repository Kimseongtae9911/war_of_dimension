"""DB 없는 로컬 네 참가자 매치에서 선택 응답 → 게임 시작의 wire 순서를 검사한다."""
import argparse
import json
import socket
import struct
import time
from contextlib import ExitStack
from pathlib import Path


def packet(kind, payload=b''):
    return bytes([2 + len(payload), kind]) + payload


def exact(stream, count):
    data = bytearray()
    while len(data) < count:
        chunk = stream.recv(count - len(data))
        if not chunk:
            raise RuntimeError('Incomplete response: connection closed')
        data.extend(chunk)
    return bytes(data)


def receive(stream):
    size, kind = exact(stream, 2)
    if size < 2:
        raise RuntimeError('Invalid packet size')
    return kind, exact(stream, size - 2)


def until(stream, wanted):
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        kind, data = receive(stream)
        if kind == wanted:
            return data
    raise TimeoutError(f'Missing response {wanted}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--boss-job', type=int, choices=(4, 5), default=4)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with ExitStack() as stack:
        logins, lobbies = [], []
        for index in range(4):
            name = f'pt{args.boss_job}{index}'.encode().ljust(10, b'\0')
            login = packet(1, name + bytes(10))
            stream = stack.enter_context(socket.create_connection(('127.0.0.1', 8910), timeout=15))
            stream.sendall(login)
            until(stream, 1)
            logins.append(login)
            lobbies.append(stream)
        for index, stream in enumerate(lobbies):
            stream.sendall(packet(4, struct.pack('<?BI', True, int(index == 3), index)))
        slots = {}
        for login, stream in zip(logins, lobbies):
            data = until(stream, 5)
            slot, address, port = struct.unpack('<i22sh', data)
            if address.split(b'\0', 1)[0] != b'127.0.0.1' or port != 8911:
                raise RuntimeError('Unexpected local match destination')
            game = stack.enter_context(socket.create_connection(('127.0.0.1', 8911), timeout=15))
            game.sendall(login)
            until(game, 1)
            slots[slot] = game
        if set(slots) != set(range(4)):
            raise RuntimeError('Incomplete match slots')
        jobs = [0, 1, 3, args.boss_job]
        for slot, game in slots.items():
            game.sendall(packet(11, struct.pack('<ih', slot, jobs[slot])))
        for game in slots.values():
            received = {}
            while len(received) != 4:
                kind, data = receive(game)
                if kind == 15:
                    slot, job = struct.unpack('<ih', data)
                    received[slot] = job
            if [received[i] for i in range(4)] != jobs:
                raise RuntimeError('Job echo mismatch')
        # 보스 수동 선택은 CS 20..39 / SC 21..40. 나머지 슬롯은 자동 선택한다.
        boss_raw = 20 if args.boss_job == 4 else 30
        slots[3].sendall(packet(9, struct.pack('<iii', 3, 0, boss_raw)))
        slots[2].sendall(packet(9, struct.pack('<iii', 2, 3, 94)))
        seen = {slot: {} for slot in slots}
        for slot, game in slots.items():
            while len(seen[slot]) < 2:
                kind, data = receive(game)
                if kind == 12:
                    player, storage, skill = struct.unpack('<iii', data)
                    seen[slot][(player, storage)] = skill
        for slot, game in slots.items():
            ready = packet(10, struct.pack('<i?', slot, True))
            game.sendall(ready[:1])
            time.sleep(.01)
            game.sendall(ready[1:])
        results = []
        for slot, game in slots.items():
            while True:
                kind, data = receive(game)
                if kind == 12:
                    player, storage, skill = struct.unpack('<iii', data)
                    seen[slot][(player, storage)] = skill
                if kind == 16:
                    if data or len(seen[slot]) != 16:
                        raise RuntimeError('Game start preceded complete skill selection')
                    break
            skills = [[seen[slot][(player, storage)] for storage in range(4)] for player in range(4)]
            for player in range(3):
                if any(not 48 + jobs[player] * 12 <= skill < 60 + jobs[player] * 12 for skill in skills[player]):
                    raise RuntimeError('Invalid auto-selected hero skill')
            boss_begin = 21 + (args.boss_job - 4) * 10
            if any(not boss_begin <= skill < boss_begin + 10 for skill in skills[3]):
                raise RuntimeError('Invalid auto-selected boss skill')
            if skills[3][0] != boss_raw + 1 or skills[3][3] != boss_begin + 9 or skills[2][3] != 94:
                raise RuntimeError('Manual selection overwritten or ultimate index changed')
            results.append({'slot': slot, 'jobs': jobs, 'skillsBeforeStart': skills})
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps({'ok': True, 'splitReadyHeader': True,
            'bossJob': args.boss_job, 'clients': results}, indent=2) + '\n', encoding='utf-8')
        print(f'Four-client skill selection before game start: PASS (boss {args.boss_job})')


if __name__ == '__main__':
    main()
