# 개발 환경 정비 작업

사용자 요청 순서대로 진행한다. GitHub 저장소 생성·remote 설정·업로드는 보류한다. 초기 커밋은 후속 사용자 요청에 따라 공통 commit 스킬 절차로 준비한다.

## 1. 로컬 실행 설정

- [x] 기존 의존성 TBB 4.2.3.1 복원과 핵심 모듈 Debug x64 빌드 확인
- [x] 누락된 실행 리소스 확인 및 복원
- [x] 빌드·시작·중지·실행 점검 스크립트 구성
- [x] 로비·게임 서버 연결과 클라이언트 시작 확인

## 2. 에이전트 개발 환경

- [x] 다른 프로젝트의 공통 에이전트 지침·문서 인덱스·도구 설정 비교
- [x] AGENTS.md와 Claude/Gemini 진입점, 작업·문서 지침 구성
- [x] 로컬 코드 탐색 도구와 검증 명령 설정 및 점검

## 3. Visual Studio 2026

- [x] 프로젝트·솔루션을 v145/VS18 기준으로 명시적으로 이전
- [x] x64 Debug/Release 빌드 및 마이그레이션 후 실행 점검
- [x] 변경·검증·기존 경고 문서화

## 4. 파일 정리

- [x] 프로젝트 참조·런타임 사용·원본 복구 가능성을 근거로 파일 분류
- [x] 개인 IDE 파일·기존 빌드 산출물·중복 백업 등 확인된 불필요 파일 제거
- [x] 필요한 모델·텍스처·외부 라이브러리·실험 코드 보존 여부 기록
- [x] 정리 후 빌드·실행 점검
- [x] 최종 Git/LFS 및 초기 커밋 후보 확인

## 현재 확인

- 설치된 Visual Studio는 2026(18.10.12217.157), C++ toolset은 v145다. 초기 기준선 확인 후 프로젝트·솔루션을 이전했고 현재 빌드는 프로젝트의 v145 설정을 직접 사용한다.
- 원본은 `LOCAL_TEST`가 이미 켜져 있고 `WITH_DATABASE`는 꺼져 있다. 우선 원본의 DB 없는 로컬 개발 모드로 실행한다. 실제 DB 연동과 운영 배포 검증은 이 단계에 포함되지 않는다.
- TBB는 최초 기준선 복원에 사용했으나 실제 소스·링크·런타임에서 미사용임을 확인하여 정리했다. 현재 빌드는 TBB 패키지 복원 없이 통과한다.
- 정리 후 `scripts/Build.ps1 -Configuration Debug -Rebuild`, `scripts/Build.ps1 -Configuration Release -Rebuild`: 핵심 3개 모듈 각각 PASS. 루트 `NewWod.sln` 직접 Debug x64 빌드도 PASS.
- 정리 후 `scripts/Test-Local.ps1 -Configuration Debug`, `scripts/Test-Local.ps1 -Configuration Release`: listener, 게임 서버가 로비에 연결한 TCP 세션, 클라이언트 게임 창 확인 PASS. 점검 프로세스는 모두 중지했다.
- `scripts/Test-AgentEnvironment.ps1`: 220개 C++ 소스 compilation database와 경로 검사 PASS, Serena 핵심 3개 파일 심볼 조회 PASS, Archify doctor PASS. 현재 에이전트 세션의 MCP 재연결은 별도다.
- 핵심 비 UTF-8 텍스트 20개를 UTF-8로 변환했다. 실제 문자열과 게임 로직은 유지한다. 기존 narrowing·signedness 등 빌드 경고는 후속 리팩토링 대상으로 기록했다.
- 로컬 런타임 다이어그램의 schema·deliver 검증과 light/dark 화면 크기별 overflow 점검 PASS. 1440×900 light, 2048×1320 dark 결과를 직접 확인했다. 의존성 정리는 연결 구조를 바꾸지 않아 검증된 JSON/HTML을 재생성하지 않았다.
- 불필요 바이너리·압축 백업·개인 설정·빌드 캐시 20개 항목, 148.7MiB를 저장소 밖 `C:\GitFolder\NewWodCleanupBackup\20261004`에 보관했다. 필요한 HeightMesh를 복원하고 모델·텍스처·FMOD·외부 헤더·실험 코드와 자료는 보존했다.
- 최종 초기 커밋 후보는 17,500개 파일이다. 일반 Git blob 중 100MiB 초과 없음, LFS 11개 pointer·실제 에셋 SHA-256/크기·로컬 object 존재 확인 PASS. 생성 로그·캐시·compilation database와 정리 대상은 제외했다. 원본 checkout은 clean이며 새 저장소 main의 초기 커밋·remote·GitHub 업로드는 없다.
- `.gitignore`의 모델 `.obj` 예외를 일반 `*.obj` 규칙 뒤에 두어 신규 HeightMesh와 향후 필수 리소스가 실제로 추적되도록 확인했다. 설정·문서·스크립트 whitespace와 PowerShell 구문, 인코딩 변환 20개 파일의 원문 문자열 일치 확인 PASS.

## 5. 초기 커밋

- [x] 사용자 요청에 따라 기존 최신 코드·필수 에셋·개발 환경 전체를 초기 커밋 범위로 준비
- [ ] 정확한 메시지·staged snapshot 미리보기 후 사용자 승인
- [ ] 승인된 snapshot으로 초기 커밋 생성 및 작업 트리 확인

GitHub 업로드·remote 설정은 이 요청에 포함하지 않는다. 앞선 빌드·로컬 실행·에이전트 검증 결과를 사용하며 커밋 준비 중 게임 소스나 실행 설정은 변경하지 않는다.

초기 snapshot 전체의 `git diff --cached --check`는 원본·복사된 Archify 코드 6,286개 파일의 기존 공백 문제 80,500건으로 exit 2다. 원본 또는 복사된 패키지 외의 경로에서는 문제가 없고, 새 개발 환경·문서·스크립트 범위 검사는 PASS다. 초기 이전에서 원본·vendor 코드 전체를 공백 정규화하지 않는다. 결과는 Git에서 제외되는 `artifacts/logs/initial-commit-whitespace.txt`에 남겼다. 커밋 작성자 설정과 일반 staged blob의 100MiB 제한도 확인했다.
