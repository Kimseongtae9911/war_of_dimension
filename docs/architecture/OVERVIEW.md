# 현재 코드 구성

서버 공통 기반은 [ServerCore 구현](SERVER_CORE.md)과
[현재 의존성 구조도](../diagrams/server-core/README.md)에 정리했다.
로비·게임 서버가 공통 정적 라이브러리를 사용하고 Protocol은 Shared로 분리했다.
[구현 계획](SERVER_CORE_PLAN.md)과 [계획 설계도](../diagrams/server-core-plan/README.md)는 구현 전 기록이다.
새 더미·agent/GUI 제어·월드 관찰·상세 서버 문서·콘텐츠 데이터화는 1~6단계로 순차 진행한다.
시나리오 더미·로컬 웹 관측의 [첫 구현](../development/SERVER_LAB.md)과 [구조도](../diagrams/server-lab/README.md)를 추가했다.
지형 탭은 더미/NPC 수신 위치·이동 궤적·HP·공격/제거 사건을 표시하며 매치별 대표 수신과 별도 월드 API를 사용한다.
로비 클라이언트의 고정 NPC 배치와 게임 NPC 종류별 표시를 제공한다. 게임 업데이트 경로에서 복사한 매치 일정으로 미니언 예약 스폰·웨이브 검사·게이트 해제 남은 시간을 표시한다.
전체 월드 관찰·agent 제어·전체 서버 문서화는 아직 완료하지 않았다.

## 로컬 실행 구성

[인터랙티브 구성도](../diagrams/local-runtime/runtime.architecture.html), [JSON 원본](../diagrams/local-runtime/runtime.architecture.json), [근거와 재생성](../diagrams/local-runtime/README.md)을 함께 관리한다.

- 클라이언트는 로비에 TCP 8910으로 연결한다. DirectX 12·Direct2D로 화면과 UI를 그리며 FMOD로 음원을 재생한다.
- 게임 서버도 로비에 연결하고 TCP 8911 listener를 준비한다.
- 매칭 후 클라이언트는 전달받은 게임 서버 주소로 전환한다. listener·서버 연결·로그인 화면 시작과 DB 없는 Title 영웅 테스트의 8910→8911 전환 및 실제 인게임 렌더링을 확인했다. 멀티플레이 전투 전체는 검증하지 않았다.
- 로비는 매칭·사용자·거래·DB 관련 코드를 포함한다. 현재 개발 모드는 DB 없이 실행한다.
- 공유 wire format은 `Shared/Protocol/protocol.h`에 있다. 두 서버와 클라이언트가 공통 헤더를 참조하며 이전 서버 경로에는 forwarding header를 둔다. x64 MSVC ABI와 상수를 유지했다.
- 실행 리소스는 각 모듈 폴더의 상대 경로로 읽는다. 빌드 결과와 에이전트 캐시는 소스와 분리한다.

## 개발 기반 변경의 경계

인게임 UI DDS는 BC7 압축 상태로 기존 장면 로딩 단계에서 사전 준비한다. 원본 픽셀/atlas를 유지하는 wrap 경계 패딩과 DDS 내용 영역 정보→texture→mesh 상수→atlas 계산 후 UV 보정을 추가했다. 새로운 지연/비동기 로딩이나 공용 UI geometry 캐시는 사용하지 않는다. [UI 구현·실측](../portfolio/UI_TEXTURE_COMPRESSION.md), [로딩·UV 흐름도](../diagrams/ui-bc7/README.md)를 참조한다.

장면의 별도 NPC·타인·스킬 컨테이너도 초기 GPU copy fence 완료 후 임시 upload를 반환한다. NPC 32bpp DDS 24장은 같은 해상도·단일 mip의 BC7으로 저장하며 기존 R8 Metallic 1장은 유지한다. 미니언·일반 몬스터의 객체 CB는 2프레임 `ObjectConstantArena`의 256byte draw 영역을 사용한다. 용량이 모자라면 64KiB 페이지를 추가하고 제출 fence 이후 재사용·종료 해제한다. 개별 draw 값과 skinning pose를 유지한다. [최종 구현·실측·화면](../portfolio/NPC_MEMORY_OPTIMIZATION.md), [객체 상수 수명도](../diagrams/npc-memory/README.md)를 참조한다.

