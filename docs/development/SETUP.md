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

`Build.ps1 -Module Client`처럼 특정 모듈만 선택할 수 있다. `-Configuration Release`와 `-Rebuild`도 지원한다. 빌드 결과는 `artifacts/bin/<Configuration>/<Module>`에, 중간 결과는 `artifacts/obj`에, 빌드·서버 실행 로그는 `artifacts/logs`에 둔다. 클라이언트용 FMOD DLL은 실행 파일 옆으로 복사한다.

실행 순서는 로비 서버 → 게임 서버 → 클라이언트다. 서버는 숨긴 백그라운드 프로세스, 클라이언트는 조작 가능한 게임 창으로 시작한다. 모델과 CSV의 상대 경로를 유지하기 위해 작업 디렉터리는 각 모듈 원본 폴더로 지정한다. 다른 프로세스가 8910/8911 포트를 사용하면 시작을 중단한다.

```powershell
./scripts/Test-Local.ps1 -Configuration Debug
./scripts/Test-Local.ps1 -Configuration Release
```

점검 스크립트는 자신이 시작한 프로세스를 종료하고 JSON 결과를 남긴다. 확인 범위는 서버 listener, 서버 간 TCP 연결, 클라이언트 게임 창 시작이다. 스크립트 결과만으로 UI 렌더링·로그인·전투 전체를 검증했다고 간주하지 않는다. 로그인 화면 렌더링은 별도 화면 점검으로 확인했다.

`Stop-Local.ps1`은 `.runtime/processes.json`의 실행 파일 경로와 시작 시간이 일치하는 PID만 중지한다. 기존 서버 구현에 정상 종료 인터페이스가 없어 개발 프로세스를 종료하는 방식이다. 운영 서비스의 graceful shutdown은 후속 리팩토링 범위다.

## 로컬 모드와 리소스

원본 `protocol.h`의 `LOCAL_TEST`가 켜져 있고 `WITH_DATABASE`는 꺼져 있다. 루프백 주소와 로비 8910, 게임 8911 포트를 사용한다. DB 없는 기존 개발 모드를 재현한다. 실제 ODBC DSN과 저장 프로시저, DB 인증·거래 동작은 검증하지 않았다.

필수 `Server/Game_Server/Resource/HeightMesh.obj`는 원본에서 Git ignore로 빠져 있었다. 원본 `Resource.zip`의 비어 있지 않은 파일을 복원했다. 원본 로컬 폴더의 같은 이름 파일은 0바이트라서 사용하지 않았다. 리소스 폴더의 `.obj`는 데이터이므로 Git ignore 예외로 관리한다.

모델·텍스처·음원을 소스 분석 대상과 혼동하여 제거하지 않는다. 외부 서버 주소나 DB 연동을 활성화할 때는 별도의 설정·보안·동작 검증이 필요하다.
