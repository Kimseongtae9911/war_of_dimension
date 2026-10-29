"""실제 로비→ODBC→격리 LocalDB 검증. Test-Database.ps1이 프로세스 수명을 소유한다."""
import argparse
import json
import socket
import struct
import sys
import threading
import time
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / '.runtime/db-test-deps'))
import pyodbc

ABI = json.loads((ROOT / 'Shared/Protocol/tests/abi-x64-msvc.json').read_text(encoding='utf-8-sig'))
C = dict(ABI['constants'])
T = {item['name']: item for item in ABI['types']}


def packet(name, fmt='', *values):
    data = struct.pack('<' + fmt, *values)
    assert len(data) + 2 == T[name + '_PACKET']['size'], name
    return bytes((len(data) + 2, C[name])) + data


class Peer:
    def __init__(self):
        self.sock = socket.create_connection(('127.0.0.1', 8910), timeout=8)
        self.sock.settimeout(8)
        self.seen = []

    def close(self):
        self.sock.close()

    def send(self, name, fmt='', *values):
        self.sock.sendall(packet(name, fmt, *values))

    def exact(self, size):
        data = b''
        while len(data) < size:
            chunk = self.sock.recv(size - len(data))
            if not chunk:
                raise EOFError('peer closed')
            data += chunk
        return data

    def until(self, name):
        for _ in range(2000):
            size, kind = self.exact(2)
            data = self.exact(size - 2)
            self.seen.append(kind)
            if kind == C[name]:
                return data
        raise AssertionError('response missing: ' + name)

    def auth(self, name, signup=False, password=b'testpass'):
        self.send('CS_SIGN_UP' if signup else 'CS_LOGIN', '10s10s', name.encode(), password)
        return self.until('SC_LOGIN_INFO')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    connection = args.config.read_text(encoding='utf-8-sig').strip()
    # 이 검사는 전용 로컬 DB에만 fixture를 쓴다.
    assert 'Server=(localdb)\\MSSQLLocalDB;' in connection and 'Database=NewWodTest;' in connection
    db = pyodbc.connect(connection, autocommit=True)
    prefix = 'Q' + uuid.uuid4().hex[:6]
    buyer, seller = prefix + 'a', prefix + 'b'
    peers, cases, samples = [], [], []

    def peer():
        value = Peer()
        peers.append(value)
        return value

    def check(name, condition=True, **extra):
        assert condition, name
        cases.append({'case': name, 'passed': True, **extra})

    def scalar(sql, *values):
        return db.execute(sql, *values).fetchone()[0]

    def locked_query(client, seconds=.25, disconnect=False):
        locked = threading.Event()
        errors = []
        def lock():
            try:
                with pyodbc.connect(connection, autocommit=False) as owner:
                    owner.execute('SELECT COUNT(*) FROM dbo.auction_data WITH (TABLOCKX,HOLDLOCK)').fetchone()
                    locked.set()
                    time.sleep(seconds)
                    owner.rollback()
            except BaseException as error:
                errors.append(type(error).__name__)
                locked.set()
        thread = threading.Thread(target=lock)
        thread.start()
        assert locked.wait(3) and not errors
        start = time.perf_counter()
        client.send('CS_OPEN_AUCTION')
        client.send('CS_CHANGE_CHANNEL', 'h', 0)
        time.sleep(.03)
        return thread, start, errors

    try:
        a, b = peer(), peer()
        a.auth(buyer, True)
        b.auth(seller, True)
        check('signup and DB-backed login', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', buyer) == 2000)
        failed = peer()
        failed.send('CS_SIGN_UP', '10s10s', buyer.encode(), b'testpass')
        check('duplicate signup', failed.until('SC_LOGIN_FAIL') == b'\x02')
        failed.close()
        wrong = peer()
        wrong.send('CS_LOGIN', '10s10s', buyer.encode(), b'wrong')
        check('wrong password', wrong.until('SC_LOGIN_FAIL') == b'\x01')
        wrong.close()
        # 동일 TCP batch 안의 DB 쓰기 3건 뒤 조회: 순서와 실제 반영 확인.
        a.sock.sendall(packet('CS_SHOP', 'i', 0) * 3 + packet('CS_OPEN_CUSTOMIZE'))
        amounts = []
        for _ in range(3):
            a.until('SC_SHOP')
            amounts.append(struct.unpack('<i', a.until('SC_TOKEN_NUM'))[0])
        a.until('SC_CUSTOMIZE_PARTS')
        check('pipelined write/write/read FIFO', amounts == [1900, 1800, 1700] and scalar('SELECT SUM(PartCount) FROM dbo.customize_data WHERE Player_ID=?', buyer) == 3)
        model = [0] * 28
        model[0], model[4] = 1, 2
        a.send('CS_CUSTOMIZE', '28h', *model)
        a.until('SC_MODEL_CUSTOMIZE')
        check('appearance persisted without token overwrite', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', buyer) == 1700 and scalar('SELECT Chr_Sex FROM dbo.user_data WHERE Player_ID=?', buyer) == 1)
        a.send('CS_STAKE_TOKEN', 'iH', 10, 100)
        check('staking success', a.until('SC_STAKE_TOKEN') == b'\0\0')
        a.until('SC_STAKE_TOKEN_INFO')
        check('staking persistence', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', buyer) == 1690)
        for run in range(5):
            thread, start, errors = locked_query(a)
            unrelated_start = time.perf_counter()
            b.send('CS_CHANGE_CHANNEL', 'h', 0)
            b.until('SC_CHANGE_CHANNEL')
            unrelated_ms = (time.perf_counter() - unrelated_start) * 1000
            seen_start = len(a.seen)
            a.until('SC_CHANGE_CHANNEL')
            blocked_ms = (time.perf_counter() - start) * 1000
            response_order = a.seen[seen_start:]
            thread.join()
            check(f'real SQL lock isolation and session order {run}', not errors and C['SC_AUCTION_PARTS_NUM'] in response_order and response_order.index(C['SC_AUCTION_PARTS_NUM']) < response_order.index(C['SC_CHANGE_CHANNEL']) and blocked_ms >= 180)
            samples.append({'run': run, 'sql_blocked_session_ms': blocked_ms, 'unrelated_session_ms': unrelated_ms})
        # DB 오류 후 성공 응답/메모리 변경을 하지 않고 rollback, 다음 요청은 계속 처리.
        db.execute('UPDATE dbo.user_data SET Token=0 WHERE Player_ID=?', buyer)
        owned = scalar('SELECT SUM(PartCount) FROM dbo.customize_data WHERE Player_ID=?', buyer)
        a.send('CS_SHOP', 'i', 0)
        try:
            a.until('SC_SHOP')
            raise AssertionError('SQL failure acknowledged as success')
        except (EOFError, ConnectionResetError):
            pass
        check('failed write rolls back item grant', scalar('SELECT SUM(PartCount) FROM dbo.customize_data WHERE Player_ID=?', buyer) == owned)
        a.close()
        db.execute('UPDATE dbo.user_data SET Token=100000 WHERE Player_ID=?', buyer)
        a = peer()
        a.auth(buyer)
        check('worker survives SQL failure')
        db.execute('INSERT dbo.customize_data VALUES(?,1,2,1)', seller)
        b.send('CS_REGISTER_AUCTION', '10shhi', b'forged', 1, 2, 40000)
        b.send('CS_OPEN_AUCTION')
        b.until('SC_AUCTION_PARTS_NUM')
        check('registration uses authenticated name', scalar('SELECT COUNT(*) FROM dbo.auction_data WHERE Player_ID=?', seller) == 1)
        a.send('CS_GET_AUCTION_INFO', 'i', 0)
        listing = a.until('SC_AUCTION_INFO')
        check('auction first page query contract', listing[:10].split(b'\0')[0] == seller.encode() and struct.unpack_from('<H', listing, 14)[0] == 40000)
        a.send('CS_BUY_AUCTION', '10shhH', seller.encode(), 1, 2, 40000)
        a.until('SC_TOKEN_NUM')
        check('unsigned price and atomic purchase', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', buyer) == 60000 and scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', seller) == 42000 and scalar('SELECT COUNT(*) FROM dbo.auction_data WHERE Player_ID=?', seller) == 0)
        # 소켓 종료 직후 같은 계정 재접속: 오래된 결과가 새 세션을 오염시키지 않는다.
        transient = peer()
        transient.auth(buyer)
        thread, start, errors = locked_query(transient)
        transient.close()
        thread.join()
        recovered = peer()
        recovered.auth(buyer)
        check('disconnect during pending DB and reconnect', not errors)
        # 5초 query timeout은 로비 worker를 점유하지 않으며 이후 큐도 진행한다.
        timeout_peer = peer()
        timeout_peer.auth(buyer)
        thread, start, errors = locked_query(timeout_peer, seconds=6.0)
        try:
            timeout_peer.until('SC_AUCTION_PARTS_NUM')
            raise AssertionError('query timeout did not fail')
        except (EOFError, ConnectionResetError):
            elapsed = time.perf_counter() - start
        thread.join()
        check('query timeout and fail closed', not errors and 4 <= elapsed <= 7, elapsed_seconds=elapsed)
        recovered.send('CS_OPEN_AUCTION')
        recovered.until('SC_AUCTION_PARTS_NUM')
        check('queue progresses after timeout')
        result = {'ok': True, 'cases': cases, 'real_sql_lock_runs': samples, 'scope': 'real LobbyServer TCP + ODBC Driver + isolated LocalDB; no production DB or blockchain'}
    finally:
        for item in peers:
            item.close()
        # 테스트가 만든 두 계정만 정리한다. 저장소/사용자 DB는 삭제하지 않는다.
        for table in ['auction_data', 'customize_data', 'user_data']:
            db.execute(f'DELETE dbo.{table} WHERE Player_ID IN (?,?)', buyer, seller)
        db.close()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f"Database integration: PASS ({len(cases)} cases)")


if __name__ == '__main__':
    main()
