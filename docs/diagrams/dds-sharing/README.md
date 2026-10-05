# 모델 DDS 자원 공유와 소유권

이 문서는 DDS 공유 적용 시점의 구조와 검증 기록이다. 이후 NPC 조기 upload 순회·BC7·상수 arena도 적용했으며 [후속 구현](../../portfolio/NPC_MEMORY_OPTIMIZATION.md)을 따른다. 전달된 DDS JSON/HTML과 당시 해시는 유지한다.

[검증된 HTML](dds-sharing.html) · [JSON 원본](dds-sharing.architecture.json) · [구현·검증·후속 설명](../../portfolio/NPC_RESOURCE_SHARING_REVIEW.md#후속-적용-모델-dds-공유)

근거는 `CMaterial::LoadTextureFromFile`, `CTexture`의 공유 슬롯, `SharedDdsTexture::Load`와 `CScene::CreateShaderResourceViews`다. 모델 material은 정규 DDS 경로·device·용도로 GPU texture와 upload를 공유하며, 각 CTexture가 공유 소유권을 가진다. 캐시는 weak 참조만 보관한다. 같은 Scene 힙의 읽기 전용 SRV도 재사용하지만 root parameter와 handle 저장 공간은 개별이다. 새 Scene 힙에는 descriptor를 다시 만든다. UI·파티클의 직접 로더는 유지한다.

GPU copy가 완료된 후 upload를 해제하고, 마지막 소유자 해제 시 남은 upload/default도 회수한다. NPC의 조기 upload 해제 순회를 추가한 구조도가 아니다. 같은 device의 미완료 다른 command list는 거부하며, 현재 모델 초기화의 단일 list·fence 순서를 전제로 한다. 본문은 한글이고 고정 Viewer UI와 HTML lang은 영어다. Archify 2.16으로 작성했다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/dds-sharing/dds-sharing.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/dds-sharing/dds-sharing.architecture.json docs/diagrams/dds-sharing/dds-sharing.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/dds-sharing/dds-sharing.html --json
```

- validate·deliver: showcase 9/9, 오류/경고 0.
- JSON SHA-256: `a3565595acc0fa820527a17329c00679cf474b17495617c316746d3ade0bd444` (2,031 bytes).
- HTML SHA-256: `a54849206b883b5b9a3519d5d75d8b2f5f38a5b9d39b79befe0a2c64ed51fba0` (705,286 bytes).
- 네 해상도 1440×900, 1600×1000, 1920×1080, 2048×1320 자동 containment/캡처 PASS.
- `visual_review: passed`, `correction_rounds: 1`(labelDy 충돌 수정). 1440×900 light와 2048×1320 dark를 직접 열어 레이블·노드·카드가 잘리거나 겹치지 않음을 확인했다. 자동 receipt의 `visualReview: pending`은 유지한다.
- 캡처와 자동 receipt sidecar는 로컬 근거이며 Git에서 제외한다.
