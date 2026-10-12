# 게임 tick 비용 계측

[JSON](profile.workflow.json) · [HTML](profile.workflow.html) · [분석 도구 계약](../../development/SERVER_LAB.md#게임-tick-구간별-비용-분석).

실제 CMatch의 기존 순서에서 큰 구간과 내부 지형/충돌/송신을 측정한다. Scope는 worker별 frame 로컬 값을 누적하고 frame 종료 때 atomic 통계에 반영한다. 기존 reporter는 게임 객체를 추가 순회하지 않고 통계 JSON만 직렬화한다. Server Lab은 이 필드를 API/원시 결과에 보존하며 tick_profile.py가 측정 내부 표본을 차분한다.

큰 구간의 순서는 위상 자체로 드러나므로 구간 사이 화살표에는 반복 설명을 붙이지 않았다. reporter로 가는 화살표는 atomic snapshot, 분석 도구로 가는 화살표는 1초 표본 저장을 명시한다. JSON/HTML은 구현의 계측 경로를 표현하며 로직 최적화 결과를 나타내지 않는다.

근거: `Server/Game_Server/CMatch.cpp`, `CNetworkMgr.cpp`, `MathUtil.h`, `GameUtil.cpp`, `Server/ServerCore/include/ServerCore/TickProfile.h`, `src/Net.cpp`, `src/Telemetry.cpp`, `tools/server_lab/tick_profile.py`.

Archify2.16 workflow v2. 한국어 설명이며 고정 Viewer UI와 html lang은 영어다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/game-tick-profile/profile.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/game-tick-profile/profile.workflow.json docs/diagrams/game-tick-profile/profile.workflow.html --quality showcase --json
```

검증: showcase9/9·composition 오류0/경고0. JSON SHA-256 `04b0f0af8431e18caed2b699a1468e75d775b99c631dea44477bd33654c45008`·2322bytes, HTML `9afee19d146d970e2478a7150a215091fba54b6e19102137d3ace1acece0d0c7`·708606bytes.
기존 local-file 브라우저 보안 거절을 우회하지 않아 visual-check/실제 HTML 시각 검토는 수행하지 않았다. 정적 검증과 시각 검토를 구분한다.
