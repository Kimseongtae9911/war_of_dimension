# ServerCore 구현 구조도

[HTML 보기](server-core.architecture.html) · [JSON 원본](server-core.architecture.json)

현재 작업 트리에 적용한 1단계와 2026-10-10 잔여 공통 기반 추출 구조다.
공통 수신·disconnect는 SessionHandler가, 로비 작업 예약은 JobScheduler가 담당한다.
Resource의 I/O 풀·세션/소켓 재사용 구현도 Core가 제공하고 서버별 pch.h는 구성만 선택한다.
GameObject·MoveObject 기반과 공통 위치/방향/속도 상태도 Core가 제공한다.
게임은 DirectX 충돌 타입/변환과 콘텐츠를, 로비는 이동/시야 규칙을 유지한다.
CClient는 가상 함수로 패킷 전달과 사용자 정리를 확장한다.
클라이언트와 두 서버는 `Shared/Protocol` 선언을 사용한다.
로비와 게임 서버는 `ServerCore` 정적 라이브러리를 각각 링크한다.
Windows 네트워크 자원은 각 프로세스가 독립적으로 소유한다.

근거는 [구현 문서](../../architecture/SERVER_CORE.md)와
[1단계 검증 기록](../../architecture/evidence/server-core-implementation-20261007.json),
[추가 추출 검증 기록](../../architecture/evidence/server-core-extraction-20261010.json)이다.
GameObject 추가 추출의 현재 결과는 [작업 목록 58번](../../../tasks/todo.md)에 기록한다. evidence JSON은 이전 snapshot이다.
새 파일이 아직 커밋되지 않아 특정 commit의 구현으로 표시하지 않았다.
이전 미구현 설계는 [계획 구조도](../server-core-plan/README.md)로 보존한다.

## 검증

- [delivery receipt](delivery-receipt.json): showcase 9/9, 오류 0·경고 0.
- [화면 검사](server-core.architecture.visual-check.json): 1440×900·1600×1000·1920×1080·2048×1320의 light/dark containment 통과.
- 생성된 PNG 4장을 직접 확인했다. 본문·선·라벨이 겹치지 않고 영역 안에 표시됐다.
- 한국어 도메인 설명을 사용한다. Archify viewer의 고정 UI와 HTML 언어 fallback은 영어다.
- receipt의 JSON/HTML byte와 SHA-256을 유지한다. `.gitattributes`에 해당 두 파일의 텍스트 정규화 예외를 둔다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/server-core/server-core.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/server-core/server-core.architecture.json docs/diagrams/server-core/server-core.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/server-core/server-core.architecture.html --json
```

재생성할 때 새 receipt와 화면 검사 결과를 함께 저장한다.
HTML만 수정하지 않는다.

최종 JSON: 3,711byte · SHA-256 `35ac9248b4516cb1f6937b6fedec544821555a1c576139b52537fa3e3fce6f8e`.
최종 HTML: 707,297byte · SHA-256 `91e61c429c797d18450f35ce7aa91abba201352d9e3016760d72e64284c7ea33`.
자동 visual receipt의 `visualReview: pending`은 유지하고, 직접 PNG 4장 확인 결과를 이 README에 기록한다.
