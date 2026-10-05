# 선택 효과 구성과 공용 파티클 풀

[검증된 HTML](particle-selection.workflow.html) · [JSON 원본](particle-selection.workflow.json) · [구현·실측](../../portfolio/PARTICLE_BUFFER_POOL.md)

서버의 선택 전송 순서와 Frozen 스냅샷·필요 효과 판별은 유지한다. 선택 정보 미완료 시 전체 종류의 효과 객체를 유지한다. 효과 객체 생성과 대형 GPU 버퍼 할당을 분리했으며, 스킬·슬롯·환경 효과 모두 장면의 공용 풀에서 최초 표시 시 버퍼를 임대한다.

같은 버퍼 영역을 활성 효과끼리 동시에 쓰지 않는다. geometry 내용 캐시에 넣지 않으며 32byte 입자 형식이 같은 저장 공간을 재사용한다. 초기 용량은 300,000개이고 부족하면 확장한다. 본문은 한글, 고정 Viewer UI와 HTML lang은 영어다. Archify 2.16으로 작성했다.

## 재생성 및 검증

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/particle-selection/particle-selection.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/particle-selection/particle-selection.workflow.json docs/diagrams/particle-selection/particle-selection.workflow.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/particle-selection/particle-selection.workflow.html --json
```

- validate·deliver: showcase 9/9, 오류/경고 0.
- JSON SHA-256: `abb0fec0a04ffc096458ff89ba4569643dafa6c10f88a8d34c5cc593f897b9f8` (3,806 bytes).
- HTML SHA-256: `06b64015bf9a52169e4d805e85d2916c5b805245c009d823887bf05800c4fd8f` (708,946 bytes).
- 네 해상도(1440×900, 1600×1000, 1920×1080, 2048×1320)의 자동 containment/캡처 PASS.
- `visual_review: passed`, `correction_rounds: 0`. 1440×900 light와 2048×1320 dark를 직접 열어 노드·레이블·카드 잘림 및 겹침이 없음을 확인했다. 자동 receipt의 `visualReview: pending`은 그대로 유지한다.
- 캡처와 자동 receipt sidecar는 로컬 근거이며 Git에서 제외한다.
