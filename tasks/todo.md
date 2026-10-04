# 개발 환경 정비 작업

사용자 요청 순서대로 진행한다. 1~5는 초기 이전·개발 환경·커밋의 기록이고, 6은 현재 통합 솔루션·메시 공유 구현이다. 초기 커밋은 사용자 요청으로 GitHub에 업로드했으며 현재 구현의 추가 커밋·push는 별도 요청에 따라 진행한다.

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
- 최종 초기 커밋 후보는 17,500개 파일이다. 일반 Git blob 중 100MiB 초과 없음, LFS 11개 pointer·실제 에셋 SHA-256/크기·로컬 object 존재 확인 PASS. 생성 로그·캐시·compilation database와 정리 대상은 제외했다. 당시 원본 checkout은 clean이었고 새 저장소 main의 초기 커밋·remote·GitHub 업로드 전이었다. 후속 완료는 5번에 기록했다.
- `.gitignore`의 모델 `.obj` 예외를 일반 `*.obj` 규칙 뒤에 두어 신규 HeightMesh와 향후 필수 리소스가 실제로 추적되도록 확인했다. 설정·문서·스크립트 whitespace와 PowerShell 구문, 인코딩 변환 20개 파일의 원문 문자열 일치 확인 PASS.

## 5. 초기 커밋

- [x] 사용자 요청에 따라 기존 최신 코드·필수 에셋·개발 환경 전체를 초기 커밋 범위로 준비
- [x] 정확한 메시지·staged snapshot 미리보기 후 사용자 승인
- [x] 승인된 snapshot으로 초기 커밋 생성 및 작업 트리 확인

후속 사용자 요청으로 초기 커밋 `ed3428e`와 GitHub 공개 저장소 `Kimseongtae9911/war_of_dimension`의 main push를 완료했다. LFS 11개 원격 객체 및 로컬·원격 HEAD 일치, clean 작업 트리를 확인했다. 아래 snapshot 공백 검사는 초기 이전 당시의 기록이다.

초기 snapshot 전체의 `git diff --cached --check`는 원본·복사된 Archify 코드 6,286개 파일의 기존 공백 문제 80,500건으로 exit 2다. 원본 또는 복사된 패키지 외의 경로에서는 문제가 없고, 새 개발 환경·문서·스크립트 범위 검사는 PASS다. 초기 이전에서 원본·vendor 코드 전체를 공백 정규화하지 않는다. 결과는 Git에서 제외되는 `artifacts/logs/initial-commit-whitespace.txt`에 남겼다. 커밋 작성자 설정과 일반 staged blob의 100MiB 제한도 확인했다.

## 6. 통합 솔루션 및 메시 공유

- [x] 루트 통합 솔루션을 기본 빌드로 사용하고 서버 전용 solution filter 구성
- [x] 이름과 transform을 제외한 geometry 내용 기반 캐시 및 공유 소유권 구현
- [x] 스킨드 geometry 공유와 본·애니메이션 상태의 분리, UI/파티클 독립 유지
- [x] D3D12 공유·변경 데이터·device 경계·해제 순서 및 실제 에셋 중복 검증
- [x] 통합 Debug/Release 빌드·로컬 실행·Serena와 구조 문서 점검

기존 루트 솔루션에는 이미 핵심 3개 프로젝트가 포함되어 있었다. 개별 솔루션을 호출하던 빌드 경로를 루트로 통합하고 Servers·Client·Development 폴더, 서버 filter, 공유 실행 프로필을 추가했다. 파일·에셋의 디스크 위치와 개별 솔루션은 유지한다. Visual Studio 프로필 GUI 실행은 직접 점검하지 않았으며 서버 준비를 보장하는 Start-Local 절차를 함께 문서화했다.

프레임마다 geometry를 재생성하던 로더를 device·SHA-256·내용 길이 기반 캐시로 변경했다. weak_ptr 캐시, 객체의 공유 소유권, atomic 참조 수, 스킨드 base alias 소유권으로 해제 순서를 처리한다. 본 index/weight·bind-pose·프레임 연결은 공유하지 않는다. 메시 이름만 해시에서 제외하며 transform·material은 기존 프레임 상태를 유지한다. GPU 업로드 fence 동기화는 호출자가 담당한다.

