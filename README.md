# War_Of_Dimension

## 로컬 개발 시작

Windows, PowerShell 7, Visual Studio 2026의 C++ 워크로드/MSVC v145가 필요하다. 핵심 로비 서버·게임 서버·클라이언트는 x64 Debug/Release로 빌드한다.

```powershell
./scripts/Build.ps1 -Configuration Debug
./scripts/Start-Local.ps1 -Configuration Debug
./scripts/Stop-Local.ps1
```

Visual Studio에서는 `NewWod.sln`을 연다. 실행 시 각 모듈 폴더를 작업 디렉터리로 사용해야 하므로 로컬 실행 스크립트를 권장한다. 현재는 기존 DB 없는 로컬 모드를 사용한다.

- [빌드·실행 안내](docs/development/SETUP.md)
- [에이전트 공통 지침](AGENTS.md) · [도구 설정](docs/development/AGENTS.md)
- [VS2026 이전](docs/development/VS2026.md) · [파일 정리 기록](docs/development/CLEANUP.md)
- [문서 인덱스](docs/INDEX.md) · [리팩토링 우선순위](docs/REFACTORING_PLAN.md)

GitLab 최신 코드에서 새 로컬 이력을 준비하고 있다. 초기 커밋은 사용자 요청에 따라 준비하며 GitHub 업로드는 보류 상태다.

## 노션 링크
김성태(서버) : https://seongtae07.notion.site/52c74a22708540079e299263b25fcac9?pvs=4

강현석(클라이언트) : https://kanghyunsuk.notion.site/06906308da1f480b9713a939561d751c?v=92c270edb61441ed94ae6dfb28b14a97&pvs=4

### 영상 확인하기(아래 이미지 클릭)
[![Video Label](http://img.youtube.com/vi/ru5ujFk1leQ/0.jpg)](https://youtu.be/ru5ujFk1leQ)
