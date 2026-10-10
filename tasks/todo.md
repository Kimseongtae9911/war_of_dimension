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

이 단계는 로컬에서 검토한 뒤 후속 요청으로 7번과 함께 커밋·push를 완료했다.

## 7. 통합 솔루션 SLNX 전환

- [x] 공식 도구로 루트 NewWod.sln을 NewWod.slnx로 변환
- [x] 서버 filter와 빌드·실행 프로필·개발 문서 참조 갱신
- [x] SLNX 통합 Debug/Release 및 서버 filter 빌드, 설정 보존 확인
- [x] 변경된 snapshot으로 커밋 미리보기 갱신

사용자의 커밋·push 요청 이후 미리보기 승인을 기다리던 중 SLNX 전환 요청을 반영한다. 이전 미리보기의 staged tree는 더 이상 최종 snapshot이 아니다. 기존 메시 공유·네트워크·자원 수명 구조는 그대로라 검증된 다이어그램 JSON·HTML은 재생성하지 않는다.

`dotnet sln NewWod.sln migrate`로 변환한 뒤 `Build.ps1 -Configuration Debug`, `Build.ps1 -Configuration Release`, `Build.ps1 -Configuration Debug -Module Servers` PASS. 서버 filter 로그에 클라이언트가 포함되지 않는 것을 확인했다. XML의 프로젝트 3개·x64 플랫폼·개발 파일 경로를 기존 솔루션과 비교하고 launch JSON 내용 보존을 확인했다. 기존 루트 `.sln`은 `artifacts/logs/NewWod-before-slnx.sln`에 로컬 백업하고 제거했다. launch 파일은 같은 내용의 `.slnxLaunch`로 이전했다. 개별 모듈 솔루션은 보존한다. Visual Studio GUI의 프로필 실행은 여전히 미검증이다.

후속 사용자 승인으로 6~7은 커밋 bf7b4cee와 GitHub main push를 완료했다.

## 8. 클라이언트 메모리 분석 및 포트폴리오 기록

- [x] 인게임 실제 로딩 경로·영웅 모델/애니메이션/컨트롤러 소유권 확인
- [x] 에셋 CPU 배열·D3D12 버퍼 정렬·애니메이션 메모리 재현 계산 도구 구성
- [x] 개선 전 로딩 방식과 현재 방식의 동일 시나리오 메모리 측정
- [x] 영웅 1개 모델과 인게임 전체 결과, 절감 범위·남은 원인·재현 절차 문서화

개선 전 참조는 초기 커밋 ed3428e, 메시 공유 적용 기준은 bf7b4cee다. 전체 프로세스 실측·GPU segment usage·계산한 배열/버퍼 용량·직렬화 크기를 구분한다. 측정용 분기는 명시적 CLI 프로세스에서만 활성화하며 일반 실행 동작은 유지한다. 과거 로더를 현재 바이너리에 재현한 A/B 측정은 과거 commit의 바이너리 자체 측정과 구분해 표시한다.

2026-10-05 KST, RTX 4070 SUPER의 Release x64에서 각 3개 독립 프로세스로 A/B 측정 완료. 실제 Title→INGAME 자원 생성·GPU 완료 대기·기존 upload 해제 경로를 사용하고 네트워크/로딩 렌더 스레드/전투 프레임은 제외했다. 진입 시 private commit 중앙값 6,951.54→5,768.31MiB(17.02%), Working Set 2,867.15→2,105.12MiB(26.58%), DXGI LOCAL 4,069.55→3,662.88MiB(9.99%). 지표를 합산하지 않는다.

실제 모델 16종/18회 로딩을 끝까지 파싱했다. ModularModel 1회의 애니메이션 행렬은 6,947 keys×788 frames×64bytes=334.12MiB이며 이번 변경으로 줄지 않는다. 현재 영웅 슬롯의 model hierarchy는 2회 로딩, 컨트롤러는 local 중복 슬롯을 포함해 4개다. 첫 로딩 CPU 배열 계산은 377.64→376.99MiB, 두 번째 영웅 모델 로딩 구간의 private commit 증가량은 560.71→426.53MiB다. 화면의 영웅 수로 독립 모델 비용을 곱하지 않는다.

Release/Debug 클라이언트 빌드, 두 구성의 MeshSharing 12개 검사·실제 3개 모델 GPU 감사, 로컬 listener·서버 연결·클라이언트 창 시작 PASS. 빌드의 기존 narrowing/signedness 경고는 유지하며 전체 전투·FPS·장시간 누수·다른 boss·로그인에서 이어진 진입은 미검증이다. Serena Object.cpp 심볼 조회 PASS(194개), compilation database 223개 소스. 측정은 공유 구조를 바꾸지 않아 기존 Archify 구조도를 재생성하지 않았다.

문서: `docs/portfolio/CLIENT_MEMORY_OPTIMIZATION.md`. 공개용 수치 근거 JSON/CSV와 PNG/SVG 2종을 작성했고, 문제·선택한 설계·영웅 비용·재현 절차·후속 우선순위를 기록했다. 그래프는 matplotlib 3.11.2로 생성 후 직접 시각 점검했다. 일반 raw 로그와 패키지는 artifacts 아래에 있어 Git에서 제외된다. 이번 계측·문서 변경은 별도 요청 전까지 커밋·push하지 않는다.

최종 검증: 6회 완료/16종/18회 일치, Python 계산의 Plane1·ModularModel 레코드/unique 수와 네이티브 Release GPU 감사 일치, 영웅 행렬 공식·측정 소스 SHA-256 일치, parser의 잘린 token/음수 count 거부 PASS. 잘못된 측정 CLI mode는 exit 2로 종료 PASS. PowerShell 전체 구문, 프로젝트/filter 신규 파일 등록, 신규 텍스트 UTF-8·공백·크기, 포트폴리오 상대 링크, 변경 diff 공백 PASS. 실행 중인 검증 프로세스 0개다.

후속 맵 로딩 질문에 따라 동일 문서에 자원 목록을 추가했다. Plane1은 전체 hierarchy 5,594 frames/5,185 mesh records, 고유 geometry 78개, map DDS 2종의 픽셀 payload 합 8MiB다. object/material CB는 공유하지 않으며 작은 요청이 다수 committed buffer로 분리되는 비용을 구분해 기록했다. Scene 초기화는 미니언 12개·몬스터 7종/9개·boss·스킬 풀·파티클 texture 15종을 함께 만든다. Minotaur texture 픽셀 payload 240MiB, 파티클 객체당 300,000×32×2=18.31MiB 대형 DEFAULT payload를 확인했다. 실제 전체 파티클 수와 비용별 residency는 아직 계측하지 않았다. 기존 A/B raw checkpoint의 맵 로딩 구간 증가량만 추가 분석하고 기존 측정 근거는 변경하지 않았다. Serena Scene.cpp 79개 심볼 조회 PASS. 게임 로직 변경이 없는 문서/에셋 분석이므로 빌드·실행을 반복하지 않는다.

## 9. 현재 클라이언트 메모리의 구간별 분해

- [x] 맵 전체 생성·파티클 풀·캐릭터·UI/공통 자원의 checkpoint 추가
- [x] 실제 파티클 개수·버퍼 descriptor/할당 정보 수집 및 현재 Release 독립 프로세스 3회 측정
- [x] 구간 증가량의 합계/종료 시 감소와 실제 Working Set·DXGI 차이를 검증
- [x] 기존 근거를 보존하고 비용 분해·맵의 기여·최적화 우선순위 문서화

앞선 동일 시나리오의 5,768MiB private commit(약 5.63GiB)을 기준으로 비용을 분해한다. 사용자가 확인한 별도 UI의 메모리 항목을 직접 읽은 것은 아니다. 이 단계는 비용 분석이며 리소스 용량/로딩 정책 최적화는 적용하지 않는다. loading render thread/네트워크를 제외한 동일 시나리오에서 구간을 세분화하고 계측 분기 외 일반 동작은 유지한다.

Release 독립 프로세스 3회 평균 private commit 5,766.11MiB/Working Set 2,104.25MiB/LOCAL 3,662.88MiB. 파티클 131개(전체 스킬 풀 100+선택 효과 12+환경 19)의 대형 buffer payload 2,398.68MiB, 실제 descriptor의 할당 정보 합 2,407.13MiB. 파티클 구간 private commit 2,420.57MiB(41.98%)/LOCAL 2,408.15MiB/Working Set 7.60MiB를 확인했다. 현재 맵 전체 구간 142.32MiB(2.47%), 과거 동일 범위 비교 중앙값은 1,449.34→142.00MiB다. 8번 에셋 분석에서 미계측이던 파티클 수를 이번에 확정했다.

영웅 구간 private commit 1,088.22MiB/Working Set 998.02MiB 및 애니메이션 행렬 2회 중복 668.24MiB를 확인했다. NPC 구간 948.46MiB, 공통 자원 631.60MiB, UI/dissolve 300.97MiB. 기존 upload 해제 순회의 전용 컨테이너 누락을 후속 후보로 기록했다. 8192² shadow map은 논리 texel 용량 256MiB로 구분했다. 비용 구간 차이를 최종 retained heap 소유권이나 CPU/GPU 합산으로 표현하지 않는다.

Release/Debug 클라이언트 빌드, 양 구성의 MeshSharing 12개 검사·실제 3개 에셋 GPU 감사·일반 로컬 시작 PASS. Serena Scene.cpp 79개 심볼 PASS. 수집값의 byte 단위 합계, 3회 파티클 count/용량/stride 일치, 현재 소스 9개·보관 과거 소스 11개 및 실행 파일 SHA-256 일치, 보고서 재생성 일치, 입력 실패 경로 4종 PASS. 그래프 직접 점검 완료. 기존 A/B 근거를 보존하고 별도 JSON/CSV/PNG/SVG 및 `docs/portfolio/CLIENT_MEMORY_BREAKDOWN.md`로 기록했다. 계측은 자원 소유권·로딩 순서를 바꾸지 않아 기존 검증된 Archify 구조도를 재생성하지 않는다. 커밋·push는 진행하지 않았다.

## 10. 메시 공유 전후의 구간별 메모리 비교

- [x] 동일 세부 checkpoint 바이너리에서 legacy/shared 각 3회 교대 측정
- [x] 구간별 private commit·Working Set·DXGI 전후 통계 및 합계 검증
- [x] 기존 근거를 보존하고 비교 JSON/CSV·그래프·포트폴리오 문서 추가

사용자의 후속 요청에 따라 현재 바이너리의 기존 legacy geometry 분기로 메시 공유 전 로딩 방식을 재현한다. 과거 commit 실행 파일 자체를 측정했다고 표현하지 않는다. 네트워크·전투 제외 조건과 모델/파티클 시나리오를 같게 유지하고 새로운 최적화는 적용하지 않는다. 기존 현재 모드 3회와 별도 날짜의 과거 3회를 섞지 않고 이번에는 동일 바이너리로 양 모드를 교대 실행한다.

2026-10-05 KST, Release RTX 4070 SUPER에서 legacy/shared 각 3회 교대 실행 완료. 구간 private commit 평균은 파티클 2,419.96→2,422.04MiB, 맵 1,448.13→142.62MiB, 영웅 1,223.24→1,089.20MiB, NPC 949.93→950.18MiB였다. 파티클 131개·용량·stride·대형 buffer Width 및 할당 정보는 양 모드가 같았다. GPU 제출·대기·기존 upload 해제 구간 private commit −263.63→0.25MiB까지 포함해 최종 평균 6,941.19→5,770.25MiB, 순감소 1,170.94MiB(16.87%)를 확인했다. Working Set 2,859.86→2,104.74MiB(26.40%), LOCAL 4,069.55→3,662.88MiB(9.99%). 생성 구간 감소량을 최종 retained 비용 절감으로 단정하지 않는다.

`Build-ClientMemoryBreakdown.py --compare`로 모드별 3회 통계·구간 차이·최종 차이·JSON/CSV 및 PNG/SVG를 생성했다. 기존 최초 A/B와 현재 전용 근거는 보존했다. 실행 파일 SHA-256 cffa1503… 및 계측 소스 9개 일치, 6회 완료/교대 순서, 각 지표 byte 단위 합계/종료 값, 비교 차이 합계, 파티클 동일 시나리오 PASS. 보고서/전체 CLI 재생성 JSON 일치·CSV 11행, 비교 실패 입력 7종 및 충돌 summary CLI 거부 PASS. 기존 단일 모드 보고서 재생성과 과거 보관 소스 11개/실행 파일 해시 검증도 PASS. 상대 링크·UTF-8·공백·크기·그래프 직접 점검 PASS, 잔여 검증 프로세스 0개. 직전 검증과 같은 C++/바이너리이므로 게임 빌드는 반복하지 않았다. 자원 소유권·흐름 변경이 없어 기존 Archify 구조도를 유지한다. 포트폴리오 분석/인덱스와 개발 환경 재현 절차를 갱신했고 커밋·push는 하지 않았다.

## 11. 인게임 확정 외형으로 영웅 선택 파츠 로딩

- [x] 커스터마이징 수신 완료·인게임 진입 스냅샷 및 기본 외형 덮어쓰기 확인/수정
- [x] 선택 파츠의 geometry/skin/material만 생성하고 본·애니메이션 매핑 보존
- [x] 실제 에셋·남녀/서로 다른 외형·fallback·Debug/Release 검증 및 메모리 재측정
- [x] 변경 구조·포트폴리오 기록과 실제 인게임 렌더링 스크린샷 제공

