# 시나리오 더미와 실시간 서버 관측

`tools/server_lab`은 실제 로컬 서버에 TCP로 연결하는 새 더미와 로컬 웹 대시보드다.
Python 3.12 이상과 표준 라이브러리만 사용한다. 기존 `Stress_Test`·`CDummyClient`와 독립적이다.
`LOCAL_TEST`·DB 없는 서버가 대상이며, 운영 인증·DB·블록체인을 검증하지 않는다.
[구현 구조도](../diagrams/server-lab/README.md), [작업·검증 기록](../../tasks/todo.md#61-시나리오-더미실시간-로컬-서버-관측-첫-구현)을 함께 참조한다.
성능 검사·병목 조사·개선 전후 비교를 수행하는 에이전트는 [성능 검사 지침](../guides/SERVER_PERFORMANCE.md)에 따라 필요한 계측과 시나리오를 보완한다.

## 실행과 중지

저장소 루트 PowerShell에서 실행한다. 스크립트는 PATH의 실제 Python을 우선 사용하고, 없으면 이미 설치된 Codex Python 런타임을 재사용한다.
Windows 설치 안내용 alias는 실행하지 않는다. `-Python`에 실제 실행 파일 경로를 지정해 명시적으로 선택할 수도 있다.

```powershell
./scripts/Start-ServerLab.ps1 -Configuration Release -Build
# http://127.0.0.1:8790 에서 시나리오 실행
./scripts/Stop-ServerLab.ps1
```

`Start-ServerLab.ps1`은 계측을 켠 두 서버와 monitor를 숨김 프로세스로 시작한다.
기존 로컬 서버/실행 기록이나 점유된 포트가 있으면 실패한다. `-Port`로 웹 포트를 바꿀 수 있다.
게임 TCP 8910·8911은 그대로 사용한다. `Stop-ServerLab.ps1`은 기록한 PID·실행 파일·시작 시각을 확인하고
monitor의 취소/종료 및 자신이 시작한 서버의 정상 종료를 요청한다. 서버가 먼저 비정상 종료했다면 실패를 보고한다.
다른 서버를 사용하려면 `-ExistingServers`를 지정한다. 이 경우 서버 계측은 사전에 켜야 하며 종료 스크립트가 해당 서버를 종료하지 않는다.

웹 없이 같은 runner를 실행할 수도 있다. 서버는 먼저 시작한다.

```powershell
python tools/server_lab/dummy_client.py --scenario tools/server_lab/scenarios/match-actions.json --clients 4 --seed 1
python tools/server_lab/dummy_client.py --scenario tools/server_lab/scenarios/lobby-cycle.json
```

CLI 종료 코드는 통과 `0`, 실패 `1`, 사용자 중지 `130`이다. Ctrl+C도 연결을 닫고 결과를 남긴다.
기본 결과는 Git 제외 `artifacts/logs/server-lab/runs/<run_id>.json`·`.csv`다.
CLI의 결과에는 서버 시계열이 포함되지 않는다. 웹 실행은 같은 runner에 monitor의 서버 시계열을 결합한다.
외부/GUI 클라이언트나 별도 CLI를 웹 실행과 동시에 접속시키면 매칭 참가자가 섞일 수 있으므로 검사 서버를 독점한다.

## 제공 시나리오와 판정

| 시나리오 | 동작과 판정 |
|---|---|
| `match-actions` | 3영웅·1보스 매칭 → 게임 서버 전환 → 직업 응답 4종 → 시작 전 선택 스킬 16개 → READY·로딩·초기 NPC → 이동·정지·기본 공격 응답 |
| `lobby-cycle` | 로비 로그인 → 연결 종료 → 같은 이름으로 재로그인. 더미별 행동을 순차 실행 |
| `lobby-movement` | 로비 병렬 이동과 정지 검사. 현재 큐의 응답 지연이 발생하면 실패 단계와 timeout을 기록 |
| `world-watch` | 웹 기본 선택. 게임 진입 → 이동·정지·기본 공격 → 15초 관측 → 반대 방향 이동·정지 → 15초 관측. 약 40초 동안 더미·NPC 표시 확인 |
| `timeline-watch` | 게임 진입 후 190초 관측. 미니언 예약/생성과 180초 자기장 게이트 해제를 확인하는 약 3분 20초 시나리오 |

이동은 자신의 방향 응답과 유한한 X/Z 위치 변화, 게임 정지는 방향 `0`을 검사한다.
로비 정지에는 명시적 ACK가 없어 지연 패킷 유예 후 0.4초 동안 자신의 이동 응답이 없음을 관찰한다.
이는 서버 내부 정지 상태의 직접 검증과 다르다. 기본 공격은 `SC_SKILL` 승인 응답까지만 검사하고 피해·명중·승패를 판정하지 않는다.
게임 로그인 응답의 `id=0`은 기존 ACK 관례이며, 실제 플레이어 ID는 로비의 매치 슬롯을 사용한다.
첫 구현은 게임 내 재접속을 지원하지 않는다.

단계마다 기대 응답과 timeout이 있고, 실패 시 나머지 행동을 취소하고 연결을 정리한다.
수신은 TCP 분할/합침을 처리하며 제어 응답은 4,096개, 이동은 객체별 최신 값만 보관한다.
이동·NPC 업데이트의 빈도가 높아도 이전 로그인·선택 응답을 밀어내지 않는다.
요청은 실제 `Shared/Protocol` wire를 사용하며 codec의 크기·packing·manifest 필드 배치를 ABI 기준과 대조한다.
서버가 사용하지 않는 `CS_MOVE.move_time`은 `0`으로 보낸다. 서로 다른 시계의 차이를 RTT로 계산하지 않는다.

## 시나리오 작성

웹 목록은 `tools/server_lab/scenarios/*.json`에서 읽는다. CLI는 `--scenario`로 임의 파일을 받을 수 있다.
형식 검사는 실행 전에 수행하며 알 수 없는 필드를 거부한다.

```json
{
  "schema_version": 1,
  "name": "보스 이동 후 기본 공격",
  "description": "준비된 4인 매치에서 보스 행동을 검사합니다.",
  "setup": "match",
  "clients": 4,
  "timeout_s": 15,
  "boss_job": 5,
  "execution": "parallel",
  "actions": [
    {"op": "move", "actors": "boss", "direction": 1, "duration_s": 1},
    {"op": "stop", "actors": "boss"},
    {"op": "skill", "actors": "boss", "slot": 1},
    {"op": "wait", "duration_s": 1}
  ]
}
```

`setup`은 `lobby|match`, `clients`는 1~32이며 match는 4의 배수다. 게임 매치 준비는 4명씩 순차 진행해 매칭 혼입을 막는다.
`execution`은 준비가 끝난 그룹의 행동 실행을 `parallel|sequential`로 지정한다. lobby의 그룹은 1명, match는 4명이다.
`actors`는 `all|heroes|boss`이며 lobby에서는 `all`만 지원한다. `seed`는 실행 화면/CLI에서 지정하며 더미별 난수 입력을 재현한다.
스레드 실행 순서나 서버의 시간 경과까지 결정적으로 재현하는 seed는 아니다.

지원 행동은 `move`, `stop`, `skill`, `wait`, `reconnect`다.
방향은 `1,2,4,5,6,8,9,10` 또는 `random`, 스킬 슬롯은 `1~5`, reconnect는 lobby 전용이다.
보스 직업은 `4|5`, timeout은 응답 대기당 0.1~60초, 행동 시간은 0.05~120초, 행동 수는 1~100개다.
화면에서 더미 수·seed를 바꿀 수 있고 한 monitor는 한 실행만 허용한다.

## 지형·더미·NPC 탭

화면을 `서버 지표`와 `지형 · 더미 · NPC` 탭으로 나눴다. 공통 시나리오 실행/중지와 서버 지표의 1초 갱신은 탭 전환 중에도 유지한다.
지형은 서버 `CServer::Initialize()`에서 읽는 다음 원본을 monitor가 직접 읽는다. 별도 패키지·외부 지도 서비스는 사용하지 않는다.

| 선택 | 높이 원본 | NavMesh 원본 | 위치 필터 |
|---|---|---|---|
| 로비 | `Server/Lobby_Server/Resource/HeightMesh2.obj` | `NavMeshData2.obj` | TCP 8910 |
| 게임 | `Server/Game_Server/Resource/HeightMesh.obj` | `NavMeshData3.obj` | TCP 8911 |

`자동 · 현재 더미`는 게임 서버에 접속한 더미가 있으면 게임, 그 외에는 로비를 선택한다. 직접 지형을 선택할 수도 있다.
월드 좌표에 별도 축척/오프셋을 적용하지 않는다. +X는 오른쪽, +Z는 위이며 가로·세로에 같은 화면 배율을 사용한다.
HeightMesh의 전체 삼각형에서 픽셀 중심 높이를 보간해 512×512 PNG로 만들고 가장 높은 표면을 높이색으로 표시한다.
빈 영역은 투명하며 수직면·다층 구조의 아래쪽 면과 클라이언트의 텍스처/건물 외형은 표현하지 않는다.
이 영상은 서버 충돌/높이 판정 자체의 검증 결과가 아니다. NavMesh의 실제 X/Z 삼각형은 청록선으로 겹쳐 표시하며 켜고 끌 수 있다.

휠 또는 +/− 버튼으로 확대하고 드래그로 이동한다. `전체 보기`로 중심/배율을 복원한다.
게임의 매치 그룹은 runner가 순서대로 준비한 더미 4명 묶음이며 서버 내부 match ID와 구분한다.
같은 지형 위에 여러 매치가 겹치므로 그룹별로 필터할 수 있다. 원은 더미이며 NPC는 종류별 기호·색상·약어와 범례로 구분한다. 각각 표시를 켜고 끌 수 있다.
게임 NPC는 미니언·레드 드래곤·그린 드래곤·골렘·곰·미노타우르·상자·비홀더 8종을 구분한다.
더미 표에는 ID·매치 슬롯 player ID·X/Y/Z·관측 동작·좌표 변화 속도(units/s)를 표시한다.
NPC 표에는 수신 종류·ID·그룹·X/Y/Z·관측 동작·HP·공격 수신 횟수를 표시한다. HP 응답 전에는 `—`다.
최근 15초 이동 궤적과 방향 화살표, 최근 1.2초 공격/스킬의 붉은 원, 제거 표시를 겹친다.
선택 목록 또는 표의 ID를 누르면 객체를 확대·추적한다. 드래그/전체 보기는 자동 추적을 해제한다.
연결 종료 뒤에는 더미의 마지막 위치를 회색으로 남기며 NPC는 종류 색상과 종료 표시를 유지한다. 실행이 바뀌면 새 실행만 표시하고 로비→게임 전환에서는 이전 맵의 더미 궤적을 비운다.

로비 서버에는 활성 NPC 생성·이동 broadcast가 없다. 실제 로비 NPC는 클라이언트 `Scene.h::m_npcPositions`의 고정 배치다.
monitor가 이 원본을 읽어 상점(N5)·경매(N6)·블록체인(N7)·꾸미기(N8)를 표시하며, `고정 배치 · 클라이언트`로 출처를 구분한다.
실시간 수신 좌표나 서버 NPC 상태로 취급하지 않으며 HP·공격 횟수는 `—`다. 웹 더미 접속 여부와 관계없이 로비 지도에서 볼 수 있다.

`GET /api/world`는 서버 지표와 분리한 수신 관측 snapshot이다. 지형 탭을 보는 동안 약 0.2초마다 요청하며 중복 요청을 겹치지 않는다.
위치는 수신값 사이를 약 0.2초 보간해 그린다. 미래 위치는 예측하지 않으며 비활성 탭에서는 월드 요청/애니메이션을 중단한다.
`observation.py`가 매치별 연결된 게임 더미 중 가장 작은 ID를 대표 수신자로 선택해 같은 NPC broadcast를 중복 저장하지 않는다.
대표 연결이 닫히면 다음 연결로 전환한다. NPC 키는 `(그룹, NPC ID)`이고 다른 매치의 같은 NPC ID를 합치지 않는다.
객체당 궤적은 최소 100ms 간격·최근 120점, NPC는 그룹당 최대 128개, NPC 사건은 최근 100개(화면은 20개)다.
상한 초과 패킷 수를 화면에 표시한다. 실제 현재 게임의 NPC 슬롯은 매치당 21개다.

실시간 위치 대상은 웹 더미 자신의 위치와 해당 매치에 전달된 게임 NPC 응답이다. 로비 고정 배치를 별도로 겹쳐 표시하며 GUI/외부 CLI 참가자·전체 서버 월드는 포함하지 않는다.
이동/위치 유지·대기 응답·공격 수신·제거는 관측 표현이며 서버 FSM·타깃·명중·피해 판정이 아니다.
더미는 NPC 애니메이션 완료 ACK를 자동으로 보내지 않는다. 실제 클라이언트의 전투/애니메이션 진행까지 재현하는 시나리오는 후속 범위다.
NPC 종류는 ADD 응답을 유지한다. 서버의 일반 몬스터 ADD/MOVE/REMOVE가 실제 NPC 종류를 보내도록 수정했으며 wire 상수·배치·크기는 유지했다.
일반 몬스터는 근처 타깃이 없으면 이동/공격하지 않을 수 있다. 미니언은 타깃이 없어도 경로를 따라 건물로 이동해야 한다.
미니언 정지는 `NpcCsv` 변환 시 `MaxSpeed` 누락으로 최대 속도가 0이 되던 서버 오류였다. 기반 `tabledata::NpcInfo` 전체 복사로 수정했다.
수정 전 약 19초간 스폰 좌표 유지, 수정 후 첫 웨이브 4마리의 실제 좌표 변화·경로 궤적을 확인했다. 타깃 부재만으로 모든 NPC 정지를 정상이라고 판단하지 않는다.
타깃을 가까이 배치하는 전투 시나리오와 애니메이션 완료 ACK 검증은 후속 범위다.

`GET /api/terrain?map=lobby|game`은 고정 원본만 허용하고 높이 PNG·좌표 범위·NavMesh·원본 경로/개수/SHA-256을 제공한다. 로비 응답에는 고정 NPC 좌표·종류·출처 SHA-256도 포함한다.
첫 요청 때 생성하고 monitor 프로세스에서 맵당 한 번 보관한다. 매초 전송하는 `/api/state`에 지형을 넣지 않는다.
브라우저도 디코딩한 영상을 재사용한다. `다시 불러오기`는 API를 다시 요청하며 원본 에셋 변경을 반영하려면 monitor를 재시작한다.
에셋 누락/LFS 미수신/잘못된 좌표·면·로비 배치 개수 불일치는 오류로 표시한다. 에셋 원본은 수정하지 않았다.

## 게임 일정과 남은 시간

게임 지도 위에 매치별 게임 경과 시간, 자기장 게이트 해제까지 남은 시간, 다음 미니언 웨이브 검사, 예약된 개별 스폰을 표시한다.
현재 CSV의 미니언 웨이브 검사 간격은 38초다. 비활성 슬롯을 최대 4개 골라 2·4·6·8초 뒤 활성화를 예약하며 매치의 미니언 상한은 12마리다.
첫 검사는 게임 시작 직후 발생한다. 웨이브 검사는 빈 슬롯을 확인하는 시각으로, 상한에 도달하면 새 스폰 예약이 없을 수 있다.
게이트의 기준은 `CGameMgr::m_fenceReleaseSeconds`(현재 게임 시간 180초)다. 화면의 카운트다운이 0이 되어도 서버의 fence=false 수신 전에는 `해제됨`으로 표시하지 않는다.

계측 활성화 시 기존 `CMatch` 게임 업데이트 실행 경로(IOCP worker)에서 약 1초마다 일정 값을 복사한다.
실제 예약에 사용한 시각, 게임 경과·게이트 상태와 활성 미니언 수를 `MatchTelemetry`의 잠금 보호 저장소에 발행한다. 예약 슬롯/수집 주기는 atomic으로 관리한다.
별도 reporter는 복사본만 읽어 직렬화·파일 교체한다. reporter가 가변 CMatch/CGameMgr 상태를 직접 읽거나 게임 worker가 파일/HTTP I/O를 수행하지 않는다.
기존 매치 업데이트·타이머의 실행 구조를 단일 스레드로 변경하지 않았으며 기존 동시성 문제 전체를 해결했다고 간주하지 않는다.

monitor는 서버 내부 match ID를 runner 그룹 번호로 간주하지 않는다. 로그인 이름 byte 4개의 정확한 집합으로 현재 실행의 그룹과 연결한다.
식별용 값은 브라우저/결과에 전달하지 않으며 일정은 `/api/world`와 종료 world에만 포함한다. 지표 시계열에 같은 일정을 중복 저장하지 않는다.
최근 서버 표본 사이의 시간만 보간하며 3.5초 이상 수집 지연, 연결 해제, 실행/매치/서버 종료 뒤에는 마지막 표본에서 고정한다.
서버 재시작은 instance로 구분한다. 게임 종료 시각의 전체 이벤트 기록이나 월드 리플레이를 제공하는 기능은 아니다.

## 관측 지표와 저장

두 서버의 `CServer::Run()`은 `WOD_METRICS_DIRECTORY`가 있을 때만 `TelemetryReporter`를 활성화한다.
별도 스레드가 약 1초마다 프로세스 자원·Core 네트워크·JobQueue 누적 카운터를 읽어
`<directory>/LobbyServer.json`·`GameServer.json`을 임시 파일에서 원자적으로 교체한다.
일반 `Start-Local.ps1` 실행은 환경 변수를 지정하지 않으면 계측을 켜지 않는다.
수집 실패는 한 번 로그에 남기고 게임 worker를 종료시키지 않는다. reporter는 worker join 뒤, Core 자원 해제 전에 종료한다.

| 지표 | 의미 |
|---|---|
| CPU | 프로세스 user+kernel 시간 증가량 / 경과 시간 / 전체 논리 코어 수. 한 코어 사용 시 전체 CPU의 일부로 표시 |
| Private bytes / working set | Windows 프로세스별 private commit / 현재 물리 메모리 상주량. 그래프는 private bytes |
| 송수신량 | 성공한 Core I/O 완료 bytes의 구간 증가량. 화면 그래프는 Rx+Tx KiB/s |
| 소켓·pending I/O·풀 대여 | listener·사전 생성 소켓·서버 간 연결을 포함한 Core 자원 수. 접속자 수와 다름 |
| I/O 오류 | Core가 기록한 오류 누적 수. 접속 종료 이후 전송 실패도 포함하며 게임 기능 실패 횟수와 다름 |
| queued / running | 계측 시작 이후 submitted-started-cleared / 현재 실행 callback 수. 개별 atomic을 읽는 근사 순간값 |
| jobs/s·평균/최대 실행 시간 | 완료 작업의 구간 처리량 / 구간 실행 시간 합÷실행 수 / 프로세스 계측 이후 최대값. 실행 시간은 µs 단위로 절삭 |
| collect_us | snapshot 필드 수집·직렬화 시간. 파일 교체 I/O 시간은 제외 |
| 단계 p50/p95/p99 | 최근 최대 10,000개 성공 단계 완료 시간. 대기·준비 단계 포함, 네트워크 RTT와 다름 |

작업 큐의 allocation·우선순위·budget·실행 잠금은 유지했다. 활성화 시 atomic/시계 호출 비용이 추가되고,
`IocpService::Stats()`는 네트워크 mutex 안에서 소켓을 순회한다. 계측 오버헤드가 0이라고 가정하지 않는다.
한 번 실행하는 중간에 계측을 켜거나 끄지 않는다. enqueue 대기 시간·대상별 공정성·tick 지연 histogram은 아직 없다.

monitor와 브라우저도 약 1초마다 갱신한다. 화면은 최근 120개 표본, 내부 버퍼는 600개다.
실행 결과에는 최대 최근 3,600개 실행 중 서버 표본, 전체 assertion, 입력 시나리오·seed·ABI SHA-256을 저장한다.
1시간을 초과한 실행은 앞선 서버 표본이 보존되지 않을 수 있다. JSON은 시계열과 종료 시 제한된 `world` 관측(더미/NPC/궤적/사건)을 포함하며 CSV는 단계 assertion 표다.
전체 시간의 월드 snapshot이나 리플레이를 저장하지 않는다. 과거 JSON 다운로드는 가능하지만 과거 월드를 화면으로 재생하는 기능은 없다.
최근 20개 결과를 선택해 다운로드할 수 있으며 화면 타임라인은 최근 100개 사건이다.
누적 결과 파일의 자동 삭제는 없으므로 장기간 사용 시 `artifacts`의 용량을 관리한다.

표본이 3.5초 이상 오래되면 수집 지연, 파일이 없으면 계측 없음, schema 오류면 잘못된 계측, 정상 최종 표본이면 관측 종료로 표시한다.
`stopped`는 reporter 종료 표시이며 서버 종료 코드나 Core 자원 해제 성공 판정과 구분한다. 최종 자원 회수는 서버 종료 로그에서 확인한다.
누락 구간의 그래프를 `0`으로 채우지 않는다. 재시작으로 instance가 바뀌면 이전 누적값과 차분하지 않는다.
월드 화면은 수신한 더미/NPC의 X/Z 평면 관측이며 서버 내부 전체 상태와 3D 모델 애니메이션은 포함하지 않는다.
HTTP는 `127.0.0.1`에만 bind하며 localhost Host/Origin을 검사하고 외부 CORS를 열지 않는다.

## 에이전트의 API 접근

같은 PC에서 실행되는 에이전트는 브라우저 없이 HTTP JSON으로 화면의 원본 관측값을 읽을 수 있다.
Server Lab이 실행 중이어야 하며 기본 주소는 `http://127.0.0.1:8790`이다. 다른 PC·원격 실행 환경의 localhost와는 공유되지 않는다.

| 요청 | 확인 가능한 값 |
|---|---|
| `GET /api/state` | 서버 최신 지표·신선도·최근 120개 시계열, 현재 실행·단계별 판정·연결 수, 최근 결과 ID |
| `GET /api/world` | 현재 실행의 더미·NPC 좌표/방향·궤적·HP/관측 동작·사건·매치별 일정과 관측 상태 |
| `GET /api/terrain?map=lobby` 또는 `map=game` | 지형 높이 PNG·NavMesh·좌표 범위·원본 SHA-256, 로비 고정 NPC |
| `GET /api/scenarios` | 등록된 시나리오·행동·기본 실행 조건 |
| `GET /api/result?id=<run_id>&format=json` 또는 `format=csv` | 저장된 실행 결과. JSON에는 assertion·서버 시계열·종료 world, CSV에는 assertion |

```powershell
$state = Invoke-RestMethod http://127.0.0.1:8790/api/state
$world = Invoke-RestMethod http://127.0.0.1:8790/api/world
$state.servers | ConvertTo-Json -Depth 10
$world.npcs | Select-Object group, id, npc_type, x, y, z, action
$world.timelines | ConvertTo-Json -Depth 10
```

실행을 요청받은 에이전트는 `POST /api/run`에 `{"scenario":"world-watch","clients":4,"seed":1}`을 보내 시작하고,
`POST /api/stop`에 `{}`를 보내 취소할 수 있다. 실행 중 다시 시작하면 `409`이며 상태는 `/api/state`에서 확인한다.
API 값의 범위와 보존 상한은 화면과 같다. 전체 서버 월드·서버 FSM/타깃·명중 판정·전체 시간의 월드 리플레이는 제공하지 않는다.
로비 NPC는 클라이언트 고정 배치이며, 종료·오래된 표본을 현재 실시간 상태로 간주하지 않는다.

## 재현 가능한 검사와 알려진 문제

```powershell
./scripts/Test-ServerLab.ps1 -Configuration Debug
./scripts/Test-ServerLab.ps1 -Configuration Release
./scripts/Test-ServerCore.ps1 -Configuration Debug
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Debug -Python python
# Server Lab이 실행 중일 때 장시간 일정 검사(약 3분 20초)
python tools/server_lab/tests/integration.py --timeline --output artifacts/logs/server-lab/npc-timeline-integration-Release.json
# 첫 웨이브 미니언 4마리 이동까지 검사
python tools/server_lab/tests/integration.py --minions --output artifacts/logs/server-lab/minion-integration-Release.json
```

Server Lab 검사는 서버를 새로 시작해 Python 단위 검사, 로비 4인 재접속, 게임 4/8인 행동, 중복 실행 거부,
실행 중 취소·연결 0, 취소 후 재실행, JSON/CSV 다운로드·서버 시계열·정상 종료 표본·Core 잔여 자원 0을 확인한다.
계측 출력 경로에 파일을 두는 I/O 실패 주입에서도 로그인·재접속과 정상 종료가 가능하며 오류는 한 번만 기록되는지 검사한다.
로비 이동 진단은 통과를 가정하는 이 회귀 묶음에 넣지 않는다.
지형 검사는 OBJ 인덱스/잘못된 파일·높이 보간/겹친 면/+Z 방향·PNG alpha/CRC·실제 에셋 개수·캐시·HTTP 맵 제한/누락 오류를 포함한다.
실제 monitor API에서도 로비/게임 지형과 반복 요청의 동일성을 검사한다. 탭·위치 겹침·확대/필터·키보드 전환은 브라우저에서 확인한다.
월드 단위 검사는 NPC codec/ABI 필드, 매치 분리·중복/대표 전환, 이동/공격/HP/제거/재생성, 저장 상한·snapshot 분리,
잘못된 길이/비유한 좌표, 로비→게임 초기화, TCP 분할과 NPC 업데이트 5,000개 사이의 제어 응답 보존을 포함한다.
실제 서버 검사는 수신 위치·NPC·그룹 분리·종료 후 대표 해제·더미 궤적·저장 JSON을 확인한다. NPC 이동/공격 발생 여부는 별도 관측이며 통과를 가정하지 않는다.
일정 검사는 정확한 그룹 연결·오래된/종료 관측 고정·잘못된 표본 거부·로비 원본 좌표 검증을 포함한다.
선택적인 `--timeline` 통합 검사는 실제 예약·8마리 이상 활성화·180초 이후 서버의 게이트 해제·종료 타이머 고정을 검사한다.
`--minions`는 `world-watch`에서 첫 웨이브 N0~N3의 유한한 좌표와 스폰 이후 4 units 초과 이동, 종료/저장 상태를 검사한다.
일반 몬스터의 이동을 통과 조건으로 강제하지 않으며 전체 전투/애니메이션 ACK 검증을 대체하지 않는다.

이번 검증에서 기존 로비의 IOCP 직접 reset과 content worker reset이 경합해 중복 정리·배열 assertion을 일으켰다.
IOCP는 정리 작업을 예약하고 content worker 한 곳에서 정리하도록 수정했으며,
Core `JobTarget`은 재등록되더라도 재사용 초기화 전까지 정리 callback을 한 번만 수행한다.
단위 검사와 실제 반복 접속/취소 검증을 추가했다.

로비 이동 중 반복 `ProcessUpdate`가 우선순위 큐에 즉시 재등록되어 다른 대상/정지 요청이 timeout되는 현상도 관찰했다.
게임 참가자 연결 종료 뒤에도 기존 매치 업데이트의 전송 시도로 I/O 오류가 증가했다.
도구는 이를 성공으로 바꾸지 않고 지연·오류와 실패 단계를 표시한다. 원인별 개선은 [후속 작업](../../tasks/todo.md)에 남겼다.
현재 계측 결과를 JobQueue 개선 전 기준 벤치마크나 개선 후 비교 결과로 사용했다고 주장하지 않는다.
성능 비교 전 동일 빌드·입력·계측 조건·워밍업·반복 횟수·소스 hash와 오버헤드를 확정해야 한다.
장시간·최대 32인 부하, 운영 기능, 전체 전투 및 실제 GUI 게임 플레이는 검증 범위 밖이다.
