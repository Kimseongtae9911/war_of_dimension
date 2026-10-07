"""ServerCore 회귀: 분할·batch·잘못된 프레임·반복 접속. 제품용 더미는 아니다."""
import argparse
import json
import socket
import struct
import time
from pathlib import Path


def frame(stream):
    data = bytearray()
    while len(data) < 2:
        chunk = stream.recv(2-len(data))
        if not chunk:
            return None
        data.extend(chunk)
    size, kind = data
    if size < 2:
        raise AssertionError('invalid response length')
    while len(data) < size:
        chunk = stream.recv(size-len(data))
        if not chunk:
            raise AssertionError('truncated response')
        data.extend(chunk)
    return kind, bytes(data[2:])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    cases = []
    for port in (8910, 8911):
        for name, data in [('zero', b'\0'), ('one', b'\1'), ('unknown', bytes([2,255])), ('wrong-size', bytes([2,1]))]:
            with socket.create_connection(('127.0.0.1',port), timeout=5) as stream:
                stream.settimeout(5)
                stream.sendall(data)
                # 초기 RTT 등 이미 보낸 응답 이후 EOF/reset를 확인한다.
                try:
                    for _ in range(32):
                        if frame(stream) is None:
                            break
                    else:
                        raise AssertionError('invalid peer kept alive')
                except ConnectionResetError:
                    pass
            cases.append({'port':port, 'case':name, 'rejected':True})
    for repeat in range(4):
        with socket.create_connection(('127.0.0.1',8910), timeout=5) as stream:
            stream.settimeout(5)
            stamp = time.time_ns()//100
            rtt = bytes([18,39]) + struct.pack('<qq',stamp,stamp)
            stream.sendall(rtt*12)  # 로비 5개 budget보다 많은 batch
            stream.sendall(bytes([3]))
            time.sleep(.01)
            stream.sendall(bytes([40,0]))
            for _ in range(32):
                response = frame(stream)
                if response is None:
                    raise AssertionError('transition not delivered')
                kind, data = response
                if kind == 63:
                    slot, ip, port = struct.unpack('<i22sh',data)
                    assert ip.split(b'\0')[0] == b'127.0.0.1' and port == 8911
                    break
            else:
                raise AssertionError('budget remainder lost')
        cases.append({'case':'batch12+split-transition', 'repeat':repeat, 'passed':True})
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps({'ok':True,'cases':cases},indent=2)+'\n',encoding='utf-8')
    print(f'ServerCore network regression: PASS ({len(cases)} cases)')


if __name__ == '__main__':
    main()
