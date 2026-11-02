"""동일 Release 서버의 순차 읽기 비용과 실제 probe SQL 수를 전후 비교한다."""
import argparse
import hashlib
import json
import os
import re
import socket
import statistics
import subprocess
import sys
import time
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from database_reconnect import ROOT, codec, pyodbc, request_stop, wait_for


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--instance', required=True)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--after', type=Path, required=True)
    args = parser.parse_args()
    assert re.fullmatch(r'NewWodTestProbe[0-9a-f]{8}', args.instance)
    connection = args.config.read_text(encoding='utf-8-sig').strip()
    assert f'Server=(localdb)\\{args.instance};' in connection and f'Database={args.instance};' in connection
    assert not (ROOT / '.runtime/processes.json').exists()
    logs = ROOT / 'artifacts/logs/database-request-cost'
    logs.mkdir(parents=True, exist_ok=True)
    db = pyodbc.connect(connection, autocommit=True)
    account = 'P' + uuid.uuid4().hex[:7]
    db.execute('EXEC dbo.SignUp ?,?', account, codec.password_hash()).close()
    db.execute('EXEC dbo.SaveShopBuyInfo ?,1,2', account).close()
    rows, counted, runs = [], {}, []
    process = peer = game = stdout = stderr = None
    event_name = 'WodProbe' + uuid.uuid4().hex
    procedure = db.execute("SELECT OBJECT_ID('dbo.GetPlayerCustomizeParts')").fetchone()[0]

    def drop_events():
        db.execute(f"IF EXISTS(SELECT 1 FROM sys.server_event_sessions WHERE name='{event_name}') DROP EVENT SESSION [{event_name}] ON SERVER").close()

    def count_requests():
        drop_events()
        db.execute(f"""CREATE EVENT SESSION [{event_name}] ON SERVER
ADD EVENT sqlserver.sql_statement_completed(WHERE ([sqlserver].[client_app_name]=N'NewWodRequestCost')),
ADD EVENT sqlserver.sp_statement_completed(WHERE ([object_id]=({procedure}) AND [sqlserver].[client_app_name]=N'NewWodRequestCost'))
ADD TARGET package0.ring_buffer(SET max_memory=4096)
WITH (MAX_MEMORY=4096 KB,MAX_DISPATCH_LATENCY=1 SECONDS)""").close()
        db.execute(f'ALTER EVENT SESSION [{event_name}] ON SERVER STATE=START').close()
        request(100)
        time.sleep(1.2)
        cursor = db.execute("SELECT CAST(t.target_data AS nvarchar(max)),s.dropped_event_count FROM sys.dm_xe_session_targets t JOIN sys.dm_xe_sessions s ON s.address=t.event_session_address WHERE s.name=?", event_name)
        xml, dropped = cursor.fetchone()
        cursor.close()
        tree = ET.fromstring(xml)
        assert tree.attrib.get('truncated', '0') == '0' and dropped == 0
        statements = [(event.attrib['name'], event.findtext("data[@name='statement']/value", '')) for event in tree.findall('event')]
        probes = sum(bool(re.fullmatch(r'SELECT\s+1;?', text.strip(), re.I)) for kind, text in statements if kind == 'sql_statement_completed')
        queries = sum(text.lstrip().upper().startswith('SELECT SHOPTYPE,SHOPNUM,PARTCOUNT') for kind, text in statements if kind == 'sp_statement_completed')
        assert queries == 100, (queries, [(kind, text[:100]) for kind,text in statements[:10]])
        drop_events()
        return {'requests': 100, 'probe_sql': probes, 'actual_selects': queries, 'dropped_events': dropped, 'truncated': False}

    def request(count):
        elapsed = []
        for _ in range(count):
            start = time.perf_counter_ns()
            peer.send('CS_OPEN_CUSTOMIZE')
            response = peer.until('SC_CUSTOMIZE_PARTS')
            # 실제 소유 아이템 행이 반환되는지 확인한다. 빈 성공 응답으로 대체할 수 없다.
            columns = len(response) // 3
            assert response[0] == 1 and response[columns] == 2 and response[columns * 2] == 1
            elapsed.append((time.perf_counter_ns() - start) / 1e6)
        return elapsed

    try:
        for round_index in range(5):
            for variant in (('before','after') if round_index % 2 == 0 else ('after','before')):
                exe = getattr(args, variant).resolve()
                prefix = f'{round_index}-{variant}'
                stdout = (logs / (prefix + '.out.log')).open('wb')
                stderr_path = logs / (prefix + '.err.log')
                stderr = stderr_path.open('wb')
                env = os.environ.copy()
                env.update(WOD_DB_MODE='odbc', WOD_DB_CONNECTION=connection + 'APP=NewWodRequestCost;ConnectRetryCount=0;', WOD_DB_REWARD_DIRECTORY=str(logs / (prefix + '-rewards')))
                process = subprocess.Popen([str(exe)], cwd=ROOT/'Server/Lobby_Server', env=env, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
                wait_for(lambda: 'Server Initialize Finish' in stderr_path.read_text(encoding='utf-8',errors='replace') or process.poll() is not None)
                assert process.poll() is None
                game = socket.create_connection(('127.0.0.1',8910), timeout=5)
                peer = codec.Peer()
                peer.auth(account)
                request(20)
                values = request(200)
                rows.append({'round': round_index, 'variant': variant, 'requests': 200, 'median_ms': statistics.median(values), 'min_ms': min(values), 'max_ms': max(values), 'total_ms': sum(values), 'samples_ms': values})
                # SQL 수 측정은 별도100건으로 수행한다. 지연 비교에는 XE를 켜지 않는다.
                if round_index == 0:
                    counted[variant] = count_requests()
                    assert counted[variant]['probe_sql'] == (100 if variant == 'before' else 0), counted
                peer.close()
                peer = None
                request_stop(process)
                assert process.wait(timeout=15) == 0
                stdout.close()
                stderr.close()
                log = stderr_path.read_text(encoding='utf-8', errors='replace')
                assert 'ServerCore stop pending=0 sockets=0 leased=0' in log
                drain = json.loads(re.search(r'Database drain (\{[^\n]+\})', log)[1])
                assert drain['pending'] == 0 and drain['failed'] == 0 and drain['outcome_unknown'] == 0
                runs.append({'round':round_index,'variant':variant,'normal_exit':True,'zero_resources':True,'database':drain})
                game.close()
                game = process = None
                print(f'PASS request cost {round_index} {variant}: median={statistics.median(values):.4f}ms',flush=True)
        summary = {}
        for variant in ('before','after'):
            medians = [row['median_ms'] for row in rows if row['variant']==variant]
            summary[variant] = {'median_ms':statistics.median(medians), 'min_run_median_ms':min(medians), 'max_run_median_ms':max(medians)}
        result = {'ok':True,'configuration':'Release','requests_per_run':200,'warmup':20,'runs':5,'rows':rows,'sql_counts':counted,'summary':summary,'server_runs':runs,'binary_sha256':{variant:hashlib.sha256(getattr(args,variant).read_bytes()).hexdigest() for variant in ('before','after')},'scope':'private LocalDB; sequential one client; no XE during timing; 100-request XE counts separate; no WAN, maximum throughput or p99 claim'}
        (logs/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        print(json.dumps({'ok':True,'summary':summary,'sql_counts':counted}),flush=True)
    finally:
        if peer:
            peer.close()
        if process and process.poll() is None:
            request_stop(process)
            try:
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        if game:
            game.close()
        if stdout and not stdout.closed:
            stdout.close()
        if stderr and not stderr.closed:
            stderr.close()
        drop_events()
        db.close()


if __name__ == '__main__':
    main()
