# 에이전트 개발 환경

이 디렉터리의 설명은 VAFK Server의 `AGENTS.md` → 작업 지침 인덱스 → 영역 문서 구조를 이 C++ 프로젝트에 맞춰 적용한 결과다.

## 공통 지침과 MCP

- `AGENTS.md`를 Codex·Claude Code·Gemini/Antigravity가 공유한다.
- `CLAUDE.md`, `GEMINI.md`는 공통 지침으로 연결하는 진입점이다.
- Codex는 `.codex/config.toml`, Claude Code는 `.mcp.json`, Gemini/Antigravity는 `.gemini/settings.json`의 Serena 설정을 사용한다.
- 설치된 `serena-agent` 1.2.0, Node.js를 사용한다. 이 컴퓨터에 이미 설치된 `serena` CLI를 재사용한다. 다른 컴퓨터에서는 해당 도구를 먼저 설치해야 한다.
- 다른 프로젝트의 DB 규칙·비밀·Notion 서버·전용 도구는 복제하지 않았다. 공통 문서·도구 구조만 이 프로젝트에 맞췄다.

현재 열린 에이전트 세션이 프로젝트 설정을 다시 읽어야 MCP 도구가 나타날 수 있다. 저장소 루트에서 새 세션을 시작하거나 클라이언트의 MCP 서버 재연결 기능을 사용한다. CLI 점검 성공과 현재 채팅에 MCP 도구가 로드된 상태는 구분한다.

## C++ 심볼 탐색

```powershell
./scripts/New-CompilationDatabase.ps1
./scripts/Test-AgentEnvironment.ps1
serena project index-file Server/Game_Server/CServer.cpp .
```

`compile_commands.json`은 프로젝트의 `ClCompile` 목록, C++20, configuration, MSVC·Windows SDK include 경로로 생성한다. PCH와 unity build의 중간 파일을 조회 대상으로 사용하지 않는다. 새 소스를 프로젝트에 추가하거나 toolset·SDK·설정을 바꾸면 다시 생성한다. 컴퓨터별 절대 경로를 포함하는 생성 파일은 Git에서 제외한다.

Serena의 C++ 지원은 [공식 C/C++ 설정 안내](https://oraios.github.io/serena/03-special-guides/cpp_setup.html)에 따라 compilation database를 사용한다. `.serena/project.yml`에서 clangd 22.1.6을 고정하고 Serena가 관리하도록 했다. vendor 헤더·에셋·실험 프로젝트·빌드 결과는 기본 symbol index에서 제외하되, 필요하면 파일 검색으로 별도 조사한다.

기존 일부 클라이언트 소스는 CP949였고 서버·다른 클라이언트 소스는 UTF-8이었다. VS2026 이전 단계에서 핵심 모듈의 비 UTF-8 텍스트 파일을 엄격하게 변환·검증하여 에이전트와 컴파일러의 해석을 맞췄다.

## 구조 문서 도구

VAFK의 프로젝트 Archify 2.16 패키지를 `.agents/skills/archify`에 같은 버전으로 복사했다. 갱신은 별도 요청으로 수행한다.

```powershell
node .agents/skills/archify/bin/archify.mjs doctor
```

구조·흐름 변경은 [Archify 지침](../guides/archify.md)을 따른다. 현재 로컬 구성은 [런타임 다이어그램](../diagrams/local-runtime/runtime.architecture.html)에 있다. 설명은 한글이고 고정 Viewer UI는 영어다.
