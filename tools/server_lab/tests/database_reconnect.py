"""독립 LocalDB에서 실제 단절→재접속→FIFO·보상 재개를 검사한다. 운영/공용 DB 중지 금지."""
import argparse
import ctypes
import hashlib
import importlib.util
import json
import os
import re
import socket
import struct
import subprocess
import sys
import time
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / '.runtime/db-test-deps'))
import pyodbc
pyodbc.pooling = False
spec = importlib.util.spec_from_file_location('db_codec', Path(__file__).with_name('database.py'))
codec = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codec)


def wait_for(predicate, timeout=20):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        value = predicate()
        if value:
            return value
        time.sleep(.02)
    raise AssertionError('condition timed out')


def request_stop(process):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenEventW.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_wchar_p]
    kernel.OpenEventW.restype = ctypes.c_void_p
    kernel.SetEvent.argtypes = [ctypes.c_void_p]
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    event = wait_for(lambda: kernel.OpenEventW(2, False, f'Local\\Wod.Server.Stop.{process.pid}'))
    try:
        assert kernel.SetEvent(event)
    finally:
        kernel.CloseHandle(event)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=['Debug', 'Release'], required=True)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--instance', required=True)
    args = parser.parse_args()
    assert re.fullmatch(r'NewWodTestReconnect[0-9a-f]{8}', args.instance)
    connection = args.config.read_text(encoding='utf-8-sig').strip()
    assert f'Server=(localdb)\\{args.instance};' in connection and f'Database={args.instance};' in connection
    assert not (ROOT / '.runtime/processes.json').exists(), 'existing server run must be stopped first'
    master = connection.replace(f'Database={args.instance};', 'Database=master;')
    logs = ROOT / 'artifacts/logs/database-reconnect' / args.configuration
    logs.mkdir(parents=True, exist_ok=True)
    journal = logs / ('rewards-' + uuid.uuid4().hex)
    outpath, errpath = logs / 'server.out.log', logs / 'server.err.log'
    cases, samples, peers = [], [], []
    process = game = db = admin = stdout = stderr = None
    account = 'R' + uuid.uuid4().hex[:7]
    exe = ROOT / f'artifacts/bin/{args.configuration}/LobbyServer/Lobby_Server.exe'

    def check(name, condition=True, **evidence):
        assert condition, name
        cases.append({'case': name, 'passed': True, **evidence})
        print('PASS ' + name, flush=True)

    def peer():
        item = codec.Peer()
        peers.append(item)
        return item

    def text():
        return errpath.read_text(encoding='utf-8', errors='replace')

    def offline():
        nonlocal db
        if db:
            db.close()
            db = None
        admin.execute(f'ALTER DATABASE [{args.instance}] SET OFFLINE WITH ROLLBACK IMMEDIATE')

    def online():
        nonlocal db
        admin.execute(f'ALTER DATABASE [{args.instance}] SET ONLINE')
        db = pyodbc.connect(connection, autocommit=True, timeout=5)

    def scalar(sql, *values):
        cursor = db.execute(sql, *values)
        value = cursor.fetchone()[0]
        cursor.close()
        return value

    try:
        admin = pyodbc.connect(master, autocommit=True, timeout=5)
        db = pyodbc.connect(connection, autocommit=True, timeout=5)
        db.execute('EXEC dbo.SignUp ?,?', account, codec.password_hash()).close()
        env = os.environ.copy()
        env.update(WOD_DB_MODE='odbc', WOD_DB_CONFIG=str(args.config.resolve()), WOD_DB_REWARD_DIRECTORY=str(journal))
        env['WOD_DB_CONNECTION'] = connection + 'APP=NewWodReconnect;ConnectRetryCount=0;'
        stdout, stderr = outpath.open('wb'), errpath.open('wb')
        process = subprocess.Popen([str(exe)], cwd=ROOT / 'Server/Lobby_Server', env=env, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        print(f'private reconnect fixture pid={process.pid}', flush=True)
        wait_for(lambda: 'Server Initialize Finish' in text() or process.poll() is not None)
        assert process.poll() is None
        game = socket.create_connection(('127.0.0.1', 8910), timeout=5)
        a, b = peer(), peer()
        a.auth(account)
        b.auth('I' + uuid.uuid4().hex[:7], True)
        check('initial authenticated SQL connection')

        for repeat in range(5):
            disconnects, reconnects = text().count('Database disconnected '), text().count('Database reconnected ')
            before = scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account)
            ledger = scalar('SELECT COUNT(*) FROM dbo.reward_applied WHERE Player_ID=?', account)
            start = time.perf_counter()
            offline()
            a.seen.clear()
            # 실제 조회의 오류로 장애를 감지한다. 이후 미실행 구매·채널 변경은 순서대로 대기한다.
            a.send('CS_OPEN_CUSTOMIZE')
            a.send('CS_SHOP', 'i', 1)
            a.send('CS_CHANGE_CHANNEL', 'h', 0)
            wait_for(lambda: text().count('Database disconnected ') > disconnects)
            detected_ms = (time.perf_counter() - start) * 1000
            independent_start = time.perf_counter()
            b.send('CS_CHANGE_CHANNEL', 'h', 0)
            b.until('SC_CHANGE_CHANNEL')
            independent_ms = (time.perf_counter() - independent_start) * 1000
            check(f'other session progresses during outage {repeat}', process.poll() is None, response_ms=independent_ms)
            game.sendall(codec.packet('GL_TRANSACTIONS', '10sh20s', account.encode(), 17, b'2026-11-01 00:00:00'))
            wait_for(lambda: len(list(journal.glob('*/*.pending'))) == 1)
            # 실패한 재접속을 최소 1회 거친 뒤 복구한다.
            time.sleep(1.4)
            if repeat == 0:
                old_engine = admin.execute("SELECT CONVERT(int,SERVERPROPERTY('ProcessID'))").fetchone()[0]
                admin.close()
                admin = None
                subprocess.run(['SqlLocalDB', 'stop', args.instance, '-k'], check=True, capture_output=True)
                subprocess.run(['SqlLocalDB', 'start', args.instance], check=True, capture_output=True)
                admin = pyodbc.connect(master, autocommit=True, timeout=5)
                new_engine = admin.execute("SELECT CONVERT(int,SERVERPROPERTY('ProcessID'))").fetchone()[0]
                check('private SQL engine stopped and restarted during outage', old_engine != new_engine)
            a.sock.settimeout(.15)
            try:
                a.until('SC_SHOP')
                raise AssertionError('DB request completed while database offline')
            except socket.timeout:
                pass
            finally:
                a.sock.settimeout(15)
            restored = time.perf_counter()
            online()
            a.until('SC_SHOP')
            a.until('SC_CHANGE_CHANNEL')
            applied_ms = (time.perf_counter() - restored) * 1000
            wait_for(lambda: text().count('Database reconnected ') > reconnects)
            wait_for(lambda: not list(journal.glob('*/*.pending')))
            wait_for(lambda: scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account) == before - 100 + 17)
            check(f'queued write executes once and resumes session FIFO {repeat}', a.seen.index(codec.C['SC_SHOP']) < a.seen.index(codec.C['SC_CHANGE_CHANNEL']))
            check(f'queued reward applied once {repeat}', scalar('SELECT COUNT(*) FROM dbo.reward_applied WHERE Player_ID=?', account) == ledger + 1)
            check(f'same server process survives reconnect {repeat}', process.poll() is None)
            samples.append({'run': repeat, 'detected_ms': detected_ms, 'independent_response_ms': independent_ms, 'restore_to_resumed_ms': applied_ms, 'outage_to_resumed_ms': (time.perf_counter() - start) * 1000})

        # probe가 없으므로 첫 요청이 쓰기인 경우에는 불명확한 결과로 처리하고 자동 반복하지 않는다.
        before = scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account)
        unknown_before = text().count('Database write outcome unknown')
        offline()
        a.send('CS_SHOP', 'i', 1)
        try:
            a.until('SC_SHOP')
            raise AssertionError('first failed write incorrectly reported success')
        except (EOFError, ConnectionResetError):
            pass
        a.close()
        wait_for(lambda: text().count('Database write outcome unknown') > unknown_before)
        online()
        time.sleep(1.5)
        check('first failed write detects outage without automatic replay', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account) == before)
        a = peer()
        a.auth(account)

        # 실제 엔진 연결을 KILL하고 새 연결을 통해 동일 요청이 완료되는지 검사한다.
        sessions = admin.execute("SELECT session_id FROM sys.dm_exec_sessions WHERE program_name='NewWodReconnect'").fetchall()
        db.close()
        db = None
        killed = 0
        for row in sessions:
            try:
                admin.execute(f'KILL {int(row[0])}')
                killed += 1
            except pyodbc.Error:
                pass
        a.send('CS_OPEN_CUSTOMIZE')
        a.until('SC_CUSTOMIZE_PARTS')
        db = pyodbc.connect(connection, autocommit=True)
        check('killed SQL session reconnects without lobby restart', killed >= 1 and process.poll() is None)

        def definition(name):
            return scalar('SELECT OBJECT_DEFINITION(OBJECT_ID(?))', 'dbo.' + name)

        def restore(sql):
            db.execute(re.sub(r'\bCREATE(?:\s+OR\s+ALTER)?\s+PROC(?:EDURE)?\b', 'CREATE OR ALTER PROCEDURE', sql, count=1, flags=re.I)).close()

        def active_session():
            cursor = admin.execute("SELECT s.session_id FROM sys.dm_exec_sessions s JOIN sys.dm_exec_requests r ON s.session_id=r.session_id WHERE s.program_name='NewWodReconnect' AND r.wait_type='WAITFOR'")
            row = cursor.fetchone()
            cursor.close()
            return int(row[0]) if row else None

        # 테스트 전용 SP 지연 중 실제 연결을 끊어, 안전한 조회만 재실행하는지 확인한다.
        original = definition('GetPlayerCustomizeParts')
        try:
            restore("CREATE PROCEDURE dbo.GetPlayerCustomizeParts @id varchar(10) AS BEGIN SET NOCOUNT ON; WAITFOR DELAY '00:00:04'; SELECT ShopType,ShopNum,PartCount FROM dbo.customize_data WHERE Player_ID=@id; END")
            a.send('CS_OPEN_CUSTOMIZE')
            victim = wait_for(active_session)
            admin.execute(f'KILL {victim}')
            restore(original)
            a.until('SC_CUSTOMIZE_PARTS')
            check('in-flight read retried after actual SQL connection kill')
        finally:
            restore(original)

        # 전용 DB에서만 COMMIT 후 응답을 지연한다. 확정 결과를 모르는 쓰기는 재실행하면 안 된다.
        original = definition('SaveToken')
        before = scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account)
        try:
            restore("CREATE PROCEDURE dbo.SaveToken @id varchar(10), @token int AS BEGIN SET NOCOUNT ON; UPDATE dbo.user_data SET Token=Token+@token WHERE Player_ID=@id; IF @@TRANCOUNT>0 COMMIT TRANSACTION; WAITFOR DELAY '00:00:04'; END")
            a.send('CS_SHOP', 'i', 1)
            victim = wait_for(active_session)
            check('fixture SQL committed before response loss', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account) == before - 100)
            admin.execute(f'KILL {victim}')
            restore(original)
            try:
                a.until('SC_SHOP')
                raise AssertionError('unknown write reported success')
            except (EOFError, ConnectionResetError):
                pass
            a.close()
            wait_for(lambda: 'Database write outcome unknown' in text())
            wait_for(lambda: account + ' Disconnect' in text())
            time.sleep(1.5)
            check('uncertain committed write not replayed', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account) == before - 100)
        finally:
            restore(original)
        a = peer()
        a.auth(account)

        # 장시간 장애에서 클라이언트 기한은 해제하고, 내구성 보상은 복구까지 유지한다.
        disconnects = text().count('Database disconnected ')
        offline()
        game.sendall(codec.packet('GL_TRANSACTIONS', '10sh20s', account.encode(), 19, b'2026-11-01 00:00:00'))
        wait_for(lambda: len(list(journal.glob('*/*.pending'))) == 1)
        a.send('CS_OPEN_CUSTOMIZE')
        a.send('CS_CHANGE_CHANNEL', 'h', 0)
        wait_for(lambda: text().count('Database disconnected ') > disconnects)
        a.sock.settimeout(40)
        start = time.perf_counter()
        a.until('SC_CHAT')
        a.until('SC_CHANGE_CHANNEL')
        expired_ms = (time.perf_counter() - start) * 1000
        check('30 second request expiry releases session with temporary notice', 28000 <= expired_ms < 37000, elapsed_ms=expired_ms)
        a.sock.settimeout(15)
        before = None
        online()
        a.until('SC_TOKEN_NUM')
        wait_for(lambda: not list(journal.glob('*/*.pending')))
        check('durable reward survives client request deadline and resumes')

        offline()
        game.sendall(codec.packet('GL_TRANSACTIONS', '10sh20s', account.encode(), 23, b'2026-11-01 00:00:00'))
        wait_for(lambda: len(list(journal.glob('*/*.pending'))) == 1)

        # DB 정지 상태에서 게임 서버 프로세스만 정상 종료한다. 무한 재접속 대기는 금지한다.
        b.send('CS_OPEN_AUCTION')
        start = time.perf_counter()
        request_stop(process)
        assert process.wait(timeout=12) == 0
        stop_ms = (time.perf_counter() - start) * 1000
        check('shutdown interrupts disconnected backoff', stop_ms < 10000, elapsed_ms=stop_ms)
        check('zero leaked server resources', 'ServerCore stop pending=0 sockets=0 leased=0' in text())
        check('shutdown retains durable pending reward', len(list(journal.glob('*/*.pending'))) == 1)
        drain = json.loads(re.search(r'Database drain (\{[^\n]+\})', text())[1])
        check('all accepted work released', drain['pending'] == 0 and drain['accepted'] == drain['completed'])
        check('failed reconnect attempts and request expiry observed', drain['reconnect_attempts'] > drain['reconnects'] and drain['expired'] >= 1)
        result = {'ok': True, 'configuration': args.configuration, 'cases': cases, 'samples': samples, 'database': drain, 'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'binary_sha256': hashlib.sha256(exe.read_bytes()).hexdigest(), 'scope': 'private LocalDB; real TCP/ODBC offline/online and KILL; no production DB, power loss, WAN or full GUI'}
        (logs / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps({'ok': True, 'checks': len(cases), 'configuration': args.configuration}), flush=True)
    finally:
        if process and process.poll() is None:
            request_stop(process)
            try:
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        for item in peers:
            item.close()
        if game:
            game.close()
        if stdout:
            stdout.close()
        if stderr:
            stderr.close()
        if db:
            db.close()
        if admin:
            try:
                admin.execute(f'ALTER DATABASE [{args.instance}] SET ONLINE')
            finally:
                admin.close()


if __name__ == '__main__':
    main()