로비/READY 편집 경로는 전체 모델을 유지한다. 인게임의 OtherClient 모델 계층은 기존 공유 구조이므로 참가 영웅의 파츠 합집합, 로컬 모델은 자신의 확정 파츠를 사용한다. 수신 미완료는 전체 로딩 fallback으로 안전하게 처리한다. 최초 A/B 실행 파일/계측 소스를 보존한 뒤 변경하고 기존 메모리 수치를 새 구현 결과로 재해석하지 않는다.

전체 788개 프레임·61개 clip·6,947개 keyframe을 보존하고 미선택 메시/skin/material 생성을 제외했다. 각 keyframe에서는 선택 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관한다. DDS 중복 참조의 소유 파츠가 빠진 경우 실제 DDS로 복구한다. 모델 생성 전/자기 자신 add packet도 기록하고 외형 스냅샷은 매치 중 불변이다. 인게임 기본 외형 덮어쓰기·F7 변경을 막았고 원본 `.bin`/프로토콜 형식은 유지했다.

실제 서버 연결 테스트 중 발견한 로비 `Job::Execute`의 실행 누락, 큐의 shared Job 소유권/API와 callable 제약, 테스트 전환의 배열 delete[]/슬롯 용량, Title의 ID 확정 전 READY 전환을 복구했다. Debug 감사의 정상 Direct2D 종료를 추가했다. Debug/Release 전체 빌드, HeroSelection 12개 검사와 원본 대비 행렬 1,576,969개 byte 일치, MeshSharing 12개 검사·실제 3종 GPU 감사, 일반 로컬 기동, 분할 패킷의 실제 로비→게임 전환 응답 PASS. compilation database 225개·Serena·에이전트 환경 PASS. 기존 signedness/narrowing 및 D3D12 initial state 경고는 남는다.

2026-10-05 Release RTX 4070 SUPER의 동일 최종 실행 파일에서 full/selected 각 3회 독립 프로세스 교대 측정 완료. 영웅 생성 구간 private commit 1,123.27→215.92MiB, 행렬 668.24→96.25MiB(85.60% 감소), 전체 진입 private commit 5,833.01→4,928.64MiB(904.36MiB/15.50% 감소), Working Set 2,133.17→1,291.35MiB, LOCAL 3,694.88→3,632.02MiB. 2회 모델 로딩의 스킨드 메시 1,440→91, 애니메이션 열 1,576→227. 파티클 131개는 동일하며 약 2.4GiB 구간은 남는다. Private/Working Set/DXGI를 합산하지 않는다. 외형 fixture·DDS 복구를 포함한 동일 바이너리 비교이므로 이전 geometry 공유 결과의 절대값과 직접 이어 붙이지 않는다.

최종 소스 18개와 실행 파일 SHA-256 `70f6b983225c74afe946e2fceac16f4596ee00482d0795ccb746768aed7d46f8` 일치·reference 사본 보존, 구간/차이의 byte 합계·파티클/모델 통계·JSON/CSV 재생성·잘못된 입력 8종 거부 PASS. 이전 메모리 근거 재생성과 보관 해시도 확인했다. `docs/portfolio/HERO_SELECTED_PARTS.md`·근거 JSON/CSV·PNG/SVG·Archify JSON/HTML을 작성했다. Archify showcase 9/9, 오류/경고 0, 네 해상도 및 light/dark 자동 검사와 직접 화면 점검 PASS.

DB 없는 실제 로컬 서버를 통한 Title 4번 영웅 테스트에서 TCP 8911 연결과 영웅·무기·맵·미니맵·스킬 UI 렌더링을 확인해 1922×1112 실제 창을 `docs/portfolio/evidence/figures/hero-selected-parts-ingame.jpg`로 보존했다. 캡처의 실제 JPEG 형식에 맞춰 확장자를 정리했으며 픽셀 데이터는 변환하지 않았다. 서버의 기존 CoolTime 0/MP Consumption 0 진단은 남고 전체 전투·네 사용자·DB/블록체인·매치 재진입은 미검증이다. 원본 단일 모델 파일 파싱 및 타인 모델의 기존 공유 계층 구조도 남는다. 개발/측정 프로세스는 중지했고 커밋·push는 하지 않았다.

## 12. 선택 외형의 애니메이션 변환 행렬 용어 정리

- [x] “선택한 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관”으로 설명 통일
- [x] clip·keyframe·프레임 노드·행렬 보관 대상의 정의 및 기존/현재 구조 명시
- [x] 모든 61개 clip과 6,947개 keyframe 유지, 선택 스킬별 clip 로딩 미적용 명시
- [x] 구조도 JSON/HTML 문구 동기화, 문서 링크·UTF-8·공백 및 구조도 검증

사용자의 용어 정리 요청에 따라 포트폴리오·아키텍처·구조도 설명을 수정한다. 외형 번호 스냅샷과 선택 조건 포인터 전달, 파일 파싱과 보관 메모리 감소를 구분한다. 구현과 측정 근거는 변경하지 않아 게임 빌드·실행을 반복하지 않는다. 구조도의 모호한 “애니메이션 축소”는 “필요 행렬 보관”으로 수정하고 Archify 재검증 후 HTML을 갱신한다. 커밋·push는 진행하지 않는다.

검증 결과: 로더의 `keptFrameIndices`·행렬 복사·대상 프레임 포인터 매핑과 설명을 대조했다. Archify showcase validate/deliver 9/9·오류/경고 0, 네 해상도의 자동 검사 PASS. 새 HTML의 1440×900 light 및 2048×1320 dark를 직접 열어 레이블·카드 잘림/겹침이 없음을 확인했다. 문서 상대 링크·UTF-8·공백, JSON/HTML 전달 해시 PASS. 구현과 공개 측정 근거는 보존했다.

## 13. 메모리 분석·선택 외형 최적화 커밋 준비

- [x] 완료한 구현·계측·검증·포트폴리오 변경 및 신규 파일 검토
- [x] 해당 파일만 stage하고 검증·snapshot fingerprint·한글 메시지 미리보기 준비
- [x] 공통 commit 스킬에 따른 미리보기 승인 후 커밋 및 요청된 GitHub push

사용자의 커밋·push 요청에 따라 8~12번의 완료 작업을 함께 준비한다. 저장소는 `C:/GitFolder/NewWod`, 브랜치는 `main`, 원격은 `Kimseongtae9911/war_of_dimension`이다. `NewWod.slnx`의 로컬 `GitFolder/NewWod`에 의존하는 프로젝트 경로 변경은 이번 작업에서 제외하고 작업 트리에 보존한다. 실제 Debug/Release 빌드·GPU/행렬 검사·로컬 전환 검증과 현재 소스/바이너리 해시를 확인한 후 준비한 snapshot을 미리보기로 제시한다. 공통 스킬의 승인 전에는 커밋·push하지 않는다.

준비 검증: 관련 파일 59개만 stage했고 신규 파일은 최대 약 692KiB로 신규 대용량 LFS 등록 대상은 없다. 기존 메모리 근거와 영웅 전후 비교 재생성·원시 합계·실패 입력·현재 소스 18개 및 실행 파일 해시 확인 PASS. staged Python/JSON 구문·PowerShell 구문·UTF-8·문서 링크·신규 파일 및 변경 줄 공백 검사 PASS. 전체 기존 소스 줄까지 공백 검사를 확장했을 때 기존 GameFramework.cpp 공백이 감지되어 신규/변경 줄로 범위를 맞췄으며 기존 줄은 정규화하지 않았다. Archify 9/9·오류/경고 0 및 앞선 직접 시각 점검 결과를 확인했다. 원격 main을 fetch하여 HEAD와 일치함을 확인했다. 미리보기 승인 대상에서 제외한 변경은 `NewWod.slnx` 하나이며 신규 미추적 파일은 없다.

후속 미리보기 승인으로 커밋 ee60ac16과 GitHub main push를 완료했다. 제외한 `NewWod.slnx` 변경은 보존한다.

## 14. 선택 스킬에 필요한 파티클 버퍼 생성

- [x] 참가자 전체 선택 스킬·기본 공격·후속 스킬·타워 효과의 생성 의존 관계 확인
- [x] 선택 스킬 스냅샷과 필요한 종류의 파티클 풀만 생성, 수신 미완료 fallback 및 수명 보존
- [x] 실제 GPU 버퍼·스킬 의존 관계·실패 경로와 Debug/Release 검증
- [x] 동일 바이너리 전체/선택 풀 비교, 포트폴리오와 Archify 흐름도 기록

자신뿐 아니라 네 참가자의 선택 스킬 효과를 포함한다. 기본 공격·타워·환경 효과는 유지하며 기존 파티클 용량과 스킬 객체 풀 크기는 이번 범위에서 바꾸지 않는다. 서버가 자동 선택 결과보다 먼저 게임 시작을 전송하는 순서를 점검하고 선택 완료 정보로 인게임 자원을 생성하도록 한다. 기존 공개 측정 근거와 reference 소스/실행 파일을 보존하며 새로운 비교는 별도 시나리오/근거 파일에 기록한다. 원본 저장소와 제외된 로컬 SLNX 변경은 수정하지 않는다. 커밋·push는 별도 요청 전까지 진행하지 않는다.


검증 결과: Debug/Release 전체 빌드와 선택 파티클 검사 PASS. 종류별 100→45, 타인·기본 공격·타워·BigBang 후속·Archer 타이머·Programmer·중복 선택과 16개 미완료 슬롯, 잘못된 범위·스냅샷 고정·초기화 검사 PASS. 실제 D3D12 인게임 자원 생성·제출·대기 후 파티클 객체/텍스처/정보의 반복 해제도 두 구성에서 PASS. 서버의 보스 자동 선택 직업 4/5 정규화와 CS 20..39→SC 21..40 응답을 복구하고, 실제 네 TCP 참가자의 Ogre/Programmer Debug/Release 네 조합에서 수동 선택 보존·자동 선택 범위·궁극기·선택 16개가 게임 시작 전에 도착함·분할 READY header를 검증했다. wire 상수·field·packing은 유지한다.

최종 Release 실행 파일 SHA-256 b955f4289cf5caec4969decf374e3d7c82d7cd16ab460f8c7bd1e8829a0b96a0, 계측 소스 24개와 reference 사본 일치 PASS. 네 참가자의 고정 스킬 fixture와 geometry 공유·영웅 선택 외형을 양쪽에서 유지한 동일 바이너리 각 3회 교대 측정에서 파티클 127→72, 두 대형 버퍼 payload 2,325.44→1,318.36MiB, D3D12 allocation 2,333.625→1,323.000MiB였다. 최종 Private commit 4,857.28→3,837.25MiB(1,020.02MiB/21.00% 감소), Working Set 1,292.15→1,287.61MiB, DXGI LOCAL 3,558.49→2,547.44MiB. 지표는 합산하지 않는다. 이전 131개 시나리오와의 절대값 직접 비교도 하지 않는다. 원시 실행 근거·JSON/CSV·PNG/SVG와 선택 규칙/용어를 `docs/portfolio/PARTICLE_SELECTED_SKILLS.md`에 정리했다. byte 합계·모델 통계·반복 횟수·재생성·잘못된 측정 입력 9종 거부 PASS. 소스/바이너리는 `artifacts/logs/particle-selected-skills-verified`에 보존한다.

Release 기존 영웅 행렬 1,576,969개 일치·메시 공유 12개와 실제 3종 에셋 감사, Debug 일반 로컬 서버 연결·게임 창 기동 PASS. compilation database 227개·Serena 신규 선택 파일 색인·에이전트 환경 PASS. Archify showcase 9/9·오류/경고 0·네 해상도 자동 검사와 light/dark 직접 점검 PASS. 기존 signedness/narrowing·swprintf·D3D12 initial state 경고는 남는다. 모든 스킬의 전투 시각 검사·전체 매치 재진입·DB/블록체인은 미검증이고 기존 보스 일반 스킬 자동 선택의 중복 문제도 남는다. 스킬 객체 풀·원본 에셋·300,000개 용량은 유지한다. 실행 프로세스는 중지했고 로컬 SLNX 변경을 보존했으며 커밋·push는 하지 않았다.

## 15. 종류별 동시 효과용 파티클 5개의 필요성 검토

- [x] 서버 할당·재사용·패킷 ID와 클라이언트 버퍼 인덱싱 대조
- [x] 20종의 풀/시전자/고정 ID 분류와 다중 시전자·고갈 위험 확인
- [x] 실제 측정과 예상 절감량을 구분해 포트폴리오·정적 근거·인덱스 기록
- [x] 소스 해시·분류/계산·UTF-8·문서 링크·변경 줄 공백 검증

검토 요청에 따라 구현은 변경하지 않았다. 기존 5는 MAX_SKILL_OBJECT의 매치·종류별 서버 객체 풀 상한이며 실제 최적 동시 효과 수의 실측 근거는 없다. 14종은 첫 비활성 0~4번을 할당하므로 선택한 영웅 수로 클라이언트 벡터를 줄이면 중첩 투사체·높은 ID의 효과를 누락시킬 수 있다. StormArrow/BigBang 후속은 영웅 0~2, SCL은 단일 보스 0, 타워 투사체 풀은 PATH_NUM 4다. ArrowRain/DarknessRay는 모든 시전자에 0을 사용해 덮어쓰기·서로 다른 종료 시점의 표시 충돌 위험이 있다. 14종 풀 고갈 시 return 0과 호출부 -1 실패 검사 불일치도 확인했다.

