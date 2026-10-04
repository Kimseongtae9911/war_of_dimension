# 로컬 실행 구성도

- 종류: architecture, 현재 구현을 설명하는 구성도.
- 원본: `runtime.architecture.json`, 산출물: `runtime.architecture.html`.
- Archify 버전: 2.16. 한글 설명을 사용하며 고정 Viewer UI는 영어다.
- 근거: `Client/WarOfDimension/NetworkManager.cpp`의 Initialize/Connect, `Server/Game_Server/CNetworkMgr.cpp`의 Initialize, `Server/Lobby_Server/CNetworkMgr.cpp`의 Initialize/ServerConnect, `Server/Game_Server/protocol.h`의 LOCAL_TEST/포트 상수, `scripts/Start-Local.ps1`.
- 검증: showcase 9/9, composition 오류·경고 0. 1440×900, 1600×1000, 1920×1080, 2048×1320 자동 overflow 검사 통과. 1440×900 light와 2048×1320 dark의 실제 캡처를 확인했다. 자동 receipt의 visualReview는 pending이며, 이 문서의 별도 확인 기록으로 시각 검토를 구분한다.
- 명세 SHA-256: `d5b1579bc31f767c50b6d1840cebf1467676eafb5401fc2b4365e80e980a88ea` (1,412 bytes).
- HTML SHA-256: `7857d4d1b8efef149f8e29b3d0667fa896d5fb6529f8f8acaaf7bb4bbb4dfad1` (701,146 bytes).

저장소 루트에서 재생성한다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/local-runtime/runtime.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/local-runtime/runtime.architecture.json docs/diagrams/local-runtime/runtime.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/local-runtime/runtime.architecture.html --json
```

TCP 연결 시작 방향을 표시하며 응답은 같은 연결을 통해 반대 방향으로 오간다. DB 없는 개발 모드를 표현한다. 실제 DB 연동과 전체 매칭·전투 흐름을 검증한 그림은 아니다.