모델 material의 DDS는 device·정규 파일 경로·용도별로 읽기 전용 GPU texture와 upload를 공유한다. 각 material의 root parameter/handle 저장은 독립이고 같은 Scene 힙의 SRV는 재사용한다. weak cache는 수명을 연장하지 않으며 GPU 완료 후 upload 및 마지막 소유자 종료 시 전체 자원을 해제한다. [적용과 검증](../portfolio/NPC_RESOURCE_SHARING_REVIEW.md#후속-적용-모델-dds-공유), [DDS 소유권 구조도](../diagrams/dds-sharing/README.md)를 참조한다.

개발 진입점은 로비·게임 서버·클라이언트를 함께 포함하는 `NewWod.slnx`다. 서버 전용 `NewWod.Servers.slnf`도 같은 프로젝트를 사용한다. 공유 실행 프로필과 서버 준비를 기다리는 스크립트의 사용법은 [개발 환경](../development/SETUP.md)에 있다.

파일에서 읽은 동일한 정점·인덱스 geometry는 device별 내용 캐시를 통해 공유한다. 각 프레임의 transform·material은 유지하고 스킨드 메시의 본 연결·가중치·애니메이션 상태는 별도로 소유한다. [메시 공유 구현](MESH_SHARING.md)과 [공유 구조도](../diagrams/mesh-sharing/mesh-sharing.architecture.html)에 소유권과 검증 결과를 기록했다. 이 변경은 TCP 연결 구성을 바꾸지 않아 기존 로컬 실행 구성도는 재생성하지 않았다.

VS toolset·인코딩·빌드 경로·개발 문서 변경은 패킷 형식이나 게임 규칙을 바꾸지 않는다. ServerCore에서 패킷 형식 검사와 I/O 수명·정상 종료를 적용했다. 콘텐츠 전체 수명과 큰 클래스 책임 분리는 [리팩토링 검토](../REFACTORING_PLAN.md)의 후속 작업이다.

인게임에서는 확정된 외형의 선택 파츠만 생성하고, 각 keyframe에서 선택한 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관한다. 모든 61개 애니메이션과 keyframe은 유지하며 선택 스킬별 clip 로딩은 미적용이다. 전체 프레임 계층·본 링크·base geometry 공유를 유지하며 로비/READY 편집 및 외형 미수신은 전체 모델을 사용한다. [선택 파츠 구현·실측](../portfolio/HERO_SELECTED_PARTS.md)과 [변경 흐름](../diagrams/hero-selection/README.md)에 근거를 기록한다. 실제 진입 검증 중 발견한 로비 Job 실행 누락과 Title의 서버 응답 전 장면 전환도 복구했으며 wire format은 유지했다.

파티클 효과 객체는 네 참가자의 확정 스킬 합집합과 기본 공격·타워·후속 효과에 필요한 종류만 생성한다. 선택 슬롯과 환경 효과도 유지한다. 모든 효과의 두 대형 GPU 버퍼는 장면 공용 풀에서 최초 표시 시 임대하며, 활성 효과는 독립 블록을 사용한다. 비활성 블록은 GPU fence 완료 후 종류 간 재사용하고 부족하면 풀을 확장한다. 개별 입자 용량도 포화 통계를 통해 확장하며 게임 종료·장면 전환 시 풀 전체를 해제한다. 서버 선택 전송 순서와 미완료 정보의 전체 종류 fallback은 유지한다. [선택 흐름](../diagrams/particle-selection/README.md), [공용 풀 수명](../diagrams/particle-buffer-pool/README.md), [구현·실측·한계](../portfolio/PARTICLE_BUFFER_POOL.md)를 참조한다.
