"""격리 LocalDB에서 정산·계정 권위·보상 복구·입력 포화·중지 정책을 검증한다."""
import argparse
import hashlib
import importlib.util
import json
import os
import socket
import struct
import subprocess
import sys
import threading
import time
import uuid
import ctypes
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / '.runtime/db-test-deps'))
import pyodbc
spec = importlib.util.spec_from_file_location('db_codec', Path(__file__).with_name('database.py'))
codec = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codec)


def wait_for(predicate, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(.02)
    raise AssertionError('condition timed out')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=['Debug', 'Release'], required=True)
    parser.add_argument('--config', type=Path, required=True)
    args = parser.parse_args()
    connection = args.config.read_text(encoding='utf-8-sig').strip()
    assert 'Server=(localdb)\\MSSQLLocalDB;' in connection and 'Database=NewWodTestRecovery;' in connection
    assert not (ROOT / '.runtime/processes.json').exists(), 'existing server run must be stopped first'
    logs = ROOT / 'artifacts/logs/database-consistency' / args.configuration
    logs.mkdir(parents=True, exist_ok=True)
    journal_root = logs / ('rewards-' + uuid.uuid4().hex)
    db = pyodbc.connect(connection, autocommit=True)
    cases, peers, runs = [], [], []
    prefix = 'F' + uuid.uuid4().hex[:6]
    account, missing, legacy, malformed, contender = prefix + 'a', prefix + 'b', prefix + 'c', prefix + 'd', prefix + 'e'
    process = game = stdout = stderr = None
    index = 0
    exe = ROOT / f'artifacts/bin/{args.configuration}/LobbyServer/Lobby_Server.exe'
    core = ROOT / f'artifacts/bin/{args.configuration}/ServerCore.Tests/ServerCore.Tests.exe'

    def check(name, condition=True, **extra):
        assert condition, name
        cases.append({'case': name, 'passed': True, **extra})

    def execute(sql, *values):
        cursor = db.execute(sql, *values)
        while cursor.nextset():
            pass
        cursor.close()

    def scalar(sql, *values):
        cursor = db.execute(sql, *values)
        result = cursor.fetchone()[0]
        cursor.close()
        return result

    def peer():
        item = codec.Peer()
        peers.append(item)
        return item

    def pending():
        return list(journal_root.glob('*/*.pending'))

    def start():
        nonlocal process, game, stdout, stderr, index
        index += 1
        env = os.environ.copy()
        env.update(WOD_DB_MODE='odbc', WOD_DB_CONFIG=str(args.config.resolve()), WOD_DB_REWARD_DIRECTORY=str(journal_root))
        env.pop('WOD_DB_CONNECTION', None)
        stdout = (logs / f'run-{index}.out.log').open('wb')
        stderr = (logs / f'run-{index}.err.log').open('wb')
        process = subprocess.Popen([str(exe)], cwd=ROOT / 'Server/Lobby_Server', env=env, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        print(f'DB recovery fixture run={index} pid={process.pid}', flush=True)
        # 최초 TCP 연결을 게임 서버로 취급하는 기존 로컬 계약을 사용한다.
        def initialized():
            if process.poll() is not None:
                raise AssertionError(f'lobby exited during startup: {process.returncode}')

            return 'Server Initialize Finish' in (logs / f'run-{index}.err.log').read_text(encoding='utf-8', errors='replace')
        wait_for(initialized, 15)
        game = socket.create_connection(('127.0.0.1', 8910), timeout=5)

    def stop(already_requested=False):
        nonlocal process, game
        if process is None:
            return
        if not already_requested and process.poll() is None:
            kernel = ctypes.WinDLL('kernel32', use_last_error=True)
            kernel.OpenEventW.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_wchar_p]
            kernel.OpenEventW.restype = ctypes.c_void_p
            kernel.SetEvent.argtypes = [ctypes.c_void_p]
            kernel.CloseHandle.argtypes = [ctypes.c_void_p]
            event = wait_for(lambda: kernel.OpenEventW(2, False, f'Local\\Wod.Server.Stop.{process.pid}'), 10)
            try:
                assert kernel.SetEvent(event)
            finally:
                kernel.CloseHandle(event)
        assert process.wait(timeout=20) == 0
        if game:
            game.close()
        stdout.close()
        stderr.close()
        text = (logs / f'run-{index}.err.log').read_text(encoding='utf-8', errors='replace')
        assert 'ServerCore stop pending=0 sockets=0 leased=0' in text, text[-500:]
        dispatcher = re.search(r'Lobby dispatcher stop (\{[^\n]+\})', text)
        database = re.search(r'Database drain (\{[^\n]+\})', text)
        runs.append({'run': index, 'normal_exit': True, 'zero_resources': True,
                     'dispatcher': json.loads(dispatcher[1]) if dispatcher else None,
                     'database': json.loads(database[1]) if database else None})
        process = game = None

    try:
        execute('EXEC dbo.SignUp ?,?', account, codec.password_hash())
        # 등록 시 차감한 아이템을 정확히 1회 돌려주고, 조회 count도 만료를 반영한다.
        execute('EXEC dbo.SaveShopBuyInfo ?,1,2', account)
        for repeat in range(5):
            execute("EXEC dbo.RegisterAuction ?,1,2,100,'2000-01-01'", account)
            barrier = threading.Barrier(2)
            failures = []
            def settle():
                try:
                    with pyodbc.connect(connection, autocommit=True) as other:
                        barrier.wait(3)
                        cursor = other.execute('EXEC dbo.SettleExpiredAuctions')
                        while cursor.nextset():
                            pass
                except BaseException as error:
                    failures.append(type(error).__name__)
            workers = [threading.Thread(target=settle) for _ in range(2)]
            for worker in workers:
                worker.start()
            for worker in workers:
                worker.join(8)
            owned = scalar('SELECT PartCount FROM dbo.customize_data WHERE Player_ID=? AND ShopType=1 AND ShopNum=2', account)
            listings = scalar('SELECT COUNT(*) FROM dbo.auction_data WHERE Player_ID=?', account)
            check(f'concurrent expiry restores once {repeat}', not failures and all(not worker.is_alive() for worker in workers) and owned == 1 and listings == 0, owned_part_count=owned, expired_listings=listings, concurrent_settlers=2)
        execute("EXEC dbo.RegisterAuction ?,1,2,100,'2000-01-01'", account)
        check('count query settles expiry', scalar('EXEC dbo.GetAuctionPartsNum') == 0 and scalar('SELECT PartCount FROM dbo.customize_data WHERE Player_ID=? AND ShopType=1 AND ShopNum=2', account) == 1)

        execute('EXEC dbo.SignUp ?,?', contender, codec.password_hash())
        for repeat in range(4):
            execute('EXEC dbo.SaveShopBuyInfo ?,4,?', account, repeat)
            execute('DECLARE @until datetime2=DATEADD(millisecond,80,SYSDATETIME()); EXEC dbo.RegisterAuction ?,4,?,100,@until;', account, repeat)
            barrier = threading.Barrier(2)
            errors, purchased = [], []
            def purchase():
                with pyodbc.connect(connection, autocommit=False) as buyer:
                    barrier.wait(3)
                    time.sleep(.15 if repeat % 2 else 0)
                    try:
                        for sql, values in [
                            ('EXEC dbo.BuyAuction ?,?,4,?,100', (account, contender, repeat)),
                            ('EXEC dbo.SaveToken ?,-100', (contender,)),
                            ('EXEC dbo.SaveToken ?,100', (account,)),
                        ]:
                            cursor = buyer.execute(sql, *values)
                            while cursor.nextset():
                                pass
                        buyer.commit()
                        purchased.append(True)
                    except pyodbc.Error as error:
                        buyer.rollback()
                        if '50017' not in str(error):
                            errors.append(type(error).__name__)
            def expire():
                with pyodbc.connect(connection, autocommit=True) as expirer:
                    barrier.wait(3)
                    time.sleep(.1)
                    try:
                        cursor = expirer.execute('EXEC dbo.SettleExpiredAuctions')
                        while cursor.nextset():
                            pass
                    except pyodbc.Error as error:
                        errors.append(type(error).__name__)
            workers = [threading.Thread(target=purchase), threading.Thread(target=expire)]
            for worker in workers:
                worker.start()
            for worker in workers:
                worker.join(8)
            owned = scalar('SELECT SUM(PartCount) FROM dbo.customize_data WHERE Player_ID IN (?,?) AND ShopType=4 AND ShopNum=?', account, contender, repeat)
            money = scalar('SELECT SUM(Token) FROM dbo.user_data WHERE Player_ID IN (?,?)', account, contender)
            check(f'purchase versus expiry conserves item and tokens {repeat}', not errors and all(not worker.is_alive() for worker in workers) and owned == 1 and money == 4000 and scalar('SELECT COUNT(*) FROM dbo.auction_data WHERE Player_ID=?', account) == 0, item_count=owned, combined_tokens=money, purchased=bool(purchased))

        # 이전 v2 자격 증명의 이전과 중단 시 rollback을 검사한다.
        execute('INSERT dbo.user_data(Player_ID,Player_Password) VALUES(?,?)', legacy, 'testpass')
        execute('INSERT dbo.user_data(Player_ID,Player_Password) VALUES(?,?)', malformed, 'pbkdf2-sha256$600000$' + 'g' * 32 + '$' + 'a' * 64)
        execute('UPDATE dbo.WodLocalSchema SET Version=2')
        migrate_args = ['pwsh', '-NoProfile', '-File', str(ROOT / 'scripts/Initialize-LocalDatabase.ps1'), '-Name', 'NewWodTestRecovery', '-ConfigPath', str(args.config.resolve())]
        interrupted = subprocess.run(migrate_args, capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        check('invalid credential migration rolls back', interrupted.returncode != 0 and scalar('SELECT Version FROM dbo.WodLocalSchema') == 2 and scalar('SELECT LEN(Player_Password) FROM dbo.user_data WHERE Player_ID=?', legacy) == 8)
        execute('DELETE dbo.user_data WHERE Player_ID=?', malformed)
        migrated = subprocess.run(migrate_args, capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        check('legacy credential migrated atomically', migrated.returncode == 0 and scalar('SELECT Version FROM dbo.WodLocalSchema') == 3 and scalar('SELECT LEN(Player_Password) FROM dbo.user_data WHERE Player_ID=?', legacy) == 118)

        start()
        a, duplicate = peer(), peer()
        a.auth(account)
        migrated_peer = peer()
        migrated_peer.auth(legacy)
        check('migrated credential accepted by native CNG verifier')
        migrated_peer.close()
        duplicate.send('CS_LOGIN', '10s10s', account.encode(), b'testpass')
        check('duplicate login denied', duplicate.until('SC_LOGIN_FAIL') == b'\x03')
        duplicate.close()
        alias = peer()
        alias.send('CS_LOGIN', '10s10s', account.lower().encode(), b'testpass')
        check('case-insensitive account alias denied', alias.until('SC_LOGIN_FAIL') == b'\x03')
        alias.close()
        # DB를 별도로 갱신해 메모리의 이전 잔액과 다른 조건을 만든다.
        execute('UPDATE dbo.user_data SET Token=2100 WHERE Player_ID=?', account)
        a.send('CS_SHOP', 'i', 0)
        a.until('SC_SHOP')
        amount = struct.unpack('<i', a.until('SC_TOKEN_NUM'))[0]
        check('purchase returns authoritative balance', amount == 2000 and scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account) == amount, response=amount)
        # 실제 게임 링크 보상 수신→복구 기록→SQL 원장→온라인 잔액까지 검사한다.
        before = scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', account)
        reward_start = time.perf_counter()
        game.sendall(codec.packet('GL_TRANSACTIONS', '10sh20s', account.encode(), 17, b'2026-10-29 00:00:00'))
        reward = struct.unpack('<i', a.until('SC_TOKEN_NUM'))[0]
        reward_ms = (time.perf_counter() - reward_start) * 1000
        check('reward persisted and published', reward == before + 17 and scalar('SELECT COUNT(*) FROM dbo.reward_applied WHERE Player_ID=?', account) == 1, local_end_to_end_ms=reward_ms)
        wait_for(lambda: not pending())
        a.close()
        time.sleep(.05)
        reconnected = peer()
        reconnected.auth(account)
        check('owner released after disconnect')

        # 입력 포화: SQL 대기 중 400개 패킷을 보내도 cleanup은 전달돼 재접속 가능하다.
        lock_entered = threading.Event()
        def hold_auction(seconds=1.0):
            with pyodbc.connect(connection, autocommit=False) as owner:
                owner.execute('SELECT COUNT(*) FROM dbo.auction_data WITH(TABLOCKX,HOLDLOCK)').fetchone()
                lock_entered.set()
                time.sleep(seconds)
                owner.rollback()
        locker = threading.Thread(target=hold_auction)
        locker.start()
        assert lock_entered.wait(3)
        reconnected.send('CS_OPEN_AUCTION')
        time.sleep(.05)
        reconnected.sock.sendall(codec.packet('CS_CHANGE_CHANNEL', 'h', 0) * 400)
        try:
            reconnected.until('SC_CHANGE_CHANNEL')
            raise AssertionError('full inbox accepted flooding client')
        except (EOFError, ConnectionResetError):
            pass
        locker.join(5)
        reconnected.close()
        time.sleep(.1)
        again = peer()
        again.auth(account)
        check('overflow disconnect completes cleanup and releases owner')
        again.close()
        time.sleep(.05)

        # 계정이 없어서 실패한 보상을 유실하지 않는다.
        game.sendall(codec.packet('GL_TRANSACTIONS', '10sh20s', missing.encode(), 23, b'2026-10-29 00:00:01'))
        record = wait_for(lambda: pending()[0] if pending() else None)
        wait_for(lambda: 'Reward pending:' in (logs / f'run-{index}.err.log').read_text(encoding='utf-8', errors='replace'))
        data = record.read_bytes()
        operation = data.splitlines()[1].decode()
        check('failed reward retained for recovery', scalar('SELECT COUNT(*) FROM dbo.reward_applied WHERE Operation_ID=?', operation) == 0)
        stop()
        check('default inbox limit and overflow counter observed', runs[-1]['dispatcher'] is not None and runs[-1]['dispatcher']['inbox_limit_jobs'] == 256 and runs[-1]['dispatcher']['inbox_limit_bytes'] == 65536 and runs[-1]['dispatcher']['inbox_rejected'] >= 1)
        execute('EXEC dbo.SignUp ?,?', missing, codec.password_hash())
        start()
        wait_for(lambda: scalar('SELECT COUNT(*) FROM dbo.reward_applied WHERE Operation_ID=?', operation) == 1)
        wait_for(lambda: not pending())
        check('failed reward replayed after restart', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', missing) == 2023)
        stop()
        # DB commit 후 파일 삭제 전에 종료된 상태를 재현한다.
        with record.open('wb') as output:
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        start()
        wait_for(lambda: not pending())
        check('committed reward replay not duplicated', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', missing) == 2023 and scalar('SELECT COUNT(*) FROM dbo.reward_applied WHERE Operation_ID=?', operation) == 1)

        # 중지 제한을 넘겨도 SQL 작업을 강제로 끊지 않고 PID와 기록을 유지한다.
        last = peer()
        last.auth(account)
        lock_entered.clear()
        locker = threading.Thread(target=hold_auction, args=(4.0,))
        locker.start()
        assert lock_entered.wait(3)
        last.send('CS_OPEN_AUCTION')
        time.sleep(.1)
        recorder = ROOT / '.runtime/Record-DbRecoveryTest.ps1'
        recorder.write_text('''param([int] $ServerPid,[string] $Executable,[string] $StatePath)
$process = Get-Process -Id $ServerPid
if (Test-Path -LiteralPath $StatePath) { throw 'existing state' }
@{Module='LobbyServer';ProcessId=$ServerPid;Executable=$Executable;StartTimeUtc=$process.StartTime.ToUniversalTime().ToString('O')} | ConvertTo-Json | Set-Content -LiteralPath $StatePath -Encoding utf8
''', encoding='utf-8')
        state = ROOT / '.runtime/processes.json'
        subprocess.run(['pwsh', '-NoProfile', '-File', str(recorder), '-ServerPid', str(process.pid), '-Executable', str(exe), '-StatePath', str(state)], check=True, capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        limited = subprocess.run(['pwsh', '-NoProfile', '-File', str(ROOT / 'scripts/Stop-Local.ps1'), '-TimeoutSeconds', '1'], capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        check('stop timeout preserves live process and run state', limited.returncode != 0 and process.poll() is None and state.exists())
        locker.join(6)
        stop(already_requested=True)
        final_stop = subprocess.run(['pwsh', '-NoProfile', '-File', str(ROOT / 'scripts/Stop-Local.ps1')], capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        check('drain completes after timeout and run state clears', final_stop.returncode == 0 and not state.exists())
        last.close()

        # 서로 다른 payload를 같은 ID로 재처리하면 거부한다.
        try:
            execute('EXEC dbo.ApplyReward ?,?,?', operation, missing, 24)
            raise AssertionError('conflicting reward payload accepted')
        except pyodbc.Error:
            check('conflicting reward identity rejected', scalar('SELECT Token FROM dbo.user_data WHERE Player_ID=?', missing) == 2023)
        result = {'ok': True, 'configuration': args.configuration, 'cases': cases, 'server_runs': runs, 'scope': 'isolated LocalDB; real LobbyServer TCP, reward disk records, SQL ledger and restart; no production or blockchain'}
    finally:
        for item in peers:
            item.close()
        if process is not None:
            stop()
        state = ROOT / '.runtime/processes.json'
        if state.exists():
            recorded = json.loads(state.read_text(encoding='utf-8-sig'))
            records = recorded if isinstance(recorded, list) else [recorded]
            if all(Path(item['Executable']) == exe for item in records):
                subprocess.run(['pwsh', '-NoProfile', '-File', str(ROOT / 'scripts/Stop-Local.ps1')], capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        for table in ['reward_applied', 'auction_data', 'customize_data', 'user_data']:
            execute(f'DELETE dbo.{table} WHERE Player_ID IN (?,?,?,?,?)', account, missing, legacy, malformed, contender)
        db.close()
    (logs / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Database consistency: PASS ({len(cases)} cases, {len(runs)} server runs)')


if __name__ == '__main__':
    main()
