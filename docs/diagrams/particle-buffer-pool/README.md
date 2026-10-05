# 모든 파티클의 공용 GPU 버퍼 수명

[검증된 HTML](particle-buffer-pool.workflow.html) · [JSON 원본](particle-buffer-pool.workflow.json) · [구현·실측](../../portfolio/PARTICLE_BUFFER_POOL.md)

`CScene::RenderParticle`는 비활성 효과를 먼저 반환하고 활성 효과에 독립 블록을 임대한다. `ParticleBufferPool`은 빈 블록을 재사용하거나 추가로 생성한다. 제출 후 단조 증가 fence로 완료를 확인하며, 개별 입자 포화 시 다음 프레임에 더 큰 버퍼를 임대하고 유효 입자를 복사한다. 장면 전환·종료는 GPU 완료를 기다린 뒤 효과와 확장분·유휴분을 모두 해제한다.

같은 버퍼 영역을 활성 효과끼리 동시에 쓰지 않는다. geometry 내용 캐시에 넣지 않으며 32byte 입자 형식이 같은 저장 공간을 재사용한다. 초기 용량은 300,000개이고 부족하면 확장한다. 본문은 한글, 고정 Viewer UI와 HTML lang은 영어다. Archify 2.16으로 작성했다.

## 재생성 및 검증

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/particle-buffer-pool/particle-buffer-pool.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/particle-buffer-pool/particle-buffer-pool.workflow.json docs/diagrams/particle-buffer-pool/particle-buffer-pool.workflow.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/particle-buffer-pool/particle-buffer-pool.workflow.html --json
```

- validate·deliver: showcase 9/9, 오류/경고 0.
- JSON SHA-256: `24252ce1d6bf0d58c31c767b827b93c193d7a3ae5531409c532c119c8718fbe0` (3,796 bytes).
- HTML SHA-256: `64072114c45ee6d9f871b295eb8e5c4db31f1388d934f974d95cb4d3b3f66dbb` (709,709 bytes).
- 네 해상도(1440×900, 1600×1000, 1920×1080, 2048×1320)의 자동 containment/캡처 PASS.
- `visual_review: passed`, `correction_rounds: 0`. 1440×900 light와 2048×1320 dark를 직접 열어 노드·레이블·카드 잘림 및 겹침이 없음을 확인했다. 자동 receipt의 `visualReview: pending`은 그대로 유지한다.
- 캡처와 자동 receipt sidecar는 로컬 근거이며 Git에서 제외한다.
