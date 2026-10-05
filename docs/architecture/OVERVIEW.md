# 현재 코드 구성

## 로컬 실행 구성

[인터랙티브 구성도](../diagrams/local-runtime/runtime.architecture.html), [JSON 원본](../diagrams/local-runtime/runtime.architecture.json), [근거와 재생성](../diagrams/local-runtime/README.md)을 함께 관리한다.

- 클라이언트는 로비에 TCP 8910으로 연결한다. DirectX 12·Direct2D로 화면과 UI를 그리며 FMOD로 음원을 재생한다.
- 게임 서버도 로비에 연결하고 TCP 8911 listener를 준비한다.
- 매칭 후 클라이언트는 전달받은 게임 서버 주소로 전환한다. listener·서버 연결·로그인 화면 시작과 DB 없는 Title 영웅 테스트의 8910→8911 전환 및 실제 인게임 렌더링을 확인했다. 멀티플레이 전투 전체는 검증하지 않았다.
- 로비는 매칭·사용자·거래·DB 관련 코드를 포함한다. 현재 개발 모드는 DB 없이 실행한다.
- 공유 wire format은 `Server/Game_Server/protocol.h`에 있고 로비·클라이언트가 상대 경로로 참조한다. 독립 Shared 모듈 분리는 후속 리팩토링 제안이다.
- 실행 리소스는 각 모듈 폴더의 상대 경로로 읽는다. 빌드 결과와 에이전트 캐시는 소스와 분리한다.

## 개발 기반 변경의 경계

개발 진입점은 로비·게임 서버·클라이언트를 함께 포함하는 `NewWod.slnx`다. 서버 전용 `NewWod.Servers.slnf`도 같은 프로젝트를 사용한다. 공유 실행 프로필과 서버 준비를 기다리는 스크립트의 사용법은 [개발 환경](../development/SETUP.md)에 있다.

파일에서 읽은 동일한 정점·인덱스 geometry는 device별 내용 캐시를 통해 공유한다. 각 프레임의 transform·material은 유지하고 스킨드 메시의 본 연결·가중치·애니메이션 상태는 별도로 소유한다. [메시 공유 구현](MESH_SHARING.md)과 [공유 구조도](../diagrams/mesh-sharing/mesh-sharing.architecture.html)에 소유권과 검증 결과를 기록했다. 이 변경은 TCP 연결 구성을 바꾸지 않아 기존 로컬 실행 구성도는 재생성하지 않았다.

VS toolset·인코딩·빌드 경로·개발 문서 변경은 패킷 형식이나 게임 규칙을 바꾸지 않는다. 게임 객체 생명주기, 패킷 검증, 정상 종료와 큰 클래스 책임 분리는 [리팩토링 검토](../REFACTORING_PLAN.md)의 후속 작업이다.

인게임에서는 확정된 외형의 선택 파츠만 생성하고, 각 keyframe에서 선택한 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관한다. 모든 61개 애니메이션과 keyframe은 유지하며 선택 스킬별 clip 로딩은 미적용이다. 전체 프레임 계층·본 링크·base geometry 공유를 유지하며 로비/READY 편집 및 외형 미수신은 전체 모델을 사용한다. [선택 파츠 구현·실측](../portfolio/HERO_SELECTED_PARTS.md)과 [변경 흐름](../diagrams/hero-selection/README.md)에 근거를 기록한다. 실제 진입 검증 중 발견한 로비 Job 실행 누락과 Title의 서버 응답 전 장면 전환도 복구했으며 wire format은 유지했다.

파티클 효과 객체는 네 참가자의 확정 스킬 합집합과 기본 공격·타워·후속 효과에 필요한 종류만 생성한다. 선택 슬롯과 환경 효과도 유지한다. 모든 효과의 두 대형 GPU 버퍼는 장면 공용 풀에서 최초 표시 시 임대하며, 활성 효과는 독립 블록을 사용한다. 비활성 블록은 GPU fence 완료 후 종류 간 재사용하고 부족하면 풀을 확장한다. 개별 입자 용량도 포화 통계를 통해 확장하며 게임 종료·장면 전환 시 풀 전체를 해제한다. 서버 선택 전송 순서와 미완료 정보의 전체 종류 fallback은 유지한다. [선택 흐름](../diagrams/particle-selection/README.md), [공용 풀 수명](../diagrams/particle-buffer-pool/README.md), [구현·실측·한계](../portfolio/PARTICLE_BUFFER_POOL.md)를 참조한다.
