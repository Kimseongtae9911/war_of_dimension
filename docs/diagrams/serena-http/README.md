# 프로젝트별 Serena HTTP 연결 구조

현재 구현은 [에이전트 환경](../../development/AGENTS.md#프로젝트별-codex-serena-http-서버)을 따른다.
근거는 `.codex/config.toml`, `.serena/codex-http.yml`, `scripts/Serena.ps1`, `scripts/serena_http.py`, `scripts/Test-SerenaHttp.py`다.

NewWod 채팅은 같은 9120 서버와 활성 프로젝트를 재사용한다. 다른 8개 로컬 프로젝트는 9121~9128의 별도 서버로 분리한다. 포트 배정과 공용 실행 도구는 에이전트 환경 문서를 참조한다.
로그인 시작 바로가기는 사용자 설정이므로 Git에 넣지 않는다. CLI 기본 HTTP의 연결별 agent 종료를 래퍼가 서버 종료로 분리한다.

Archify 2.16을 사용했다. JSON은 수정 원본이며 HTML은 생성 결과다. 설명은 한글이고 고정 Viewer UI/HTML lang은 영어다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/serena-http/runtime.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/serena-http/runtime.architecture.json docs/diagrams/serena-http/runtime.architecture.html --quality showcase --json
node .agents/skills/archify/bin/archify.mjs visual-check docs/diagrams/serena-http/runtime.architecture.html --json
```

2026-10-11 검증: showcase 9/9, composition 오류/경고 0. 1440×900·1600×1000·1920×1080·2048×1320 containment PASS.
자동 visualReview는 pending이며 최소/최대 크기 light/dark PNG 4장을 별도로 직접 점검했다. 화면과 자동 receipt는 Git 제외 `artifacts/logs/serena-http/visual-check/`에 보관한다. 실제 MCP/프로세스 검증과 Windows 재로그인 미검증 범위는 `tasks/todo.md` 67번에 기록한다.

- JSON SHA-256: `0649f035746f1e5ef8551f9868f3850fa1a7181b4708a0aba7a92c1b3405e180` (1,900 bytes)
- HTML SHA-256: `10342b7d65a6613c1bda856b8dfd70441f1bb05300bbe75d3d7e5cdaaad3de9d` (704,942 bytes)
