"""로컬 서버의 작업 큐 실행 및 분할된 테스트 전환 패킷을 검사한다."""
import argparse
import json
import socket
import struct
import time
from pathlib import Path


def read_exact(stream, size):
    result = bytearray()
    while len(result) < size:
        data = stream.recv(size - len(result))
        if not data:
            raise RuntimeError('서버 전환 응답 전에 연결 종료')
        result.extend(data)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    types = []
    with socket.create_connection(('127.0.0.1', 8910), timeout=5) as stream:
        stream.settimeout(5)
        # protocol.h의 packed CS_TEST_CHANGE_SERVER: size, type, boss.
        stream.sendall(bytes([3]))
        time.sleep(.05)
        stream.sendall(bytes([40, 0]))
        for _ in range(16):
            header = read_exact(stream, 2)
            if header[0] < 2:
                raise RuntimeError('응답 패킷 길이 오류')
            payload = read_exact(stream, header[0] - 2)
            types.append(header[1])
            if header[1] == 63:
                # Windows Winsock의 INET_ADDRSTRLEN은 22다.
                if header[0] != 30:
                    raise RuntimeError(f'서버 전환 응답 field/packing 변경: {header[0]} byte')
                slot, address, port = struct.unpack('<i22sh', payload)
                address = address.split(b'\0', 1)[0].decode('ascii')
                if slot != 0 or address != '127.0.0.1' or port != 8911:
                    raise RuntimeError('서버 전환 대상/플레이어 슬롯 오류')
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps({'ok': True, 'splitHeader': True,
                    'packetTypes': types, 'heroSlot': slot, 'gameAddress': address,
                    'gamePort': port}, indent=2) + '\n', encoding='utf-8')
                print('로비 작업 큐·분할 테스트 전환 응답: PASS')
                return
        raise RuntimeError('서버 전환 응답 미수신')


if __name__ == '__main__':
    main()
