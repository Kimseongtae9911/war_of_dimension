# 리팩토링 포트폴리오 기록

문제 → 원인 분석 → 선택한 설계 → 구현 → 검증 → 수치와 한계 → 후속 개선 순서로 기록한다. 완료한 변경과 아직 적용하지 않은 제안을 구분한다. 수치는 단위·시나리오·비교 기준·반복 횟수를 명시하고 재현 도구 및 근거 데이터와 함께 보존한다.

4개 클라이언트 동시 테스트 편의성 개선을 위한 작업은 [최종 보고서](FINAL_REFACTORING_REPORT.md)에서 에이전트 개발 환경·병목 측정·공통 자원·영웅·파티클·NPC·UI·미적용 후보의 8개 항목으로 정리했다. 각 항목의 결과물과 후보별 예상 절감량을 포함하며, 실제 게임 클라이언트 4개 동시 실행은 아직 미검증이다. [종합 근거](evidence/final-report-20261006.json)에 각 단계 수치·통계·출처 해시를 보존하며, 아래 단계별 보고서는 상세 구현·재현·화면 기록으로 유지한다.

| 주제 | 현재 기록 | 상태 |
|---|---|---|
| 하늘·UI/dissolve 후속 최적화 | [검토·우선순위](SKYBOX_DISSOLVE_OPTIMIZATION_REVIEW.md), [DDS/구간 측정 근거](evidence/skybox-dissolve-review-20261006.json) | 미적용 검토. 하늘 BC7 96→24MiB payload 후보, UI upload 36.88/35.81MiB 조기 회수 후보. dissolve는 기존 BC1 0.67MiB, R8는 증가·BC4는 절감 0 |
| UI BC7 압축과 사전 로딩 | [구현·실측·품질](UI_TEXTURE_COMPRESSION.md), [전후 32쌍 GPU 화면](evidence/ui-bc7-20261005/gallery.html), [로딩·UV 흐름](../diagrams/ui-bc7/README.md) | 구현 완료. 동일 EXE 각 3회 Private 중앙값 1,964.52→1,756.43MiB, 플레이어 UI DEFAULT 147.6250→41.9375MiB. 21 DDS 압축·픽셀/atlas 보존·종료 해제 검사 |
| 장면별 자원·음원과 공통 렌더링 후보 | [검토·우선순위](STARTUP_RESOURCE_OPTIMIZATION_REVIEW.md), [PCM/DDS 조사](evidence/startup-resource-review-20261005.json) | 적용 전 검토 보존, UI 구현은 위 문서 참조. Title DDS 4.21MiB·전환 해제 경로, 음원 142개 PCM 122.86MiB, BGM streaming 생성 확인. Shadow map·위치 RT 유지 확정. [UI BC7 예상량](evidence/ui-bc7-estimate-20261005.json): 플레이어 142.29→35.62MiB, 적용 전 예상 |
| 미니언·몬스터 자원 공통화 | [검토·DDS 공유](NPC_RESOURCE_SHARING_REVIEW.md), [실측·최종 구현](NPC_MEMORY_OPTIMIZATION.md), [전후 화면](evidence/npc-memory-20261005/gallery.html), [상수 수명도](../diagrams/npc-memory/README.md) | 임시 upload 회수·DDS 공유·BC7·객체 arena 적용. 각 3회 Private 중앙값 2,656.35→1,964.46MiB, 고유 NPC texture 441.25→113.3125MiB, 객체 CB 50.5→0.5MiB. 9종 실제 셰이더 비교·GPU 수명 검증 |
| 클라이언트 메시 공유와 인게임 진입 메모리 | [분석 및 결과](CLIENT_MEMORY_OPTIMIZATION.md), [수치 근거](evidence/client-memory-20261005.json), [CSV](evidence/client-memory-20261005.csv), [맵 자원 분석](evidence/ingame-map-resources-20261005.json) | 구현 완료, 이전 로딩 경로 재현 A/B 3회씩 측정·맵 로딩 구성 분석 완료 |
| 솔루션 통합·VS2026·SLNX | [개발 환경](../development/SETUP.md), [마이그레이션](../development/VS2026.md), [작업 기록 6~7](../../tasks/todo.md) | 구현·빌드 완료, VS GUI 실행 프로필은 미검증 |
| 에이전트 개발 환경 | [설정과 검증](../development/AGENTS.md) | 설정·도구 점검 완료 |
| 메시 공유 직후 약 5,766MiB의 비용 분해 | [당시 증가 원인과 맵 비중](CLIENT_MEMORY_BREAKDOWN.md), [계측 근거](evidence/client-memory-breakdown-20261005.json), [CSV](evidence/client-memory-breakdown-20261005.csv) | 당시 경로 3회 측정, 실제 파티클 131개·버퍼 용량 확인. 후속 변경 전 역사 자료 |
| 메시 공유 전후의 항목별 비용 | [동일 조건의 구간별 비교](CLIENT_MEMORY_BREAKDOWN.md#후속-실측-메시-공유-전에는-각각-얼마였나), [전후 근거](evidence/client-memory-stage-comparison-20261005.json), [CSV](evidence/client-memory-stage-comparison-20261005.csv) | 동일 바이너리의 이전 경로 재현/현재 경로 각 3회 교대 측정, 마지막 해제 구간까지 합계 검증 |
| 불필요 파일 정리·GitLab 이전 | [정리 근거](../development/CLEANUP.md), [이전 기록](../MIGRATION.md) | 완료 |
| 확정 외형으로 영웅 선택 파츠 로딩 | [용어·구현·실측·검증 및 실제 인게임 화면](HERO_SELECTED_PARTS.md), [구조도](../diagrams/hero-selection/README.md) | 선택 외형과 공통 본·부모의 변환 행렬만 보관, 모든 clip/keyframe 유지. 전체/선택 각 3회 측정·행렬 1,576,969개 일치·Debug/Release 검증, Private commit 904.36MiB 감소 |
| 선택 스킬의 파티클 버퍼 생성 | [선택 범위·실측·검증](PARTICLE_SELECTED_SKILLS.md), [5개 풀 필요성 검토](PARTICLE_POOL_CAPACITY_REVIEW.md), [구조도](../diagrams/particle-selection/README.md) | 네 참가자의 선택 합집합·기본 공격·후속·타워 효과 유지. 고정 조합의 총 객체 127→72, 대형 GPU allocation 1,010.625MiB 감소. Debug/Release 네 클라이언트 전송 순서 검사 완료. 공용 풀 재사용은 후속 구현 문서 참조 |

현재 첫 번째 상세 사례는 메모리 최적화다. 후속 작업에서도 같은 형식으로 문서를 추가하고 이 인덱스를 갱신한다. 과거 수치 근거를 덮어쓰지 않고 새로운 측정 시나리오나 개선 단계별로 날짜가 다른 근거 파일을 추가한다.

후속 구현: [전체 파티클 공용 GPU 풀](PARTICLE_BUFFER_POOL.md). 스킬·슬롯·환경 효과의 버퍼 재사용·부족 시 확장·GPU 완료 후 반환과 종료 해제를 적용했다. 실제 Shader의 고정 재생 각 3회에서 72→19쌍, 대형 GPU allocation 973.875MiB 감소를 확인했다. 실제 전투 최대치와 구분한다.
