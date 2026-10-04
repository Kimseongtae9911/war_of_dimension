# War_Of_Dimension

## 로컬 개발 시작

Windows, PowerShell 7, Visual Studio 2026의 C++ 워크로드/MSVC v145가 필요하다. 핵심 로비 서버·게임 서버·클라이언트는 x64 Debug/Release로 빌드한다.

```powershell
./scripts/Build.ps1 -Configuration Debug
./scripts/Start-Local.ps1 -Configuration Debug
./scripts/Stop-Local.ps1
```

Visual Studio에서는 `NewWod.slnx`를 열어 로비·게임 서버와 클라이언트를 함께 개발한다. 서버 작업만 할 때는 `NewWod.Servers.slnf`를 연다. 실행 순서와 서버 준비를 확인하는 로컬 실행 스크립트를 권장한다. 현재는 기존 DB 없는 로컬 모드를 사용한다.

- [빌드·실행 안내](docs/development/SETUP.md)
- [에이전트 공통 지침](AGENTS.md) · [도구 설정](docs/development/AGENTS.md)
- [VS2026 이전](docs/development/VS2026.md) · [파일 정리 기록](docs/development/CLEANUP.md)
- [메시 공유 구현과 테스트](docs/architecture/MESH_SHARING.md) · [공유 구조도](docs/diagrams/mesh-sharing/mesh-sharing.architecture.html)
- [문서 인덱스](docs/INDEX.md) · [리팩토링 우선순위](docs/REFACTORING_PLAN.md)

GitLab 최신 코드와 필수 에셋으로 새 이력을 시작했다. 초기 커밋은 [GitHub 저장소](https://github.com/Kimseongtae9911/war_of_dimension)에 업로드했으며 대용량 에셋은 Git LFS로 관리한다.

## 노션 링크
김성태(서버) : https://seongtae07.notion.site/52c74a22708540079e299263b25fcac9?pvs=4

강현석(클라이언트) : https://kanghyunsuk.notion.site/06906308da1f480b9713a939561d751c?v=92c270edb61441ed94ae6dfb28b14a97&pvs=4

### 영상 확인하기(아래 이미지 클릭)
[![Video Label](http://img.youtube.com/vi/ru5ujFk1leQ/0.jpg)](https://youtu.be/ru5ujFk1leQ)
