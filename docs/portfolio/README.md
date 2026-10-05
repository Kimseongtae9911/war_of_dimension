# 리팩토링 포트폴리오 기록

문제 → 원인 분석 → 선택한 설계 → 구현 → 검증 → 수치와 한계 → 후속 개선 순서로 기록한다. 완료한 변경과 아직 적용하지 않은 제안을 구분한다. 수치는 단위·시나리오·비교 기준·반복 횟수를 명시하고 재현 도구 및 근거 데이터와 함께 보존한다.

| 주제 | 현재 기록 | 상태 |
|---|---|---|
| 클라이언트 메시 공유와 인게임 진입 메모리 | [분석 및 결과](CLIENT_MEMORY_OPTIMIZATION.md), [수치 근거](evidence/client-memory-20261005.json), [CSV](evidence/client-memory-20261005.csv), [맵 자원 분석](evidence/ingame-map-resources-20261005.json) | 구현 완료, 이전 로딩 경로 재현 A/B 3회씩 측정·맵 로딩 구성 분석 완료 |
| 솔루션 통합·VS2026·SLNX | [개발 환경](../development/SETUP.md), [마이그레이션](../development/VS2026.md), [작업 기록 6~7](../../tasks/todo.md) | 구현·빌드 완료, VS GUI 실행 프로필은 미검증 |
| 에이전트 개발 환경 | [설정과 검증](../development/AGENTS.md) | 설정·도구 점검 완료 |
| 현재 약 5,766MiB의 비용 분해 | [증가 원인과 맵 비중](CLIENT_MEMORY_BREAKDOWN.md), [계측 근거](evidence/client-memory-breakdown-20261005.json), [CSV](evidence/client-memory-breakdown-20261005.csv) | 현재 경로 3회 측정, 실제 파티클 131개·버퍼 용량 확인 |
| 메시 공유 전후의 항목별 비용 | [동일 조건의 구간별 비교](CLIENT_MEMORY_BREAKDOWN.md#후속-실측-메시-공유-전에는-각각-얼마였나), [전후 근거](evidence/client-memory-stage-comparison-20261005.json), [CSV](evidence/client-memory-stage-comparison-20261005.csv) | 동일 바이너리의 이전 경로 재현/현재 경로 각 3회 교대 측정, 마지막 해제 구간까지 합계 검증 |
| 불필요 파일 정리·GitLab 이전 | [정리 근거](../development/CLEANUP.md), [이전 기록](../MIGRATION.md) | 완료 |
| 확정 외형으로 영웅 선택 파츠 로딩 | [용어·구현·실측·검증 및 실제 인게임 화면](HERO_SELECTED_PARTS.md), [구조도](../diagrams/hero-selection/README.md) | 선택 외형과 공통 본·부모의 변환 행렬만 보관, 모든 clip/keyframe 유지. 전체/선택 각 3회 측정·행렬 1,576,969개 일치·Debug/Release 검증, Private commit 904.36MiB 감소 |

현재 첫 번째 상세 사례는 메모리 최적화다. 후속 작업에서도 같은 형식으로 문서를 추가하고 이 인덱스를 갱신한다. 과거 수치 근거를 덮어쓰지 않고 새로운 측정 시나리오나 개선 단계별로 날짜가 다른 근거 파일을 추가한다.
