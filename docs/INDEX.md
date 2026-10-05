# 문서 인덱스

## 작업 진입점

| 목적 | 문서 |
|---|---|
| 공통 에이전트 규칙 | [AGENTS.md](../AGENTS.md) |
| 작업별 필수 지침 | [작업 지침 인덱스](guides/INDEX.md) |
| 빌드·실행·중지 | [개발 환경](development/SETUP.md) |
| 에이전트와 코드 탐색 | [에이전트 환경](development/AGENTS.md) |
| VS2026 이전 | [마이그레이션 기록](development/VS2026.md) |
| 현재 구성·연결 | [아키텍처](architecture/OVERVIEW.md) |
| 메시 공유·자원 수명·검증 | [메시 공유 구현](architecture/MESH_SHARING.md), [구조도와 근거](diagrams/mesh-sharing/README.md) |
| 확정 외형·영웅 선택 파츠 최적화 | [구현·검증·실측](portfolio/HERO_SELECTED_PARTS.md), [로딩 흐름도](diagrams/hero-selection/README.md) |
| 선택 스킬의 파티클 버퍼 최적화 | [구현·실측·검증](portfolio/PARTICLE_SELECTED_SKILLS.md), [이전 5개 풀 필요성 검토](portfolio/PARTICLE_POOL_CAPACITY_REVIEW.md), [선택과 생성 흐름](diagrams/particle-selection/README.md) |
| 전체 파티클 공용 GPU 버퍼 풀 | [재사용·확장·종료 해제 및 실측](portfolio/PARTICLE_BUFFER_POOL.md), [현재 메모리 구성 비율](portfolio/PARTICLE_BUFFER_POOL.md#최적화-후-클라이언트-메모리-구성-비율), [수명 흐름도](diagrams/particle-buffer-pool/README.md) |
| 포트폴리오·메모리 실측·맵/영웅 비용 | [포트폴리오 인덱스](portfolio/README.md), [최초 전후 비교](portfolio/CLIENT_MEMORY_OPTIMIZATION.md), [현재 비용 분해와 구간별 전후 비교](portfolio/CLIENT_MEMORY_BREAKDOWN.md) |
| 미니언·몬스터 메모리 공통화 후보 | [현재 공유·텍스처/upload 비용·예상 효과 검토](portfolio/NPC_RESOURCE_SHARING_REVIEW.md) |
| 모델 사이 DDS 공용 자원 | [구현·GPU 검사·조기 해제/BC7/상수 arena 설명](portfolio/NPC_RESOURCE_SHARING_REVIEW.md#후속-적용-모델-dds-공유), [소유권 구조도](diagrams/dds-sharing/README.md) |
| NPC 메모리 최적화 적용 결과 | [임시 upload·BC7·상수 arena 및 3회 실측](portfolio/NPC_MEMORY_OPTIMIZATION.md), [9종 전후 스크린샷](portfolio/evidence/npc-memory-20261005/gallery.html), [상수 수명도](diagrams/npc-memory/README.md) |
| UI BC7 압축·사전 로딩과 UV 보존 | [최종 구현·실측·품질](portfolio/UI_TEXTURE_COMPRESSION.md), [전후 GPU 화면](portfolio/evidence/ui-bc7-20261005/gallery.html), [로딩·UV 흐름도](diagrams/ui-bc7/README.md) |
| 하늘 큐브맵·UI/dissolve 후속 후보 | [DDS 형식·72MiB 후보·upload 수명·GPU 비용 검토](portfolio/SKYBOX_DISSOLVE_OPTIMIZATION_REVIEW.md), [계산·기존 측정 근거](portfolio/evidence/skybox-dissolve-review-20261006.json) |
| 기동·Title·음원과 공통 렌더링 후속 최적화 | [장면별 수명·PCM 직접 조사·우선순위](portfolio/STARTUP_RESOURCE_OPTIMIZATION_REVIEW.md) |
| 코드 개선 후보 | [리팩토링 검토](REFACTORING_PLAN.md) |
| 파일 정리 근거 | [정리 기록](development/CLEANUP.md) |
| GitLab에서 로컬로 이전 | [이전 기록](MIGRATION.md) |
| 현재 작업과 검증 결과 | [작업 목록](../tasks/todo.md) |

확정 설정은 개발 환경 문서에, 구조는 아키텍처 문서에, 검증 기록은 작업 목록에 둔다. 과거 분석 결과와 현재 적용 결과를 구분한다.
