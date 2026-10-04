# 현재 코드 구성

## 로컬 실행 구성

[인터랙티브 구성도](../diagrams/local-runtime/runtime.architecture.html), [JSON 원본](../diagrams/local-runtime/runtime.architecture.json), [근거와 재생성](../diagrams/local-runtime/README.md)을 함께 관리한다.

- 클라이언트는 로비에 TCP 8910으로 연결한다. DirectX 12·Direct2D로 화면과 UI를 그리며 FMOD로 음원을 재생한다.
- 게임 서버도 로비에 연결하고 TCP 8911 listener를 준비한다.
- 매칭 후 클라이언트는 전달받은 게임 서버 주소로 전환한다. listener와 서버 연결·로그인 화면 시작까지 확인했고 멀티플레이 전투 전체는 검증하지 않았다.
- 로비는 매칭·사용자·거래·DB 관련 코드를 포함한다. 현재 개발 모드는 DB 없이 실행한다.
- 공유 wire format은 `Server/Game_Server/protocol.h`에 있고 로비·클라이언트가 상대 경로로 참조한다. 독립 Shared 모듈 분리는 후속 리팩토링 제안이다.
- 실행 리소스는 각 모듈 폴더의 상대 경로로 읽는다. 빌드 결과와 에이전트 캐시는 소스와 분리한다.

## 개발 기반 변경의 경계

VS toolset·인코딩·빌드 경로·개발 문서 변경은 패킷 형식이나 게임 규칙을 바꾸지 않는다. 게임 객체 생명주기, 패킷 검증, 정상 종료와 큰 클래스 책임 분리는 [리팩토링 검토](../REFACTORING_PLAN.md)의 후속 작업이다.
