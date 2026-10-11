# 에이전트 개발 환경

이 디렉터리의 설명은 VAFK Server의 `AGENTS.md` → 작업 지침 인덱스 → 영역 문서 구조를 이 C++ 프로젝트에 맞춰 적용한 결과다.

## 공통 지침과 MCP

- `AGENTS.md`를 Codex·Claude Code·Gemini/Antigravity가 공유한다.
- `CLAUDE.md`, `GEMINI.md`는 공통 지침으로 연결하는 진입점이다.
- Codex는 `.codex/config.toml`, Claude Code는 `.mcp.json`, Gemini/Antigravity는 `.gemini/settings.json`의 Serena 설정을 사용한다.
- 설치된 `serena-agent` 1.2.0, Node.js를 사용한다. 이 컴퓨터에 이미 설치된 `serena` CLI를 재사용한다. 다른 컴퓨터에서는 해당 도구를 먼저 설치해야 한다.
- 다른 프로젝트의 DB 규칙·비밀·Notion 서버·전용 도구는 복제하지 않았다. 공통 문서·도구 구조만 이 프로젝트에 맞췄다.

현재 열린 에이전트 세션이 프로젝트 설정을 다시 읽어야 MCP 도구가 나타날 수 있다. 저장소 루트에서 새 세션을 시작하거나 클라이언트의 MCP 서버 재연결 기능을 사용한다. CLI 점검 성공과 현재 채팅에 MCP 도구가 로드된 상태는 구분한다.

## 프로젝트별 Codex Serena HTTP 서버

2026-10-11부터 NewWod의 Codex는 `http://127.0.0.1:9120/mcp`의 지속 실행 서버에 연결한다. 채팅마다 Serena/clangd를 새로 실행하는 STDIO 설정을 대체했다. 서버 시작 시 이 저장소를 `--project`로 활성화하고 `.serena/codex-http.yml`에서 `activate_project`를 제외해 다른 채팅이 활성 프로젝트를 바꾸지 못하게 한다. `get_current_config`는 상태 확인용으로 유지한다. Claude/Gemini의 기존 STDIO 설정은 그대로다.

```powershell
./scripts/Serena.ps1 -Action InstallStartup # 지금 시작하고 현재 사용자 로그인 자동 시작 등록
./scripts/Serena.ps1                       # 이미 실행 중이면 같은 PID 재사용
./scripts/Serena.ps1 -Action Status
./scripts/Test-AgentEnvironment.ps1 -StaticOnly -SerenaHttp
./scripts/Serena.ps1 -Action Stop
./scripts/Serena.ps1 -Action RemoveStartup # 자동 시작 등록만 해제
```

사용자 Startup 폴더의 프로젝트 경로·포트 해시별 바로가기가 숨김 PowerShell을 실행한다. 서버는 `127.0.0.1`에만 바인딩한다. `.runtime/serena-http-9120.json`의 프로젝트·실행 파일·PID·시작 시각이 일치할 때만 재사용/종료하며, 로그는 같은 Git 제외 폴더에 둔다. `Stop`은 해당 서버의 Python worker·clangd 트리를 종료한다. 장애 시 `Stop` → `Start`로 복구한다. PID 불일치나 다른 프로그램의 포트 점유는 실패로 보고하며 그 프로세스를 종료하지 않는다. 자동 시작 등록 후 실제 Windows 재로그인은 별도 검증 대상이다.

Serena 1.2.0과 현재 MCP Python SDK 조합에서는 기본 HTTP 서버의 세션 lifespan 종료가 공유 agent/clangd까지 종료한다. `scripts/serena_http.py`는 도구를 한 번 등록하고 agent 정리를 HTTP 서버 종료로 옮긴다. 설치 패키지를 수정하지 않으며 Serena의 내부 `_set_mcp_tools`에 의존하므로 패키지 업데이트 후 위 HTTP 검사를 다시 실행한다. 검사에는 두 독립 세션·프로젝트 전환 거부·전체 연결 종료 뒤 재연결/실제 C++ 심볼 조회가 포함된다.

다른 프로젝트에는 **서로 다른 포트와 각 프로젝트의 `.codex/config.toml` URL**을 사용한다. 2026-10-11 사용자 요청으로 현재 컴퓨터의 로컬 프로젝트에도 같은 방식을 적용했다. 공용 실행 도구는 `%USERPROFILE%/.codex/tools/serena-http/`에 있고 NewWod 폴더에 의존하지 않는다. 등록 원본은 공용 폴더의 `projects.json`이다.

| 프로젝트 | 포트 | 활성 프로젝트 |
|---|---:|---|
| NewWod | 9120 | war_of_dimension |
| vafk_client | 9121 | vafk_client |
| vafk_server | 9122 | vafk_server |
| RunningApp | 9123 | RunningApp |
| WebPopup | 9124 | WebPopup |
| MazeRouge | 9125 | MazeRouge |
| TradeApp | 9126 | TradeApp |
| TradeWebsite | 9127 | TradeWebsite |
| VampireAFK | 9128 | VampireAFK |

공용 도구는 프로젝트별 로그인 시작을 등록하고 포트별 재사용·초기화 순서 잠금을 적용한다. 언어 서버가 준비된 뒤 HTTP listener를 연다. 기존 MCP의 다른 서버는 유지한다. vafk_server·VampireAFK의 기존 빈 언어 목록도 유지하므로 두 프로젝트의 심볼 지원은 별도 언어 설정이 필요하다. 다른 프로젝트의 새 로그/실행 기록은 `%LOCALAPPDATA%/SerenaHttp/`에 두며 NewWod는 기존 `.runtime/`을 유지한다. 대시보드는 공용 래퍼에서 비활성화한다.

```powershell
$manager = "$env:USERPROFILE/.codex/tools/serena-http/Manage-Projects.ps1"
& $manager -Action Status
& $manager -Project WebPopup -Action Start
& $manager -Project WebPopup -Action Stop
```

각 프로젝트의 `.codex/config.toml`에서 Serena의 기존 `command`/`args`를 URL로 대체했다. 전역 Codex 설정에 특정 프로젝트 URL을 넣지 않는다. 서버가 살아 있는 동안 프로젝트 활성화는 한 번만 수행하며 채팅의 `initial_instructions` 호출은 도구 사용 안내를 읽는 과정이다. 기존 STDIO 채팅의 프로세스는 해당 채팅이 종료/재연결될 때까지 남을 수 있다.

연결 구조와 근거는 [Serena HTTP 구조도](../diagrams/serena-http/README.md)에 있다. HTTP 연결과 비활성화 설정은 [OpenAI 공식 MCP 문서](https://learn.chatgpt.com/docs/extend/mcp?surface=cli)를 따른다.

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
