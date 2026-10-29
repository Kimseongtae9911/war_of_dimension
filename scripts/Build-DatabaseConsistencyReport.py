"""실행 완료된 격리 LocalDB 검사로 후속 정합성 보고서를 만든다."""
from __future__ import annotations

import hashlib
import io
import json
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/development"


def read(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8-sig"))


def digest(path: Path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def svg(fig):
    stream = io.StringIO()
    fig.savefig(stream, format="svg", bbox_inches="tight")
    plt.close(fig)
    return "<svg" + stream.getvalue().split("<svg", 1)[1]


def main():
    configurations = {}
    for config in ("Debug", "Release"):
        item = {
            "baseline": read(f"artifacts/logs/database/integration-{config}.json"),
            "consistency": read(f"artifacts/logs/database-consistency/{config}/result.json"),
            "startup": read(f"artifacts/logs/database/startup-{config}.json"),
            "core": read(f"artifacts/logs/test-server-core-{config}/checks.json"),
            "smoke": read(f"artifacts/logs/smoke-{config}.json"),
        }
        for key in ("baseline", "consistency", "startup", "core"):
            assert item[key]["ok"], (config, key)
        for key, expected in (("baseline", 25), ("consistency", 26), ("startup", 3)):
            assert len(item[key]["cases"]) == expected
            assert all(case["passed"] for case in item[key]["cases"])
        assert item["core"]["checks"] == 1694
        assert item["smoke"]["ServersConnected"]
        assert all(run["normal_exit"] and run["zero_resources"] for run in item["consistency"]["server_runs"])
        configurations[config] = item
    network = read("artifacts/logs/server-core-integration-Release/summary.json")
    assert network["Ok"] and len(network["Cases"]) == 7
    assert all(case["Passed"] for case in network["Cases"])
    previous = read("docs/development/evidence/database-additional-review-20261029.json")["reproduced"]
    assert previous["expired_auction"]["after_expiry_cleanup"] == 0
    assert previous["duplicate_login_balance"]["second_session_response"] - previous["duplicate_login_balance"]["actual_db_balance"] == 100
    inputs = [
        "Client/WarOfDimension/Scene.cpp",
        *[f"Server/Lobby_Server/{name}" for name in (
            "AccountOwners.h", "PasswordHash.h", "RewardJournal.h", "CClient.cpp", "CClient.h",
            "CDataBase.cpp", "CDataBase.h", "CDataBaseThread.cpp", "CDataBaseThread.h",
            "CNetworkMgr.cpp", "CPacketMgr.cpp", "CServer.cpp", "CUserMgr.cpp", "CUserMgr.h",
            "LobbyDispatcher.h", "Lobby_Server.vcxproj", "Lobby_Server.vcxproj.filters")],
        "Server/ServerCore/include/ServerCore/OrderedWorkQueue.h",
        "Server/ServerCore/tests/DatabaseQueueTests.cpp", "database/local-schema.sql",
        "scripts/Initialize-LocalDatabase.ps1", "scripts/Stop-Local.ps1",
        "scripts/Test-DatabaseConsistency.ps1", "scripts/Build-DatabaseConsistencyReport.py",
        "tools/server_lab/tests/database.py", "tools/server_lab/tests/database_consistency.py",
    ]
    sources = {path: digest(ROOT / path) for path in sorted(inputs)}
    binaries = {}
    for config in configurations:
        for module, name in (("LobbyServer", "Lobby_Server"), ("GameServer", "Game_Server"), ("Client", "WarOfDimension"), ("ServerCore.Tests", "ServerCore.Tests")):
            path = ROOT / f"artifacts/bin/{config}/{module}/{name}.exe"
            binaries[path.relative_to(ROOT).as_posix()] = digest(path)
    evidence = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "branch": subprocess.check_output(["git", "branch", "--show-current"], cwd=ROOT, text=True).strip(),
        "environment": {"os": platform.platform(), "build": "MSVC v145 x64, Windows C++20", "database": "isolated LocalDB, ODBC Driver 17, local Windows authentication"},
        "configurations": configurations,
        "network_release": network,
        "before_reproduction": previous,
        "sources_sha256": sources,
        "binaries_sha256": binaries,
        "scope": "Local correctness tests, real TCP/ODBC, restart simulation; not production, blockchain, GUI gameplay, maximum throughput, power-loss or source-to-database exactly-once proof",
    }
    target = OUT / "evidence/database-consistency-20261029.json"
    target.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    plt.rcParams.update({"font.family": "Malgun Gothic", "font.size": 11, "axes.spines.top": False, "axes.spines.right": False, "svg.fonttype": "none"})
    fig, axes = plt.subplots(1, 2, figsize=(10, 3.2))
    for axis, values, title, ylabel, top in (
        (axes[0], [0, 1], "만료 정산 후 보유 아이템", "개 (높을수록 보존)", 1.3),
        (axes[1], [100, 0], "구매 응답과 DB 잔액의 차이", "Token (낮을수록 일치)", 125),
    ):
        axis.bar(["수정 전 재현", "수정 후 검증"], values, color=["#da654f", "#17846e"], width=.52)
        axis.set(title=title, ylabel=ylabel, ylim=(0, top))
        axis.set_yticks([0, 1] if top < 2 else [0, 50, 100])
        for i, value in enumerate(values):
            axis.text(i, value + top * .035, str(value), ha="center", fontweight="bold")
    fig.tight_layout(pad=2)
    comparison = svg(fig)
    fig, axis = plt.subplots(figsize=(10, 2.7))
    axis.plot([1, 2, 3], [0, 1, 1], "o-", color="#17846e", linewidth=3)
    axis.set(xticks=[1, 2, 3], xticklabels=["SQL 실패 → 파일 보존", "재시작 → 보상 적용", "확정 기록 재생 → 중복 방지"], yticks=[0, 1, 2], ylim=(0, 2.2), ylabel="동일 보상 적용 횟수")
    for x, y in zip([1, 2, 3], [0, 1, 1]):
        axis.text(x, y + .12, str(y), ha="center", fontweight="bold")
    axis.grid(axis="y", alpha=.18)
    recovery = svg(fig)
    reward_ms = {config: next(case["local_end_to_end_ms"] for case in item["consistency"]["cases"] if case["case"] == "reward persisted and published") for config, item in configurations.items()}
    html = f'''<!doctype html>
<html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DB 정합성 수정과 복구 검증</title>
<style>
:root{{--ink:#152b3d;--muted:#4d6170;--paper:#f3f5f4;--line:#d6dfdd;--green:#08745f}}*{{box-sizing:border-box}}body{{margin:0;background:var(--paper);color:var(--ink);font:16px/1.65 'Malgun Gothic',sans-serif}}main{{max-width:1140px;margin:auto;padding:48px 30px 70px}}h1{{font-size:38px;line-height:1.2;margin:10px 0 20px}}h2{{font-size:25px;margin:0 0 16px}}h3{{font-size:19px;margin:0 0 8px}}p{{margin:8px 0 16px}}a{{color:#006856}}small,.muted{{color:var(--muted)}}.eyebrow{{font-size:13px;letter-spacing:.1em;color:var(--green);font-weight:bold}}.lead{{font-size:20px}}section{{margin:36px 0}}.cards,.grid{{display:grid;grid-template-columns:repeat(3,1fr);gap:16px}}.card,figure,.panel{{background:white;border:1px solid var(--line);border-radius:14px;padding:22px}}.value{{display:block;font-size:32px;font-weight:bold;line-height:1.3;color:var(--green)}}figure{{margin:18px 0}}figure svg{{display:block;width:100%;height:auto}}figcaption{{font-size:14px;color:var(--muted);margin-top:8px}}.flow{{padding:20px;background:#123744;color:white;border-radius:14px;font-size:18px;line-height:2}}table{{width:100%;border-collapse:collapse;background:white;font-size:15px}}th,td{{text-align:left;vertical-align:top;border-bottom:1px solid var(--line);padding:12px;overflow-wrap:anywhere}}th{{background:#e6eeeb}}code{{font-size:14px;overflow-wrap:anywhere}}pre{{white-space:pre-wrap;overflow-wrap:anywhere;background:#123744;color:white;padding:18px;border-radius:12px;font-size:14px}}.note{{border-left:4px solid #d3a342;padding:10px 18px;background:#fff8e8}}@media(max-width:700px){{main{{padding:28px 16px}}h1{{font-size:29px}}.cards,.grid{{grid-template-columns:1fr}}th,td{{padding:9px;font-size:13px}}.card{{padding:18px}}}}
</style><main>
<div class="eyebrow">WAR OF DIMENSION · 2026.10.29 · LOCAL DB</div>
<h1>데이터 손실을 막고<br>실패한 보상을 복구한다</h1>
<p class="lead">경매 만료·중복 로그인·잔액 적용·입력 적체·종료·보상 복구·비밀번호 저장 결함을 수정하고, 두 빌드 구성의 실제 TCP→ODBC 경로로 검증했다.</p>
<div class="cards"><div class="card"><span class="value">0 → 1개</span>만료 경매 아이템 보존<br><small>2개 정산 요청 동시 실행 × 5회/구성</small></div><div class="card"><span class="value">100 → 0 Token</span>구매 응답과 DB의 차이<br><small>수정 전 재현과 수정 후 기능 시나리오</small></div><div class="card"><span class="value">26 / 26</span>추가 정합성 검사 통과<br><small>Debug·Release 각각, 기존 DB 25개 별도</small></div></div>
<p class="note">보상 기록을 디스크에 확정한 이후의 재처리를 보호한다. 수신 원본의 고유 ID·ACK가 없는 기존 프로토콜, 디스크 확정 전 강제 종료, 운영 DB·블록체인·GUI 조작은 이번 보장 범위에 포함되지 않는다. PBKDF2와 기록 flush의 추가 비용은 존재하며 최대 처리량은 미측정이다.</p>
<section id="causes"><h2>분석한 원인</h2><div class="grid">
<article class="panel"><h3>삭제와 반환이 분리됨</h3><p>등록 때 재고를 차감한 뒤, 만료 조회에서 경매 행만 삭제했다. 아이템은 어느 소유자에게도 돌아가지 않았다.</p><code>RegisterAuction: 재고 −1<br>GetAuctionInfo: DELETE 만료 행</code></article>
<article class="panel"><h3>메모리 잔액을 다시 계산</h3><p>같은 계정의 여러 연결이 서로 다른 캐시를 가지고 구매 차액을 적용했다. 외부 갱신·보상과 비동기 완료가 교차하면 최신 잔액도 덮일 수 있었다.</p><code>cachedToken − price</code></article>
<article class="panel"><h3>실패·정체의 보존 경계 부재</h3><p>보상은 메모리에만 있었고 paused 세션 입력에는 상한이 없었다. 종료 도구가 15초 후 서버를 강제 종료할 수 있었으며 비밀번호는 평문이었다.</p></article></div></section>
<section id="solution"><h2>해결 방식</h2><div class="flow">세션 예약 → 입력 상한 → SQL 전용 FIFO → DB 확정 잔액·rowversion → 세션 완료<br>보상 수신 → 전용 기록 worker → flush·작업 ID → SQL 원장 트랜잭션 → 기록 삭제</div>
<p>만료 삭제와 재고 반환은 한 트랜잭션으로 묶었다. 계정당 소유 연결을 하나로 제한하고 <code>BalanceVersion</code>이 오래된 결과는 적용하지 않는다. 세션 입력은 256건·65,536 bytes로 제한하며 완료와 cleanup은 별도로 통과한다.</p>
<p>기록 worker가 디스크 I/O를 담당하므로 로비 잠금에서 파일 flush를 기다리지 않는다. 종료는 기록 worker join → DB drain 순서다. SQL 실패 기록은 다음 시작에 재생하고 <code>reward_applied.Operation_ID</code>로 같은 기록의 재적용을 막는다. 비밀번호는 salt 16 bytes·hash 32 bytes의 PBKDF2-HMAC-SHA256 600,000회로 저장하며 계산은 DB worker에서 실행한다.</p>
<pre>// 현재 보상 처리의 핵심 순서 (설명용 축약)
record = journal.Append(account, delta); // write + flush + promote
database.ApplyReward(record);           // ledger + token transaction
balance = database.ReadBalance(account);
journal.Complete(record);               // 확정 이후만 파일 제거</pre>
<p><a href="../diagrams/database-consistency/implementation.html">구조도</a> · <a href="../diagrams/database-consistency/README.md">선택 이유·경합·비용</a></p></section>
<section id="measurements"><h2>실제 측정한 내용</h2>
<p>Windows x64 / MSVC v145 / LocalDB / ODBC Driver 17. 격리 DB와 합성 계정만 사용했다. 아래 전후 그래프는 결함 재현과 수정 후 기능 검증이며 처리량 벤치마크가 아니다.</p>
<figure>{comparison}<figcaption>수정 전: 경매 1개 등록·만료 뒤 보유 0개, 중복 로그인 연속 구매 응답1900/DB1800. 수정 후: 만료 반환 1개, 단일 소유·DB 잔액 적용. 수정 후 잔액 검사는 외부 갱신2100 뒤 구매100으로 응답/DB2000을 대조했다. 입력 시나리오가 달라 성능 전후 비교로 해석하지 않는다.</figcaption></figure>
<figure>{recovery}<figcaption>각 구성에서 독립 서버 3회 실행. 없는 계정에 대한 SQL 실패 기록을 보존하고, 계정 생성 뒤 재시작하여 23 Token을 지급했다. 확정된 기록을 복원해 재시작해도 원장1건·잔액2023이 유지됐다. 실제 정전 실험은 아니다.</figcaption></figure>
<table><thead><tr><th>검사</th><th>Debug</th><th>Release</th><th>판정 범위</th></tr></thead><tbody>
<tr><td>Core / ABI</td><td>1694 PASS</td><td>1694 PASS</td><td>110 types / 106 constants 동일</td></tr>
<tr><td>기존 DB TCP 통합</td><td>25/25</td><td>25/25</td><td>로그인·구매·rollback·timeout·순서</td></tr>
<tr><td>추가 정합성</td><td>26/26</td><td>26/26</td><td>만료/구매 경합·보상 재시작·이전 rollback</td></tr>
<tr><td>설정 실패 / DB 없는 smoke</td><td>3/3 · 연결 PASS</td><td>3/3 · 연결 PASS</td><td>서버 모드, 클라이언트 창 미검사</td></tr>
<tr><td>서버·클라이언트 빌드</td><td>PASS</td><td>PASS</td><td>경매 최초 페이지 0기반 수정 포함, GUI 클릭 미검사</td></tr>
<tr><td>기존 네트워크 회귀</td><td>이번 미실행</td><td>7그룹 PASS</td><td>12개 네트워크·4인 전환2종·초기화 실패4종</td></tr>
</tbody></table>
<p>추가 검사에서 구매/만료 경합 4회/구성 모두 아이템1개·합계4000 Token을 유지했다. 400건 입력 flood는 상한 초과 후 연결을 닫고 소유권을 해제해 재접속했다. 1초로 줄인 종료 대기 한도에서도 살아 있는 프로세스와 실행 기록을 유지했고, SQL 잠금 해제 후 정상 drain·자원0을 확인했다.</p>
<p>보상17 Token의 수신부터 온라인 잔액 통지까지 관측값은 Debug {reward_ms['Debug']:.3f} ms, Release {reward_ms['Release']:.3f} ms이다. 각1회·루프백·낮은 부하의 관측이며 p99나 서비스 처리량을 뜻하지 않는다. 기본 PBKDF2 비용을 포함한 로그인 지연의 별도 전후 측정은 하지 않았다.</p>
<p>계정 비밀번호의 평문 이전과 잘못된 형식의 이전 실패 rollback을 검증했다. 새 스키마 v3·18개 프로시저 준비 및 반복 실행을 확인했다. 기존 전체 빌드 경고가 해소됐다고 주장하지 않는다. 새 검증의 모든 서버는 정상 종료했고 pending/sockets/leased=0이다.</p></section>
<section id="remaining"><h2>추가 개선할 부분</h2><table><thead><tr><th>항목</th><th>영향</th><th>상태·근거</th><th>다음 대응</th></tr></thead><tbody>
<tr><td>보상 원본 ID·ACK</td><td>디스크 확정 전 유실, 별도 수신 중복 식별 불가</td><td>기존 account/time/delta wire 유지</td><td>송신 outbox와 서버 인증, 고유 event ID·ACK 계약 추가</td></tr>
<tr><td>프로토타입 거래·서버 인증</td><td>실서비스 재화 신뢰 경계가 부족</td><td>CreateTransactionPacket은 시험용 랜덤 지급, 첫 게임 연결은 별도 상호 인증 없음</td><td>운영 모드에서 시험 지급 금지·검증된 거래 증명·서버 인증 설계</td></tr>
<tr><td>인증 전송·길이·시도 제한</td><td>저장 hash만으로 네트워크 인증 안전성 완성 불가</td><td>기존 TCP·password field 길이 유지, PBKDF2는 단일 DB worker 비용 추가</td><td>TLS/인증 프로토콜·길이 확장·rate limit·독립 인증 실행 경계 검토</td></tr>
<tr><td>계정 동일성과 다중 서버</td><td>Unicode collation·다른 프로세스 사이 소유권</td><td>로컬 ASCII 대소문자·후행 공백 검사만 통과</td><td>DB canonical account ID와 분산 lease·운영 collation 검사</td></tr>
<tr><td>SQL 처리량·검색 비용</td><td>단일 worker의 대기, OFFSET 경매·집계 비용</td><td>최대 부하·실데이터 실행 계획 미측정</td><td>큐 대기/SQL 시간 분리 계측 후 인덱스·cursor paging·계정 shard 검토</td></tr>
<tr><td>복구 운영·마이그레이션</td><td>손상 기록은 시작 차단, 원장 장기 증가, 비ASCII 평문 이전</td><td>자동 삭제·추측 변환 없음, 운영 DB 미적용</td><td>보존/백업/알림 정책·운영 schema signature·별도 이전 절차</td></tr>
</tbody></table></section>
<footer><p><a href="DATABASE_CONSISTENCY.md">재현 명령·검증 범위</a> · <a href="evidence/database-consistency-20261029.json">원시 수치·소스/바이너리 SHA-256</a> · <a href="DATABASE_IMPROVEMENT.html">초기 비동기 개선 보고서 (88d310a5)</a></p><small>저장된 로컬 검증 결과로 생성했다. 초기 snapshot은 main에 반영됐으며 이 보고서는 후속 수정 브랜치의 검증을 기록한다.</small></footer>
</main></html>'''
    (OUT / "DATABASE_CONSISTENCY.html").write_text("\n".join(line.rstrip() for line in html.splitlines()) + "\n", encoding="utf-8")
    print(json.dumps({"ok": True, "report": "docs/development/DATABASE_CONSISTENCY.html", "evidence": str(target.relative_to(ROOT)), "cases_per_configuration": {"baseline": 25, "consistency": 26, "core": 1694}}, ensure_ascii=False))


if __name__ == "__main__":
    main()