현재 측정 fixture에서는 타워의 5번째 객체 한 개가 확실한 미사용 후보다. 적용 시 72→71개, 두 대형 DEFAULT allocation 18.375MiB 감소를 예상하지만 미구현·미측정이다. 20종의 현재 ID 주소 범위 합계 83은 기존 0번 충돌을 보존한 정적 계산이므로 정상 다중 영웅 권장 용량으로 제시하지 않는다. 5개 논리 슬롯을 유지하면서 실제 사용 시 GPU 버퍼를 생성하고 전투 활성 최대치·최대 ID·생존 시간·고갈 횟수를 계측하는 후속 개선을 제안했다.

검증: compile_commands.json 227개·Serena 상태를 확인하고 CGameMgr.cpp 37개 심볼 색인 PASS. 20종/14종 할당 자동 대조 및 현재 소스 10개 SHA-256, 기존 계측 소스 24개·Release 실행 파일 해시 보존, JSON 계산·구문·UTF-8·상대 링크·공백 검사 PASS. 기존 선택 흐름도를 현재 코드와 대조했고 구조·흐름 변경이 없어 Archify JSON/HTML 재생성은 하지 않았다. 실행 코드 변경이 없어 빌드·게임 실행을 반복하지 않았고 실제 전투 최대 중첩·최초 사용 생성 성능은 미검증이다. 로컬 SLNX 변경을 보존했으며 커밋·push는 하지 않았다.

## 16. 동일 파티클 자원의 공유 가능 범위 설명

- [x] CParticleObject의 공통 텍스처·난수·Shader 참조와 개별 CParticleMesh 생성 확인
- [x] Stream Output/Draw 상태 갱신·버퍼 교환·출력 카운터를 대조해 공유 제약 설명
- [x] 공통 버퍼 영역 할당·비활성 버퍼 재사용·동일 진행 상태의 다중 렌더링을 미구현 제안으로 기록

동일 종류의 정의·텍스처·Shader는 공유할 수 있고 현재 텍스처·난수 텍스처·Shader도 공유한다. 큰 두 버퍼는 효과별 입자 상태이므로 독립 효과끼리 같은 주소를 사용하면 상태 갱신·카운터·재시작 처리가 섞인다. 공통 공간의 별도 영역과 GPU 완료 후 재사용은 가능하지만 독립 입자 총 상태량은 남는다. 각 효과의 300,000개 예약을 그대로 두는 버퍼 통합만으로 큰 절감을 주장하지 않는다. 동일 시점·로컬 움직임을 재생하는 제한된 효과의 계산 결과 공유는 별도 Shader/렌더링 설계로 구분했다. 기존 풀 검토 문서에 설명을 추가했고 구현·패킷·측정 근거·Archify 산출물은 변경하지 않았다. UTF-8·링크·변경 줄 공백 확인 PASS. 실행 코드 변경이 없어 빌드·게임 실행을 반복하지 않았으며 새 설계의 실전 성능은 미검증이다.

## 17. 파티클 재사용의 조건별 절감량과 용량 초과 처리 검토

- [x] 현재 72개 중 스킬 45개·슬롯/환경 27개를 구분해 조건별 allocation 계산
- [x] 효과 수 부족·개별 입자 용량 부족·예산 상한·GPU 반환 시점의 처리 제안 정리
- [x] 기존 포화 시 재시작·통계 경로와 공식 D3D12 수명·출력 통계 근거 대조
- [x] 예상 근거 JSON·문서 링크·UTF-8·계산·소스 해시·변경 줄 공백 검증

스킬 버퍼만 재사용하고 쌍당 300,000개 용량을 유지할 때, 풀 유지량 9/18/27/45쌍의 예상 전체 대형 allocation은 661.500/826.875/992.250/1,323.000MiB이며 절감은 661.500/496.125/330.750/0MiB다. 예산 안의 실제 보유 수에는 잔존 입자·예비·fence 대기를 포함하며 평균 활성 수와 구분한다. 실전 동시 사용량 미측정이므로 특정 수치를 기대 성과로 확정하지 않았다. CPU Private/Working Set/DXGI 감소량으로 대입하지 않는다. 조건·byte 계산을 particle-buffer-reuse-estimates-20261005.json에 보존했다.

범위 밖 쓰기 대신 예산 안의 새 버퍼/영역 확보와 상태 전환, GPU fence 완료 후 반환을 제안했다. 초과 시 장식 입자 생성량 제한·핵심 공격 범위 표시 유지와 기록, 임시 증설의 구/신 동시 비용·통계 사후 검출의 한계를 명시한다. 기존 ParticlePostRender의 MAX_PARTICLES 도달 시 재시작과 비활성 SO 통계 경로를 확인했다. Microsoft 공식 출력 통계·fence 수명 관리·명시적 할당 자료를 대조했다. 소스/측정/실행 파일은 변경하지 않았다. 실행 코드·구조도 변경이 없어 빌드·플레이 테스트·Archify 재생성은 반복하지 않았고 실제 재사용 성능·포화 시 시각 품질은 미검증이다. 구현·커밋·push는 하지 않았다.

## 18. 모든 파티클의 공용 GPU 버퍼 재사용·확장·종료 해제

- [x] 스킬·슬롯·환경 효과에 공용 풀 연결, 활성 효과별 독립 상태 유지
- [x] 빈 버퍼 재사용·부족 시 추가 할당·GPU fence 완료 후 반환
- [x] 개별 입자 용량 초과 검출·확장과 상태 복사, 실패 시 기존 상태 보존
- [x] 게임 종료·장면 전환·반복 해제와 실제 GPU 성장/재사용/격리 검사
- [x] Debug/Release 빌드·회귀·동일 조건 메모리 비교 및 포트폴리오·Archify 기록

사용자 요청에 따라 전체 파티클에 적용한다. 네트워크 ID와 효과별 시뮬레이션 상태는 유지하고, 같은 저장 형식의 두 대형 버퍼를 공용 풀에서 임대한다. 비활성 효과의 버퍼는 마지막 GPU fence 이후 재사용하며 부족하면 확장한다. 기존 측정 원본/reference는 보존하고 새 비교는 별도 파일에 기록한다. 로컬 SLNX 변경은 보존하며 커밋·push는 진행하지 않는다.

검증 결과: 스킬·타워·선택 슬롯·점프·코인·장벽의 다섯 생성 경로를 장면 공용 풀에 연결했다. 효과 객체/서버 ID는 유지하고 최초 표시 시 두 대형 버퍼를 임대한다. 비활성 효과를 먼저 반환하고 stride·용량이 맞는 GPU 완료 블록을 재사용한다. 없으면 추가 생성하며 개별 입자 포화는 다음 프레임에 더 큰 블록으로 유효 상태를 복사한다. 장면 전환·앱 종료 및 ReleaseParticles는 GPU 완료 후 임대를 반환하고 확장분·유휴분을 전부 해제한다. 이전 cap 도달 시 전체 재시작을 제거했다. 사후 통계이므로 초과 프레임의 미기록 입자는 복구하지 않는 한계를 명시했다.

Debug/Release 클라이언트 빌드·실제 WARP GPU 검사 PASS. 동시 임대/종류별 상태 격리, gate로 GPU를 멈춘 미완료 fence의 재사용 방지, 풀 2→3쌍 확장, 4→12개 입자 용량과 GPU 상태 보존, 새 종류 seed/counter 초기화, 두 번 생성·종료 후 잔여 쌍 0, 범위 초과 거부를 확인했다. GPU 오류 0·경고 각 12개이며 실제 장치 OOM은 유발하지 않았다. Debug 실제 게임 Shader의 고정 재생·반복 ReleaseParticles도 PASS. 선택 스킬 두 구성, Release 메시 공유 12개/실제 3종 에셋 및 영웅 행렬 1,576,969개, Debug 로컬 서버/TCP/게임 창 기동 PASS. compilation database 229개와 Serena 공용 풀 13개 심볼 색인·에이전트 환경 PASS.

Release 실행 파일 ced83cfa0ae04e77b58bffae130da6e716398f573ee06976f8a9123c8c557fac에서 dedicated/pooled 각 3회 교대 측정. 합성 활성 순서 4→19→8→0→18을 각 2프레임 재생해 논리 객체 72개 유지, 실제 대형 버퍼 72→19쌍·누적 재사용 30회, allocation 1,323.000→349.125MiB(973.875MiB/73.61% 감소), Private commit 3,838.65→2,778.36MiB(1,060.29MiB/27.62%), Working Set 1,289.75→1,289.69MiB, DXGI LOCAL 2,548.22→1,574.34MiB를 확인했다. 실제 전투 최대치로 해석하거나 다른 메모리 지표를 합산하지 않는다. 계측 소스 28개·추가 Shader/테스트 5개와 실행 파일 reference, 원시 실행·JSON/CSV·PNG/SVG를 보존했다. 결과 재생성·byte 합계·입력 변조 6종 거부·Debug/Release 현재 소스/실행 파일 해시 PASS.

포트폴리오 PARTICLE_BUFFER_POOL.md와 인덱스·현재 아키텍처·설정을 갱신하고 이전 수치/제안은 당시 기록으로 명시했다. 새 공용 풀 수명도와 선택 흐름도는 Archify 2.16 showcase 9/9·오류/경고 0, 네 해상도 자동 검사 및 두 크기 light/dark 직접 점검 PASS. JSON/HTML 해시·UTF-8·문서 상대 링크·PowerShell/Python 구문·공백 검사 PASS. 기존 빌드 경고와 GPU 경고는 남는다. 실전 장시간 전투·전체 매치 재진입·DB/블록체인 및 메모리 압박 시 품질 조절은 미검증이다. 기존 서버 효과 ID 충돌·풀 고갈 반환은 변경하지 않았다. 로컬 SLNX 변경을 보존했으며 커밋·push하지 않았다.

## 19. 파티클 최적화 커밋·push 준비

- [x] 선택 스킬 구성·공용 GPU 풀·검증·측정 문서의 완료 변경 검토
- [x] 현재 계측 소스와 Release 실행 파일 해시 및 공개 결과 재생성 확인
- [ ] 공통 commit 스킬의 미리보기 승인 후 커밋 및 GitHub push

사용자의 커밋·push 요청에 따라 14~18번 작업을 함께 준비한다. 저장소는 `C:/GitFolder/NewWod`, 브랜치는 `main`이다. 원격 fetch 후 HEAD와 origin/main 일치를 확인했다. 기존 `NewWod.slnx`의 로컬 프로젝트 경로 변경은 제외하고 작업 트리에 보존한다. 앞선 Debug/Release 빌드·GPU 수명·회귀 검증 결과를 확인했고 계측 소스 28개 및 Release 바이너리의 해시가 그대로임을 확인했다. GPU 오류 0·경고 각 12개와 실전 최대 동시 효과 수 미검증 범위는 유지한다. 정확한 staged snapshot과 한글 메시지를 제시한 뒤 승인받아 커밋·push한다.

준비 검증: 작업 파일 56개를 명시적으로 stage했다. 공개 측정 JSON/CSV 재생성·소스/실행 파일 해시·구조도 전달 해시·UTF-8·JSON/Python/PowerShell 구문 검사 PASS. staged 공백 검사에서 새 matplotlib SVG의 경로 줄 끝 공백을 발견해 정리했으며 XML 요소·속성·텍스트의 의미가 동일함을 확인했다. 실행 소스와 측정값은 변경하지 않았다. 제외된 변경은 NewWod.slnx 하나다.

## 20. 공용 풀 적용 후 클라이언트 메모리 구성 비율

- [x] 최신 pooled 3회 원시 checkpoint와 현재 소스 28개·Release 실행 파일 해시 확인
- [x] Private commit 구간별 순증가 비율 및 로딩 완료/고정 재생 후 총량 집계
- [x] 포트폴리오·JSON/CSV·PNG/SVG·재생성 스크립트 기록 및 이전 분석의 시점 명시
- [x] 구간/지표 byte 합계·비율 합계·잘못된 입력 4종·그림 직접 점검

커밋 38bb9ee7이 GitHub main에 반영된 이후 구성 비율을 요청받았다. 기존 최종 측정의 pooled 3회 자료를 재집계했으며 재실행/실전 최대치 측정으로 주장하지 않는다. Private 2,778.36MiB 중 미니언·몬스터·보스 34.16%, 공통 자원 22.71%, UI 10.72%, 파티클 고정 재생 9.71%, 영웅 7.74%, 하늘 6.92%, 맵 6.37%다. 파티클 준비/객체/재생 합계는 10.38%이고 별도 GPU allocation과 합산하지 않는다. 순증가량을 최종 자원 소유량으로 해석하지 않는다. 현재 런타임 소스 변경이 없어 기존 검증을 유지하고 빌드·전투 실행은 반복하지 않았다. NewWod.slnx를 보존하며 이번 집계는 커밋·push하지 않았다.

## 21. 미니언·몬스터 자원 공통화 가능성 검토

- [x] compilation database와 Serena 상태 확인, 모델 로딩·인스턴스·애니메이션 소유권 조사
- [x] 최신 3회 실측의 미니언/몬스터/보스 구간 및 에셋·DDS 비용 분해
- [x] 기존 공유 범위·추가 공통화·용량 감소 후보와 예상 효과/위험 문서화
- [x] 근거 해시·계산·문서 링크 및 기존 Archify 구조도와 설명 대조

사용자의 검토 요청으로 구현은 변경하지 않는다. 미니언 반복 생성과 몬스터 동일 모델 반복 사용, 종류 간 동일 DDS와 geometry, 개별 pose/track 및 GPU 상수 버퍼의 경계를 확인한다. 실제 현재 자원 공유와 설계 제안을 구분하며 기존 최종 측정·reference 및 로컬 SLNX 변경을 보존한다.

