# 시나리오 더미·실시간 관측 구현 구조

[HTML 보기](server-lab.architecture.html) · [JSON 원본](server-lab.architecture.json) · [delivery receipt](delivery.json)

실제 첫 구현의 요청/응답과 계측 경로다. 스킬 버전은 프로젝트 Archify 2.16이다.
근거는 [사용·계약 문서](../../development/SERVER_LAB.md), `tools/server_lab/runner.py`·`observation.py`·`monitor.py`,
`Server/ServerCore/src/Telemetry.cpp`, 양쪽 `CServer::Run()`, 게임 `CMatch`·`MatchTelemetry.h`, 클라이언트 `Scene.h`다.
기존 게임 TCP와 별도 로컬 JSON snapshot을 사용한다. 전체 서버 월드·리플레이·성능 전후 비교는 아직 구현하지 않았다.
수집 스레드가 Core 통계를 읽으며 게임 worker는 파일/HTTP I/O를 수행하지 않는다.
관측 활성화에 따른 atomic·시계 호출 및 Stats mutex 비용은 별도 측정 대상이다.
지형 원본 → monitor의 `/api/terrain` → 지형/클라이언트 탭 경로를 추가했다.
`tools/server_lab/terrain.py`가 양쪽 `CServer.cpp`와 동일한 HeightMesh/NavMesh를 읽고 높이 영상과 X/Z 삼각형을 캐시한다.
runner가 더미 자신의 위치와 매치별 대표 더미의 NPC 수신을 제한된 궤적/사건으로 보관한다.
monitor는 지표 1초 `/api/state`와 월드 0.2초 `/api/world` 경로를 분리하고 브라우저는 위치 보간·객체 추적·레이어 필터를 제공한다.
로비 고정 NPC 4명의 클라이언트 원본과 게임 NPC 8종의 기호/색상/범례를 추가했다.
게임 ADD/MOVE/REMOVE의 종류 값은 실제 종족으로 수정했으며 패킷 배치·크기는 유지한다.
기존 CMatch 업데이트의 IOCP worker 실행 경로에서 일정 값을 복사해 잠금 보호 저장소에 발행하며 별도 reporter는 이 복사본만 직렬화한다.
monitor가 현재 실행의 정확한 참가자 집합과 매치를 연결하고 브라우저가 웨이브 검사/실제 예약 스폰/게이트 해제 카운트다운을 구분한다.
수집 지연·실행 종료에서는 마지막 일정 표본을 고정한다. 기존 매치 실행 구조는 유지하며 게임 worker의 파일/HTTP I/O는 없다.
결과 JSON에는 종료 시 제한된 `world` 관측과 일정 복사본을 추가하며 CSV assertion 형식은 유지한다.

showcase 검증 9/9, 오류 0·경고 0. 실제 HTML의 1440×900·1600×1000·1920×1080·2048×1320 containment를 통과했다.
smallest/largest light/dark PNG 4장을 직접 열어 본문·노드·선·라벨의 영역/겹침을 확인했다.
자동 visual receipt의 `visualReview: pending`은 유지하고 직접 점검 결과를 이 문서에 기록한다.
화면 검사 sidecar/PNG는 재생성 가능한 로컬 산출물로 Git에서 제외한다.
한국어 본문을 사용하며 renderer의 고정 Viewer UI와 HTML 언어 fallback은 영어다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/server-lab/server-lab.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/server-lab/server-lab.architecture.json docs/diagrams/server-lab/server-lab.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/server-lab/server-lab.architecture.html --json
```

최종 JSON: 4,740byte · SHA-256 `a2fa1c3a44ad2c4dd1923eedde01862d26e5329272f9eef6cbc838e756c5801c`.
최종 HTML: 713,464byte · SHA-256 `619e6ef46e816a75e0ad12014725d16703a6f48af470f1f435e714307c1f86c0`.
정확한 byte를 유지하도록 두 파일의 Git 텍스트 정규화를 제외한다. HTML만 수정하지 않는다.
