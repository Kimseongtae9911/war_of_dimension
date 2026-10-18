# 동접 누적과 한계 확인

[흐름도](capacity.html) · [수정 원본](capacity.workflow.json).

근거: `tools/server_lab/capacity.py`의 exercise/analyze_result/main, `runner.py`의 capacity 시나리오 분기와 단계 저장, `monitor.py`의 표본 수집. 연결을 유지하며 추가하고 단계별 고정 주기 이동·정지 후 응답을 검증한다. 단계 추가·집계가 명백한 세 연결은 라벨을 생략했다. 서버 로직은 이 흐름도 변경 범위가 아니다.

계측 근거: 게임 `CNetworkMgr.cpp`의 ProcessTimerEvent가 예정→Post 직전 지연과 단조 시계 시각을 `TaggedIoContext`에 기록하고, MatchUpdate가 실행될 때 두 histogram을 확정한다. `Telemetry.cpp`→ServerFeed→저장 표본에서 조회한다. 신규 필드는 wire와 게임 정책을 바꾸지 않는다. 실제 결과는 [동접 보고서](../../development/SERVER_CAPACITY.md)에 연결한다.

유지 검사 근거: `settlement.py`가 마지막 stop 이후 처리 패킷 수·빈 패킷/송신 큐의 서로 다른3개 표본을 확인한다. monitor가 RunState에 전달한 최신 복사본을 사용하며, 새 격리 로비와 활성 계측이 필요하다. `settlement` 판정/마지막 수치를 API·결과에 남긴다.

프로젝트 Archify 2.16 workflow v2의 validate/deliver showcase 9/9, composition 오류0/경고0. JSON SHA-256 `c95d0cc5f3cca1490957839f9ccedccd3dac16ef215c934bd6819b7b2de318d1`, HTML SHA-256 `6d459263f946ba755f0f33b4b031ce5462f81505fd03f726fa0cb235b7c7fd30`.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/server-capacity/capacity.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/server-capacity/capacity.workflow.json docs/diagrams/server-capacity/capacity.html --quality showcase --json
```

기존 local-file 보안 거절을 우회하지 않으므로 실브라우저 시각/containment 검사는 수행하지 않았다. 한국어 설명을 사용하고 고정 Viewer UI와 html lang은 영어 fallback이다. 설치 스킬은 변경하지 않았다.