검토 결과: 모델은 미니언 1회→12개, 일반 몬스터 7종 1회씩→9개(Chest/Beholder 각 2개)로 이미 공유한다. 최신 pooled 3회 실측 구간을 미니언 Private 28.55MiB·일반 몬스터 836.86MiB·Ogre 83.63MiB로 나누었다. 9개 모델과 DDS 25개를 파싱·해시 확인했고 geometry 26개는 모두 다른 내용이다. 일반 몬스터 clip 행렬은 16.20MiB, 비압축 텍스처 payload는 400MiB이며 Minotaur 240MiB/로더 Private 증가 488.69MiB가 큰 비용이다.

NPC upload 해제 순회 누락(일반 몬스터 texture upload 후보 400MiB), 모델 사이 동일 DDS 2장의 중복 8MiB, 300개 미니언 object CB의 arena 예약량 18.75→0.125MiB, 공유 material CB 반복 생성의 포인터 덮어쓰기를 확인했다. Minotaur의 동일 해상도 BC7 단일 mip 예시는 240→60MiB이며 아직 에셋 변경·품질/감소 실측은 하지 않았다. 후보의 메모리 지표·중첩·GPU 완료 조건을 문서화하고 공유 clip과 개별 pose를 분리하는 방향을 제안했다. compilation database 229개·Serena Object.cpp 194 심볼, 기존 Archify showcase 9/9·오류/경고 0, 현재 소스/에셋/원시 근거 해시·계산·UTF-8·문서 링크 검사 PASS. 실행 소스·에셋·구조도가 그대로여서 게임 빌드/전투 실행·HTML 재생성은 반복하지 않았다. 커밋·push하지 않았다.

## 22. 모델 DDS 공용 자원 적용 및 upload·압축·상수 버퍼 설명

- [x] 모델 material의 DDS resource를 device·정규화 경로·용도별로 공유하고 binding은 개별 유지
- [x] GPU 초기 업로드 순서·반복 해제·마지막 소유자·잘못된 DDS 및 실제 Chest/Beholder 검사
- [x] Debug/Release 빌드·관련 회귀·공용 자원 수명과 실제 Shader 고정 재생 검증
- [x] 업로드 해제 누락 범위·BC7 및 상수 buffer arena 용어, 문서·Archify JSON/HTML 기록

사용자는 검토 2번의 구현을 요청했고 1·3·4번은 설명을 요청했다. 조기 upload 해제 순회·에셋 압축·상수 arena는 이번에 적용하지 않는다. 모델 DDS의 공유 자원과 wrapper/binding 수명을 분리하고 기존 측정 근거·reference·로컬 SLNX 변경을 보존한다.

적용 결과: SharedDdsTexture가 device·정규 경로·resource 용도별 DEFAULT/UPLOAD를 공유하고 weak cache로 마지막 소유자 수명을 유지한다. material별 wrapper/binding 저장은 독립, 같은 Scene SRV 힙의 읽기 전용 descriptor는 재사용하며 힙 교체 시 캐시를 비운다. CTexture 복사의 소유권·binding과 남은 upload 종료 회수를 보완했다. NPC 조기 해제 순회·압축·arena는 미적용이다.

검증: Debug/Release Client 빌드 및 DDS 12개 범주 PASS. 실제 Chest/Beholder 4개 texture 참조→2개 GPU resource, DEFAULT/UPLOAD 각각 8MiB 중복 제거, GPU 첫 mip pixel readback 일치·upload 반복 해제·새 Scene 힙·복사 wrapper 마지막 소유자·live DDS=0·GPU errors=0. 기존 모델 UPLOAD 초기 상태 경고 1328은 16개씩 기록했다. Release 메시 12개/실제 모델 3종 audit·영웅 행렬 1,576,969개 비교 PASS. 실제 Shader 고정 재생 1회와 particle 종료 PASS(Private 2,656.47MiB, WS 1,229.85MiB, LOCAL 1,518.34MiB). 반복 A/B 성과로 사용하지 않는다. 에이전트 환경 231개 소스·Serena 3개 핵심 파일·Archify doctor PASS. DDS 구조도 showcase 9/9·오류/경고 0, 4해상도 containment 및 1440 light/2048 dark 직접 점검 PASS. 기존 signedness/narrowing 등 경고 유지. 실제 멀티플레이 전투·압축 화질·pose 분리·NPC 조기 upload 정리·arena는 미검증. 구현/근거 JSON·문서·해시/링크 검사 기록, 기존 baseline·로컬 SLNX 변경 보존. 커밋·push하지 않았다.

## 23. NPC 임시 upload 회수·텍스처 BC7·상수 버퍼 arena 적용

- [x] 실제 게임 Shader의 고정 카메라 몬스터 전후 캡처 경로와 변경 전 화면·메모리·소스·에셋 보존
- [x] GPU 완료 후 누락된 NPC/타인/스킬의 임시 upload 순회와 반복 해제 검증
- [x] NPC 객체 상수 버퍼를 frame별 독립 영역의 확장 가능한 arena에 배치, 공유 material CB 생성 멱등화
- [x] 몬스터 DDS를 해상도·채널 해석·색 공간 유지 BC7로 변환하고 동일 조건 후 화면·픽셀/품질·실측 비교
- [x] Debug/Release·GPU 수명/실패 경로·관련 회귀·로컬 실행 및 문서·근거·Archify JSON/HTML 기록

사용자가 1·3·4번의 작업과 3번 변경 전후 몬스터 스크린샷을 요청했다. 실제 에셋/Shader의 화면을 먼저 보존하고 동일 fixture로 비교한다. 기존 원본 저장소와 선행 작업의 기준선은 보존하며 커밋·push는 이번 요청 범위가 아니다.

적용 결과: GPU 초기 copy fence 이후 minion/monster/OtherClient/tower/skill/static skill model/particle texture를 순회해 임시 upload를 회수한다. 지속적으로 갱신하는 CB·particle counter는 유지한다. 공유 material CB 생성은 멱등으로 변경했다. 미니언 300·일반 몬스터 508개 논리 객체 CB를 2프레임 256byte draw 영역으로 통합하고 부족하면 64KiB 페이지를 확장한다. 기존 CB 예약 50.5→0.5MiB, 정상 종료·확장분 마지막 소유자 해제 후 page 0을 확인했다. Ogre·영웅 CB와 개별 pose/clip은 유지한다.

에셋: Microsoft DirectXTex may2026 고정 도구/SHA 검증, BC7 `-bc x`로 24 DDS를 동일 해상도/단일 mip/UNORM/채널 배치에 변환했다. 기존 R8 Metallic 1장은 압축 이득이 없어 원본 유지. 25개 고유 NPC DDS의 실제 DEFAULT allocation 441.25→113.3125MiB(327.9375MiB 감소), Minotaur 240→60MiB. DDS 해시·헤더·payload 검증 후 LFS 등록. GPU backbuffer 원본 PNG 9종 앞·뒤 전후 36장과 동일 카메라 근거를 공개 포트폴리오에 보존했다. 캡처 양쪽은 1·4번 적용 후라 BC7 시각 차이만 비교한다. 실제 클라이언트 Shader의 고정 idle 모델 화면이며 네트워크 매치/맵 UI 화면이 아니다. 전후 직접 점검, 25 DDS RGBA와 18 화면 foreground RGB MSE/PSNR/최대 오차 기록. 모든 조명·거리·애니메이션의 무손실 동일 화질로 주장하지 않는다.

Release 최종 EXE `64c50c4fa1617b904f4109088e3db8affc2be3ed32d9aada6c4d6fcb78dbda0e`, 비교 EXE `2622d1df0d6a5841e2238b777cbd46cb69d13a910bcfd97fe05a72c58f6ef356`. 동일 선택 외형/직업/스킬/pooled 합성 순서를 전후 각각 3회 순차 측정했다. 마지막 ingame_ready 중앙값 Private 2,656.35→1,964.46MiB(691.89MiB/26.05% 감소), WS 1,233.63→897.06MiB, LOCAL 1,518.3438→1,190.4062MiB. 파티클 19쌍/재사용 30회 양쪽 일치, 공유 DDS upload 28개/125.3125MiB가 복사 완료 후 0개/0byte. 전체 UPLOAD 0을 의미하지 않고 지표·중복 절감은 합산하지 않는다. 초기 5.8GiB·공용 풀 2,778MiB·DDS 기능 검사 1회 근거는 당시 자료로 유지한다.

검증: Debug/Release Client 빌드·arena native 10범주·DDS native 12범주 PASS. 600 draw 성장·GPU readback 값 보존·프레임 격리·미완료 gate fence 대기·잘못된 입력/상태 거부·scope 복구·material 1회 생성·종료 page/resource 0 확인. 실제 모델 렌더/반복 upload 정리/OnDestroy 두 구성 PASS. 기존 UI 종료의 중복 ReleaseWrappedResources를 제거하고 Flush 후 GPU 완료를 기다려 device removed 오류를 보완했다. 최종 Debug 초기화 GPU 오류 0·기존 state ignored 1328 경고 1,511개, 캡처/종료 오류·경고 0, removedReason 0. DDS native 기존 1328 경고 16개씩과 particle pool 기존 경고 12개, 기존 compiler 경고는 유지한다.

Release 메시 12범주·실제 모델 3종 audit, 영웅 행렬 1,576,969개 일치, 선택 파티클 및 공용 풀 성장/종료 회귀 PASS. Release 로컬 TCP 서버 연결·클라이언트 창 기동과 234개 소스 compilation database·Serena 3파일·Archify doctor PASS. 구조도 showcase 9/9·오류/경고 0 및 4해상도 containment, 1440 light/2048 dark 직접 점검 PASS. 구현/측정/변환/검증/품질/36 PNG 해시·JSON/CSV 집계·재현은 NPC_MEMORY_OPTIMIZATION.md에 기록했다. 실제 장치 OOM·장시간 네트워크 전투·전체 매치 재진입·운영 DB/블록체인은 미검증이다. 로컬 SLNX 변경과 원본 저장소를 보존했으며 커밋·push하지 않았다.

최종 문서·산출물 검사: Playwright/실제 Chrome에서 갤러리 9모델×2시점의 선택 18조합·전후 원본 PNG 36개 1920×1080 로드·링크 변경·페이지 오류 0·가로 overflow 0 PASS. 갤러리 실제 화면도 직접 점검했다. 문서 상대 링크 151개, JSON/Python/PowerShell 구문, 추가 소스·EXE·에셋·PNG SHA, 수정 C++ 19개 strict UTF-8, project XML/source 경로, Git diff 공백 검사 PASS. SLNX SHA f3447f2a3a56e5a04b4cae2d11f7e5ccef60bc0ee116713eed3c48156d4f2562 보존. 실행 프로세스는 중지했고 임시 검사/캡처 sidecar는 Git 제외를 확인했다.

## 24. DDS 공유·NPC 메모리 최적화 커밋·push 준비

- [x] 작업 범위·원격 main·최신 소스/실행 파일/에셋/스크린샷 해시·측정 근거 검토
- [x] 한글 커밋 메시지·정확한 staged snapshot 미리보기 준비, SLNX 기존 수정 제외
- [x] 공통 commit 스킬의 미리보기 승인 후 커밋 및 GitHub main push

사용자가 완료 작업의 커밋·push와 메모리 현황 표를 요청했다. 20~23번의 메모리 비율 분석·NPC 후보 검토·DDS 공유·임시 upload 회수·BC7·객체 arena·전후 화면/문서/검증을 함께 준비한다. 저장소는 C:/GitFolder/NewWod, 브랜치는 main이며 fetch 후 HEAD와 origin/main은 38bb9ee76645b9f7e6cea16cab37d4613c4bb005로 일치했다. 최신 소스/EXE/25 DDS/36 PNG 해시·집계와 151개 문서 링크 검사를 통과했고 직전 Debug/Release 빌드·native GPU·회귀·로컬 실행 결과를 확인했다. 기존 경고와 전체 네트워크 전투 미검증 범위는 유지한다. NewWod.slnx는 제외·보존하고, 실행 로그/바이너리/생성 DB/임시 검사는 Git에 포함하지 않는다. 메시지와 staged fingerprint를 제시한 뒤 승인받아 커밋·push한다.

Stage 검토에서 원본 R8 Metallic의 새 LFS 속성에 맞게 해당 파일만 --renormalize했다. 실제 DDS bytes/SHA는 06e84337b5412950b78efa4950466958376a75615d60e507fe3b7a0b0cac7110으로 그대로이고 Git index만 LFS pointer로 전환한다. 압축 24장과 원본 유지 1장 모두 staged LFS pointer·내용 해시/크기를 검증한다. 공개 측정 JSON/CSV 재생성·현재 소스/바이너리/에셋/PNG와 추가 소스 해시 PASS, staged 공백 검사 PASS. 기존 실행 검증 이후 기능 변경은 없으므로 게임 테스트를 반복하지 않는다.

NPC 수명도 v2 JSON은 검증/전달된 원본이 CRLF인 반면 Git의 기본 정규화가 LF로 바꾸는 것을 확인했다. 해당 JSON 하나에 -text를 지정해 frozen bytes와 receipt SHA-256을 그대로 stage한다. 원본 JSON/HTML은 수정하지 않았으며 두 DDS/NPC 구조도의 staged bytes가 전달 당시 SHA와 일치하는지 검사한다.

완료: 사용자 승인 후 076d2692cb843f6263425f66d108983e3bf7e3f9를 생성하고 GitHub main에 push했다. LFS 25개 업로드 및 ls-remote의 동일 commit을 확인했다. 기존 NewWod.slnx 변경은 제외·보존했다.

## 25. 기동·Title·렌더링·음원 메모리 구간 설명

