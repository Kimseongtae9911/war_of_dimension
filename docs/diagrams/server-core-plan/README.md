# ServerCore 공통화 설계도

상태: **설계·미구현**. 서버의 현재 실행 구성이 아닌 1단계 목표 의존성이다.
로비·게임 서버가 공통 정적 라이브러리와 패킷 헤더를 사용하도록 분리한다.
Core는 콘텐츠·월드·DB의 소유자가 아니며, 두 서버는 독립 프로세스다.

- [구현 계획](../../architecture/SERVER_CORE_PLAN.md)
- [인터랙티브 HTML](server-core.architecture.html)
- [JSON 원본](server-core.architecture.json)
- [소스 조사·파일 쌍·해시](../../architecture/evidence/server-core-audit-20261007.json)
- [최종 delivery receipt](delivery-receipt.json)
- [desktop 자동 검사](server-core.architecture.visual-check.json)
- [light/dark 화면 모음](server-core.architecture.visual-check.html)

화살표는 사용 코드에서 공통 기반으로 향하는 **소스 의존성**이다.
실선은 Core 사용, 점선은 패킷 계약 사용이다.
클라이언트와 후속 더미의 Shared/Protocol 참조는 하단 설명에 기록했다.
Core는 패킷 struct에 의존하지 않고 프레임 정책을 주입받는다.
통신 연결·배포·서버 간 메모리 공유를 나타내는 그림이 아니다.

출처는 현재 `CNetworkMgr.cpp` 두 파일과 `protocol.h`이며,
기준 revision은 `86f83a94f76e7218c4cd2f34d992f9d2d02174f1`이다.
신규 Core 노드는 제안이므로 구현 출처 링크를 붙이지 않았다.
제안의 책임·소유권·검증 조건은 구현 계획에서 확인한다.

## 검증 결과

| 항목 | 결과 |
|---|---|
| diagram_type | `architecture` |
| validation | showcase **9/9**, 오류 0, 경고 0 |
| repository evidence | 현재 기준 revision의 소스 3개 확인 |
| desktop containment | 1440×900, 1600×1000, 1920×1080, 2048×1320 overflow 없음 |
| 화면 검토 | 최소·최대 크기의 light/dark 4개 화면 직접 확인. 선 교차·라벨 가림·카드 잘림 없음 |
| visual_review | `passed` |
| correction_rounds | `1` — I/O 라벨의 노드 겹침 수정 |

자동 `visual-check` receipt의 `visualReview: pending`은 그대로 보존한다.
직접 이미지 확인 결과는 위 표에 별도로 기록했다.
viewer의 고정 UI·`html lang`은 English이고 작성된 제목·설명은 한글이다.

| 파일 | SHA-256 | byte |
|---|---|---:|
| JSON | `f994445c642258addb9f9151297123a3e6e929a8d95b0e27d278817cc9329915` | 2,726 |
| HTML | `13649158cb2d8861c8803b77a7ba8255aa40fde88b82c6890aaef617246a65f7` | 706,044 |

최종 통과 이후 JSON·HTML을 수정하지 않았다.
검증된 byte와 해시를 유지하도록 `.gitattributes`의 문서별 정규화 예외를 사용한다.

## 재검증·재생성

저장소 루트에서 실행한다. 출처를 검증하려면 `--repo-root .`가 필요하다.
수정한 원본은 검증·delivery를 다시 수행하고 새 receipt와 화면 근거를 함께 갱신한다.
HTML을 직접 수정하지 않는다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/server-core-plan/server-core.architecture.json --quality showcase --repo-root . --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/server-core-plan/server-core.architecture.json docs/diagrams/server-core-plan/server-core.architecture.html --quality showcase --repo-root . --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/server-core-plan/server-core.architecture.html --json
```

これは 계획 문서 검증이다. 서버 빌드·플레이·DB·부하 검증 결과를 포함하지 않는다.
