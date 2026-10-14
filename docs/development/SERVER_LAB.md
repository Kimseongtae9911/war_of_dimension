# 시나리오 더미와 실시간 서버 관측

`tools/server_lab`은 실제 로컬 서버에 TCP로 연결하는 새 더미와 로컬 웹 대시보드다.
Python 3.12 이상과 표준 라이브러리만 사용한다. 기존 `Stress_Test`·`CDummyClient`와 독립적이다.
`LOCAL_TEST`·DB 없는 서버가 대상이며, 운영 인증·DB·블록체인을 검증하지 않는다.
[구현 구조도](../diagrams/server-lab/README.md), [작업·검증 기록](../../tasks/todo.md#61-시나리오-더미실시간-로컬-서버-관측-첫-구현)을 함께 참조한다.
성능 검사·병목 조사·개선 전후 비교를 수행하는 에이전트는 [성능 검사 지침](../guides/SERVER_PERFORMANCE.md)에 따라 필요한 계측과 시나리오를 보완한다.
Core 큐 자체의 반복 측정은 [JobQueue 기준 측정 도구/실측](JOBQUEUE_BASELINE.md)을 사용하며 웹 더미의 서버 전체 부하와 구분한다.
실제 서버의 반복 성능 측정·입장 제한 설정·새 대기/tick 지표는 [서버 기준 측정](SERVER_BASELINE.md)을 따른다.

## 게임 tick 구간별 비용 분석

`WOD_TICK_PROFILE=1`로 게임 서버를 시작하면 기존 tick의 실행 경로에서 선택 활성화 계측을 수행한다. 기본값은 꺼짐이다. 일반 metrics 수집도 켜야 API/결과에 저장된다.
게임 규칙·패킷·예약 간격·JobQueue 처리 정책은 바꾸지 않는다. [계측 흐름도](../diagrams/game-tick-profile/README.md)를 참조한다.

```powershell
python tools/server_lab/tick_profile.py --output artifacts/logs/game-tick-profile-new --clients 4 16 64 --repeats 5
python tools/server_lab/tick_profile.py --output artifacts/logs/game-tick-profile-new --analyze
```

기존 `perf-match`를 실제 서버 재시작·입장 소프트 제한 해제·Release·계측 켬으로 실행한다. 각 인원에서 profile 켬을5회 측정하며, 최소 인원에서는 같은 seed의 profile 끔/켬 순서를 교차한다. 기본 총20회다. 성능 비교 전용 서버 포트를 사용하므로 기존 실행 기록이 있으면 시작하지 않는다. 실행 중 빌드·소스 변경·다른 부하 검사를 수행하지 않는다.
더미 행동·워밍업5초·측정20초·250ms 발행과 기능 검사/timeout은 기존 시나리오 그대로다. 실패와 정리 실패를 결과에 보존하며 임계값을 완화하지 않는다.

`GET /api/state`의 GameServer `tick_profile`과 저장 결과의 `samples[].servers[].tick_profile`에 누적값을 보존한다. 분석 결과는 `analysis.json`, 실제 실행별 결과는 `*/report.json`·`*/lab/runs/*.json`이다.
`schema_version=1`, `enabled`, `stages.<name>.{calls,wall_ns,cycles,cycle_samples,cycle_errors,work}`를 제공한다. 누락/꺼짐은 미계측이고 0비용으로 해석하지 않는다. 프로세스 재시작 시 초기화되며, 실행 중 플래그 변경은 지원하지 않는다.

| 구간 | 의미 |
|---|---|
| `total` | MatchUpdate handler의 profile frame. 기존 tick_duration은 frame 누적/회수 비용까지 포함 |
| `game_clock/jobs/player_update/player_send/minion_update/monster_update/periodic/observation` | 서로 겹치지 않는 큰 구간. 제외한 예약·순회·계측 등의 잔차는 별도 표시 |
| `player_move/map_collision/nav_ray/height_ray/client_collision/npc_collision/astar` | 부모 구간 안의 세부 비용. 부모/자식 시간이나 cycle을 중복 합산하지 않음 |
| `height_ray_player/height_ray_minion/height_ray_monster` | HeightMesh 비용의 호출 주체별 부분집합. 전체 height_ray와 중복 합산하지 않음 |
| `send_lock/send_body/native_send` | tick에서 직접 Send한 잠금 획득 대기, 획득 후 처리, 배치 생성/native 발행. native_send는 send_body의 부분집합이며 비동기 완료 handler 비용은 제외 |

wall은 steady_clock 경과 시간으로 OS 스케줄링·대기를 포함한다. CPU cycle은 `QueryThreadCycleTime`의 현재 worker 값 차분이며 시간으로 변환하지 않는다([공식 계약](https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-querythreadcycletime)). `cycle_samples/errors`로 수집 성공/실패를 구분한다. cycle 비중의 분모는 계측한 tick의 cycle이며 전체 프로세스 CPU 비중이 아니다. 프로세스 CPU는 기존 GetProcessTimes 지표로 따로 비교한다.
Ray의 `work`는 반환까지 방문한 삼각형/노드 수이며 높이 필터로 건너뛴 항목도 포함한다. 동적 메모리/파일 I/O 없이 frame 로컬에 모으고 종료 때 고정 수의 atomic 누적값에 반영한다. reporter 동시 snapshot은 근사값이며 누적 감소·서버 재시작·stale/형식 오류는 분석 오류로 보존한다. 결과는 measure→verify 내부 reporter 표본의 차분이다.
최소 인원 계측 켬/끔 대조는 해당 부하의 비용만 검증한다. NPC 난수·매치 나이·스폰 수·외부 시스템 부하는 완전히 고정되지 않으며, 다중 매치 결과를 순수 확장 비율이나 최대 수용량으로 해석하지 않는다.

## 실행과 중지

### 로비 단일 채널 이동 프로파일

로비도 시작 전 `WOD_TICK_PROFILE=1`로 상세 프로파일을 켤 수 있다. `GET /api/state`와 저장 결과 `samples[].servers[]`의 LobbyServer `tick_profile`을 조회한다. 1ms 간격 조건을 통과해 실제로 실행된 `CClient::Move`가 frame(`total`)이며, 패킷 실행·IOCP·타이머 전체 비용은 포함하지 않는다.

`lobby_nav`·`lobby_height`는 각 지형 ray 호출의 wall/cycle, `lobby_view`는 주변 유저 탐색과 주변 유저에 대한 전송 요청의 wall/cycle이다. `map_collision`은 두 지형 검사의 부모 구간이다. 기존 `height_ray`는 Triangle 벡터 RayCast 공통 내부 계측으로 로비에서는 NavMesh 호출까지 포함하므로, 이를 순수 HeightMesh 비용으로 해석하거나 부모와 합산하지 않는다. Ray의 `work`는 실제 방문한 삼각형 수다.

상세 계측을 끈 성능 비교와 켠 원인 분석을 구분한다. 원시 누적값은 reporter의 근사 snapshot이며 동일 instance·서로 다른 sequence·측정 구간 내부의 차분을 사용한다. 시작/로그인 시 첫 Move 간격은 기존 초기화 문제의 영향을 받을 수 있어 기준 측정 구간에 포함하지 않는다.

로비 지형 개선의 [HTML 보고서](LOBBY_THROUGHPUT.html)와 [실측·검증 근거](LOBBY_THROUGHPUT.md)를 참조한다.

로비 인덱스의 준비 여부·면 수·셀 수·참조 수·전역 후보 수·인덱스/정적 캐시 bytes는 LobbyServer `content.terrain_index`에 내보낸다. monitor API와 저장 샘플에서는 현재 `admission.terrain_index` 경로로 조회할 수 있다. 시작 시 소유 스레드가 JSON을 만든 뒤 reporter에는 고정 문자열을 전달하므로 가변 월드 객체를 순회하지 않는다.

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
`-UncapAdmission`은 LOCAL_TEST 로비 채널 인원을 기존 저장 용량인 3,000명까지 확장한다. 기본 채널은 4명이며 게임의 매치 4인 규칙과 저장 범위 검사는 유지한다.
`-Directory`로 실행별 관측/결과 디렉터리를 분리할 수 있다. `-DisableMetrics`는 계측 끔 대조용이며 해당 실행의 작업/tick 계측은 누락으로 표시한다.

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
| `perf-lobby` | 전체 로그인 후 고정 주기의 이동/정지, 5초 워밍업·20초 측정·모든 더미의 위치 변화/최종 정지 검사 |
| `perf-match` | 모든 4인 매치 준비 후 같은 구간에서 이동/정지 부하. 기존 NPC/타이머 실행 유지 |

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

`setup`은 `lobby|match`, `clients`는 양의 정수이며 match는 4의 배수다. 도구의 32명 상한은 제거했으며 서버 저장 용량은 별도다. 게임 매치 준비는 4명씩 순차 진행해 매칭 혼입을 막는다.
`execution`은 준비가 끝난 그룹의 행동 실행을 `parallel|sequential`로 지정한다. lobby의 그룹은 1명, match는 4명이다.
`actors`는 `all|heroes|boss`이며 lobby에서는 `all`만 지원한다. `seed`는 실행 화면/CLI에서 지정하며 더미별 난수 입력을 재현한다.
스레드 실행 순서나 서버의 시간 경과까지 결정적으로 재현하는 seed는 아니다.

지원 행동은 `move`, `stop`, `skill`, `wait`, `reconnect`다.
방향은 `1,2,4,5,6,8,9,10` 또는 `random`, 스킬 슬롯은 `1~5`, reconnect는 lobby 전용이다.
보스 직업은 `4|5`, timeout은 응답 대기당 0.1~60초, 행동 시간은 0.05~120초, 행동 수는 1~100개다.
화면에서 더미 수·seed를 바꿀 수 있고 한 monitor는 한 실행만 허용한다.

성능 시나리오는 일반 `actions` 대신 `performance: {warmup_s, duration_s, interval_s}`를 사용한다.
각 범위는 0.1~120초·1~120초·0.02~10초다. 전체 준비 이후 고정 주기로 이동/정지를 발행하고, ACK 대기로 요청량을 줄이지 않는다.
결과의 `phases`·`generator_lag_ms`·더미별 `measure_moves/measure_distance`와 서버 구간 표본을 함께 본다.
웹의 전체 작업/패킷 작업 대기 p99·업데이트 지연·게임 tick 실행 시간·handler packets/s는 `/api/state`와 저장 결과에서도 조회할 수 있다.
`servers[].performance`에 누적 histogram과 미실행 패킷의 수/최장 대기가 있고 파생 p99는 1초 차분의 bucket 상한이다. 표본 없음은 null이다.
로비의 업데이트 지연은 기존 1ms 이동 계산 조건을 초과한 간격이며 고정 tick deadline과 다르다.
저장된 `samples`에는 observer CPU와 PID/생성 시각을 확인한 외부 서버 CPU 누적 표본도 포함한다.

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

### 로비 이동 예약 (2026-10-13)

로비는 유저별 타이머 예약/갱신 대기 하나를 유지하고, 실행 완료 후 15ms 뒤 다음 작업을 예약한다.
타이머는 기존 유저 FIFO에만 작업을 추가하며 단일 로직 worker와 패킷 순서를 유지한다.
`SC_MOVE_PLAYER`의 필드·크기는 유지한다. 본인에게 입력 처리 시 좌표/새 방향을 보내고, 정지 방향0도 보낸다.
주변 유저에게는 예약 갱신에서 최신 상태만 전달한다. 과부하에서 실제 갱신이 늦어지며 66.7Hz를 보장하지 않는다.
초기 입력부터 방향을 정규화한 목표 속도(현재 최대10 units/s)를 사용한다. 이전 갱신당 속도 누적과 감속은 로비에서 제거했다.
클라이언트 예측/보간 전체 개편·이동 입력 sequence ACK·timestamp 검증·연속 충돌 개선은 후속 범위다.

`GET /api/state`의 로비 `admission`에 `movement_interval_us=15000`, `movement_single_pending=true`가 추가된다.
로비의 기존 `tick_timer_lateness_us`는 예약 기한→타이머 dispatch, `tick_dispatch_wait_us`는 dispatch→generation이 유효한 로직 갱신 시작까지다.
모두 steady_clock microseconds 누적 histogram이고 기존 reporter→API→저장 결과 경로를 따른다.
`lobby_movement_gap_us`는 broadcast Move 직전의 적분 시각 차이며 입력 처리의 중간 적분 뒤에는 짧아질 수 있다.
`tick_delay_us`는 그 차이에서 현재15ms를 뺀 0 이상 값이다. 이전1ms 목표의 값과 직접 비교하지 않는다.
실제 송신/화면 갱신 간격·본인 정지 응답 시간과 구분한다. 상세 구현·측정은 [이동 보고서](LOBBY_MOVEMENT.md)를 참조한다.

실제 로컬 소켓 회귀:

```powershell
python tools/server_lab/tests/lobby_movement.py --output artifacts/logs/lobby-movement-functional.json
```

서버 시작/중지는 기존 PowerShell 스크립트가 소유한다. 검사는 분할 이동·본인/근처 유저의 최종 방향0·200개 입력 응답 순서·정지 뒤 갱신 중단·재시작·이동 중 재접속10회를 확인한다.
`--legacy`는 이전 실행 파일의 이동 거리/갱신 수 비교용이며 명시적 정지 응답 검사를 생략한다.

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

### 동접 계단 부하

`capacity-match`·`capacity-lobby`는 기존 연결과 이동 부하를 유지하면서 참가자를 추가한다. 게임은 항상 4인 매치다. 각 단계는 추가 로그인 → 3초 워밍업 → 20초 측정 → 전체 참가자 이동/위치 변화/정지 확인이며, 다음 단계의 준비 중에도 기존 참가자는 이동한다. `clients`는 탐색 상한이다. 단계 목록 사이의 값을 지정하면 마지막 단계로 추가한다. 계단 단계의 매치 나이·미니언 수가 서로 달라지므로 매회 재시작하는 `perf-match` 기준 검사와 같은 조건으로 비교하지 않는다.

```powershell
python tools/server_lab/capacity.py --scenario capacity-match --clients 512 --seed 1 --profile --output artifacts/logs/capacity-game-new
python tools/server_lab/capacity.py --scenario capacity-lobby --clients 512 --seed 1 --output artifacts/logs/capacity-lobby-new
# 관측기 포화 시 시각 월드를 끄고 플레이어 이동·정지를 계속 검증
python tools/server_lab/capacity.py --scenario capacity-match --clients 1000 --seed 3 --profile --headless --output artifacts/logs/capacity-game-headless-new
python scripts/Measure-CapacityBoundary.py --scenario perf-match --clients 16 20 24 --repeats 5 --profile --headless --output artifacts/logs/capacity-boundary-new
./scripts/Measure-ServerThreads.ps1 -Output artifacts/logs/thread-cpu-new.jsonl -Seconds 30
```

`GET /api/state`의 `run.capacity_stages`와 저장 결과 JSON은 요청/준비 완료/검증 성공 인원, 단계 상태·오류, 측정 시작/끝 epoch ms, 실제 발행 수, 발행 지연 p99/max, 관측기 전체 CPU(core %)를 보존한다. 실제 더미 종료 연결 수와 서버 정상 종료/잔여 자원도 별도 판정한다. 기존 `phases`의 이름이 반복되므로 여러 단계 실행을 기존 `baseline.summarize_run` 하나로 요약하지 않는다. `capacity.analyze_result`가 각 단계 시각으로 서버 누적 histogram을 차분한다. percentile은 기존과 같이 bucket 상한이다.

발행 지연은 예정 시각 대비 각 입력 주기 시작 시각의 차이다. 모든 소켓의 마지막 write/drain 완료 지연이나 네트워크 RTT를 뜻하지 않는다. 관측/더미 프로세스 CPU는 같은 프로세스 안의 수신·HTTP·표본 작업도 포함한다.

CLI는 `GET /api/progress`에서 busy·run ID/상태·phase·counters·capacity_stages만 읽어 대형 시계열/월드를 매초 직렬화하지 않는다. 상세 관측은 기존 `/api/state`·`/api/world`와 저장 결과에 유지한다. NPC 관측 대표는 해당 그룹의 4명만 조회하며, 매치 일정 연결도 첫 8그룹 제한 없이 실제 게임 참가자 그룹에 적용한다.

`--headless`는 `WOD_LAB_HEADLESS=1`을 자식 관측 프로세스에 전달한다. 시각 월드·NPC 좌표 의미 검사는 꺼지지만 모든 TCP 프레임 수신, 알려진 NPC/플레이어 관측 프레임 길이, 플레이어 유한 좌표·자신의 이동/위치 변화·정지 검증은 유지한다. API/저장 결과는 `world_observation=false`, world는 `enabled=false`로 표시한다. 서버의 NPC 로직은 계속 실행된다. 전체 관측 켬/끔 결과를 같은 조건으로 합치지 않는다. 수신 루프는32프레임마다 제어 코루틴에 실행 기회를 주며, 추가 로그인 중 발행 지연이1초를 넘으면 생성기 한계로 중단한다. 단계 종료의 CPU/지연 중단 기준도 유지한다.

추가 게임 계측 `performance.tick_timer_lateness_us`는 예정 epoch 시각부터 타이머가 IOCP Post를 호출하기 직전까지의 초과 시간(0 미만은0), `tick_dispatch_wait_us`는 Post 직전부터 MatchUpdate 시작까지의 단조 시계 경과 시간이다. 후자는 IOCP 내부 진입 잠금·큐 대기·스케줄링을 포함한다. 같은 context에 실어 성공적으로 실행된 MatchUpdate에서 각각 기록하며, pool Reset은 전달 시각/지연을 지운다. 기존 `tick_delay_us`(예정→handler, system_clock)와 새 두 구간은 시계/표본 순간 차이가 있어 bit 단위 합계 일치를 강제하지 않는다. 계측을 켠 프로세스에서만 추가 수집하며, 기존 schema1의 선택 histogram이고 구버전/미계측은 null로 남긴다. 화면·API·저장 JSON의 원시 performance에 제공한다.

기능 실패 또는 발행 p99가 입력 주기(250ms)를 초과하거나 관측기 CPU가 0.9코어 이상이면 증가를 중단한다. 후자는 서버 동접 한계가 아니라 부하 생성기 한계로 기록한다. 통과한 탐색 상한을 절대 최대 동접으로 보고하지 않는다. 목표 주기를 유지하는 인원과 낮은 갱신률에서도 기능만 통과한 인원도 구분한다.

게임의 `match_observation`은 서버가 발행한 복사본의 등록 개수·최근 3.5초 이내 미종료 매치 개수·그 매치의 활성 미니언 합계다. 실제 접속자 수를 소켓 수나 이 등록 개수로 대신하지 않는다. 오래된 복사본과 종료 매치는 `fresh_active`에서 빠지므로 서버 확정 연결 수와는 다르다. `Measure-ServerThreads.ps1`은 실행 기록의 PID/시작 시각을 확인하고 스레드별 누적 CPU ms를 JSONL로 저장한다. 스레드 이름/함수 주소는 수집하지 않으므로 높은 CPU만으로 함수의 신원을 단정하지 않는다.

`soak-lobby`는 새로 시작한 격리 로비의 5초 워밍업·120초 유지·최종 입력 정리를 검사한다. `performance.settle_s`는 lobby에서만 선택 가능한1~60초 제한이며 이 시나리오는20초를 사용한다. 다른 참가자가 없는 새 서버와 활성 계측·monitor가 필요하다. 마지막 stop 발행 이후 서버의 처리 패킷 수가 이 실행의 총 발행 수 이상이고, 패킷/송신 대기열이 모두0인 서로 다른 reporter 표본3개가 연속되어야 통과한다. 누락/stale/중복 표본은 성공으로 세지 않으며 재시작/sequence 감소는 실패한다. wire ACK 추가 없이 프로세스 합계를 확인하는 검사이므로 다른 클라이언트가 있는 서버에는 적용하지 않는다.

API/저장 결과의 `settlement`에 판정·경과ms·기대 입력 수·마지막 처리/대기 수·연속 빈 표본 수·오류를 보존한다. 정리 검사 실패 후에도 개별 이동/정지 검사를 남긴다. 기본20초 성능/누적 시나리오의 기존 정지 검사는 자기 이동의0.4초 수신 중단(게임은 direction0 응답)이므로 적체된 최종 명령 전체의 완료를 보장하지 않는다. `settlement`가 없는 과거 성공을 대기열 정리 성공으로 해석하지 않는다.

```powershell
python scripts/Measure-CapacityBoundary.py --scenario soak-lobby --clients 64 128 --repeats 1 --seed-start 3 --headless --output artifacts/logs/capacity-soak-new
```

동접 실측·경계 반복·계측 켬/끔 대조는 [동접 조사 보고서](SERVER_CAPACITY.md)를 참고한다. 120초 유지는 별도 진단이며 장시간 운영 안정성 검증은 아니다.

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
위 첫 구현의 검증은 기준 벤치마크가 아니다. 이후 2026-10-11에 계측·워밍업·반복·소스 hash를 고정한 [실제 서버 4/16/64명 기준 측정](SERVER_BASELINE.md)을 추가했다.
최적화 구현·개선 후 비교·장시간·최대 수용량 탐색·운영 기능·전체 전투 및 실제 GUI 게임 플레이는 검증 범위 밖이다.

## 전송 적체 진단 관측

서버 snapshot의 선택 `send_queue` 필드는 active/대기 context 합계(`count`)·한 연결 최대(`max_socket_count`)와 프로세스 누적 native 발행(`native_calls`)·배치(`batches`)·포함 프레임(`frames`)을 제공한다.
native_calls는 부분 전송 재발행/즉시 실패를 포함하며 frames는 성공 수신 수가 아니다. 개별 완료 통지만 남은 context는 count에서 제외된다.
기존 1초 수집·재시작 초기화를 따르며 API·저장 결과가 보존한다. 화면에도 전송 대기와 연결별 최대를 표시한다.
Peer 수신 오류는 port/phase/시각/closing과 함께 남는다. 정상 EOF/종료와 측정 중 오류를 구분하고 재접속은 현재 error를 지운다.
성능 요약은 pending I/O/전송 대기의 peak/end와 native 호출·배치·프레임 증분/s를 추가 계산한다. 과거 결과의 없는 지표는 생략한다.
`Measure-ServerPerformance.ps1 -Stage before|after|diagnostic`은 실험 단계 식별용이며 부하를 바꾸지 않는다.
[원인 분리와 동일 조건 전후 비교](JOBQUEUE_IMPROVEMENT.md)를 참조한다.

## 게임 지형 공간 검색 검사

게임 서버는 NavMesh·HeightMesh를 로드한 뒤 4단위 XZ 공간 인덱스를 공유한다. `GET /api/state`의 GameServer `terrain_index`와 저장된 `samples[].servers[]`에서 준비 상태·규모를 조회할 수 있다. 원본 reporter에서는 `content.terrain_index`다.

`schema_version=1`이며 `nav`/`height`에 `ready`(구축 성공), `triangles`(원본 면 수), `cell_size`(월드 단위), `cells`, `references`(셀별 중복 ID 포함), `global_candidates`(모든 위치에서 순서 병합할 면 수), `bytes`(인덱스 vector capacity 합)가 있다. 마지막 값은 구축 임시 메모리나 전체 프로세스 사용량이 아니다. worker 시작 전 만든 복사본이며 실행 중 갱신하지 않는다. 이전 실행 파일에 없으면 누락 상태로 남기고, `ready=false`일 때는 기존 전체 검색을 사용한다.

정적 삼각형 계산 캐시에는 `prepared_triangles`(유효 캐시 개수), `prepared_bytes`(캐시 vector capacity bytes)가 추가된다. 기존 `bytes`에는 이 캐시를 포함하지 않는다. 법선·변·내적·분모 역수를 초기 로드에 계산하며, 캐시가 없거나 원본 개수가 맞지 않으면 기존 계산을 사용한다. 관측값은 API와 저장 결과에서 같은 이름으로 조회한다.

```powershell
$state = Invoke-RestMethod http://127.0.0.1:8790/api/state
($state.servers | Where-Object role -eq GameServer).terrain_index | ConvertTo-Json -Depth 4
./scripts/Test-ServerTerrain.ps1 -Configuration Release
./scripts/Test-ServerTerrain.ps1 -Configuration Debug
python tools/server_lab/terrain_compare.py --baseline artifacts/logs/game-tick-profile-20261012 --output artifacts/logs/terrain-compare-new
```

`Test-ServerTerrain`은 Core/ABI와 실제 메시의 기존 참조 Ray 대비 bit 단위 높이/노드/첫 hit 비교를 실행한다. `terrain_compare.py`는 기준 측정의 frozen 게임 실행 파일·현재 게임 실행 파일을 교차 배치해 같은 perf-match seed를 실행하고 종료 후 개선 파일을 복원한다. 기본은 4/16/64명·조건별5쌍·양쪽 계측 켬·입장 소프트 제한 해제다. 기존 서버가 실행 중이면 거부하며 실행 중 소스/데이터/실행 파일을 변경하지 않는다. 기준 데이터/시나리오 hash·종료 자원·실행 파일 복원/fingerprint·누락 profile을 검사하고 실패도 저장한다. 기준/개선 파일·소스 snapshot·원시 결과·분석은 출력 경로에 남는다.

`tick_profile`의 Ray `work`는 실제 방문한 후보 삼각형 수다. 일반 수직 하향 Ray를 가속하며 비수직/비정상/극단 Y·무효 인덱스는 선형 fallback한다. 원본 면 순서·높이 필터/보정·스킬 의미를 유지하며 품질 기준의 변경은 후속 작업이다. [전후 보고서](TERRAIN_SPATIAL_INDEX.html)·[계약/검증](TERRAIN_SPATIAL_INDEX.md)을 참조한다.

### 지형 메시 품질 진단

`terrain_audit.py`는 원본 OBJ를 수정하지 않고 SHA-256, 최장변/면적/XZ 면적 분포, 중복 좌표 면·퇴화·수직·가늘기와 `o`/`g` 출처별 개수를 JSON으로 저장한다. face는 현재 서버가 읽는 양수 정점 번호 3개 형식만 허용하며 잘못된 입력을 조용히 제외하지 않는다.

```powershell
python tools/server_lab/terrain_audit.py Server/Game_Server/Resource/NavMeshData3.obj Server/Game_Server/Resource/HeightMesh.obj --output artifacts/logs/terrain-quality.json
```

분류는 겹칠 수 있고 double 기하 진단이므로 서버 float 전역 후보와 개수가 다를 수 있다. 크기가 다르거나 수직이라는 이유만으로 불필요한 면으로 판정하지 않는다. 기존 OBJ에는 원본 오브젝트 이름이 없지만, 원본 Unity 프로젝트 없이도 연결 영역을 직접 검토하여 제외 대상을 지정할 수 있다. [캐시 실측·품질 진단](TERRAIN_CACHE.md)을 참조한다.

OBJ 개편 도구·후보 경로 선택 기능은 성능 이득이 부족하여 롤백했다. 이전 실험은 [롤백된 OBJ 개편 보고서](TERRAIN_REWORK.md)에 보존하며, 현재 서버는 기존 Resource의 OBJ를 사용한다.