- [x] 최신 공개 측정 3회의 초기화 checkpoint를 다시 집계하고 실제 생성 코드와 대조
- [x] Title 화면 자원과 공통 렌더링·음원·shadow map의 측정 범위를 구분

기존 632.22MiB 항목은 scene_setup_ready까지의 누적 Private Bytes다. 3회 평균 순증가를 나누면 framework 준비까지 121.3385MiB, Title/렌더링/UI 생성 127.4922MiB, FMOD 초기화·지정 Sound 디렉터리 로딩 125.8451MiB, 후속 BGM 재생/GPU 제출·정리/shadow map/장면 전환 준비 257.5456MiB다. 각 자원의 최종 보유량을 직접 계측한 값으로 해석하지 않는다. Title 화면은 배경 및 SignIn/SignUp DDS와 UI 객체를 생성하며 비용은 두 번째 구간에 포함된다. 마지막 구간의 주요 생성 자원은 8192×8192·32bit shadow map(논리 texel 256MiB)이며 Title 이미지 단독 비용으로 부르지 않는다. GameFramework.cpp, Scene.cpp, Shader.cpp, UILayer.cpp, SoundManager.cpp, ShadowMap.cpp/h와 measurements.json을 대조했다. 실행 코드 변경이 없어 빌드·게임 재실행은 반복하지 않았다.

## 26. 장면별 자원·음원 및 공통 렌더링 최적화 후보 검토

- [x] Title 종료 시 객체·재질·텍스처·GPU 복사 버퍼의 참조/해제 경로 확인
- [x] FMOD 장면별·선택 스킬별 로딩과 BGM streaming 가능성, 파일/호출 범위 조사
- [x] shadow map·G-buffer·skybox·인게임 UI의 비용과 후보 우선순위 문서화
- [x] 코드·측정 근거·문서 링크·에이전트 색인 검증 및 기존 구조도 대조

검토 결과: Title UI 3개와 player/camera는 장면 전환의 반환 경로가 있으며 DDS payload 합계 4.207703MiB다. 기동 632.22MiB 전체를 Title 잔존량으로 해석하지 않는다. 음원은 전환 시 Stop_All만 호출해 상주한다. 포함된 FMOD 2.00.01을 NOSOUND로 조사하여 142개 생성 성공, PCM 122.86MiB 중 BGM 86.48MiB, 비인게임 BGM 75.44MiB를 확인했다. BGM 4곡 stream 생성 성공, 인게임 BGM 재생 전 FMOD 증가 126.18KiB, 독립 조사 종료 FMOD 0byte. 실제 재생·루프 안정성은 미검증이다. 중복 Jump.wav의 emplace 실패 시 반환 누락 경로도 확인했다.

UI shader/dissolve의 임시 upload 조기 순회 누락 및 UI ReleaseUploadBuffers의 빈 부모 호출을 확인했다. skybox payload 96→24MiB BC7 후보, 승패 UI 각각 25.43MiB·Speed 41.66MiB·Blood 8.06MiB, shadow map 256→64MiB/4096 후보, 위치 RT 31.64→15.82MiB 형식 후보와 화질 검증 조건을 기록했다. GPU/PCM payload·allocator·Private Bytes를 합산하지 않는다. scripts/Measure-StartupResources.py와 공개 JSON, STARTUP_RESOURCE_OPTIMIZATION_REVIEW.md 및 문서 인덱스를 추가했다. 현재 구현은 변경하지 않으며 UI upload→BGM→skybox/UI→shadow/G-buffer 순서로 제안한다.

검증: 234개 compilation source/에이전트 환경·Serena GameFramework 36/Shader 208/SoundManager 14 심볼·Archify doctor PASS. 공개 근거의 DLL/142 음원/9 DDS/14 source SHA, 그룹/PCM 합계와 스크립트 구문·문서 상대 링크·Git 공백 검사를 확인했다. 기존 DDS/arena 구조도와 범위를 대조했고 구현 구조가 그대로라 frozen JSON/HTML을 재생성하지 않았다. 게임 빌드/전투를 반복하지 않았고 실제 Title 전환 live 자원·audio playback·압축/렌더링 품질은 미검증이다. 원본 저장소·SLNX 기존 변경을 보존하며 이번 검토는 커밋·push하지 않았다.

## 27. Shadow map 유지와 위치 RT/UI 동적 로딩 부작용 설명

- [x] 사용자 지시로 shadow map 8192 해상도 유지, 후보 문서 갱신
- [x] 현재 world position의 조명·shadow 소비 경로와 FP16 간격 계산 확인
- [x] UI 이벤트/생성 경로와 즉시 효과·결과 UI의 동적 로딩 적합성 구분

Shader의 world position 저장→DeferredLighting/shadow transform 경로와 Microsoft의 half 형식을 대조했다. FP16의 좌표 구간별 간격/최근접 오차를 계산하고, 8192 shadow를 유지해도 위치 정밀도 변경이 그림자/조명에 영향을 줄 수 있음을 기록했다. 서버 충돌/실제 transform과 구분하며 위치 RT는 미적용이다. 승패 UI는 필요한 한 장의 비동기 준비 후보, Speed/Blood는 최초 스킬/피격에 즉시 필요하므로 사전 준비 권장이다. 파일 I/O·GPU upload/fence·게시 단계와 cold/warm cache/최대 frame time 검증을 구분했다. 정확한 UI 동적 로딩 시간은 미측정이며 수치를 추정 성과로 제시하지 않는다. 문서/지시만 갱신, 실행 코드·구조도가 그대로라 게임 빌드·rendering 검증·Archify 재생성을 반복하지 않았다.
## 28. 위치 G-buffer 유지와 UI BC7 사전 로딩 절감량 계산

- [x] 사용자 지시로 6번 위치 RT 형식 변경도 제외하고 5번 shadow map 유지와 함께 문서 반영
- [x] INGAME UI DDS의 실제 역할별 생성 범위·헤더·payload 및 BC7 패딩 포함 예상량 계산
- [x] BC7 압축 상태의 GPU 상주·샘플링과 사전 로딩, 손실 압축·UV·실측 한계 문서화

현재 INGAME UI 고유 DDS 21개 중 역할별 20개가 생성되며 모두 32bit RGBA 계열 단일 mip이다. BC7 payload는 ceil(width/4)×ceil(height/4)×16으로 계산했고 4배수 패딩·UV/atlas 보존이 필요하다. 대형 UI 네 장 100.57→25.17MiB(75.40MiB 절감), 플레이어 20종 142.29→35.62MiB(106.67MiB 절감), 보스 20종 138.02→34.55MiB(103.47MiB 절감) 예상이다. 네 장은 20종의 부분집합이며 합산하지 않는다. 모든 대상의 BC7 화질 채택을 가정한 DDS/texel payload 계산으로 GPU actual allocation·전체 Private Bytes 절감과 구분한다. 압축 DDS를 그대로 업로드해 GPU에서 압축 블록으로 상주하고 샘플링 시 하드웨어가 해석하는 경로를 기존 helper/loader와 Microsoft 설명으로 확인했다. 사전 로딩은 첫 표시의 파일 로딩을 피하며 실제 로딩 시간은 미측정이다.

근거 JSON의 21 DDS/Shader SHA·헤더·byte 계산·역할별 합계, 문서 링크·공백 검사를 완료했다. 구현/에셋·구조 흐름 변경 없이 문서와 계산 근거만 추가해 빌드·게임·Archify 재생성은 반복하지 않았다. BC7 변환·화질·메모리 실측은 미적용이며 원본 저장소·기존 SLNX를 보존했다. 커밋·push하지 않았다.

## 29. UI BC7 압축과 사전 로딩 적용

- [x] 원본 DDS·코드·실행 파일 보존과 동일 fixture 전 메모리 측정
- [x] 픽셀·wrap 경계·atlas를 보존한 BC7 변환 및 패딩 UV 전달
- [x] 실제 UI GPU 렌더링 전후 비교·역할별 allocation·실패/수명 검사
- [x] Debug/Release 빌드·로컬 실행·후 메모리 실측·품질 및 포트폴리오/Archify 기록

사용자가 UI 압축 후 사전 로딩 적용을 요청했다. 그림자 8192와 위치 FP32는 유지한다. 결과 UI를 포함한 기존 생성 시점은 유지하며 다른 음원/skybox/upload 회수 후보는 이번 범위에 포함하지 않는다. 기존 SLNX 수정 및 앞선 검토 자료를 보존하고 원본 저장소는 수정하지 않는다. 커밋·push는 별도 요청 전까지 진행하지 않는다.

적용 결과: 고유 21개 RGBA32 UI DDS를 단일 mip BC7_UNORM으로 변환했다. 원본 내용 픽셀을 보존하고 비4배수 11개에 1px wrap gutter 및 4배수 패딩을 추가했다. DDS reserved1의 UIB7/v1 내용 영역을 기존 파일 bytes에서 검사하여 CTexture/독립 UI mesh 상수로 전달한다. atlas/progress 계산 후 frac(uv)×scale+offset, Speed blur의 각 샘플에도 적용하며 일반 DDS/blur는 identity를 유지한다. 기존 장면 사전 생성/사용 시점은 그대로다. DDS load/내용 계약/upload 실패의 자원은 RAII로 반환한다. 8192 shadow와 FP32 위치 RT는 실제 resource 검사·원본 source hash 대조로 유지 확인했다. UI upload 조기 회수·음원·skybox 등 다른 제안은 미적용이다.

실측: 같은 Release EXE e8c0949a0c22a9cb0824bc770592b7dd31329891051f1fb4b0bb891d54c3f462로 원본 에셋 3회→BC7 에셋 3회를 순차 실행했다. Private 중앙값 1,964.52→1,756.43MiB(208.09MiB/10.59% 감소), WS 897.17→790.44MiB, DXGI LOCAL 1,190.40625→1,084.71875MiB. 파티클 19쌍/재사용 30회·직업/스킬/외형 fixture는 같다. payload 플레이어 142.29→35.68MiB·보스 138.02→34.61MiB, 실제 DEFAULT 각각 147.625→41.9375MiB·143.4375→40.5625MiB, 남은 UPLOAD 각각 143.4375→36.875MiB·139.125→35.8125MiB다. 예상 대비 wrap gutter 비용을 구분하고 지표를 합산하지 않는다.

검증: Debug/Release Client 빌드 및 두 역할 실제 UI 16화면·20 texture·9개 내용 계약 오류 거부·copy 참조/UV 보존·GPU 완료 후 OnDestroy의 DEFAULT/UPLOAD 40개 잔여 참조 0 PASS. 역할별 전후 GPU PNG 64개와 전체 스킬 칸/아이템/게이지 5단계/Speed 4프레임/밝고 어두운 alpha 화면을 보존했다. 원본 픽셀/atlas 동일 배치·내용 영역 보존을 검사하고 실제 화면 및 국소 최대 오차를 직접 비교했다. DDS 최소 채널 PSNR 37.08dB, 화면 foreground 최소 41.34dB/최대 오차87(255단계)을 기록하며 무손실 품질로 주장하지 않는다.

기존 경고: Debug 초기화 ID1328 플레이어1511/보스1435, 추가 비교 grid 이후78/50이며 종료 진단에 같은 누적 경고가 남는다. 오류/removedReason 0. DDS 회귀 기존1328 경고16개씩과 Shader.cpp 기존 C4267은 유지한다. Release 메시12범주/실제3모델·DDS Debug/Release 회귀·일반 로컬 연결/클라 창 기동 PASS. 에이전트235 source·Serena capture9심볼 및 Archify 최종v2 showcase9/9·4해상도 containment·1440light/2048dark 직접 점검 PASS. 첫 구조도 overflow 후보는 로컬 artifacts에 보존하고 현재 후보를 새 v2로 전달했다. 갤러리32선택/64개1920×1080 원본 이미지·링크·페이지 오류0·가로 overflow0 PASS. 공개 JSON/CSV/변환·근거 스크립트 및 포트폴리오/인덱스/개발/구조 문서를 갱신했다.

반복 매치 재진입·장시간 네트워크 전투·모든 해상도/DPI·모든 Twinkle 시각·실제 OOM·운영 DB/블록체인은 미검증이다. 정상 종료 검사도 해당 UI 자원 범위이며 전체 누수0으로 보고하지 않는다. 원본 저장소 및 기존 SLNX SHA f3447f2a3a56e5a04b4cae2d11f7e5ccef60bc0ee116713eed3c48156d4f2562를 보존했다. 커밋·push하지 않았다.

## 30. UI 압축 작업 커밋 준비와 하늘·dissolve 후속 검토

- [x] 완료한 UI 압축·앞선 후보 검토 자료의 diff·LFS·근거 해시를 확인하고 커밋 미리보기 준비
- [x] Space 큐브맵 6면·mip·채널과 dissolve의 실제 형식·사용 채널·기존 측정 구간 확인
- [x] 추가 절감 후보·수명·화질/효과 부작용·검증 조건을 별도 포트폴리오 근거로 기록

사용자가 현재 작업 커밋·push와 후속 검토를 요청했다. 원본 저장소와 기존 NewWod.slnx 변경은 제외·보존한다. 실행 코드·에셋은 추가 변경하지 않고 검토 자료를 작성한다. 공통 commit 스킬은 준비된 메시지·staged snapshot의 미리보기 승인 후 커밋을 요구하므로, 승인 전에는 커밋·push하지 않는다. HEAD/origin/main은 fetch 후 076d2692cb843f6263425f66d108983e3bf7e3f9로 일치했다.

