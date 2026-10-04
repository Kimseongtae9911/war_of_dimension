# 로컬 이전 준비

이 문서는 1차 이전 준비 시점의 기록이다. 후속 실행 환경·VS2026·에이전트 설정·파일 정비의 현재 상태는 [개발 환경](development/SETUP.md)과 [작업 목록](../tasks/todo.md)을 참조한다.

## 원본과 대상

- 원본: https://gitlab.com/hhs4/war_of_dimension
- 원본 브랜치: `main`
- 기준 커밋: `78ac46327904995c120fdaa66a6310e5c88de32a`
- 로컬 대상: `C:\GitFolder\NewWod`
- 향후 대상: 개인 GitHub 계정 `Kimseongtae9911`의 공개 `war_of_dimension` 저장소
- 이전 방식: 최신 tracked 파일을 가져와 새 이력 시작. 이전 브랜치·커밋 이력은 원본에 남겨 둔다.
- GitHub 저장소 생성·push: 사용자 요청으로 보류. 로컬 remote는 설정하지 않는다.
- 초기 커밋: 아직 생성하지 않았다.

원본 로컬 저장소 `C:\GitFolder\war_of_dimension`의 HEAD가 GitLab 최신 커밋과 일치하고 작업 트리가 깨끗함을 확인했다. 원본 Git 인덱스의 tracked 파일 17,298개를 `git checkout-index`로 복사했다. 원본 저장소와 원격은 변경하지 않았다.

## 로컬 Git와 LFS

새 저장소를 `main` 브랜치로 초기화하고 Git LFS를 로컬 설정으로 설치했다. 최신 파일 중 50MiB 이상인 에셋 11개, 합계 약 1,595.5MiB를 `.gitattributes`에 명시적으로 등록했다.

| 파일 | 크기(MiB, 반올림) |
|---|---:|
| `Client/WarOfDimension/Model/ModularModel.bin` | 378.4 |
| `Client/WarOfDimension/Model/Plane1.bin` | 317.5 |
| `Client/WarOfDimension/Model/Plane.bin` | 317.1 |
| `Client/WarOfDimension/Skybox/Space.dds` | 96.0 |
| `Client/WarOfDimension/Skybox/Night.dds` | 84.4 |
| `Client/WarOfDimension/Model/LobbyScene.bin` | 79.2 |
| `Client/WarOfDimension/Image/GUI/SkillInfo.dds` | 66.9 |
| `Client/WarOfDimension/Model/Textures/t_minotaur1_AlbedoTransparency.dds` | 64.0 |
| `Client/WarOfDimension/Model/Textures/t_minotaur1_AO.dds` | 64.0 |
| `Client/WarOfDimension/Model/Textures/t_minotaur1_MetallicSmoothness.dds` | 64.0 |
| `Client/WarOfDimension/Model/Textures/t_minotaur1_Normal.dds` | 64.0 |

원본 이력에는 더 큰 배포 압축 파일도 있지만, 새 이력에는 과거 객체를 가져오지 않는다.

LFS 대상은 Git 인덱스에서 pointer로 저장되고, 작업 폴더에는 실제 에셋을 유지한다. 새로 50MiB 이상인 에셋을 추가할 때는 추가 전에 LFS 대상도 등록한다.

## 인덱스의 범위

원본 `.gitignore`에는 `*.obj`와 `*.meta`가 포함된다. 원본에서 tracked인 NavMesh `.obj`는 실행 리소스이므로 일반적인 `git add .`로는 이전에서 빠질 수 있다. 원본 tracked 목록을 명시적인 pathspec으로 사용하여 해당 파일도 포함해 stage한다.

IDE 개인 설정인 `.idea/`와 빈 `Server/blockchainTest/private_key.pem`은 로컬에 두고 stage 대상에서 제외한다. 현재 파일에서 비밀키를 검출한 것은 아니며, 나중에 같은 파일에 키를 생성해 실수로 등록하는 것을 방지하기 위한 제외다.

추가한 문서와 `.gitattributes`, `.gitignore`의 이전용 설정을 포함해 초기 커밋 후보로 stage했다. 게임 소스 코드는 수정하지 않았다.

## 확인 결과

- 초기 커밋 후보: 17,294개 파일. 원본에서 IDE 파일 5개와 빈 키 파일 1개를 제외하고 문서 2개를 추가했다.
- 원본 Git 인덱스와 새 인덱스를 대조하여 의도한 제외 외에 누락이 없음을 확인했다. 기존 파일 중 Git 객체가 달라진 것은 LFS 에셋 11개와 이전 설정 파일 2개뿐이다. C++ 소스·헤더는 원본과 동일하다.
- LFS pointer 11개의 SHA-256과 크기를 실제 에셋과 대조했다. 로컬 LFS 객체도 11개가 존재하며 크기가 일치한다.
- stage된 일반 Git blob 중 100MiB 초과 파일은 0개다.
- 로컬 커밋 수는 0이며 remote는 없다.
- 빌드 확인은 TBB NuGet 패키지 누락으로 실패했다. 상세 내용은 리팩토링 검토 문서에 기록했다.

## 다음 단계

리팩토링 우선순위는 [REFACTORING_PLAN.md](REFACTORING_PLAN.md)를 참조한다. 초기 커밋과 GitHub 업로드는 별도로 진행한다. 사용자가 보류를 해제하기 전에는 remote 추가, 저장소 생성, push, LFS upload를 실행하지 않는다.
