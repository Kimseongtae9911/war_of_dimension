# NPC 객체 상수 버퍼의 수명

[검증된 HTML](npc-memory.lifecycle.html) · [JSON 원본](npc-memory.lifecycle.v2.json) · [구현·실측·전후 화면](../../portfolio/NPC_MEMORY_OPTIMIZATION.md)

근거는 `ObjectConstantArena::BeginFrame/Write/SubmitFrame/WaitForIdle`, `CGameObject::UpdateShaderVariables`와 Framework의 제출, Scene의 종료다. 256byte 영역을 draw마다 기록하며 2프레임의 fence 완료 뒤 재사용한다. 용량이 모자라면 기존 주소를 유지하면서 페이지를 추가한다. 마지막 소유자 해제 시 확장 페이지도 반환한다. 초기 논리 slot 808개는 총 8페이지·0.5MiB를 예약한다. 본 pose 버퍼와 임시 복사용 upload는 별개다.

종료 노드는 GPU 완료 후 반환, 완료 노드는 다음 프레임 재사용이 가능해짐을 뜻한다. 첫 노드의 fence 대기가 다음 프레임에서 반복된다. 자동 주 경로와 확장/종료의 방향이 명확하므로 간선에 같은 내용을 중복 표기하지 않았다. 본문은 한글이며 고정 Viewer UI·HTML lang은 영어다. Archify 2.16을 사용했다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate lifecycle docs/diagrams/npc-memory/npc-memory.lifecycle.v2.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver lifecycle docs/diagrams/npc-memory/npc-memory.lifecycle.v2.json docs/diagrams/npc-memory/npc-memory.lifecycle.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/npc-memory/npc-memory.lifecycle.html --json
```

- validate/deliver: showcase 9/9, 오류/경고 0.
- JSON SHA-256 `9f407db9824c68d09bc9064ee31cb72c0b9d1ca4d07ba5780971beb1be85306a`, 3,594 bytes.
- HTML SHA-256 `a869906f0a9472b1f14eb29abdb53f1b203b7d5ee95cdce2311c93faf7135798`, 707,477 bytes.
- 네 해상도 1440×900·1600×1000·1920×1080·2048×1320 containment PASS. 1440 light와 2048 dark의 실제 캡처를 직접 열어 노드·방향·카드·레이블이 잘리거나 겹치지 않음을 확인했다.
- `visual_review: passed`, `correction_rounds: 2`. 최소 viewBox 높이 수정 후, 작은 viewport의 40px 세로 초과는 중복된 보기 선택을 제거한 새 v2 후보로 해결했다. 전달된 v2 JSON/HTML은 변경하지 않았다. 자동 receipt의 `visualReview: pending`은 유지하며 로컬 캡처/sidecar는 Git에서 제외한다.