검토: Space.dds는 BGRA32 2048×2048 6면/단일 mip 96MiB·전체 alpha255. 같은 해상도 BC7 단일 mip 24MiB로 payload 72MiB 절감 후보이며 6면 경계/반짝임·밝기 판정/회전 레이어를 비교해야 한다. 하늘 upload는 현재 fence 후 이미 반환한다. dissolve.dds는 BC1 1024×1024 11 mip, payload 699,064byte(0.666679MiB)로 기존에 압축됐다. BC4는 절감0, R8는 약0.666654MiB 증가, 512 BC1은0.5MiB 절감으로 우선순위가 낮다. dissolve는 공통 t26 한 장·객체 상태 상수이며 UI shader 소비자가 아니다. ingame_ui_dissolve_ready는 UI 생성과 dissolve 합친 구간으로, 기존 각3회 Private 순증가 중앙값297.48→88.19MiB·LOCAL148.5117→42.8242MiB를 확인했다. 개별 DDS 크기로 해석하지 않는다.

UI upload 실제 잔존 플레이어36.8750/보스35.8125MiB와 dissolve staging의 fence 후 반환 후보를 기록했다. dissolve 개별 upload allocation은 새로 계측하지 않았으며 Private 절감으로 단정하지 않는다. dissolve 일반 draw sample 생략·Speed 225 sample blur는 GPU 시간 후보로 구분한다. SharedDdsTexture cube 지원·weak cache와 순차 Scene 반환을 확인했고 상주 cache는 최종 메모리 절감이 아니다. 새 검토 문서·독립 근거 JSON/읽기 전용 스크립트와 인덱스를 추가했다.

검증: 에이전트235 source PASS, Serena Framework36/Object195심볼 PASS. DDS6면/mip payload·전체 alpha·후보 byte 계산·기존6실행 구간 대조 PASS. 현재 UI23소스/21DDS/64PNG/동일Release EXE/역할별native GPU·수명 결과 및 frozen JSON/HTML 해시 PASS, 새 검토 근거13소스/2DDS 해시·Python구문·109문서링크·Git공백 PASS. 직전 Debug/Release UI빌드·실제렌더링/종료·DDS/메시/로컬실행 결과를 유지하며 새 기능 변경이 없어 반복하지 않았다. 기존 C4267·GPU1328 경고와 전체 네트워크 전투 미검증 범위는 유지한다. 현재 구조/흐름 변경이 없어 기존 Archify 수명도 대조만 수행했고 재생성하지 않았다. 원본·SLNX 보존. 커밋·push는 미리보기 승인 대기다.

완료: 2026-10-06 사용자 승인 후 fingerprint cd8832a598d76fe398aa122079b08e15edd49405와 정확한 메시지를 재확인해 e85eba42958acfdaff5713eb188d7959b39b2fe8를 생성했다. 126파일·85 LFS pointer/내용 검증 범위를 커밋했고 GitHub main push 성공, 신규 고유 LFS59개/55MB 업로드 및 ls-remote의 동일 HEAD를 확인했다. 기존 NewWod.slnx만 남겨 보존했다.

## 31. 단계별 리팩토링 최종 보고서

- [x] 승인된 UI 압축·후속 검토 snapshot 커밋 및 GitHub main push
- [x] 개발 기반·원인 분석·공유 자원·영웅·파티클·NPC·UI를 큰 항목별로 종합
- [x] 각 단계의 전후 측정 조건·통계 기준·검증·미적용 후보와 기존 보고서 근거 연결
- [x] 최종 보고서·인덱스·근거 수치/링크 검산 및 기존 사용자 변경 보존

사용자는 기존 작업 커밋·push 후 단계별 보고서를 바탕으로 최종 정리를 요청했다. 승인된 snapshot은 먼저 push했고 최종 보고서는 별도 문서로 작성한다. 기존 단계별 보고서/측정/사진을 덮어쓰지 않고 현재 구현과 역사 자료·제안을 구분한다. 코드·자원 구조는 변경하지 않으므로 빌드/게임을 반복하거나 frozen 구조도를 재생성하지 않는다.

완료: docs/portfolio/FINAL_REFACTORING_REPORT.md에 8개 큰 항목으로 문제·진행 작업·결과/검증·한계·기존 상세 보고서 링크를 정리했다. 초기 이전·개발/에이전트 환경·VS2026/SLNX·정리, 병목 계측, geometry/DDS 공유, 선택 외형/변환 행렬, 선택 파티클/공용 GPU 풀, NPC upload/BC7/상수 arena, UI BC7/UV, 검증·미적용 후속 후보를 종합했다. 각단계 통계/fixture가 다른 절감량은 합산하지 않으며 최신 오프라인 Private 중앙값1,756.43MiB·WS790.44MiB·LOCAL1,084.72MiB를 실전 peak와 구분한다. 19개 기존 보고서와 11개 데이터셋 해시·6개 Private 비교·최신3회 중앙값·GPU allocation 예시·6개 커밋을 독립 JSON 근거로 보존했고 상대 링크108개와 8개 항목 구조를 확인했다. docs/INDEX.md와 포트폴리오 인덱스를 갱신했다.

최종 검증은 종합 수치·출처/보고서 해시·byte 차이·기존 전후 통계·미적용 하늘/BC4/R8 계산·문서 링크·UTF-8·Git공백 검사다. 승인된 커밋e85eba42958acfdaff5713eb188d7959b39b2fe8와 GitHub main 일치, 원본/사용자SLNX 보존을 확인했다. 문서 추가 이후 새 빌드·게임·측정은 수행하지 않았다. 최종 보고서·근거·인덱스·이번 작업 기록은 push 후 추가한 로컬 문서이며 승인된 이전snapshot의 커밋에는 포함하지 않았다.


## 32. 테스트 편의성 목적의 최종 보고서 재작성

- [x] 4개 클라이언트 동시 테스트의 메모리 제약과 작업 목적 명시
- [x] 1번에서 저장소 이전·실행 가능 상태 확보 내용을 제거하고 에이전트 개발 환경으로 한정
- [x] 병목 측정·적용 항목 3~7에 결과물 요약, 8번은 미적용 조사와 예상 절감량만 정리
- [x] 종합 근거 JSON·수치·8개 항목·문서 링크·UTF-8·사용자 변경 보존 검증

사용자 요청에 따라 기존 단계별 기록은 유지하고 최종 보고서만 목적·구성을 재작성한다. 오프라인 단일 클라이언트 측정과 실제 게임 클라이언트 4개 동시 실행을 구분한다. 미적용 후보의 절감량은 allocation/payload/PCM/논리 GPU texel 기준을 명시하고 중첩 대안은 합산하지 않는다. 이번 범위는 문서 변경이며 구현·측정·구조도는 변경하지 않고 빌드/게임 실행을 반복하지 않는다. 커밋·push는 요청되지 않았다.

완료: 1번은 공통 지침·도구 설정·compilation database·Serena/clangd·빌드/검증 진입점·Archify 근거 관리로 한정했다. 2번 병목 측정, 3~7 적용한 공유/영웅/파티클/NPC/UI 항목에 결과물 요약을 추가했다. 8번의 종합 검증 표·일반 코드 구조 후보는 제거하고 하늘·UI/dissolve·음원·Title·후속 메모리 후보와 사용자 제외 후보만 남겼다. 하늘 BC7 72MiB, UI upload 플레이어36.8750/보스35.8125MiB, 비인게임 BGM PCM75.44MiB 등 계산 근거와 중첩/미산정 구분을 포함했다.

검증: 기존 6개 단계 Private 비교·최신 3회 중앙값 검산, 16개 출처 문서/11개 데이터셋 해시, 21개 미적용 후보의 수치·미산정 근거, 8개 항목/결과물/4개 동시 테스트 미검증 표기, 상대 링크104개 PASS. 보고서와 종합 JSON 해시·UTF-8·공백·표 구조 및 Git 공백 검사 PASS. GitHub main과 기준 커밋e85eba42 일치, 사용자 NewWod.slnx SHA-256 불변을 확인했다. 이번 변경은 로컬 문서에만 있으며 새 빌드·게임 측정·커밋·push는 수행하지 않았다.

## 33. 병목 측정 결과표 추가

- [x] 기존 비용 분해 자료에서 자원 생성 구간 10개의 측정 기준·지표 확인
- [x] 최종 보고서 2번에 Private Bytes·비중·Working Set·DXGI LOCAL 결과표와 최종 평균 추가
- [x] 원시 JSON·표 수치·합계·종합 근거 해시·문서 링크·사용자 변경 보존 검증

표는 2026-10-05 메시 공유 직후, 영웅·파티클·NPC·UI 최적화 이전의 독립 프로세스 3회 평균이다. 당시 병목 분석 기준선을 최신 최적화 후 구성 비율과 구분하고, checkpoint 순증가량과 지표별 합계 조건을 명시한다. 기존 측정 자료를 재사용하는 문서 변경이며 게임 측정·빌드·커밋·push는 수행하지 않는다.

검증: 표의 10개 구간·40개 수치와 최종 평균을 원시 JSON에 대조하고, 반올림 전 3개 지표의 구간 합계가 최종 평균과 일치함을 확인했다. 종합 근거에 당시 baseline의 범위·통계·구간/합계를 추가하고 보고서 SHA-256 갱신, 105개 상대 링크·UTF-8·공백·기존 8개 항목 검증 PASS. 사용자 NewWod.slnx SHA-256과 기준 commit/remote 일치를 유지했다. Git 공백 검사 통과, 작업 기록의 LF→CRLF 안내는 기존 저장소 정규화 안내다.

## 34. 적용 항목 하단에 메모리 감소량 추가

- [x] 전체 성과의 6개 비교를 적용 항목 3~7에 대응
- [x] 각 항목 하단에 Private Bytes 감소량·감소율·전후 값·통계/비교 조건 추가
- [x] 전체 성과 표·원시 데이터와 일치 및 보고서 해시·링크·기존 사용자 변경 보존 검증

사용자 요청에 따라 3~7번 하단에 메모리 감소량 요약을 추가한다. 5번의 선택 파티클과 공용 GPU 풀은 별도 비교로 표시하고 합산하지 않는다. 3번의 geometry 측정에 모델 DDS 추가 절감을 포함한 것으로 해석하지 않는다. 문서 변경만 진행하며 새 측정·빌드·커밋·push는 수행하지 않는다.

검증: 5개 적용 항목 하단의 6개 감소량·감소율·전후 값을 원시 JSON과 대조하고 평균/중앙값 표시를 확인했다. 종합 근거에 해당 항목 번호·하단 요약 포함 여부 및 갱신한 보고서 SHA-256을 기록했다. 기존 2번 병목 표·8개 항목·105개 상대 링크·UTF-8·공백 검증 PASS. 사용자 NewWod.slnx SHA-256과 기준 commit/remote 일치 유지. Git 공백 검사 통과, 작업 기록 LF→CRLF 정규화 안내만 남는다.


## 35. 최종 보고서 문장과 줄바꿈 정리

- [x] 긴 문장·중복 설명·불필요한 수식어 축약
- [x] 측정값·검증·한계·참조 링크를 분리하고 긴 표 셀에 줄바꿈 추가
- [x] 기존 수치·측정 조건·자료 링크 보존 및 근거 해시·문서 형식 검증

기존 8개 항목과 2번 결과표·3~7번 감소량을 유지한다. 이번 범위는 최종 보고서의 가독성 개선이다. 단일 클라이언트 오프라인 측정과 미검증 4개 동시 테스트, 적용/미적용 상태 및 지표 구분은 유지한다. 구현·측정·구조도·커밋·push는 변경하지 않는다.

완료: 52개 문구를 다듬고 반복 성과 설명을 감소량 요약으로 모았다. 최신 측정값·검증·한계는 짧은 문단/목록으로 나눴으며 참조 링크는 각각 표시한다. 표의 긴 셀과 여러 설명이 이어지는 줄에는 렌더링 줄바꿈을 추가했다.

검증: 수정 전 수치 토큰·자료 링크 전체 보존, 원시 데이터의 6개 성과/2번 병목 표/21개 후보 대조, 8개 항목·5개 감소량 요약·105개 상대 링크 PASS. 굵은 표시·표 열·UTF-8·공백·긴 일반 문장 분리 및 갱신한 보고서 해시를 확인했다. 사용자 NewWod.slnx SHA-256과 기준 commit/remote는 유지됐다. 새 빌드·게임 측정·커밋·push는 수행하지 않았다.

## 36. 에이전트 구성 파일·인덱스 구조 첨부

- [x] 실제 진입 문서·MCP/Serena 설정·문서 인덱스·검증 스크립트 경로 확인
- [x] 최종 보고서 1번에 디렉터리 트리·파일 역할·문서 확인 순서 추가
- [x] 첨부 경로·링크·생성 파일 제외·근거 해시·기존 항목/수치 보존 검증

전체 저장소 대신 에이전트 환경 관련 파일을 발췌한다. CLAUDE/GEMINI의 공통 지침 연결과 compile_commands.json의 로컬 생성/Git 제외를 명시한다. 설정 내용·개인 경로는 첨부하지 않으며 파일 역할만 설명한다. 문서 변경으로 구현·측정·구조도·커밋·push는 변경하지 않는다.

검증: 트리의 파일·디렉터리 경로를 계층대로 파싱해 실제 존재와 대조했다. 구성 파일 25개의 목록과 생성 파일 제외·문서 확인 순서를 종합 근거에 기록하고 보고서 SHA-256을 갱신했다. 기존 6개 성과·2번 결과표·21개 미적용 후보·8개 항목·5개 감소량 요약 및 상대 링크109개 PASS. UTF-8·공백·사용자 NewWod.slnx SHA-256·기준 commit/remote 일치를 확인했다. 구현 변경이 없어 빌드·게임 측정을 반복하지 않았으며 커밋·push는 수행하지 않았다.