- `Build.ps1 -Configuration Debug`, `Build.ps1 -Configuration Release`: 루트 솔루션의 핵심 3개 모듈 PASS. `Build.ps1 -Configuration Debug -Module Servers`: 서버 filter PASS.
- `Test-MeshSharing.ps1` Debug·Release: 실제 D3D12 WARP·하드웨어에서 12개 검사 PASS. created 8, reused 4, 종료 live 0. 동시 최초 로딩·자기 핸들 재지정·마지막 소유자·만료 후 재생성 포함.
- `Test-MeshSharing.ps1 -AuditAssets` Debug·Release: 실제 base geometry GPU 생성/재사용 PASS. LobbyScene_No 291→38개, Plane1 5,185→78개, ModularModel 724→698개. ModularModel의 이름이 다른 중복 26개 확인. 전체 장면 렌더링·FPS·실측 VRAM은 측정하지 않았다.
- `Test-Local.ps1` Debug·Release: listener·서버 간 연결·클라이언트 창 시작 PASS, 테스트 프로세스 중지. DB·로그인·전투 전체는 미검증이다.
- `Test-AgentEnvironment.ps1`: 222개 소스 compilation database·경로, Serena Game 5/Lobby 4/Client 18개 심볼, Archify doctor PASS.
- 메시 구조도: architecture showcase 9/9, 오류·경고 0, deliver PASS. 4개 화면 크기의 overflow 검사 PASS, 1440×900 light·2048×1320 dark 직접 시각 점검 PASS. 네트워크 연결은 변하지 않아 기존 local-runtime 구성도는 유지했다. 해시·재생성 근거는 `docs/diagrams/mesh-sharing/README.md`에 있다.
- 최종 PowerShell 9개 구문·프로필/filter JSON 경로·클라이언트 프로젝트 소스 존재·신규 파일 UTF-8/공백·변경 diff 공백 검사 PASS. HEAD와 origin/main은 초기 커밋 그대로이며 실행 중인 검증 프로세스는 0개다. 기존 빌드 경고는 이번 범위에서 일괄 수정하지 않았다.

현재 요청의 구현 변경은 별도 커밋·push 요청 전까지 로컬에서 검토한다.

## 7. 통합 솔루션 SLNX 전환

- [x] 공식 도구로 루트 NewWod.sln을 NewWod.slnx로 변환
- [x] 서버 filter와 빌드·실행 프로필·개발 문서 참조 갱신
- [x] SLNX 통합 Debug/Release 및 서버 filter 빌드, 설정 보존 확인
- [x] 변경된 snapshot으로 커밋 미리보기 갱신

사용자의 커밋·push 요청 이후 미리보기 승인을 기다리던 중 SLNX 전환 요청을 반영한다. 이전 미리보기의 staged tree는 더 이상 최종 snapshot이 아니다. 기존 메시 공유·네트워크·자원 수명 구조는 그대로라 검증된 다이어그램 JSON·HTML은 재생성하지 않는다.

`dotnet sln NewWod.sln migrate`로 변환한 뒤 `Build.ps1 -Configuration Debug`, `Build.ps1 -Configuration Release`, `Build.ps1 -Configuration Debug -Module Servers` PASS. 서버 filter 로그에 클라이언트가 포함되지 않는 것을 확인했다. XML의 프로젝트 3개·x64 플랫폼·개발 파일 경로를 기존 솔루션과 비교하고 launch JSON 내용 보존을 확인했다. 기존 루트 `.sln`은 `artifacts/logs/NewWod-before-slnx.sln`에 로컬 백업하고 제거했다. launch 파일은 같은 내용의 `.slnxLaunch`로 이전했다. 개별 모듈 솔루션은 보존한다. Visual Studio GUI의 프로필 실행은 여전히 미검증이다.
