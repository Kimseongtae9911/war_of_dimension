# 인게임 영웅 선택 파츠 로딩

[검증된 HTML](hero-selection.workflow.html) · [JSON 원본](hero-selection.workflow.json) · [구현·실측 기록](../../portfolio/HERO_SELECTED_PARTS.md)

## 근거와 범위

2026-10-05 구현을 나타낸다. `NetworkManager::StoreIngameAppearance`와 `FreezeIngameAppearances`가 매치의 확정 외형을 고정하고, `ModelPartSelection`이 성별·파츠 번호로 필요한 자원을 선택한다. `CGameFramework::ChangeScene`의 타인 모델은 세 영웅의 합집합, `CGamePlayer`의 로컬 모델은 자신의 파츠를 사용한다.

`CGameObject::LoadFrameHierarchyFromFile`은 전체 프레임 계층을 유지하며 미선택 메시·스킨·재질 생성을 생략한다. `LoadAnimationFromFile`은 각 keyframe에서 선택한 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관한다. 모든 61개 애니메이션과 keyframe은 유지한다. 선택 스킬별 clip 로딩은 미적용이며, 구조도의 “필요 행렬 보관”은 외형에 따른 변환 대상의 선택을 뜻한다. 공통 무기와 미지원 이름은 유지하고 로비/READY 및 수신 미완료는 전체 모델을 사용한다. 기존 base geometry 공유와 타인 모델의 공유 계층을 유지한다.

내용은 한글이며 Archify 2.16의 고정 Viewer UI와 HTML `lang`은 영어다. 별도 서버 없이 여는 workflow HTML이고 모션은 꺼져 있다.

## 재생성 및 검증

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/hero-selection/hero-selection.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/hero-selection/hero-selection.workflow.json docs/diagrams/hero-selection/hero-selection.workflow.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/hero-selection/hero-selection.workflow.html --json
```

- validate·deliver: showcase 9/9, composition error 0, warning 0, exit 0.
- JSON SHA-256: `b9effe5d33ea40ab2e9c40890d19dd6c610e414c77189aa9143c5b2081548b2a` (2,535 bytes).
- HTML SHA-256: `29b7e5ae055906bf21b334166aa9eef1e349d8bfc19c79b23f85b039d723b647` (708,554 bytes).
- 자동 화면 검사: 1440×900, 1600×1000, 1920×1080, 2048×1320 overflow·readability·Viewer 배치 PASS. 1440×900와 2048×1320 light/dark 캡처 PASS.
- `visual_review: passed`, `correction_rounds: 0`. 1440×900 light와 2048×1320 dark PNG를 직접 열어 노드·레이블·연결·카드의 잘림과 겹침이 없음을 확인했다. 자동 receipt의 `visualReview: pending`은 자동 측정과 직접 점검을 구분하므로 수정하지 않는다.

자동 캡처·receipt는 재생성 가능한 로컬 sidecar로 Git에서 제외한다. JSON 수정 후 validate와 deliver를 다시 실행한다.

2026-10-05 용어 정리에서 “애니메이션 축소”를 “필요 행렬 보관”으로 수정하고 위 결과로 재검증했다. 코드·구조·메모리 측정값은 변경하지 않았다.
