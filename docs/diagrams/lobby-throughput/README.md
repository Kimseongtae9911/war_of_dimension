# 로비 단일 채널 지형 검색 개선

Archify 2.16, workflow v2. 본문은 한글이며 고정 Viewer UI와 HTML lang은 영어 fallback이다.
최종 구현·실측은 [보고서](../../development/LOBBY_THROUGHPUT.md), 작업·검증은 [todo 94번](../../../tasks/todo.md)을 따른다.

## 코드 근거

- `Server/Lobby_Server/CServer.cpp`: IOCP 4개와 로직 소비자 1개, 상세 프로파일 선택 활성화.
- `Server/Lobby_Server/CClient.cpp`: 기존 이동 간격·재등록·패킷 전송 유지, Move frame 비용 측정.
- `Server/Lobby_Server/GameUtil.cpp`: 최초 로드 후 공간 인덱스 구성, NavMesh/HeightMesh ray에 인덱스 전달.
- `Server/Game_Server/TerrainSpatialIndex.h`, `MathUtil.h`: 기존 공유 중인 읽기 전용 인덱스/정적 계산 캐시, 순서 보존·선형 fallback.
- `Server/ServerCore/tests/TerrainTests.cpp`: 실제 게임/로비 OBJ의 첫 교차·float 높이·node와 병렬 읽기 등가 검사.

화살표 중 유저 작업→이동량 계산은 다음 실행 단계라는 의미가 양 끝점에 드러나 별도 라벨을 생략했다. 나머지는 좌표·후보·결과의 의미를 표시한다.

## 재생성

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/lobby-throughput/lobby.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/lobby-throughput/lobby.workflow.json docs/diagrams/lobby-throughput/lobby.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/lobby-throughput/lobby.html --json
```

최종 validate/deliver: showcase9/9·오류0·경고0. receipt는 `artifacts/logs/lobby-throughput-diagram-receipt.json`에 보존했다.
JSON SHA-256 `593f1d2fd5a40333868516aa712aa97498c6a3a0f7cf351b583d31d98b2f02fe`, HTML SHA-256 `d0809206eb140df2dfddb5d0572225cbeed1d00b3224065fd61f678811bc0f15`.
로컬 파일 접근 차단 이력으로 visual-check·전체 HTML 브라우저 시각 점검은 수행하지 않았다. 자동 검증은 실제 시각 점검의 대체가 아니다.
