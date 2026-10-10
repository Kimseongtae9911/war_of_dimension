# 로컬 개발 환경

NPC 객체 상수 arena 검사는 `./scripts/Test-ObjectConstants.ps1 -Configuration Debug`와 `Release`로 실행한다. GPU frame 격리·부족 시 확장·fence 완료 후 재사용·종료 잔여 page 0을 검사한다. `./scripts/Capture-Monsters.ps1 -Configuration Release -OutputDirectory artifacts/logs/npc-recheck`는 실제 클라이언트 셰이더로 9종 앞/뒤 PNG를 생성하고 upload 회수·정상 종료를 확인한다. 압축·메모리 재현과 한계는 [NPC 최적화 문서](../portfolio/NPC_MEMORY_OPTIMIZATION.md)를 따른다.

모델 DDS 공유 검사는 `./scripts/Test-DdsSharing.ps1 -Configuration Debug`와 `Release`로 실행한다. WARP와 하드웨어 device 격리, 실제 Chest/Beholder의 texture 4→2, GPU 픽셀 비교·binding 독립·upload와 마지막 소유자 해제를 확인한다. [DDS 구현·검증](../portfolio/NPC_RESOURCE_SHARING_REVIEW.md#후속-적용-모델-dds-공유)과 [구조도](../diagrams/dds-sharing/README.md)를 참조한다.

파티클 공용 풀 검사는 `./scripts/Test-ParticleBufferPool.ps1 -Configuration Debug`와 `Release`로 실행한다. 실제 GPU에서 종류 간 재사용·fence 대기·용량 확장·상태 복사·종료 해제를 검사한다. 전체 종류의 동일 재생 비교는 `Measure-ClientMemory.ps1 -Scenario ParticleReuse`이며 [재현과 검증 범위](../portfolio/PARTICLE_BUFFER_POOL.md)를 따른다.

UI 압축 검사는 `./scripts/Capture-UiTextures.ps1 -Configuration Release -Role player -OutputDirectory artifacts/logs/ui-recheck/player`로 수행한다. `-Role boss`와 Debug도 지원하며 역할별 UI 20종의 실제 할당량·내용 영역 실패 경로·16개 GPU 화면·정상 종료 DEFAULT/UPLOAD 40개 반환을 검사한다. [UI 구현·실측·품질](../portfolio/UI_TEXTURE_COMPRESSION.md)을 참조한다.

## 사전 조건

- Windows와 PowerShell 7.
- Visual Studio 2026의 Desktop development with C++ 워크로드, MSVC v145, Windows SDK.
- 저장소의 모델·텍스처·음원과 FMOD 런타임·import library. LFS checkout 시 실제 에셋을 받아야 한다.
- 현재 사용하지 않는 오래된 TBB NuGet 의존성은 기준선 확인 후 정리했다. 빌드 시 설치 도구와 포함된 리소스를 점검하며 패키지 복원은 필요하지 않다.
- 에이전트용 도구는 [에이전트 환경](AGENTS.md)을 참조한다.

## 빌드와 실행

저장소 루트에서 실행한다.

```powershell
./scripts/Build.ps1 -Configuration Debug
./scripts/Start-Local.ps1 -Configuration Debug
./scripts/Stop-Local.ps1
```

기본 빌드는 `NewWod.slnx`를 사용한다. 실행 프로젝트 3개와 ServerCore 정적 라이브러리·테스트를 포함한다. `Build.ps1 -Module Servers`는 서버·Core·테스트가 들어 있는 `NewWod.Servers.slnf`를 빌드한다. `-Module Client`, `-Module LobbyServer`, `-Module GameServer`는 해당 프로젝트와 의존성을 빌드한다. `-Configuration Release`와 `-Rebuild`도 지원한다. 빌드 결과는 `artifacts/bin/<Configuration>/<Module>`에, 중간 결과는 `artifacts/obj`에, 빌드·서버 실행 로그는 `artifacts/logs`에 둔다. 클라이언트용 FMOD DLL은 실행 파일 옆으로 복사한다.

실행 순서는 로비 서버 → 게임 서버 → 클라이언트다. 서버는 숨긴 백그라운드 프로세스, 클라이언트는 조작 가능한 게임 창으로 시작한다. 모델과 CSV의 상대 경로를 유지하기 위해 작업 디렉터리는 각 모듈 원본 폴더로 지정한다. 다른 프로세스가 8910/8911 포트를 사용하면 시작을 중단한다.

```powershell
./scripts/Test-Local.ps1 -Configuration Debug
./scripts/Test-Local.ps1 -Configuration Release
```

점검 스크립트는 자신이 시작한 프로세스를 종료하고 JSON 결과를 남긴다. 확인 범위는 서버 listener, 서버 간 TCP 연결, 클라이언트 게임 창 시작이다. 스크립트 결과만으로 UI 렌더링·로그인·전투 전체를 검증했다고 간주하지 않는다. 로그인 화면 렌더링은 별도 화면 점검으로 확인했다.

`Stop-Local.ps1`은 `.runtime/processes.json`의 실행 파일 경로와 시작 시간이 일치하는 PID만 중지한다. 서버는 named event로 종료를 요청하고 15초 이내 종료 코드 0을 확인한다. 시간 초과 시 해당 프로세스를 정리하고 실패로 보고한다. 클라이언트는 기존 프로세스 종료 방식을 유지한다. 서버 종료·실패 회수의 구현과 범위는 [ServerCore](../architecture/SERVER_CORE.md)에 있다.

ServerCore 검사는 다음 순서로 실행한다. Python 3.12 이상의 실행 경로를 지정한다.

```powershell
./scripts/Build.ps1 -Module Servers -Configuration Debug
./scripts/Test-ServerCore.ps1 -Configuration Debug
./scripts/Test-ServerCoreIntegration.ps1 -Configuration Debug -Python <python.exe>
```

Release도 같은 순서로 검사한다. 통합 검사는 8910/8911 포트를 사용하므로 동시에 실행하지 않는다. 네트워크 참가자 4개의 매칭·로딩 전환을 검사하며 게임 창 4개를 렌더링하는 검사는 아니다. Core·테스트는 실행 프로필의 서버가 아니다.

## Visual Studio 통합 개발

`NewWod.slnx`의 Servers 폴더에는 로비·게임 서버, Client 폴더에는 클라이언트, Development 폴더에는 실행·검증 스크립트를 배치했다. XML 형식으로 변환했고 프로젝트·x64 Debug/Release 설정은 유지한다. 기존 루트 `.sln`은 제거했다. 원본 프로젝트와 에셋의 디스크 위치는 유지한다. 개별 모듈 솔루션도 호환을 위해 남겨 두었다.

`NewWod.slnxLaunch`에 공유 실행 프로필을 제공한다.

- `Local full stack`: 로비 → 게임 서버 → 클라이언트를 디버거로 시작한다.
- `Client (servers already running)`: 클라이언트만 디버거로 시작한다.

Visual Studio의 다중 프로젝트 실행은 프로세스 시작 순서를 지정하지만 서버 listener 준비까지 기다리지는 않는다. 처음 실행하거나 클라이언트 연결을 확실히 준비하려면 다음 순서로 사용한다.

```powershell
./scripts/Build.ps1 -Configuration Debug
./scripts/Start-Local.ps1 -Configuration Debug -ServersOnly
# Visual Studio에서 Client (servers already running) 프로필로 F5
./scripts/Stop-Local.ps1
```

이때 서버는 디버거 밖에서 실행된다. 서버 디버깅도 필요하면 실행 중인 서버 프로세스에 Attach한다. 프로필이 표시되지 않으면 솔루션 시작 프로젝트 설정에서 Multi-project launch profiles 기능을 확인한다. 각 프로젝트의 DebuggerWorkingDirectory는 모듈 폴더로 설정되어 있다. 프로필 JSON과 프로젝트 경로는 검사했고, 통합 빌드와 스크립트 실행은 검증했다. Visual Studio GUI에서 프로필을 선택해 F5로 실행하는 과정은 아직 직접 점검하지 않았다.

프로필 형식과 solution filter 동작은 [Microsoft 다중 시작 프로젝트 문서](https://learn.microsoft.com/en-us/visualstudio/ide/how-to-set-multiple-startup-projects?view=visualstudio)와 [solution filter 문서](https://learn.microsoft.com/en-us/visualstudio/msbuild/solution-filters?view=visualstudio)를 따른다.

## C++ 이름 규칙

확정된 이름 규칙은 [공통 작업 지침](../../AGENTS.md)을 따른다.
클래스 멤버는 `m_count`, 함수 인자는 `_count`처럼 구분한다.
함수 인자에 `_` 접두어를 사용할 때는 첫 글자를 소문자로 쓰고 `__`를 포함하지 않는다.
예약 식별자의 범위는 [C++ 표준 초안](https://eel.is/c++draft/lex.name#4)을 참조한다.

기존 ServerCore·로비 서버·게임 서버의 멤버·함수 인자에도 규칙을 적용했다.
정적 멤버와 서버 내부 상태 구조체의 필드도 `m_`로 구분한다.
클라이언트·공통 프로토콜·외부 코드는 변경하지 않았다.
CSV 데이터 구조체는 필드 이름을 `boost::pfr::names_as_array`로 읽어 열과 연결하므로 기존 필드 이름을 유지한다.
지역 변수·함수·타입·전역 상수는 이번 변경 대상에서 제외했다.

## C++ 코드 포맷

저장소 루트의 `.clang-format`을 사용한다.
긴 식·인자 목록의 줄 나눔 기준은 250자다.
함수 간격·`if`·마지막 `return`의 규칙은 [공통 작업 지침](../../AGENTS.md)을 따른다.
`if`·`else if`·`else` 묶음과 `while` 뒤, 마지막 `return` 앞의 빈 줄은 편집할 때 함께 유지한다.
연결된 `if`·`else if`·`else`의 본문은 한 문장이어도 중괄호로 감싼다.
외부 코드·바이너리 에셋은 일괄 포맷 대상에서 제외한다.

ServerCore 공용 헤더는 Core 프로젝트의 `Header Files`에서 확인한다.
테스트 소스는 별도 `ServerCore.Tests` 프로젝트에 표시한다.
두 프로젝트의 `Development`에는 공통 포맷 설정을 연결했다.

## 메시 공유 검증

```powershell
./scripts/Test-MeshSharing.ps1 -Configuration Debug -AuditAssets
./scripts/Test-MeshSharing.ps1 -Configuration Release -AuditAssets
```

빌드한 클라이언트의 전용 진입점을 사용해 창·네트워크 없이 D3D12 자원을 생성하고 공유·수명을 검사한다. 기본 12개 검사는 WARP와 별도 D3D12 하드웨어 adapter가 필요하다. `-AuditAssets`는 실제 3개 모델 파일의 base geometry를 WARP에서 로딩해 생성·재사용 횟수를 확인한다. 결과 JSON은 `artifacts/logs/test-mesh-sharing-<Configuration>.json`, `audit-mesh-assets-<Configuration>.json`에 남긴다. 구현 범위와 결과는 [메시 공유 문서](../architecture/MESH_SHARING.md)를 참조한다.

## 클라이언트 메모리 비교

```powershell
./scripts/Build.ps1 -Configuration Release -Module Client
./scripts/Measure-ClientMemory.ps1 -Configuration Release -Runs 3
python ./scripts/Measure-ClientAssets.py --profile artifacts/logs/client-memory/shared-Release-1.json --output artifacts/logs/client-asset-memory-ingame.json
```

전용 `--profile-client-memory <report.json> legacy|shared` 진입점이 이전 geometry 로딩 방식을 현재 바이너리에서 재현하고 현재 공유 방식과 비교한다. 각 실행은 독립된 숨김 클라이언트 프로세스에서 실제 인게임 자원을 로딩하고 GPU 완료·기존 upload 해제 후 프로세스 및 DXGI 메모리를 수집한다. 네트워크와 loading render thread는 측정 모드에서만 생략한다. 일반 실행은 기존 경로를 유지한다. 전체 로그인·전투 실행과 과거 바이너리 직접 비교는 이 측정의 범위가 아니다.

Python 3.12 이상의 실행 가능한 설치/가상 환경을 사용한다. 결과 로그는 `artifacts/logs/client-memory`에 둔다. GPU와 수 GiB 메모리가 필요하므로 다른 게임/검증을 동시에 실행하지 않는다. 의미가 다른 지표를 합산하지 않으며 [포트폴리오 문서](../portfolio/CLIENT_MEMORY_OPTIMIZATION.md)의 결과·정의·영웅 계산·그래프 재현 절차를 따른다.

현재 경로만 세부 계측할 때는 `Measure-ClientMemory.ps1 -Runs 3 -Modes shared -OutputDirectory artifacts/logs/client-memory-breakdown`을 사용한다. 기존 A/B 로그를 덮어쓰지 않고 구간/파티클 카운터를 수집한다. `Build-ClientMemoryBreakdown.py`로 JSON/CSV를 만들며 결과·재현·해석은 [현재 메모리 비용 분해](../portfolio/CLIENT_MEMORY_BREAKDOWN.md)에 있다.

구간별 전후 비교는 두 모드를 같은 바이너리로 교대 실행한 새 summary를 사용한다. `--compare`는 모드별 완료 횟수·파티클 시나리오·각 지표의 구간 합계를 확인하고 JSON/CSV를 생성한다. `--charts`를 추가하면 matplotlib이 필요하다.

```powershell
./scripts/Measure-ClientMemory.ps1 -Configuration Release -Runs 3 -OutputDirectory artifacts/logs/client-memory-stage-comparison
python ./scripts/Build-ClientMemoryBreakdown.py --compare --summary artifacts/logs/client-memory-stage-comparison/summary-Release.json --output artifacts/logs/client-memory-stage-comparison-report.json
```

## 영웅 선택 파츠 검증·측정

```powershell
./scripts/Test-HeroSelection.ps1 -Configuration Debug
./scripts/Test-HeroSelection.ps1 -Configuration Release
./scripts/Measure-ClientMemory.ps1 -Configuration Release -Runs 3 -Scenario HeroParts -OutputDirectory artifacts/logs/hero-selected-parts-final
python ./scripts/Build-ClientMemoryBreakdown.py --compare-hero --summary artifacts/logs/hero-selected-parts-final/summary-Release.json --output artifacts/logs/hero-selected-parts-report.json --charts
```

양 구성은 먼저 빌드한다. 실제 `ModularModel.bin`의 남녀·다른 파츠 조합, 본·텍스처 연결 및 모든 보존 행렬을 검사한다. `full`/`selected` 각 3회는 같은 바이너리·geometry 공유·확정 외형으로 비교한다. 기존 geometry 비교와 다른 로그 경로를 사용한다. [범위와 결과](../portfolio/HERO_SELECTED_PARTS.md)를 참조한다.

로비 작업 큐와 분할 테스트 전환 패킷은 새 로컬 서버에서 검사한다. Python이 0 이외로 종료하면 검사 실패다.

```powershell
./scripts/Start-Local.ps1 -Configuration Release -ServersOnly
try { python ./scripts/Test-LobbyTestTransition.py --output artifacts/logs/test-lobby-transition-Release.json }
finally { ./scripts/Stop-Local.ps1 }
```

## 로컬 모드와 리소스

선택 스킬 파티클은 `Test-ParticleSelection.ps1 -Configuration Debug|Release`로 의존 종류·fallback·스냅샷을 검사한다. 두 구성은 먼저 빌드한다. `Measure-ClientMemory.ps1 -Scenario ParticleSkills -Runs 3 -OutputDirectory artifacts/logs/particle-selected-skills-new`는 동일 외형·스킬 fixture에서 전체/선택 풀을 비교하고 실제 GPU 생성·제출 이후의 해제도 검사한다. 다른 GPU 검증과 동시에 실행하지 않는다. `Build-ClientMemoryBreakdown.py --compare-particles`로 JSON/CSV를 생성한다. 기존 Geometry/HeroParts 측정은 전체 파티클 종류 풀을 유지해 이전 근거와 구분한다.

새 로컬 서버에서 `Test-ParticleSelectionNetwork.py --boss-job 4|5 --output <결과.json>`를 실행하면 네 TCP 참가자의 로비 매칭, 수동·자동 선택, 분할 READY header와 게임 시작 전 16개 스킬 수신을 검사한다. 각 보스·구성은 새 서버로 실행하고 Python 종료 코드 0을 확인한다. 명령·실측·한계는 [선택 스킬 파티클 문서](../portfolio/PARTICLE_SELECTED_SKILLS.md)를 참조한다.

원본 `protocol.h`의 `LOCAL_TEST`가 켜져 있고 `WITH_DATABASE`는 꺼져 있다. 루프백 주소와 로비 8910, 게임 8911 포트를 사용한다. DB 없는 기존 개발 모드를 재현한다. 실제 ODBC DSN과 저장 프로시저, DB 인증·거래 동작은 검증하지 않았다.

필수 `Server/Game_Server/Resource/HeightMesh.obj`는 원본에서 Git ignore로 빠져 있었다. 원본 `Resource.zip`의 비어 있지 않은 파일을 복원했다. 원본 로컬 폴더의 같은 이름 파일은 0바이트라서 사용하지 않았다. 리소스 폴더의 `.obj`는 데이터이므로 Git ignore 예외로 관리한다.

모델·텍스처·음원을 소스 분석 대상과 혼동하여 제거하지 않는다. 외부 서버 주소나 DB 연동을 활성화할 때는 별도의 설정·보안·동작 검증이 필요하다.
