# 파일 정리 기록

2026-10-04 실행 기준선과 VS2026 이전을 확인한 뒤 정리했다. 원본 `C:\GitFolder\war_of_dimension` 및 GitLab 이력을 복구 근거로 보존한다.

## 제거한 범위

| 범위 | 근거 |
|---|---|
| `.idea/`의 개인 IDE 파일, 빈 `Server/blockchainTest/private_key.pem` | 프로젝트·빌드·런타임에 사용하지 않는 로컬 설정과 빈 생성 대상 |
| `Client/WarOfDimension/WarOfDimension.exe`, `Server/Game_Server.exe`, `Server/Lobby_Server.exe` | 원본에 포함된 기존 실행 파일. 현재 소스로 Debug/Release 실행 파일을 artifacts에 재생성 |
| `Client/WarOfDimension/Model/Textures/Textures.zip` | 파일 6개가 현재 텍스처 폴더 또는 상위 Model 폴더에 존재하고 SHA-256이 모두 일치. FreeLichPBR.bin은 실제 런타임 Model 폴더에 보존 |
| `Client/WarOfDimension/Image/GUI/Customize/liftUp.zip` | 해제된 파일과 SHA-256 일치 |
| `Server/Game_Server/Resource.zip` | 실행 코드가 압축 파일을 읽지 않음. 파일 22개가 리소스 폴더에 존재하며 15개 동일, 7개는 기존 최신 tracked 리소스와 다름. 누락 HeightMesh는 복원한 뒤 보존하고, 기존 최신 리소스를 과거 백업으로 덮어쓰지 않음 |
| `Client/WarOfDimension/fmodex.dll`, `fmodex64.dll`, `fmodexL.dll`, `fmodex_vc.lib`, `fmod_memoryinfo.h` | 이전 FMOD Ex 세대의 미사용 파일. 전체 소스·프로젝트 참조 검색에 사용처가 없고, 실제 클라이언트는 fmod/fmodL에 링크·로딩 |
| `Server/Game_Server/Lib/libboost_*-vc143-*.lib` 6개 | 실제 소스는 Boost.PFR의 header-only reflection만 사용. 해당 바이너리 이름의 프로젝트·빌드 참조가 없으며 현재 링크에서 사용하지 않음 |
| `Server/Game_Server/packages.config`, TBB import·복원 검사와 packages 캐시 | 원본 빌드 재현 단계에서 복원했으나 전체 소스에 TBB API·include·HAS_TBB 사용이 없고, v145 링크·런타임에 TBB 의존성 없음. 실제 사용 중인 Microsoft PPL concurrent container와는 별개 |
| 최초 기준선 빌드가 모듈 폴더에 만든 `x64/`, 중첩 `Game_Server/`, `Lobby_Server/` 산출물 | tracked 소스가 없고 artifacts 경로로 모두 재생성. `.obj` 모델 리소스는 이 범위에 포함하지 않음 |

정리 대상의 원본 경로는 NewWod 내부로 제한한다. 일괄 재귀 삭제가 자동 승인 검토에서 `blocked by policy`로 거부되어, 바이너리·개인 설정·산출물은 저장소 밖 `C:\GitFolder\NewWodCleanupBackup\20261004`로 옮겨 복구 가능하게 보관한다. 정리된 tracked 파일은 초기 커밋 후보에서도 제외한다. 최초 TBB 복원용 스크립트 대신 설치 도구·필수 리소스를 점검하는 `Check-Prerequisites.ps1`을 사용한다. 현재 빌드는 오래된 TBB NuGet 패키지를 다운로드할 필요가 없다.

## 보존한 범위

- 실행 모델·텍스처·UI 이미지·음원·CSV·NavMesh·HeightMesh, FMOD 최신 세대 DLL·import library·헤더.
- Boost·RapidJSON 헤더. 현재 dependency closure만으로 vendor 배포를 임의 축소하지 않는다. 에이전트 index에서는 vendor 폴더를 제외한다.
- `Client/Deferred`, `Client/WOD`, `Client/TestClient`, `Server/Stress_Test`, `Server/blockchainTest`. 핵심 실행과 분리했지만 테스트·참고 가치가 있어 사용 여부 판단 없이 삭제하지 않는다.
- `Script/` 모델·NavMesh 내보내기 코드, `Data/` Excel 원본·생성 도구, 기획 자료.
- 위 실험 모듈은 메타데이터만 VS2026으로 맞췄으며 실행 검증 범위가 아니다.

## 정리 후 확인

핵심 3개 모듈 Debug/Release x64 재빌드와 두 configuration의 로컬 시작 점검을 통과했다. `.obj` 리소스 보존, 프로젝트 source 참조 존재, LFS pointer·실제 파일 해시, 초기 커밋 후보에서 삭제된 파일 제외 여부도 확인했다. 세부 실행 결과는 [작업 목록](../../tasks/todo.md)에 기록했다.
