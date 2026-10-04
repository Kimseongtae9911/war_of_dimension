# War Of Dimension 에이전트 작업 지침

Codex, Claude Code, Gemini, Antigravity 및 다른 LLM 에이전트의 공통 진입점이다. 프로젝트별 정책을 도구별 파일에 복제하지 않는다.

## 프로젝트

- Windows C++20, Visual Studio 2026/v145, x64.
- 핵심 모듈: DirectX 12/FMOD 클라이언트 `Client/WarOfDimension`, IOCP 게임 서버 `Server/Game_Server`, IOCP 로비 서버 `Server/Lobby_Server`.
- 로컬 개발은 기존 `LOCAL_TEST` 모드와 DB 없는 실행을 기본으로 한다. 운영 DB·블록체인 기능을 검증했다고 간주하지 않는다.

## 필수 읽기

1. [작업 지침 인덱스](docs/guides/INDEX.md)에서 해당 작업 지침을 선택한다.
2. [문서 인덱스](docs/INDEX.md)에서 영향 영역의 문서를 읽는다.
3. 구현·프로젝트 설정·관련 테스트를 확인하고 `tasks/todo.md`에 작업과 검증을 기록한다.

## 공통 원칙

- 사용자 지시 → 이 파일·범위별 AGENTS.md → 확정 문서·계약 → 테스트 → 구현 → 과거 기록 순으로 판단한다.
- 문서와 설명은 한글, 코드 식별자·제품명·경로·프로토콜 이름은 원문으로 쓴다.
- 변경 범위를 작게 유지한다. 원본 인코딩을 확인하고, 수정한 소스는 UTF-8로 저장한다. 바이너리 에셋에는 텍스트 정규화를 적용하지 않는다.
- TCP 패킷 상수·필드 배치·packing과 동시성 경계는 계약이다. 변경 시 호환 영향과 실패·분할 수신·재시도 검증을 포함한다.
- 게임 상태와 결과의 권위는 서버에 있다. 로컬 테스트 우회와 실제 인증·DB 검증을 구분한다.
- Winsock, IOCP, 스레드, 풀 객체와 GPU/FMOD 자원의 소유권·수명을 확인한다. 경고를 숨겨 성공으로 보고하지 않는다.
- 필요한 `.obj`는 NavMesh·HeightMesh 모델 데이터다. 컴파일 산출물과 혼동해 삭제하지 않는다. 모델·텍스처·음원·런타임 DLL·외부 헤더의 참조를 확인한 뒤 정리한다.
- 신규 대용량 바이너리는 Git LFS로 등록한다. 비밀키·토큰·개인 설정·로그·빌드·생성된 compilation database는 Git에 넣지 않는다.
- 파일 삭제는 사용처·대체 파일·복구 근거를 확인하고 정리 기록에 남긴다. 원본 `C:\GitFolder\war_of_dimension`은 수정하지 않는다.
- 현재 GitHub 생성·remote 설정·업로드는 보류 상태다. 사용자의 후속 지시 없이는 실행하지 않는다. 초기 커밋은 사용자 요청에 따라 공통 `commit` 스킬의 미리보기·승인 절차로 진행한다.

## 작업 절차와 검증

- 복잡한 작업은 계획을 기록하고 단계별로 진행한다. 사용자에게 이미 승인된 범위의 통상적인 설정·수정은 계속 진행한다.
- 코드 탐색은 `rg`와 Serena를 사용한다. C++ symbol/reference 조회 전 `compile_commands.json`과 Serena 상태를 확인한다.
- 구조·흐름 변경 시 프로젝트 [Archify 스킬](.agents/skills/archify/SKILL.md)을 사용하고 근거 문서와 JSON/HTML을 함께 관리한다.
- `scripts/Build.ps1`, `scripts/Test-Local.ps1`, `scripts/Test-AgentEnvironment.ps1` 중 영향에 맞는 검증을 실행한다. 구현 검증에 필요한 경우 실패 경로 테스트를 추가한다.
- 완료 보고에는 변경 결과, 실제 검증, 기존 경고와 미검증 범위를 구분한다. 빌드 성공만으로 게임 플레이 전체 검증을 주장하지 않는다.
- 커밋 요청을 받으면 사용자 공통 `commit` 스킬의 한글 메시지·미리보기 절차를 적용한다. 커밋이나 push를 작업 완료와 자동으로 묶지 않는다.