## 37. 전체 감소량과 항목별 합산 추가

- [x] 최초 geometry 공유 전·최신 UI 적용 후 Private 중앙값 차이와 감소율 계산
- [x] 전체 성과에 전체 감소 표·항목별 단순 합계 및 비교 조건 추가
- [x] 원시 byte 계산·단순 합계·보고서 해시·기존 수치/구조·사용자 변경 보존 검증

최초/최신 3회 중앙값 차이와 6개 단계 절감량의 산술 합을 구분한다. 시나리오·바이너리·재생 조건이 다른 두 측정값 차이를 동일 조건의 전체 A/B로 표현하지 않는다. 반올림 전 byte로 계산하며 신규 실행·측정·커밋·push는 수행하지 않는다.

검증: 최초7289212928byte·최신1841754112byte의 차이5447458816byte = 5,195.10MiB/약5.07GiB, 감소율74.73%를 확인했다. 6개 단계의 반올림 전 절감량 합은5,067.88MiB/약4.95GiB이며 전체 실측 감소량으로 사용하지 않는 참고 합계로 명시했다. 종합 근거에 최초/최신 출처·통계·비교 범위·단순 합계를 기록하고 보고서 해시를 갱신했다. 기존 6개 비교·2번 결과표·21개 후보·8개 항목·109개 링크·표 구조·UTF-8·공백 검증 PASS. 사용자 SLNX와 기준 commit/remote 유지, 새 빌드·게임 측정·커밋·push는 수행하지 않았다.

## 38. 최종 보고서 커밋·push 준비

- [x] 보고서·근거·인덱스·작업 기록의 5개 파일로 범위 확정
- [x] 측정 수치·출처/보고서 해시·문서 링크·UTF-8·공백 검증
- [x] 사용자 SLNX 변경 제외 및 한글 커밋 미리보기 준비

사용자가 커밋·push를 요청했다. 공통 commit 스킬의 2단계 절차로 준비하며, 미리보기 승인 후 동일 staged snapshot을 커밋하고 origin/main에 push한다. 문서 변경만 포함하고 기존 NewWod.slnx 변경은 보존한다. 새 게임 실행·측정·빌드는 수행하지 않는다.

완료: 승인된 fingerprint 0d2acb41d3c5275a5cd4fcd95d26b02d895ef448와 메시지로 86f83a94f76e7218c4cd2f34d992f9d2d02174f1을 생성·push했다. GitHub main과 로컬 HEAD 일치, 문서 5개 반영과 기존 사용자 SLNX 변경만 남았음을 확인했다.

## 39. 서버 리팩토링 1단계 ServerCore 구현 계획

- [x] 두 서버의 동일 파일·의미 차이·의존성·프로젝트 설정 조사
- [x] ServerCore 범위·프로토콜/콘텐츠 경계·자원 수명·단계별 구현/검증 계획 작성
- [x] 1~6단계 순차 로드맵과 후속 새 더미 도구의 연결 지점 기록
- [x] 미구현 설계 Archify JSON/HTML·출처 및 문서 검증

사용자 요청은 1단계 구현 계획 작성이다. 현재 서버 소스와 동작은 변경하지 않는다. 2~4단계의 신규 시나리오 더미·agent/GUI 제어·월드 관찰, 5단계 상세 서버 문서화, 6단계 콘텐츠 데이터화 완료는 순차 후속 작업으로 구분한다. 원본 저장소와 기존 사용자 SLNX 변경을 보존하고 커밋·push는 수행하지 않는다.

검증: 최상위 동일 이름 35쌍 중 4쌍의 byte 동일·31쌍의 차이를 확인하고 소스 SHA-256·줄 위치를 근거 JSON에 기록했다. 로비 5개/게임 시작 크기 budget, 다른 OP_TYPE·풀·스레드 및 4개 수신 경로의 경계를 계획에 반영했다. 계획은 1-A~1-H 구현·검증 단위와 완료 기준으로 작성했다. compilation database 235개/서버171개, 누락 경로0개; Serena로 두 CNetworkMgr·CClient를 인덱싱(22/14/24/9 symbols)했고 Test-AgentEnvironment.ps1 -StaticOnly PASS. Archify 출처3개·showcase9/9 오류0/경고0, desktop4크기 containment와 light/dark4화면 직접 확인 PASS. 정상 종료·패킷·부하 테스트는 구현 후 수행할 계획이며 이번에는 서버 빌드·구동·DB 검증을 실행하지 않았다.

마무리: 조사 대상 89개 소스·설정의 SHA-256, 문서 상대 링크73개, frozen JSON/HTML 해시·byte, UTF-8·공백을 확인했다. 문서별 정규화 예외만 .gitattributes에 추가했고 서버/클라이언트 소스·프로젝트·개발 스크립트는 변경하지 않았다. 사용자 SLNX SHA-256과 기준 HEAD를 보존했다.

## 40. ServerCore 공통 기반 구현

- [x] 기존 빌드·LOCAL_TEST·패킷 ABI 기준 확보
- [x] 공통 정적 라이브러리·Protocol·주소·진단·Job·풀 추출
- [x] 공유 송수신·IOCP·프레임·오류·자원 수명과 서버 어댑터 연결
- [x] 정상 종료·회귀 테스트·Debug/Release·에이전트 환경 검증
- [x] 실제 구현 문서·Archify 구조도·정리 근거 작성

사용자가 1단계 구현을 요청했다. 계획의 1-A~1-H를 순차 적용한다. 새 더미 제품·GUI·월드 관찰·콘텐츠 데이터화는 다음 단계이며 이번에 함께 구현하지 않는다. 기존 사용자 SLNX 경로를 보존하며 프로젝트만 추가하고 커밋·push는 하지 않는다. 기준 빌드를 막던 LFS 와일드카드 사전 검사를 실제 Git LFS 파일 목록으로 수정한다.

완료: ServerCore 정적 라이브러리와 테스트를 추가했다. Winsock·소켓·IOCP·부분 송신·프레임·Job·풀·스레드 회수·진단을 공통화했다. Protocol은 Shared로 이동하고 이전 경로에 forwarding header를 남겼다. 서버별 OP_TYPE·content handler·작업 budget과 도메인 풀은 어댑터에 유지한다. 중복 `.cpp` 12개의 compile/filter 참조를 제거하고 복구 commit과 대체 구현을 기록했다.

수명: 연결별 callback 직렬화, 송신 완료 후 풀 반환, Disconnect 이전 native I/O 배출, 접속 세대에 따른 지연 패킷·사용자 상태 타이머 검사, 생산자 → IOCP → 콘텐츠 순서의 정상 종료를 적용했다. 부분 초기화/worker 생성 실패도 회수한다. 기존 싱글턴 자기 재해제·보스 시작 위치·로딩 재예약·몬스터 배치 범위·비활성 미니언 navigation·NPC 이벤트 대상·패킷 fall-through·누락 외형 파일의 무한 읽기 결함을 회귀 검사에 필요한 범위에서 수정했다.

검증: 서버/Core/tests Debug·Release 빌드, 구성별 Core 93항목·Protocol 구조체110종/상수106개, 네트워크12종, 보스4/5의 4인 매칭·전원 로딩 완료·첫 NPC 알림 PASS. 통합7시나리오에는 리소스 누락2종·로비 미실행·배타 포트 충돌의 실패 종료4종을 포함한다. 정상/실패 종료에서 pending·sockets·leased0을 확인했다. 마지막 패킷 분기·범위 수정 후 두 구성의 빌드/Core/전체 통합 검사를 다시 통과했다.

추가 확인: Client Debug 빌드와 실제 게임 창 시작, Release 서버 간 연결 smoke, Test-AgentEnvironment 전체와 Core Net.cpp Serena51 symbols PASS. compilation database226개·누락0개. 실제 구현 Archify showcase9/9 오류0/경고0·4viewport containment·4PNG 직접 확인 PASS. 구조도 frozen byte/해시·소스 UTF-8·문서 링크·삭제 파일 참조·기존 사용자 SLNX3경로를 확인하고 구현/정리 문서·인덱스·소스/검증 근거를 작성했다.

한계: 기존 서버/클라이언트 컴파일 경고를 유지한다. DB 없는 LOCAL_TEST이며 운영 DB·블록체인·전투 전체·장시간 부하·4개 렌더링 클라이언트는 미검증이다. NPC/매치/스킬 전체 타이머 수명과 ExtraPos/Look CSV 파싱은 후속 범위다. CPU/메모리 개선량은 측정하지 않았다. 기준 HEAD 유지, 커밋·push는 수행하지 않았다.

## 41. ServerCore 커밋·push 준비

- [x] 계획·구현·테스트·문서·구조도와 프로젝트 추가를 커밋 범위로 확정
- [x] 기존 사용자 SLNX 경로/ID와 이에 맞춘 로컬 filter 경로는 작업 트리에 보존
- [x] 소스 해시·문서 링크·UTF-8·공백·staged 프로젝트/filter 정합성 검사
- [x] 한글 메시지와 staged fingerprint를 준비하고 승인 절차 적용

사용자가 커밋·push를 요청했다. 공통 commit 스킬에 따라 미리보기를 준비한다.
승인 후 동일 snapshot을 커밋하고 origin/main으로 push한다.
SLNX/filter에는 Core/test 프로젝트 추가만 stage하고 기존 상대 경로를 유지한다.
사용자의 기존 긴 SLNX 경로·ID와 로컬 filter 호환 경로는 미커밋으로 보존한다.
직전 Debug/Release 검증 이후 구현 변경은 없어 게임 검사를 반복하지 않는다.

## 42. 클라이언트 장면 로딩 전후 비교

- [x] 메모리 최적화 이전/현재 코드·에셋과 동일한 측정 조건 확보
- [x] Title→Lobby, Lobby→Ready→Ingame, Ingame→Lobby를 실제 장면 로더로 반복 측정
- [x] 독립 프로세스 교대 실행·GPU 완료·로딩 화면 포함 여부와 결과 검증
- [x] 최종 보고서에 세 구간 결과표만 추가하고 근거 해시 갱신

사용자는 후속 서버 작업 전에 클라이언트 로딩 시간을 비교하도록 요청했다.
최적화 전 ed3428e9와 현재 d02737a5의 격리된 Release 빌드를 비교한다.
로그인/매칭/선택 대기는 제외하고 실제 장면 자원 준비 시간을 측정한다.
로비→인게임에는 READY 장면 생성도 포함한다. 보고서에는 최종 결과와 짧은 측정 조건만 기록한다.
사용자 솔루션 경로 변경과 원본 저장소를 보존하고 커밋·push는 하지 않는다.

완료: 두 commit의 Release x64를 artifacts 아래 격리 빌드했다. 개선 전 UI 21개·NPC 텍스처 25개를 당시 Git/LFS 내용으로 복원했다. 동일한 측정 코드와 외형·스킬·1920×1080 조건으로 예열 각 1회 제외, 독립 프로세스를 전후 교대로 각 5회 실행했다. 기존 메모리 profiler를 사용하지 않아 정상 로딩 화면 worker와 장면 자원 로더를 유지했다.

검증: 측정 10회·예열 2회 모두 정상 완료했다. 장면 전환부터 BGM 선택·목적지 첫 프레임 GPU fence까지 측정하고 GPU 오류·stderr가 없음을 확인했다. 로비→인게임은 각 실행의 Lobby→Ready·Ready→Ingame을 합산한 뒤 중앙값을 계산했다. 중앙값은 타이틀→로비 8.92→7.10초, 로비→인게임 40.17→15.55초, 인게임→로비 14.77→8.06초다. 원시 결과는 Git 제외 경로에 보존하고 최종 수치·바이너리/측정 코드/원시 결과 해시를 근거 JSON에 기록했다.

범위: 캐시를 비우지 않은 오프라인 자원 로딩 비교다. 로그인·매칭·선택·서버 응답 대기, 전투 전체·4클라이언트 동시 실행은 측정하지 않았다. 제품 소스는 변경하지 않았고 기존 메모리 수치를 재측정하지 않았다.

## 43. 로딩 측정 보고서 커밋·push 준비

- [x] 최종 보고서·측정 근거·인덱스·작업 기록과 솔루션/filter 7개 파일을 커밋 범위로 확정
- [x] 10회 측정·예열 2회 결과, 중앙값·해시·기존 메모리 수치·문서 링크 81개 재검증
- [x] 솔루션/filter 경로를 저장소 기준 상대 경로로 정리하고 기존 프로젝트 ID 유지
- [x] 프로젝트 5개·서버 filter 4개·실행 프로필 참조를 검증하고 한글 메시지 미리보기 갱신

사용자가 커밋·push를 요청했다. 공통 commit 스킬에 따라 미리보기 승인 후 동일 staged snapshot을 커밋하고 origin/main에 push한다. 제품 소스 변경이 없어 빌드·게임 측정을 반복하지 않는다. 공백 검사는 통과했고 Git의 기존 LF→CRLF 안내를 확인했다.

범위 갱신: 사용자가 솔루션/filter도 함께 커밋하도록 요청했다. 긴 `../../../../../../../../GitFolder/NewWod/` 접두어를 제거하고 기존 5개 프로젝트 ID를 유지했다. 경로 정리 전후 실제 대상 파일이 동일함을 확인했다. 수정 전 파일은 Git 제외 artifacts에 보관했다. 새 범위와 메시지의 미리보기 승인 전에는 커밋하지 않는다.

