# UI BC7 사전 로딩과 UV 보존 흐름도

[현재 HTML](ui-bc7.dataflow.html), [수정 원본 v2 JSON](ui-bc7.dataflow.v2.json), [전달 receipt](delivery.json), [4해상도 시각 검사](ui-bc7.dataflow.visual-check.json)를 함께 관리한다. 설명은 한글이며 고정 Viewer UI와 html lang은 영어다.

근거는 [최종 구현·실측](../../portfolio/UI_TEXTURE_COMPRESSION.md), `Convert-UiTextures.py`, `UiTextureLayout.h`, `stdafx.cpp`, `Object.cpp`, `Mesh.h/cpp`, `Common.hlsl`, `UI.hlsl`이다. 기존 사전 로딩→BC7 GPU 상주→샘플링과 DDS 내용 영역→mesh 상수→atlas 계산 후 UV 보정의 경로를 표현한다. UPLOAD 조기 회수·동적 로딩·장면별 음원 등 미구현 제안은 이 흐름에 넣지 않는다.

v2는 첫 후보의 화면 세로 overflow를 수정해 행 간격과 도면 높이를 줄인 최종 버전이다. 이전 후보와 실패 진단은 로컬 `artifacts/logs/ui-archify-first`에 보존했으며 최신 성공 결과로 제시하지 않는다. 최종 showcase 9/9, composition 오류/경고 0과 1440×900·1600×1000·1920×1080·2048×1320 containment PASS. 1440 light/2048 dark 화면을 직접 점검했다. 자동 receipt의 visualReview pending과 실제 점검 기록은 구분한다. frozen JSON/HTML bytes는 Git 텍스트 정규화에서 제외한다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate dataflow docs/diagrams/ui-bc7/ui-bc7.dataflow.v2.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver dataflow docs/diagrams/ui-bc7/ui-bc7.dataflow.v2.json docs/diagrams/ui-bc7/ui-bc7.dataflow.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/ui-bc7/ui-bc7.dataflow.html --json
```
