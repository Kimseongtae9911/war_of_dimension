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
- [ ] 공통 commit 스킬에 따른 미리보기 승인 후 커밋 및 요청된 GitHub push

사용자의 커밋·push 요청에 따라 8~12번의 완료 작업을 함께 준비한다. 저장소는 `C:/GitFolder/NewWod`, 브랜치는 `main`, 원격은 `Kimseongtae9911/war_of_dimension`이다. `NewWod.slnx`의 로컬 `GitFolder/NewWod`에 의존하는 프로젝트 경로 변경은 이번 작업에서 제외하고 작업 트리에 보존한다. 실제 Debug/Release 빌드·GPU/행렬 검사·로컬 전환 검증과 현재 소스/바이너리 해시를 확인한 후 준비한 snapshot을 미리보기로 제시한다. 공통 스킬의 승인 전에는 커밋·push하지 않는다.

준비 검증: 관련 파일 59개만 stage했고 신규 파일은 최대 약 692KiB로 신규 대용량 LFS 등록 대상은 없다. 기존 메모리 근거와 영웅 전후 비교 재생성·원시 합계·실패 입력·현재 소스 18개 및 실행 파일 해시 확인 PASS. staged Python/JSON 구문·PowerShell 구문·UTF-8·문서 링크·신규 파일 및 변경 줄 공백 검사 PASS. 전체 기존 소스 줄까지 공백 검사를 확장했을 때 기존 GameFramework.cpp 공백이 감지되어 신규/변경 줄로 범위를 맞췄으며 기존 줄은 정규화하지 않았다. Archify 9/9·오류/경고 0 및 앞선 직접 시각 점검 결과를 확인했다. 원격 main을 fetch하여 HEAD와 일치함을 확인했다. 미리보기 승인 대상에서 제외한 변경은 `NewWod.slnx` 하나이며 신규 미추적 파일은 없다.
