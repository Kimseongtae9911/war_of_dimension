# 지형 정적 계산 캐시 흐름

초기 로드에서 불변 계수를 계산하고 RayCast에서 원본 삼각형 ID로 재사용한다. [HTML](cache.html) · [수정 원본 JSON](cache.workflow.json) · [실측 보고서](../../development/TERRAIN_CACHE.html).

근거: `Server/Game_Server/TerrainSpatialIndex.h`의 Build/Prepared/Reset, `MathUtil.h`의 RayCast 4종, `GameUtil.cpp`의 TerrainIndexJson. 캐시가 없거나 무효인 분기는 기존 Ray 교차 계산에서 결과까지 반환하므로 합류 화살표를 중복하지 않았다. 라벨 없는 원본→계수와 계수 읽기→교차 계산은 노드 자체가 처리 순서를 설명하며 별도 프로토콜 의미가 없다.

Archify2.16·workflow schema2, validate/deliver showcase9/9, composition 오류0/경고0.

- JSON SHA-256 `6283d467124b62691a762c978f5a7f08c5eea43f41e06c1cd30d9378c74026f0`, 2,131bytes.
- HTML SHA-256 `b8566809922f21c4e7a54479976a029244a19e279db70344143b5b7788dcce3e`, 706,021bytes.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/terrain-cache/cache.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/terrain-cache/cache.workflow.json docs/diagrams/terrain-cache/cache.html --quality showcase --json
```

최종 성공 후 JSON/HTML을 수정하지 않았다. 기존 local-file 브라우저 보안 거절을 우회하지 않아 실제 HTML 시각/containment 검사는 미수행이다. 한글 설명을 사용하며 고정 Viewer UI와 html lang은 영어 fallback이다. 설치 스킬은 변경하지 않았다.
