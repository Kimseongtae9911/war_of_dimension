# 게임 지형 공간 검색 흐름

현재 구현의 XZ 격자 구축·읽기 전용 조회·원본 ID 순 병합·기존 판정과 fallback을 표현한다. [HTML](terrain.html) · [수정 원본 JSON](terrain.workflow.json) · [실측 보고서](../../development/TERRAIN_SPATIAL_INDEX.html).

근거: `Server/Game_Server/TerrainSpatialIndex.h`, `GameUtil.cpp`의 두 메시 로드/MapCollision, `MathUtil.h`의 RayCast/Candidates, `CServer.cpp`의 startup 통계 복사본. 미지원 Ray는 기존 선형 RayCast 안에서 결과를 반환하므로 별도 hit 노드로의 긴 중복 연결은 생략했다. 라벨 없는 로드→격자는 두 노드 자체가 순서를 설명하며 프로토콜 의미가 없다.

Archify2.16·workflow schema2, mainPath5단계. validate와 deliver 모두 showcase9/9·composition 오류0/경고0.

- JSON SHA-256 `27de3181827536ab5ad4f8590174592b440148f23eb0d6e5b1e3336e1fb146e2`, 2,026bytes.
- HTML SHA-256 `a0fe2d171e7c4d8607512961a26128b65a902130997d32aad93392534cdf3db7`, 706,095bytes.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/terrain-spatial-index/terrain.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/terrain-spatial-index/terrain.workflow.json docs/diagrams/terrain-spatial-index/terrain.html --quality showcase --json
```

최종 성공 후 JSON/HTML을 수정하지 않았다. 기존 local-file 브라우저 보안 거절을 우회하지 않아 visual-check와 실제 HTML 시각/containment 검사는 미수행이다. 한글 설명을 사용하며 고정 Viewer UI와 html lang은 영어 fallback이다. 설치 스킬은 변경하지 않았다.
