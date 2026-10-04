# 클라이언트 메시 공유 구조도

[검증된 HTML](mesh-sharing.architecture.html) · [JSON 원본](mesh-sharing.architecture.json) · [구현과 테스트](../../architecture/MESH_SHARING.md)

## 근거와 범위

2026-10-04 현재 구현을 나타낸다. 근거는 `CGameObject::LoadFrameHierarchyFromFile`, `SetSharedMesh`, `CStandardMesh::LoadSharedGeometryFromFile`, `LoadSharedGeometryDataFromFile`, `ReadMeshContentRecord`다. 소스는 `Client/WarOfDimension/Object.cpp`, `Mesh.cpp`, `MeshContent.cpp`에 있다.

동일 device와 geometry 내용은 하나의 자원을 공유하고 프레임 transform·material과 스킨드 본·애니메이션 상태는 별도로 유지한다. 캐시는 약한 참조만 보관하며 UI·파티클은 등록하지 않는다. 네트워크 구성이 그대로라 기존 local-runtime 구성도는 재생성하지 않았다.

작성 언어는 한글이다. Archify 2.16의 고정 Viewer UI와 HTML `lang`은 영어로 표시된다. 별도 서버 없이 열 수 있는 architecture HTML이며 기본 모션은 꺼져 있다.

## 재생성

저장소 루트에서 실행한다. JSON을 수정할 때마다 validate하고 최종 통과 후 JSON을 고정한 뒤 deliver한다. HTML만 편집하지 않는다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/mesh-sharing/mesh-sharing.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/mesh-sharing/mesh-sharing.architecture.json docs/diagrams/mesh-sharing/mesh-sharing.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/mesh-sharing/mesh-sharing.architecture.html --json
```

## 검증 기록

- diagram_type: `architecture`, Archify 2.16.
- validate·deliver: showcase 9/9, composition error 0, warning 0, exit 0.
- specification SHA-256: `e80e02d00d9d772f3095313b2a1568c6a0e404380c4af0e098d9c230685536f6` (1,802 bytes).
- artifact SHA-256: `767db140a5a318888dc6c5649b50f2fbe32ee4d2c1eab9716aa9ac22c5e71fd7` (704,855 bytes).
- 자동 화면 검사: 1440×900, 1600×1000, 1920×1080, 2048×1320에서 가로·세로 overflow 없음. 1440×900와 2048×1320 light/dark 캡처 PASS, exit 0.
- `visual_review: passed`, `correction_rounds: 0`. 1440×900 light와 2048×1320 dark PNG를 직접 열어 노드·관계·레이블·카드의 잘림과 겹침, 큰 화면에서의 배치를 확인했다. 자동 receipt의 `visualReview: pending`은 자동 측정과 직접 시각 점검을 구분하므로 수정하지 않았다.

자동 캡처·receipt는 재생성 가능한 로컬 sidecar로 Git에서 제외한다. 위 SHA-256은 실제 배포한 JSON·HTML 바이트에 대한 deliver receipt다.
