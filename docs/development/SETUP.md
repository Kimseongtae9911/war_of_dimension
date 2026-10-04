# 로컬 개발 환경

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

기본 빌드는 핵심 3개 프로젝트가 들어 있는 `NewWod.slnx`를 사용한다. `Build.ps1 -Module Servers`는 같은 솔루션의 서버 전용 filter인 `NewWod.Servers.slnf`를 빌드한다. `-Module Client`, `-Module LobbyServer`, `-Module GameServer`는 해당 프로젝트만 빌드한다. `-Configuration Release`와 `-Rebuild`도 지원한다. 빌드 결과는 `artifacts/bin/<Configuration>/<Module>`에, 중간 결과는 `artifacts/obj`에, 빌드·서버 실행 로그는 `artifacts/logs`에 둔다. 클라이언트용 FMOD DLL은 실행 파일 옆으로 복사한다.

실행 순서는 로비 서버 → 게임 서버 → 클라이언트다. 서버는 숨긴 백그라운드 프로세스, 클라이언트는 조작 가능한 게임 창으로 시작한다. 모델과 CSV의 상대 경로를 유지하기 위해 작업 디렉터리는 각 모듈 원본 폴더로 지정한다. 다른 프로세스가 8910/8911 포트를 사용하면 시작을 중단한다.

```powershell
./scripts/Test-Local.ps1 -Configuration Debug
./scripts/Test-Local.ps1 -Configuration Release
```

점검 스크립트는 자신이 시작한 프로세스를 종료하고 JSON 결과를 남긴다. 확인 범위는 서버 listener, 서버 간 TCP 연결, 클라이언트 게임 창 시작이다. 스크립트 결과만으로 UI 렌더링·로그인·전투 전체를 검증했다고 간주하지 않는다. 로그인 화면 렌더링은 별도 화면 점검으로 확인했다.

`Stop-Local.ps1`은 `.runtime/processes.json`의 실행 파일 경로와 시작 시간이 일치하는 PID만 중지한다. 기존 서버 구현에 정상 종료 인터페이스가 없어 개발 프로세스를 종료하는 방식이다. 운영 서비스의 graceful shutdown은 후속 리팩토링 범위다.

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

## 메시 공유 검증

```powershell
./scripts/Test-MeshSharing.ps1 -Configuration Debug -AuditAssets
./scripts/Test-MeshSharing.ps1 -Configuration Release -AuditAssets
```

빌드한 클라이언트의 전용 진입점을 사용해 창·네트워크 없이 D3D12 자원을 생성하고 공유·수명을 검사한다. 기본 12개 검사는 WARP와 별도 D3D12 하드웨어 adapter가 필요하다. `-AuditAssets`는 실제 3개 모델 파일의 base geometry를 WARP에서 로딩해 생성·재사용 횟수를 확인한다. 결과 JSON은 `artifacts/logs/test-mesh-sharing-<Configuration>.json`, `audit-mesh-assets-<Configuration>.json`에 남긴다. 구현 범위와 결과는 [메시 공유 문서](../architecture/MESH_SHARING.md)를 참조한다.

## 로컬 모드와 리소스

원본 `protocol.h`의 `LOCAL_TEST`가 켜져 있고 `WITH_DATABASE`는 꺼져 있다. 루프백 주소와 로비 8910, 게임 8911 포트를 사용한다. DB 없는 기존 개발 모드를 재현한다. 실제 ODBC DSN과 저장 프로시저, DB 인증·거래 동작은 검증하지 않았다.

필수 `Server/Game_Server/Resource/HeightMesh.obj`는 원본에서 Git ignore로 빠져 있었다. 원본 `Resource.zip`의 비어 있지 않은 파일을 복원했다. 원본 로컬 폴더의 같은 이름 파일은 0바이트라서 사용하지 않았다. 리소스 폴더의 `.obj`는 데이터이므로 Git ignore 예외로 관리한다.

모델·텍스처·음원을 소스 분석 대상과 혼동하여 제거하지 않는다. 외부 서버 주소나 DB 연동을 활성화할 때는 별도의 설정·보안·동작 검증이 필요하다.