추가 검증: 솔루션의 프로젝트 ID 5개와 실제 ProjectGuid, 서버 filter의 4개 참조, 실행 프로필의 경로가 일치한다. VS2026 MSBuild의 ValidateSolutionConfiguration을 전체 솔루션과 서버 filter에서 Debug/x64로 통과했다. C++ 재빌드·게임 실행과 GUI 재로드 재현은 수행하지 않았다.

## 44. ServerCore 파일 표시·코드 가독성 정리

- [x] Core 공용 헤더 등록 누락을 수정하고 구현/헤더/테스트 filter 구성
- [x] ServerCore의 C++ 구현·헤더·테스트에 함수 간 빈 줄·다중 행 if·마지막 return 분리 적용
- [x] 공통 포맷 설정·작업 지침에 사용자 형식 규칙 반영
- [x] 소스 토큰/프로젝트 참조·Debug/Release 서버 빌드·Core/ABI 검사와 에이전트 환경 검증

사용자는 ServerCore 파일 누락과 코드 가독성을 지적했다. Core 프로젝트는 Net.cpp만 등록했고 공용 헤더 4개와 filters가 누락돼 있었다. 공용 헤더·구현과 별도 테스트 프로젝트의 소스를 표시하고 ServerCore 소스 7개를 포맷한다. 로직·패킷·동시성·자원 수명은 유지한다. 기존 서버/클라이언트 전체나 외부 코드를 일괄 포맷하지 않는다. 커밋·push는 요청 시 별도로 진행한다.

완료: Core의 헤더 4개를 ClInclude에 등록하고 Core/test에 filters를 추가했다. Core 소스 5개와 테스트 소스 2개, 포맷 설정을 소스·헤더·개발 설정으로 구분했다. 프로젝트 XML도 여러 줄로 정리했다. 루트 .clang-format과 공통 지침에 사용자 형식 규칙을 반영하고 구현/개발 문서를 갱신했다.

검증: 7개 소스의 UTF-8·C++ 토큰 동일·포맷 반복 안정성과 clang-format 22.1.3 dry-run PASS. 프로젝트/filter 누락·중복 없음, 기존 빌드 설정과 ProjectReference 동일, MSBuild 평가에서 헤더 4개 확인. 최종 소스 상태에서 서버 Debug/Release 빌드와 구성별 Core 93항목·ABI 구조체 110종/상수 106개 PASS. Test-AgentEnvironment -StaticOnly와 compilation database 226개, Serena Net.cpp 51 symbols PASS. 문서 상대 링크 21개·공백 검사 PASS.

범위: 기존 narrowing·signedness 등 컴파일 경고는 유지했다. GUI 표시·재로드, 이번 변경의 서버 통합/전투/운영 DB 검사는 실행하지 않았다. 소스 토큰이 같고 패킷 ABI 검사를 통과했으며 이전 통합 검증 기록은 그대로 보존했다. .slnx/.slnf·기존 서버 어댑터·클라이언트·외부 코드는 수정하지 않았다. 커밋·push는 수행하지 않았다.

## 45. if 분기 뒤 간격 추가

- [x] ServerCore의 if/else 분기 묶음 뒤에 다음 문장과 구분하는 빈 줄 하나 추가
- [x] 중첩 if·중괄호 없는 분기·else 연결과 기존 마지막 return 간격 보존
- [x] 공통 지침·포맷 설정·관련 문서 갱신 및 토큰 동일·포맷 검사

사용자의 추가 형식 규칙을 기존 ServerCore 포맷 작업에 반영한다. else 연결과 닫는 중괄호 직전은 제외하고 다음 문장이 있을 때 빈 줄을 추가한다. 공백만 변경하며 이전 검증 이후 빌드·게임 실행은 반복하지 않는다. 커밋·push는 수행하지 않는다.

완료: Concurrency.h·Session.h·Net.cpp·CoreTests.cpp의 분기 뒤에 빈 줄 60개를 추가했다. 긴 조건의 줄 나눔도 포맷 기준으로 유지했다. 공통 지침과 포맷 설정·관련 문서에 후속 규칙을 반영했다.

검증: 중첩·중괄호 없는 if, dangling else, 분기 연결·범위 끝의 간격 검사 PASS. C++ 소스 7개 토큰 동일·포맷 반복 안정성·clang-format dry-run, 기존 프로젝트/filter와 빌드 설정 보존·문서 상대 링크 21개·공백 검사 PASS. 이전 단계의 Debug/Release 빌드·Core/ABI 검증 기록을 유지했고 이번 공백 변경에 빌드·게임 실행은 반복하지 않았다.

## 46. 자동 줄 나눔 기준 250자 적용

- [x] clang-format ColumnLimit을 120에서 250으로 변경하고 공통 지침에 반영
- [x] ServerCore 소스의 긴 식·인자 목록을 다시 포맷하고 함수/if/return 간격 유지
- [x] 소스 토큰 동일·250자 초과 행·포맷 반복 안정성·공백 검사

사용자는 지나친 자동 줄 나눔을 줄이고 250자를 초과할 때 나누도록 요청했다. 기존 함수·분기 구조와 빈 줄은 유지하며 공백만 정리한다. 빌드·게임 실행 및 커밋·push는 진행하지 않는다.

완료: ColumnLimit 250과 공통 지침·개발 문서를 반영했다. ServerCore C++ 소스 7개를 검사하고 Session.h·Net.cpp·CoreTests.cpp·ProtocolAbi.cpp의 식/인자 목록 줄 나눔을 정리했다. 토큰 동일·250자 초과 행 0개·함수/분기/return 간격 유지·포맷 반복 안정성·clang-format dry-run PASS. 기존 프로젝트/filter와 빌드 설정, 문서 상대 링크 21개·공백 검사도 통과했다. 기능 변경이 없어 빌드·게임 실행은 반복하지 않았다.

## 47. while 간격·연결된 if 중괄호 보완

- [x] while 뒤 빈 줄 규칙을 공통 지침과 ServerCore에 적용
- [x] 연결된 if/else-if/else의 단일 문장 본문도 중괄호로 감싸기
- [x] 기존 문장·250자·함수/분기/return 간격 보존과 포맷·Core/ABI 검증

사용자의 추가 형식 규칙을 적용한다. 연결된 분기의 단일 함수 호출 2곳에 중괄호를 추가하고 while 뒤에 다음 문장이 있을 때 빈 줄을 둔다. 변수 선언·자원 수명·분기 연결을 변경하지 않는다. 이전 작업은 보존하고 커밋·push는 수행하지 않는다.

완료: Net.cpp의 Disconnect 오류 분기와 Concurrency.h의 풀 소유 등록 분기를 중괄호로 감쌌다. 연결된 나머지 분기는 기존 중괄호를 유지했다. Net.cpp·CoreTests.cpp의 while 뒤 8곳에 빈 줄을 추가하고 공통 지침·포맷 설정·문서를 갱신했다.

검증: 기존 토큰은 추가 중괄호 2쌍 외에 동일하고 감싼 본문은 각각 단일 함수 호출이다. else 연결·do/while 경계·닫는 중괄호 직전 검사, 250자·함수/if/return 간격·포맷 반복 안정성·clang-format dry-run PASS. 프로젝트/filter·기존 빌드 설정·문서 링크 21개·공백 검사 PASS. Core/test Debug·Release 빌드와 각 93항목·패킷 ABI 110종/106상수 PASS. 이번 변경에서 전체 서버/클라이언트 재빌드·GUI·통합/전투 검사는 실행하지 않았고 기존 경고를 숨기거나 비활성화하지 않았다.

## 48. 빌드 오류·멤버 변수 접미사 확인

- [x] VS2026 전체 Debug x64 빌드와 실제 IDE 버전 확인
- [x] VS2022에서 v145 도구 누락 오류 재현 및 해결 명령 문서화
- [x] ServerCore의 멤버 변수 접미사 확인

사용자는 빌드 오류 확인과 변수 뒤 `_`의 의미를 요청했다.
기존 포맷 작업을 보존하고 실제 빌드와 설치된 도구를 비교했다.

검증: `Build.ps1 -Configuration Debug`로 전체 솔루션 빌드 PASS.
Compilation database 226개를 생성했다. 기존 변환·미사용 변수 등의 경고는 남아 있다.
NewWod를 연 IDE 프로세스는 VS2022였다.
VS2022 MSBuild로 Core의 `PrepareForBuild`를 실행해 `MSB8020`(v145 도구 누락)을 재현했다.
VS2026으로 명시적으로 여는 명령을 마이그레이션 문서에 추가했다.

`stop_`, `failure_`, `threads_` 등은 클래스 멤버를 인자·지역 변수와 구분하는 이름 규칙이다.
C++ 문법상 특별한 기능은 없으며 기존 `m_` 방식과 혼재한다.
변수 이름·제품 소스·toolset은 변경하지 않았다.
GUI 빌드·게임 실행은 검증하지 않았고 커밋·push는 수행하지 않았다.

## 49. 멤버 변수·함수 인자 이름 규칙 확정

- [x] 멤버 `m_`·함수 인자 `_` 접두어를 공통 지침에 기록
- [x] C++ 예약 식별자 조건 확인 및 개발 문서에 근거 연결
- [x] 추가 결정 대상과 실제 적용 범위 구분

사용자가 제시한 접두어 규칙을 확정했다.
함수 인자는 `_parameter`처럼 소문자로 시작하고 `__`를 포함하지 않는다.
전역 이름에는 함수 인자 규칙을 적용하지 않는다.
지역 변수·상수·정적 멤버의 세부 표기와 기존 코드 변경 범위는 아직 확정하지 않았다.

검증: C++ 표준 초안의 예약 식별자 조항을 확인했다.
문서 링크 대상·UTF-8·공백 검사를 수행했다.
제품 소스의 이름 변경·빌드·게임 실행·커밋·push는 수행하지 않았다.

## 50. 기존 서버 코드의 멤버·인자 이름 통일

- [x] 빌드 대상의 자체 코드와 외부·프로토콜 계약 범위 분류
- [x] 멤버 `m_`·함수 인자 `_` 규칙으로 선언과 참조 변경
- [x] 식별자 외 변경·누락·예약 이름 검사
- [x] 서버 Debug/Release 빌드·Core/ABI·통합 검사

사용자가 기존 코드에도 확정된 접두어 규칙을 적용하도록 요청했다.
후속 지시에 따라 대상은 ServerCore·로비 서버·게임 서버로 한정한다.
클라이언트·Shared 공통 프로토콜·외부 코드는 변경하지 않는다.
이전 작업을 보존하며 클래스 멤버·함수 인자의 이름만 변경한다.
지역 변수·함수·타입·전역 상수는 별도 변경하지 않는다.
외부 코드·패킷·파일 형식의 필드는 계약을 확인하고 보존한다.
커밋·push는 별도 요청 시 진행한다.

완료: 서버 파일 311개에 멤버 선언 216곳·함수 인자 선언 2,912곳과 사용처를 반영했다.
총 9,924곳의 식별자를 변경했다. 정적 멤버·내부 상태 구조체 필드도 `m_`로 구분한다.
자동 도구가 누락한 `offsetof` 참조를 보완하고 `_stat`와 매크로 이름 충돌을 피하도록 `_statValue`를 사용했다.
줄 끝 공백이 있던 파일 97개도 정리했다. 기존 식별자 이외의 C++ 토큰·문자열은 보존했다.
CSV reflection에 사용하는 `tabledata.h` 필드는 유지했다.
백업·수집 목록·검증 해시는 Git 제외 `artifacts/naming-refactor`에 보관했다.

검증: 서버 Debug/Release 빌드 PASS.
각 구성에서 Core 93항목·패킷 ABI 110종/상수 106개 PASS.
네트워크 12개 경로·4참가자 보스 4/5 선택과 게임 진입·정상 종료를 두 구성에서 확인했다.
초기화 실패 4종에서도 종료 코드 1과 pending/sockets/leased=0을 확인했다.
변경 대상 재조사에서 계약 예외 외 추가 이름 변경 후보 0개를 확인했다.
UTF-8·식별자/공백 외 변경 없음·보호 경로 변경 없음·공백 검사 PASS.
Test-AgentEnvironment -StaticOnly와 compilation database 226개 PASS.

한계: Clang 분석에서는 MSVC가 허용하는 기존 상속 싱글턴 정적 멤버 정의 13곳의 진단이 남았다.
해당 정의와 로직을 변경하지 않았으며 실제 v145 빌드·회귀 검사는 통과했다.
기존 변환·미사용 변수 등의 컴파일 경고는 유지했다.
운영 DB·블록체인·GUI 플레이 전체는 이번 검증 범위가 아니다.
커밋·push는 수행하지 않았다.

## 51. 서버 이름·포맷 정리 커밋·push 준비

- [x] 서버 소스·Core 프로젝트/filter·포맷 설정·관련 문서의 커밋 범위 검토
- [x] 현재 소스 해시와 기존 Debug/Release Core·ABI·통합 검사 결과 재확인
- [x] UTF-8·문서 링크·프로젝트/filter·staged 공백 검사 및 미리보기 준비
- [ ] 미리보기 승인 후 동일 snapshot 커밋 및 origin/main push

사용자가 커밋·push를 요청했다. 공통 commit 스킬의 미리보기 승인 절차를 적용한다.
소스는 마지막 검증 이후 변경되지 않아 빌드·서버 실행을 반복하지 않는다.
클라이언트·공통 프로토콜·외부 코드·빌드 산출물·로그는 커밋 범위에서 제외한다.
정확한 메시지와 staged fingerprint는 Git 제외 artifacts에 저장한다.
