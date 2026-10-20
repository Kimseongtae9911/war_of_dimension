# 개발 환경 정비 작업

## 107. 로비 채널 병렬 처리 변경의 커밋·푸시 준비 (2026-10-20)

- [x] 현재 브랜치·전체 변경·새 파일과 기존 검증 근거 검토
- [x] Core/ABI·Python 회귀 재확인과 원격 조회
- [x] 구조도 원본 바이트 보존·명시적 스테이징·승인용 메시지/snapshot 준비

현재 브랜치 `codex/lobby-channel-parallelism`의 로비 결함 수정·채널 직렬 큐/공통 worker pool·SessionWorkQueue·부하 도구/실패 보고서·설계 이유/Archify 규칙·포인터 정렬 변경을 함께 준비한다. 최신 서버 빌드 후15개 포맷 파일 hash가 유지됨을 확인했고 Core1,686검사·Protocol110종/106상수 ABI·Python50검사를 재실행해 통과했다. 밀집 측정에서 발생한 CPU/송신 적체·정지/정상 종료 실패는 보고서에 보존하며 성공처럼 설명하지 않는다. 운영 DB·전체 GUI·실제 브라우저 배치는 미검증 범위를 유지한다.

커밋 검토 중 Git 줄바꿈 변환으로 구조도 receipt의 hash가 달라질 수 있어 기존 다른 구조도와 같은 `-text whitespace=cr-at-eol` 규칙을 해당 JSON/HTML에 추가했다. 원본/HTML4개의 stage blob과 working file hash, 최신 구현/이전 제안 receipt를 대조한다. 소스 로직과 측정 수치는 바꾸지 않았다. 원격 fetch에 성공했고 현재 브랜치의 원격 head는 없어 승인 후 첫 push에서 upstream을 지정한다. commit 스킬의 정확한 메시지·stage fingerprint 미리보기에 대한 승인 후 커밋·푸시하며, 이 항목은 승인 전 준비 기록이다.

## 106. 현재 서버 작업의 포인터 선언 정렬 (2026-10-19)

- [x] PointerAlignment Left·자동 유추 해제와 공통 지침/설정 문서 반영
- [x] 현재 수정 중인 서버 C++ 파일·관련 Core 큐 선언의 포인터 공백만 정리
- [x] clang-format 선언 판정·코드 token 동일성·설정 재현·Release 서버 빌드 확인

사용자는 포인터를 변수 쪽이 아닌 타입 옆에 붙이도록 요청했다. Microsoft 기본 스타일에서 PointerAlignment를 명시하지 않아 오른쪽 정렬되던 설정을 고친다. 현재 작업 파일과 관련 공통 큐 선언을 정리하고 곱셈/역참조·본문 포맷·기존 인코딩/개행·제어문 빈 줄은 유지한다. 외부 코드·클라이언트 전체·게임 서버 전체를 일괄 재포맷하지 않는다. 선언 공백만 바뀌므로 구조도/성능 보고서는 재생성하지 않으며 새 성능 측정·커밋/push는 하지 않는다.

완료: `.clang-format`에 `PointerAlignment: Left`, `DerivePointerAlignment: false`를 명시하고 기존 `ReferenceAlignment: Left`를 유지했다. AGENTS와 개발 환경 문서에도 `SocketResource* _resource` 규칙을 추가했다. 현재 서버 작업18파일을 검사해15파일의 포인터 공백528개 편집을 적용했다. 전체 재포맷 대신 clang-format의 Left/Right 차이 중 포인터 인접 공백만 반영해 기존 분기/함수 빈 줄·곱셈/역참조·문자열을 유지했다.

검증: clang-format22.1.3 설정 dump와 포인터/이중 포인터/참조·곱셈/역참조 예제 재현, 남은 해당 포인터 정렬 수정0건,15파일의 코드 token·LF/CRLF 수 불변, UTF-8·diff 검사, Release 서버 빌드를 확인했다. 실제 서버 실행·부하/게임 플레이 검사는 새로 하지 않았다. 적용 전후 소스·preview/applied/verification은 Git 제외 `artifacts/logs/pointer-alignment-20261019/`, 빌드 로그는 `artifacts/logs/pointer-alignment-build-20261019.log`에 보존한다.

## 105. Inbox를 SessionWorkQueue로 명명 정리 (2026-10-19)

- [x] 클래스·멤버·인자·정리 함수와 기존 동시성 검사의 이름 변경
- [x] 현재 문서·보고서 생성 원본·Archify JSON/HTML 이름 일치
- [x] 이름 변경 전후 동작 코드 대조·Release 서버 빌드·기존 Core/ABI 검사

앞서 제안한 `Inbox→SessionWorkQueue`, `m_inbox→m_workQueue` 정리를 사용자가 승인했다. `ClearInbox/ClearInboxes`도 `ClearWorkQueue/ClearWorkQueues`로 맞춘다. 잠금·FIFO·소유권·스케줄링은 바꾸지 않는다. 기존 관측/저장 결과 계약인 `inbox_pending` 키는 유지하고 문서에 호환 이름임을 밝힌다. 과거 로그·측정 source-snapshot·작업 이력·미구현 제안도는 소급 수정하지 않는다. 현재 구조도와 보고서는 명칭을 재생성하며 성능을 재측정했다고 주장하지 않는다. Serena 도구는 현재 세션에 제공되지 않아 rg와 파일 탐색을 사용한다. 커밋/push는 요청받지 않았다.

완료: 런타임5파일·기존 동시성 검사1파일의 타입/멤버/인자/지역 변수/정리 함수명을 통일했다. 수정 전 snapshot에 예정된 이름 치환을 적용한 코드와 현재 코드의 token을 대조해 주석·공백 외 동작 코드 동일성을 확인했다. 기존 `inbox_pending` 문자열은 유지한다. 상세 설계·현재 Core 설명·보고서 생성 원본/HTML·구현 JSON/HTML·README·인덱스를 갱신했다. 과거 측정 근거22개 hash와 이전 제안도는 보존했다.

검증: Release 서버 빌드, Core1,686검사·Protocol110종/106상수 ABI 동일, Archify architecture showcase9/9·오류/경고0, 문서178링크·UTF-8·Python AST·JSON/HTML hash·표시 이름·diff 확인. 구조도 JSON3438bytes·HTML706492bytes이며 hash/receipt는 [README](../docs/diagrams/lobby-channel-parallelism/README.md)에 기록했다. 근거는 Git 제외 `artifacts/logs/lobby-work-queue-name-{build,core}-20261019.log`, `lobby-work-queue-name-{validation,delivery,verification}-20261019.json`, 수정 전 snapshot `lobby-work-queue-name-before-20261019/`에 남겼다. 브라우저 시각 검사는 기존 접근 제한으로 미수행이고 실제 게임 플레이·성능 부하는 새로 실행하지 않았다.

## 104. Archify 구현 의도·설계 선택 이유 문서화 (2026-10-19)

- [x] 프로젝트 Archify 스킬에 문제→선택→이유→비용과 코드 근거 문서화 규칙 추가
- [x] 로비 구현 JSON의 설명 카드와 상세 설계 이유/대안/불변 조건 보완
- [x] showcase validate/deliver·스킬 검증·문서 링크/UTF-8·동기화 확인

사용자는 흐름도 외에 설계 이유가 빠졌다고 지적하고 Archify 사용 시 구현 의도를 함께 작성하도록 요청했다. 프로젝트 스킬·연결된 작업 지침을 보완하고 현재 로비 구조도의 설명 카드와 상세 근거를 갱신한다. 서버 로직·측정 결과·이전 제안도는 변경하지 않는다. 이전 로컬 파일 접근 제한을 우회하는 브라우저/CLI 시각 검사는 하지 않고 해당 미검증을 유지한다. 커밋/push는 요청받지 않았다.

완료: [프로젝트 스킬](../.agents/skills/archify/SKILL.md)의 구현 의도 규칙과 [작업 지침](../docs/guides/archify.md)을 연결했다. [설계 선택 이유](../docs/development/LOBBY_CHANNEL_PARALLELISM.md#설계-선택-이유)에6개 선택의 문제·대안의 한계·계약·비용/근거, 큐 간 순서 실패 예시, Inbox mutex와 실행 잠금의 차이, FIFO의 실제 등록 순서 범위, deque가 필수 조건이 아님을 기록했다. [구조도](../docs/diagrams/lobby-channel-parallelism/implementation.html)의2개 카드6항목에도 이유/비용을 표시하고 README·인덱스를 갱신했다.

검증: skill-creator quick_validate 통과, Archify architecture showcase9/9·오류/경고0으로 validate/deliver 통과 후 JSON 고정. JSON3410bytes·HTML706477bytes이며 hash/receipt는 [README](../docs/diagrams/lobby-channel-parallelism/README.md)에 기록했다. 문서153개 로컬 링크·UTF-8·설명 카드 일치·현재73개 실행/소스/도구 fingerprint 불변·이전 제안도 불변·diff 검사를 확인했다. report용 venv에 PyYAML이 없어 스킬 기본 검증은 기존 bundled Python으로 수행했으며 설치를 추가하지 않았다. 근거는 Git 제외 `artifacts/logs/lobby-channel-intent-{validation,delivery,verification}-20261019.json`과 스킬 검증 log에 보존한다. 실제 브라우저 배치/시각 검사와 visual-check는 기존 접근 제한으로 미수행이다. 이번 작업에서 서버 빌드·부하 재측정·커밋/push는 하지 않았다.

## 103. 로비 채널당 인원 증가 재측정 (2026-10-18)

- [x] 기존 조건/도구·Release 실행 파일과 소스 일치·전용 실행 상태 확인
- [x] 4채널×32/64명 및 1채널×128명 각3회 비교, 4채널×128명은3/6회에서 실패로 중단
- [x] 실제 입장 분포·이동/정지·발행 lag/생성기 CPU·대기/처리율/CPU·종료 자원 대조
- [x] 실패/증원 중단 조건을 포함한 원시 근거 보존과 기존 HTML/상세 보고서 갱신

기존 32채널×4명과 다른 밀집 조건을 요청받았다. 동일 조건 안에서 worker 수만 바꾸고 매회 서버 재시작, 같은 perf-lobby(5초 워밍업/20초 측정/250ms 입력), Release LOCAL_TEST·DB 없음·계측 ON·headless를 유지한다. 탐색 비교는 기존 보고서와 같은 각3회(기본 목표5회의 예외)로 수행하고 반복 범위를 명시한다. 기능 실패·발행 p99>250ms·생성기0.9코어 이상이면 후속 증원은 중단하며 실패도 보존한다. 런타임 최적화·커밋/push는 수행하지 않는다.

결과: [HTML 보고서](../docs/development/LOBBY_CHANNEL_PARALLELISM.html#density)·[상세 근거](../docs/development/LOBBY_CHANNEL_PARALLELISM.md#채널당-인원-증가-재측정). 21회 중20회 검증 통과·1회 실패이며,32/64명·채널 및 단일128명은 각worker3회 통과했다.32명/채널의 대기 p99 상한은131.072/131.072ms·CPU8.80/14.23%,64명/채널은524.288/524.288ms·CPU11.82/22.82%였다.64명/채널·worker4는 송신 대기가 최고1,261,391건으로 늘었다.512명·worker4의2번째 실행에서120명 최종 정지 timeout·송신 대기5,540,022건·private bytes3.16GB·로비 정상 종료15초 초과가 발생해 나머지3회와 추가 증원은 중단했다. 이후128명 단일 채널 대조만 수행했으며 최대 동시 채널 callback은worker1/4 모두1이었다. 실패를 숨기거나 timeout을 완화하지 않았다.

Release 서버 빌드와 도구 Python50검사를 통과했다. 더미의 성공한 채널 배정 응답 저장·실제 분포 대조·생성기 한계 종료코드2·환경/도구 source snapshot을 보완했다. 네 묶음의 동일73개 fingerprint, 모든21회의 배정 분포·소유권 위반0건을 확인했다. 생성기 발행p99≤140ms·CPU<0.9코어였으나 동일PC 수신 영향은 미분리다. 실패 실행도 stop 잔여0 로그가 있지만 정상 종료 판정은 실패로 보존했다. 현재 검사 프로세스는 정리됐다. 로직 최적화·커밋/push는 하지 않았다. 원본은 Git 제외 `artifacts/logs/lobby-density-{c4-p32,c4-p64,c4-p128,c1-p128}-20261018/`에 보존한다.

후검증: 원본21회의 실제 이동/위치 변화·입장 분포·최종 연결0·소유권·histogram overflow·샘플80개 발행 지연·정상 종료 분모와 실패120개를 대조했다. 현재73개 fingerprint 불변, 보고서 근거22개 hash·HTML5개 절/15링크/PNG2개·AST/UTF-8·문서 링크·diff 검사에 통과했다. 그래프 실제 이미지는 확인했고 전체 브라우저 배치는 기존 접근 제한으로 미검증이다. 검증 근거는 Git 제외 `artifacts/logs/lobby-channel-density-verification-20261018.json`, 그래프/요약은 `lobby-channel-density-{report-20261018.png,analysis-20261018.json}`에 남겼다. 서버 포트8790/8910/8911의 listener와 실행 기록이 남지 않은 것도 확인했다.

## 102. 로비 채널 병렬화 수정 전후 HTML 보고서 (2026-10-18)

- [x] 기존 원본/수정 후 제거 패킷 재현과 worker1/4의 반복·최종·계측OFF 원본 대조
- [x] 결과→원인→해결→실측→추가 대응 순서의 핵심 HTML 보고서·재생성 스크립트 작성
- [x] 그래프 실제 이미지·수치/비교 범위·HTML 구조·링크/UTF-8·인덱스 확인

write-engineering-report 스킬을 적용한다. 이번 요청은 보고서 작성이며 새 서버 수정·부하 검사·커밋/push는 하지 않는다. 기능의 수정 전후는 main@54af931c 대비 재현 근거를 사용하고, 성능은 개선 코드 안의 worker1/4 대조임을 첫 화면에 밝힌다. 원본 main의 동일32채널 부하 성능은 미측정이므로 수정 전 성능처럼 대체하지 않는다. 기존 구현 구조도는 동작 변경 없이 링크로 재사용한다. 브라우저 실제 배치는 기존 로컬 파일 접근 제한을 우회하지 않고 미검증으로 구분한다.

완료: [수정 전후 HTML 보고서](../docs/development/LOBBY_CHANNEL_PARALLELISM.html)와 [재생성 스크립트](../scripts/Build-LobbyChannelReport.py)를 작성했다. 실제 제거 패킷 전후, 개선 코드 worker1/4의 반복 중앙값·최종 각1회·계측OFF를 구분해 대기 감소와 CPU 증가를 함께 표시했다. PNG 1860×573 실제 이미지 확인, HTML 5개 절 순서·태그 닫힘·로컬 링크10개·UTF-8·생성기 AST·근거9개 SHA-256·기존 구조도 불변·git diff --check를 확인했다. 결과는 Git 제외 `artifacts/logs/lobby-channel-report-verification-20261018.json`에 남겼다. 기존 구현/검증 파일은 보존했으며 이번 보고서 작업에서 새 서버 빌드·부하 실행은 하지 않았다.

## 101. 기존 로비 결함 수정·채널 직렬 큐와 공통 worker pool (2026-10-18)

- [x] 종료/채널 이동의 시야 제거 ID·수신자, 음수/가득 찬 채널·등록 실패, 매칭 취소/중복 항목 수정
- [x] 채널 직렬 실행·유저 입력 FIFO·가입/종료/타이머 전달과 제한된 worker pool 구현
- [x] 전역 기능 직렬 조정·채널 이동 경계·세션 재사용 보호, 안전한 병렬 계측/설정 추가
- [x] Release/Debug 빌드·실제 네트워크/매칭/이동/종료·제어 동시성 검사와 동일 분포 worker 비교
- [x] 구현 구조도·사용 계약·실제 결과/제한 기록

사용자는 100번 검토에서 발견한 기존 결함 수정 후 채널별 직렬 큐와 공통 worker pool 방식으로 병렬화하도록 요청했다. 기존 검토 파일은 보존한다. 전역 기능은 초기 구현에서 공용 실행 barrier의 배타 구간으로 보호하고, 일반 채널 작업은 공유 구간에서 채널별로 병렬 실행한다. 사용자별 inbox는 한 번에 하나의 작업만 실행 대상으로 발행하여 채널 이동 전후의 입력 순서를 보존한다. 비동기 전역 서비스별 세분화는 별도 후속이며 이번 구현을 완전 무잠금/전역 정지 없음으로 설명하지 않는다. 커밋·push는 요청받지 않았다.

완료: [구현·실측/제한](../docs/development/LOBBY_CHANNEL_PARALLELISM.md)·[구현 구조도](../docs/diagrams/lobby-channel-parallelism/implementation.html). 기존 제거2결함은 단일 worker 전후 TCP재현으로 수정 확인했다. 채널별 target과 bounded 공통 worker4개, 유저당 선두1작업 발행 Inbox, generation 검사, 전역 배타 조정을 도입했다. 별도 routeEpoch 대신 이동 완료 뒤 다음 작업의 소유 큐를 결정한다. 매칭 취소/중복/이전 세대 정리, 입장 실패·채널 범위·클라이언트 제거 ID 방어, DB없는 요청·채팅 수신자/문자열 검사도 보완했다. 기존 Core 큐 알고리즘/게임 실행 정책은 유지한다.

검증: Release 전체·Debug 서버 빌드, Core 각1,686·ABI110종/106상수·Python49, 기존 Release 네트워크12종/보스4·5/초기화실패4종 통합 통과. 최종 Release/Debug 신규 채널9항목, 기존 이동3항목(재접속10회), 정원1·채널1의 실패/슬롯 반환을 확인했다. 제어 C++ 검사는 두 채널 동시·동일 채널/전역 배제·200작업 이동FIFO·종료 후 거부/캡처 반환을 포함한다. 기존 형변환·미사용 경고와 운영 DB/블록체인·전체 GUI·race detector·최대 동접 미검증 범위는 유지한다.

128명·32채널×4명·250ms 입력·5초 warmup/20초 측정: worker1/4 각3회에서 패킷 대기p99 bucket상한65.536→16.384ms, CPU5.23→7.32%, PPS513.46→507.46. 마지막 legacy dummy 채널이동 호환1행 수정 뒤 최종 각1회는 p99동일·CPU5.24→7.04%·PPS506.76→514.28이다. 두 실행 파일 결과는 합산하지 않는다. 고정입력에서 최대처리량 증가를 주장하지 않으며 CPU증가와 같은 이동gap p99버킷(65.536ms)을 함께 보고한다. 계측OFF각1회 CPU5.14→8.06%, 원인비용분해/worker2튜닝은 후속이다. ON6+OFF2+최종2회 모두 통과·각묶음hash안정·양쪽정상종료 pending/sockets/leased=0. 원본은 artifacts/logs/lobby-channel-final-{comparison,metrics-off,verification}-20261018/에 보존했다.

최종 UTF-8·Python AST·프로젝트 XML·문서35링크·JSON/HTML receipt/hash·최종 binary hash·diff 검사 PASS. Core 런타임 알고리즘 변경0, 구현/제안 Archify 각각showcase9/9·오류/경고0. 종합확인은 artifacts/logs/lobby-channel-implementation-verification-20261018.json이며 실제브라우저배치는 기존접근제한으로미검증이다. 측정 뒤 CPacketMgr/LobbyDispatcher의 분기 뒤 빈 줄은 측정 snapshot과 공백 외 내용동일, 마지막 C++검사파일 포맷은 token동일성을 확인했다. 이 공백 정리 뒤 빌드/부하를 다시 수행하지 않았다. 커밋/push는 하지 않았다.

후속:
- [ ] 전역 DB/I/O를 개별 서비스 메시지로 분리하여 전역 정지 구간 축소
- [ ] 공용 송신 잠금·worker별 CPU/깨움 비용 분해와 worker2·채널 집중 부하/장시간 비교
- [ ] Inbox 상한/backpressure·채널 occupancy, 운영 DB/전체 GUI/동적 race 검증

## 100. 로비 채널 병렬화 전 공유 데이터·동시성 검토 (2026-10-18)

- [x] main `54af931c`에서 `codex/lobby-channel-parallelism` 브랜치 생성
- [x] IOCP·타이머·로직·서버 간 통신·선택 DB 경로의 실행/소유 경계 확인
- [x] 채널/사용자/시야/섹션·전역 매칭/부가 기능·Core 큐/세션/풀의 공유 상태와 경쟁 경로 조사
- [x] 채널 이동·초기화/종료·재접속·타이머 전달의 순서/수명 계약과 우선 수정 항목 정리
- [x] 현재 구현과 미구현 병렬화 제안을 구분한 검토 문서·Archify 및 검증 계획 작성

사용자는 새 브랜치에서 채널별 로직 병렬화 개선을 시작하되 공유 데이터와 동시성 문제부터 검토하도록 요청했다. 이번 단계는 코드 근거 조사와 설계/검증 조건 정리이며 worker 수·실행 정책은 아직 변경하지 않는다. 성능 향상·race 재현·전체 플레이 검증을 정적 검토만으로 주장하지 않는다. 커밋·push는 요청받지 않았다.

결과: [동시성 검토](../docs/development/LOBBY_CHANNEL_CONCURRENCY.md)·[미구현 소유권 제안도](../docs/diagrams/lobby-channel-parallelism/README.md). 현재 유저별 큐의 m_execute 보호는 있으나 채널 전체를 보호하지 않는다. IOCP 등록/로직 종료의 슬롯 잠금 불일치, 채널 이동의 generation 한계, 전역 매칭/공유 RNG/채널 간 판매자 갱신 등을 확인했다. bounded worker pool 위 채널 직렬 큐와 전역 조정자 메시지를 권장한다. 기존 uncapped 설정은 채널을 1개로 만들므로 다중 채널 비교를 위한 설정 분리도 필요하다.

Release LOCAL_TEST 기본 정원4·단일 worker에서 TCP2클라이언트로 기존 종료 REMOVE.id=-1, 채널 이동 REMOVE 수신자 오류를 실제 재현했다. GUI 충돌·동적 data race 재현을 주장하지 않는다. Build.ps1 -Module Servers -Configuration Release 성공, 실행 후 양쪽 서버 pending=0 sockets=0 leased=0으로 중지. probe/결과/EXE·원인 소스 hash는 Git 제외 .runtime/review_lobby_channels.py와 artifacts/logs/lobby-channel-review-probes-20261018.json에 보존했다. UTF-8·문서21링크·런타임 변경0·원본/산출물 및 재현hash·diff 검사 PASS, 확인 결과는 artifacts/logs/lobby-channel-review-verification-20261018.json이다. Archify showcase9/9·오류/경고0; 실제 브라우저 레이아웃은 기존 접근 제한 때문에 미검증이다. 런타임 소스 변경·다중 worker 부하·새 성능 측정·WITH_DATABASE·운영 DB/블록체인·전체 GUI는 이번 범위 밖이다.

후속 구현(101번 적용 상태):
- [x] 기존 종료/이동 제거 결함·채널 번호/등록 실패 검증과 회귀를 먼저 보완
- [x] 채널 큐·타이머/가입/정리 소유권 전달, 전역 매칭 직렬화 도입(routeEpoch 대신 선두1작업 발행과 전역 배타 이동)
- [ ] 채널별 계측·독립 정원 설정과 worker1/4 비교 완료, worker2 비교는101번 후속

## 99. 로비 개선 main 병합·미사용 지형 문서 정리 (2026-10-18)

- [x] 원격 main과 로컬 main의 동일성, 로비 개선2커밋 및 머지 충돌 없음 확인
- [x] 미사용 지형 실험 문서5파일·관련 미커밋 변경을 백업 후 제거하고 남은 링크 정리
- [x] 머지 결과의 코드 동일성·원본 OBJ/캐시 hash·Release 빌드·서버 연결/종료·문서 참조 검사

사용자가 `codex/lobby-server-improvement`를 main에 병합하고 push하도록 요청했다. main 기준은 `012d1046`, 통합할 로비 브랜치는 `4d869d98`이며 로비 지형 개선 `90a9e79f`도 포함한다. 기록/백업은 [정리 기록](../docs/development/CLEANUP.md)과 `artifacts/rollback/obj-doc-removal-20261018/manifest.json`을 따른다. 머지 커밋 생성·원격 hash 확인 결과는 Git 이력과 Git 제외 `artifacts/logs/lobby-main-merge-20261018.json`에 보존한다. 브랜치 삭제는 요청 범위에 포함하지 않는다.

검증: Runtime/Shared/scripts/tools는 로비 브랜치와 동일하며 새 구현 변경은 없다. 백업9파일과 원본 OBJ/캐시8파일 SHA-256 일치, 삭제5파일 부재·삭제 문서 링크 잔존 없음(정리 기록의 파일 목록 제외) 및 diff 검사 PASS. `Build.ps1 -Module Servers -Configuration Release`·`Test-Local.ps1 -Configuration Release -ServersOnly`의 서버 간 연결·정상 종료 PASS. 성능·전체 GUI 플레이·운영 DB/블록체인 검사를 이번 병합에서 다시 수행하지 않았다.

## 98. 로비 이동 개선 커밋·push 준비 (2026-10-13)

- [x] 로비 이동 구현·콜백 분리·회귀 도구·성능 보고서/흐름도 범위 검토
- [x] 보고서에 성능 측정 이후 함수 추출과 기능 회귀 검증 이력 보완
- [x] 이동 관련 파일 및 INDEX/todo 부분 선별 스테이징·검증·정확한 메시지 미리보기
- [x] 미리보기 승인 후 커밋·원격 push 및 hash 확인 (`4d869d98`, 2026-10-14)

사용자가 로비 이동 개선의 커밋·push를 요청했다. 96/97번과 본98번 기록만 포함하고 기존 OBJ 개편/롤백 문서와 INDEX/todo의 해당 부분은 제외한다. commit 스킬의 미리보기 승인 절차를 적용하며 이 준비 기록 시점에는 커밋·push를 실행하지 않았다. 앞선 최종 콜백 코드의 Release 로비 빌드·실제 소켓3항목 PASS를 재확인했으며 빌드/성능 검사를 이번에 다시 수행했다고 주장하지 않는다.

## 97. 이동 예약 코드의 중첩 람다 정리 (2026-10-13)

- [x] ScheduleMovementUpdate의 타이머 dispatch·유저 큐 실행을 private 함수로 분리
- [x] 예약 간격·단일 대기·SessionRef 보유·generation 잠금·지연 계측 경계 유지 확인
- [x] Release 로비 빌드와 실제 이동/정지·빠른 입력·재접속 회귀 검사

사용자 요청에 따라 ScheduleMovementUpdate와 그 보조 선언/구현만 정리한다. 실행 흐름·정책은 유지하는 함수 추출이며 기존 보고서/흐름도·타이머/공통 큐 구현은 변경하지 않는다. 기존 미커밋 변경은 보존한다.

예약은 ScheduleMovementUpdate, 타이머 도래 시 계측/유저 FIFO 등록은 EnqueueMovementUpdate, generation 잠금 아래 계측/기존 갱신 호출은 ExecuteScheduledMovementUpdate로 분리했다. 콜백은 한 단계 함수 호출만 이어가며 중첩 람다를 제거했다. SessionRef는 타이머와 큐 콜백이 값으로 보유하고, 단일 대기 플래그와 기존 RunMovementUpdate는 유지했다. Release 로비 빌드·실제 소켓3항목(분할 이동/본인·주변 정지일치,200개 입력순서/재시작,이동중재접속10회) PASS. 결과는 artifacts/logs/lobby-movement-lambda-refactor-20261013.json에 보존한다. 성능 재측정·전체 GUI 검사는 수행하지 않았으며 커밋/push는 하지 않는다.

## 96. 로비 개선 브랜치·1차 커밋·이동 처리 개선 (2026-10-13)

- [x] `codex/lobby-server-improvement` 브랜치 생성, 기존 미커밋 변경 보존
- [x] 로비 1차 지형 개선과 검증에 사용한 계측·더미 도구·기준/개선 보고서 42파일 선별 스테이징
- [x] OBJ 개편/롤백 문서·관련 INDEX/todo 부분 제외, staged diff 검사 및 흐름도9/9 검증
- [x] 정확한 메시지·staged fingerprint 미리보기 승인 후 1차 커밋·원격 push (`90a9e79f`)
- [x] 기존 이동 TODO와 클라이언트 수신/보간 표시 흐름을 확인해 이번 서버 이동 개선 범위 확정
- [x] 1ms 검사·즉시 자기 재등록 대신 15ms 후 단일 예약, 빠른 본인 정지 응답·주변 최신 상태 병합·세션 generation 유지
- [x] 초기/재접속 시각·빠른 이동/정지 반복·지연/정리·유저 순서·종료 회귀와 동일 조건 전후 처리량/대기/CPU/송신량 측정
- [x] 실제 구현·검증을 근거로 이동 개선 보고서·Archify 흐름도 갱신

사용자는 브랜치 생성 → 1차 커밋 → 이동 개선 순서를 요청했고 미리보기 뒤 커밋·push를 승인했다. 1차 42파일을 `90a9e79f42273230d49bf94af617e193f6d173ff`로 커밋하고 origin/codex/lobby-server-improvement의 동일 hash를 확인했다. 이96번 계획·OBJ 롤백 문서는 1차 커밋에서 제외했다. 후속 이동 구현은 미커밋으로 유지하며 단일 채널·유저 직렬 처리·서버 권위와 wire 호환성을 유지한다. 클라이언트 예측/보간 전체 개편은 기존 TODO로 남긴다.

조사: 이전 MovePacket은 직전 방향0이면 ProcessUpdate를 추가하고 ProcessUpdate는 이동 중 즉시 자신을 다시 넣었다. move→stop→move가 먼저 대기한 경우 U1/U2 갱신 체인이 중복될 수 있는 코드 경로를 확인했다(기존 체인 수의 실측 계측은 없음). TimerFunc는 이벤트 None·yield만 있었고 Zone::Update는 실제 실행 사용처를 확인하지 못했다. 클라이언트는 SC_MOVE_PLAYER 좌표를 보간/외삽하므로 wire는 유지했다.

구현: 타이머는 steady_clock·condition_variable로 대기하고, 도래 시 기존 유저 FIFO에 generation 검증 작업만 추가한다. 단일 content worker에서 예약 플래그를 관리하고 실행 완료 뒤 한 번만 다시 예약해 밀린 주기를 누적하지 않는다. 입력 이전 방향의 지난 이동을 적분하고 본인에게 즉시 새 방향/최종 정지를 보낸다. 주변 전송은 dirty 상태를 예약 갱신에서 병합하며 정지 상태도 한 번 전달한 뒤 종료한다. 초기/재접속 시각·속도 초기화와 CViewList의 동일 shared_mutex 중첩 잠금을 수정했다.

후보 검사: 입력마다 주변 전체 전송한 첫 후보는128명322.38패킷/s·p99상한16.78초로 악화되어 제외했다. 주변 최신 상태 병합 뒤 예비검사는513.22패킷/s·p99상한0.524초였으며 최종 반복 값과 구분한다. 갱신 횟수에 의존한 기존 속도 누적/감속 때문에 주기 변경 시 초기 이동 거리가 줄어드는 것을 확인해 로비 방향을 정규화한 목표 속도로 바꿨다(게임 물리는 유지). 세션/스케줄러 공통 코드는 변경하지 않았다. 최종 소스의 반복·장시간 검증을 진행한다.

완료: [이동 HTML 보고서](../docs/development/LOBBY_MOVEMENT.html)·[실측/계약](../docs/development/LOBBY_MOVEMENT.md)·[예약 흐름도](../docs/diagrams/lobby-movement/README.md). 이전1차 지형 적용 측정을 기준으로128명20초 전후각5회 중앙값 처리율515.81→512.16패킷/s·패킷 대기p99 bucket 상한1,048.576→524.288ms·CPU20.17→11.77%다. 새64명5회는 동일 길이의 이전 값이 없어 제어 검사로만 사용했다. 새20초10회/120초64·128명2회 모두 기능/종료 통과·실행 중hash안정. 120초128명은CPU19.32→12.27%, 최종 정리4.375→3.453초이며 측정 끝 대기106→131개 증가도 기록했다. 최종 입력 전체 처리·서로 다른빈표본3개를 확인했다. 128명120초의 더미/관측CPU0.927코어는0.9코어 여유기준을 넘었으므로 최대동접을 확정하지 않는다.

idle계측OFF·연결0·전후각5초1회 CPU8.29→표본해상도내0.00%. 부하CPU 감소의 큰 부분은 빈타이머 busy loop 제거와 일치하며 로직 처리 능력 증가만으로 설명하지 않는다. 15ms는 실행 완료 후 다음 예약 간격이며66.7Hz 보장이 아니다. 본인 비정지 응답은입력 즉시 응답도 포함하므로 실제 좌표변화율/화면품질의 근거로 사용하지 않는다.

검증: Release서버빌드·Core1,479·ABI110종/106상수·네트워크12종/보스4·5/초기화실패4종 통합7그룹·Python49 PASS. 최종소켓검사에서 약0.6초 backward5.922units·본인 응답28개·정지수신0.361ms 관측(각1회), 본인/주변최종좌표·방향0일치·200개입력응답순서·정지뒤갱신중단·재시작·이동중재접속10회 PASS. 실제API 이동정책15000us/단일대기true, lobby-cycle4명20assertion 및 양쪽pending=0 sockets=0 leased=0 확인. 소유실행기록은 제거했다. 측정 뒤CPhysic/CServer는공백/줄바꿈만정리했고 token동일성·재빌드/최종기능검사를 확인했다.

HTML5항목순서·내장PNG1장/실제이미지·링크/핵심수치·PythonAST·diff검사 PASS. Archifyvalidate/deliver showcase9/9·오류/경고0·source/artifact hash일치. 전체브라우저레이아웃은 기존로컬파일접근차단 이력으로 미검증. 검증은 artifacts/logs/lobby-movement-verification-20261013.json에 보존한다. 기존빈switch C4065는 제거됐으며 전체클라이언트/clean rebuild 경고해소를 주장하지 않는다. 운영DB/블록체인·전체GUI플레이·다중채널·클라이언트예측/보간전체개편·연속충돌·장시간반복/최대동접은 미검증 또는 후속과제다. 이동개선과 기존OBJ롤백문서는 미커밋으로 남겼다.

## 95. 로비 개선 내용 보고서 정리 (2026-10-13)

- [x] 기존 실측·검증 근거와 현재 지형 검색 호출을 대조
- [x] 결과 → 원인 → 해결 → 실제 측정 → 후속 대응 순서로 HTML 보고서 재구성
- [x] 처리량·대기 감소와 CPU 증가를 첫 화면에 표시하고 상세 수치는 펼침으로 정리
- [x] 새 구간별 비용 그래프·원시 수치·HTML 구조·링크 확인 후 검증 기록

`write-engineering-report` 스킬을 적용해 기존 LOBBY_THROUGHPUT.html과 재생성 스크립트를 정리한다. 새로운 서버 수정·부하 검사는 수행하지 않고 94번에서 확보한 실제 결과를 사용한다. 기존 Archify 검색 흐름도는 동작 변경이 없어 재생성하지 않는다.

첫 화면에 처리량 +34.7%·대기 16,377→106개·p99 상한 33.55→2.10초·CPU +3.31%p를 표시했다. HeightMesh/시야 경과 시간 그래프를 추가해 내장 PNG 3개를 사용하고, 상세 수치·반복 범위·더미 CPU·계측 조건은 4개 펼침 영역으로 정리했다. 원시 수치·HTML 5섹션 순서·링크 3개·스크립트 AST를 검사하고 PNG 3개를 실제 이미지로 확인했다. 검증 결과는 artifacts/logs/lobby-throughput-report-verification-20261013.json이다. 전체 브라우저 레이아웃은 기존 로컬 파일 접근 제한 이력 때문에 미검증으로 기록한다.

프로파일은 reporter 자체 timestamp까지 measure→verify 내부인 표본을 사용했다. 개선 후 차분 18.224초/7,032 Move의 HeightMesh 0.180ms·7.14%, 시야 2.381ms·92.07%를 보고서에 표기했다. 94번 메모의 구간 경계가 덜 엄격했던 예비값 0.186ms·7.15%·92.05%도 같은 기준으로 정정했다. 원시 측정·구현은 바꾸지 않았다.

## 94. 로비 단일 채널 처리량 개선 (2026-10-13)

- [x] 이동의 지형 검사·시야 탐색을 선택 프로파일링하고 원인 분리
- [x] 단일 소비자·FIFO·이동 주기를 유지하며 병목 구간 개선
- [x] 로비 실제 OBJ의 첫 교차·높이 결과 등가와 실패 fallback 검사
- [x] 기존 64/96/112/128명, 250ms 입력 조건으로 처리율·패킷 대기·CPU·최종 입력 정리 비교
- [x] 128명 짧은 검사 전후 5회와 120초 유지 검사 전후 각 인원 1회(장시간 반복은 후속) 수행
- [x] 결과 우선 HTML 보고서·Archify 흐름도와 검증/미검증 기록

채널 병렬화는 이번 범위에서 제외한다. 기존 미커밋 계측·동접 보고서·OBJ 롤백 문서는 보존했다. 로비 HeightMesh 124,834면의 전체 순회가 실제 Move cycle의70.65%를 사용함을 확인했다. 로드 시 기존 TerrainSpatialIndex와 정적 계산 캐시를 구성하고 NavMesh/HeightMesh ray에 전달한다. 첫 교차 순서·높이·원본 OBJ·단일 소비자·FIFO/budget·이동 주기는 유지한다. 기준 실행은 최적화 전 계측만 추가한 동일 소스/시나리오를 snapshot으로 보존했다.

결과: [HTML 보고서](../docs/development/LOBBY_THROUGHPUT.html)·[실측/재현 근거](../docs/development/LOBBY_THROUGHPUT.md). 128명120초 처리량380.45→512.58패킷/s, 종료 시 대기16,377→106개, 패킷 대기p99 bucket 상한33,554.43→2,097.15ms, CPU16.01→19.32%. 최종 입력 정리는 개선 전20초 제한 실패(8,739개 남음), 개선 후4.375초에전체64,256개 처리·대기0을3표본 확인했다. 64/96/112/128명의 이동·최종 정리 검사는 전1/4·후4/4 통과. 128명20초 seed1~5는 전후모두5/5 통과하며, 처리량 중앙값409.68→515.81패킷/s·p99상한 중앙값8,388.61→1,048.58ms다.

HeightMesh wall/Move5.156→0.180ms, cycle비중70.65→7.14%. 개선 후 시야 탐색·주변 전송이Move cycle의92.07%를 차지한다(95번에서 reporter 측정 구간을 엄격히 적용해 예비값 정정). 128명 송신 프레임은13,293→51,626개/s로 늘었다. 더미/관측 프로세스는 개선 후96명이상에서0.9코어를넘으므로 앞선 수용량 검사의 생성기 여유 기준을 충족했다고 주장하지 않는다. 낮은 지연SLO·최대동접은 미확정이다. 원시 결과는 artifacts/logs/lobby-throughput-{short,soak,profile}-{before,after}-20261013; 20실행 모두 종료/연결0·fingerprint 안정 확인(전체통과17/20, 실패3건은 개선 전 유지검사)이다.

검증: Release 서버빌드, Core1,479·ABI110종/106상수, 실제 게임/로비OBJ등가52,672건(로비24,768건), 통합/실패경로7건, Python49검사 통과. 기존 CNetworkMgr의C4065 경고는 남아 있다. 프로파일/지형의 누적값·단위·추가 필드 계약을 Server Lab에 기록했다. HTML5섹션·내장PNG2개·링크/수치 검사와 PNG실제 이미지 확인을 수행했다. Archify showcase validate/deliver9/9·오류0·경고0. 로컬파일 접근 차단 이력으로 전체HTML브라우저 레이아웃·Archifyvisual-check는 미실행이다. DB/블록체인·전체GUI플레이·다중채널병렬처리·120초검사의5회반복은 미검증이다.

추가 기능 검사: 기본 채널 용량4명 설정에서 lobby-cycle 4명 재접속20 assertion 통과, 실제 GET /api/state의 admission.terrain_index 양쪽 ready=true·면/셀/전역후보/캐시 bytes 조회 확인. 종료 후 양쪽 서버 pending=0 sockets=0 leased=0과 소유 실행 기록 제거를 확인했다. Python/monitor는 3.12.14이며 환경·빌드/소스/리소스 hash·검증 결과는 artifacts/logs/lobby-throughput-*에 보존한다.

후속 과제:
- [ ] CViewList 비용을 채널 순회·거리 검사·뷰 차이 계산·전송 요청으로 분리하고, 밀집 유저의 송신 묶음/이동 주기 개선을 검토
- [ ] 더미 수신을 여러 프로세스/호스트로 분산한 뒤 96명 이상의 최대 처리량·낮은 지연 경계를 재측정
- [ ] 기존 중첩 shared lock·첫 업데이트 시간 초기화·타이머 idle loop를 별도 수정하고 각각 회귀/성능 검증
- [ ] 120초 유지 검사를 전후 각5회 이상 교차 반복해 변동과 장시간 안정성을 확인
## 93. 캐시 적용 후 동접 증가·병목 및 결함 조사 (2026-10-13)

- [x] Release LOCAL_TEST·DB 없는 환경과 현재 캐시 기준 소스/실행 파일/입력 hash 보존
- [x] 게임 4인 매치·로비 이동/정지 동접을 단계적으로 증가시켜 기능 실패와 처리 지연 경계 탐색
- [x] 경계 인원을 반복 확인하고 서버 CPU/tick/큐/송신과 더미 발행 지연·CPU를 구분
- [x] 발견 지점의 코드·실측을 대조하고 재사용 가능한 부족 지표/시나리오 추가 및 검사
- [x] 최대 통과 인원·첫 실패/품질 저하 인원·재현 조건·미확정 원인을 HTML/근거 문서에 기록

기준 커밋은 `012d1046`이다. 입장 소프트 제한을 해제하고 원본 OBJ·게임 로직은 유지한다. 기존 OBJ 개편/롤백 문서는 보존한다. 단순 접속 최대, 이동/정지 기능 통과 최대, 목표 tick 주기를 유지하는 부하를 구분한다. 탐색 실행은 조건별 1회로 시작하고 경계는 최소 5회 대조한다. 측정 전 기능 성공 조건은 모든 참가자의 로그인·이동 응답/위치 변화·최종 정지 및 정상 종료이며, 더미 발행 지연/CPU 포화가 있으면 서버 최대 동접으로 확정하지 않는다. 게임 목표 주기는 구현에서 확인하고 명시적인 품질 기준으로 별도 집계한다. 처음부터 끝까지 누적한 단일 부하와 실행별 재시작한 독립 단계 부하를 구분한다.

경계 반복 검사의 임시 품질 기준: `NextFrameTime`의15ms(이론66.7Hz)를 기준으로 매치당60Hz 이상, 패킷 작업 대기 p99 bucket 상한50ms 이하, 더미 발행 지연 p99 25ms 이하, 기능/종료 검사 전부 통과. 제품에서 확정한 SLO가 아닌 이번 검사의 명시적 기준이다. 서버CPU에는 전체12논리 코어 정규화 비율을, 더미/관측기에는1코어 기준 비율을 함께 표시한다.

120초 보조 유지 진단은 각1회 seed3이며 최종 입력 수 충족·패킷/송신 큐0 연속3표본을20초 안에 확인한다. 64명 통과/128명 적체를 확인한 뒤96·112명을 추가 탐색한다. 세분화 측정 판정 전 입력 처리량 유지 기준을 `측정 완료 패킷/s >= 동접×4×0.99`, 정리 통과, 기능/종료 성공, 더미 발행 p99<=250ms/CPU<0.9코어로 정한다. 이는 입력 대비 처리량 판정이며 낮은 응답 지연이나 운영 SLO를 뜻하지 않는다.

결과: [HTML 보고서](../docs/development/SERVER_CAPACITY.html)·[수치/재현 근거](../docs/development/SERVER_CAPACITY.md). 게임 누적512명/128매치 이동 확인, 11.86Hz·tick 본문2.44ms·Post→handler61.37ms·CPU32.31%. 1000명 목표 확장 중596명 준비 시 더미 주기 지연1.484초로 중단해 절대 서버 상한은 미확정이다. 게임16/20/24명 각5회 기능/종료는 모두 통과했으나60Hz 품질을5회 모두 충족한 인원은 없다.

로비 누적64명 통과/128명에서11명 이동 검증 실패. 독립20초64/128명은 각5회 통과했지만128명 미처리 중앙값2555개였다. 120초 유지64명은256.06패킷/s·최종 정리4.219초로 처리량 기준 통과, 96명은입력384/s 대비369.27/s·미처리2036개·정리16.328초로 처리량 기준 미달이다. 112명은399.07/448패킷/s, 128명은433.54/512패킷/s이며 입력 중단20초 후 각각511/2742개가 남아 정리 실패했다. 따라서 확인된120초 처리량 유지 최대는64명이고96명부터 적체가 관측됐다. 엄밀한 단일 인원 상한·운영 지연 SLO는 확정하지 않는다.

적용: 동접 누적/유지 시나리오, 단계별 결과·가벼운 progress API, 게임 타이머 전달 전/후 histogram, 실제 신선 매치 수, 스레드 CPU 조회, 월드 관측 그룹 제한/전체 탐색/수신 양보 문제 수정, headless 및 최종 입력/빈 큐3표본 검증. 서버 게임 정책은 유지했다. 계측/프로파일 켬·프로파일 끔·계측 끔16명 각5회 CPU 대조도 보존했다. raw자료는 `artifacts/logs/capacity-*20261013`에 실패/취소까지 보존하고 모든 완료 실행의 정상 종료·연결0·측정 중 hash 유지를 확인했다.

검증: Release 서버 빌드·Core1479/ABI110종·106상수/실제 지형27904비교·Python49검사·Archify9/9·diff 검사 통과. 기존 컴파일 경고는 유지한다. 보고서 PNG 확인과 HTML 구조/링크 검사를 수행했으며 기존 local-file 거절을 우회하지 않아 실브라우저 시각 검사는 미수행이다. 운영DB/블록체인·근거리 전투 전체·장시간 운영 안정성은 미검증이다. 기존 OBJ 개편/롤백 문서를 보존하고 커밋·push는 하지 않았다.

후속 개선 항목(이번에는 서버 게임 정책을 변경하지 않음):

- [ ] 로비 단일 JobScheduler 경로의 이동 반복 등록·충돌/가시성 검사 구간별 비용을 분해하고 고정 주기/관심 영역 갱신을 같은 부하로 비교
- [ ] 게임 예정→Post 지연과 Post→MatchUpdate 대기를 각각 줄이는 타이머/이벤트 처리 방안 독립 비교
- [ ] 게임 InGameUpdate의 연속 중복 SendMovePacket 제거와 위치/정지·송신량 회귀 검사
- [ ] 로비 첫 Initialize의 m_updateTime 초기화 및 신규/재접속 elapsed histogram 회귀 검사
- [ ] 로비 CViewList의 동일 m_viewLock 중첩 shared 획득 제거, 읽기/쓰기 경합·종료 검증
- [ ] 로비 idle TimerFunc의 yield 루프와 고CPU 스레드 식별, blocking wait 대조
- [ ] 512명 이후 절대 서버 한계 확인용 다중 프로세스/별도 호스트 더미, 더 긴 유지 부하·근거리 전투 시나리오 추가

## 90. 지형 정적 계산 캐시·Unity 메시 품질 개선 (2026-10-13)

- [x] 이번 커밋 전 재검증: Release Core/ABI·지형27904비교·Python37검사 PASS, 캐시 소스·원본 OBJ hash 유지 확인
- [x] 캐시 범위 커밋 준비: OBJ 개편·롤백 문서는 제외하고 구현·관측·검사·읽기 전용 품질 진단·보고서만 선별; 사용자 메시지 승인 후 push·main 병합 예정
- [x] 승인한 공간 분할 변경 `88e6e295` 커밋·push, 이전 기준 실행 파일·측정 보존
- [x] Unity exporter와 원본 OBJ의 출처/분류 정보 확인
- [x] Ray의 불변 계산을 초기 로드 시 캐시하고 기존 판정과 등가 검증
- [x] 원본을 보존하는 OBJ 품질 진단 CLI·실제 메시 진단·입력 오류/출처/중복/퇴화 검사 추가
- [ ] HeightMesh 포함 대상 확정; 원본 Unity 프로젝트 부재로 OBJ 검토·편집 필요, 실제 제외 목록은 미확정
- [x] 이전 공간 분할 기준과 동일 조건 성능 비교·관측 지표/보고서/흐름도 갱신
- [ ] 1매치 CPU 증가의 원인 분리: tick 밖 스레드 비용·프로세스 CPU 대조 후 적용 판단

사용자는89번의3·4순위를 먼저 요청했다. 중복 송신·타이머 정책 변경은 이번 범위에 포함하지 않는다. 현재 HeightMesh OBJ에는 오브젝트/그룹 정보가 없어 불필요한 오브젝트를 특정할 수 없고, 저장소에는 원본 Unity 씬/프로젝트 설정이 없다. 기존 자원은 보존하며 원본 프로젝트와 지면 포함 기준을 확인한 뒤 재추출 범위를 결정한다. 삼각형 크기 차이 자체는 결함으로 단정하지 않는다.

3번 캐시는 법선·변·내적·분모 역수를 원본 float 순서로 계산해 삼각형당52byte 저장한다. 추가 캐시6.620MiB, 기존 인덱스 포함8.172MiB다. 이 단계의4번은 읽기 전용 `terrain_audit.py` 진단까지 구현했다. Unity exporter 수정·재추출과 실제 지면 제외 목록 확정은 미완료다.

실제 성능 비교30/30 PASS: 1/4/16매치의 tick 중앙값2.327→2.171/3.767→3.624/2.652→2.441ms, CPU1.099→1.593/7.013→5.949/15.851→13.320%. 16매치 예약 지연8.591→8.585ms·갱신42.384→42.406Hz로 잔여 지연은 해결되지 않았다. HeightRay1.047→0.822/NavRay0.324→0.276ms/tick, 프로세스 private bytes 표본 최대값 중앙값34.75→41.42MiB다. 공간 분할 기준의 frozen EXE와 동일 seed1~5, AB/BA·4인 매치·입장 소프트 제한 해제·5초 워밍업/20초 측정/250ms 입력·양쪽 계측 켬 조건이다.

저부하 CPU 증가 때문에 1매치 기존 끔/캐시 끔/캐시 켬 3군을5회 교차 추가 대조해15/15 PASS. 계측 끔 기존→캐시 tick2.130→1.983ms, CPU1.159→1.545%로 CPU 중앙값 증가는 남는다. 같은 캐시 끔→켬은 tick1.983→2.202ms·CPU1.545→1.764%. 실행 간 CPU 범위가 겹치며 직접 원인은 미확정이다. tick 계산 감소를 전체 CPU 개선 완료로 간주하지 않으며 저부하 CPU 원인은 후속 분석 대상으로 유지한다.

검증: Release 서버 빌드, Debug/Release Core 각1478검사·ABI110종/106상수·실제 메시27904비교, Python37검사 PASS. 실제45회 기능/정상 종료, 계측 프로필과 fingerprint/실행 파일 복원 확인. 기존 C4065/C4101/C4244/C4305/C4309 및 참조 검사 C4100/C4189/C4505 경고 유지. 전체 GUI·근거리 전투/Astar·운영 DB·Unity 통합 및 실제 HTML 시각/containment 검사는 미수행이다. Archify showcase9/9·composition 오류0/경고0, 그래프 PNG 직접 확인.

근거: `artifacts/logs/terrain-cache-20261013/`, `terrain-cache-compare-20261013/`, `terrain-cache-control-20261013/`. [HTML 보고서](../docs/development/TERRAIN_CACHE.html)·[실측·품질 계획](../docs/development/TERRAIN_CACHE.md)·[흐름도](../docs/diagrams/terrain-cache/README.md).

## 89. 공간 분할 커밋 준비·기존 실측 기반 후속 개선 검토 (2026-10-13)

- [x] 공간 분할 구현·계측/API·회귀 검사·비교 도구·보고서의 커밋 범위 검토
- [x] 이전 실측의 동일 seed 성공 쌍과 현재 서버 소스 299개 SHA-256 일치 확인·잔여 비용/지연 검토
- [x] Release 서버 빌드·Core1471검사/ABI110종·106상수·실제 지형27904비교·Python35검사·UTF-8/구문·staged diff 검사 PASS
- [x] 커밋 메시지 미리보기 승인 후 `88e6e295`를 `codex/game-tick-profiling`에 커밋·push

기준은 `artifacts/logs/terrain-spatial-compare-20261012/paired-analysis.json`의 공간 분할 적용 후 값이다. 1/4매치는 각5쌍, 16매치는 seed2~5의4쌍이며 기준 seed1 로그인 timeout은 원시 결과에 유지한다. 다음 값은 실행별 평균의 실행 간 중앙값으로, 이번 검토에서 부하 측정을 새로 수행한 값이 아니다.

| 매치 | tick 실행 ms | 예약 지연 ms | CPU % | Hz/매치 | HeightRay ms/tick | NavRay ms/tick | Send 잠금 획득 wall ms/tick |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2.386 | 1.113 | 1.651 | 62.053 | 0.754 | 0.335 | 0.229 |
| 4 | 3.722 | 1.291 | 6.476 | 61.374 | 0.849 | 0.326 | 1.667 |
| 16 | 2.628 | 8.346 | 14.726 | 42.823 | 1.031 | 0.323 | 0.730 |

stage는 중첩하므로 이 표의 열들을 합산하지 않는다. Send 잠금 wall은 잠금 획득 경과 시간으로 OS 스케줄링도 포함하며 CPU 사용 시간과 구분한다. 16매치 HeightRay/NavRay의 tick worker cycle 비중 중앙값은 각각49.29%/15.33%다. jobs 구간은 약0.0015ms/tick이지만 큐의 모든 잠금·tick 밖 작업을 포함하지 않으므로 큐 전체 비용으로 해석하지 않는다.

후속 순서와 판정 조건:

1. **동일 이동 패킷 중복 제거를 독립 검증.** `CMatch::InGameUpdate`의 같은 수신자/위치/방향 `SendMovePacket` 연속2회가 코드에 남아 있다. 4인 매치의 해당 루프는32→16프레임/tick으로 줄일 수 있으나 전체 송신·WSASend·CPU가 절반이 된다는 의미는 아니다. 중복 한 호출만 제거한 대조군에서 위치/정지/방향 갱신·수신 프레임/tick·송신 bytes/대기·native calls·종료 수명을 확인한다. 클라이언트가 중복 수신에 의존하는지도 검증한다.
2. **예약 지연의 원인을 분리한 뒤 타이머/송신 구조 개선.** 16매치 실행2.63ms에도 예약 지연8.35ms·42.82Hz가 남는다. `TimerFunc`의 미래 이벤트 pop/push·1ms sleep은 확인되지만 이것이8.35ms의 직접 원인인지는 미확정이다. 예정시각→timer dispatch, IOCP Post→handler 시작, handler 실행을 같은 이벤트 식별자와 단조 시계로 분리 수집하고 worker/타이머/tick 밖 완료 처리 CPU를 확인한다. 현재 tick_delay는 예정시각→handler 시작의 합계다. 여러 연결이 공유하는 `IocpService::m_mutex`와 완료 처리의 경합을 함께 조사하되, 원인 확인 전에 잠금 분리·worker 증설·주기 변경을 적용하지 않는다.
3. **지형 CPU는 정적 삼각형 계산 캐시부터 대조.** 법선·edge·dot·분모처럼 Ray마다 반복하는 불변 계산을 원본 float 연산 순서와 첫 hit 순서를 보존해 로드 시 캐시하는 후보를 검토한다. Nav 노드별 캐시를 연속 배열로 두는 경우 메모리 증가/접근 비용도 측정한다. 기존27,904등가 검사에 경계/겹친 면/스킬·실제 이동을 유지하고 CPU·Ray당 방문/처리 시간·메모리를 비교한다. 캐시 효과는 미측정이다.
4. **전역 후보와 품질 개선은 별도 검증.** Height3,738/Nav600개 전역 후보를 단순 삭제하지 않는다. 분류 이유별 개수뿐 아니라 실제 방문/히트/비용을 계측하고, 셀 크기2/4/8 비교는 지역 후보에 대한 효과와 전역 후보 비용을 구분한다. 정지 위치 캐시는 점프·순간이동·강제이동·지형 교체 등 무효화 조건을 확정한 뒤 정지 전용 시나리오로 검토한다. ContainsEdge·OBJ 검증·근거리 전투/Astar 품질은87번 과제를 유지한다.

각 변경은 공간 분할 완료 상태를 새 기준으로 독립 AB/BA 비교한다. 기존 perf-match 4/16/64명·4인 매치·seed1~5·5초 워밍업/20초 측정/250ms 입력·LOCAL_TEST/입장 소프트 제한 해제 조건과 실패 분모를 유지한다. 개선 후 짧아진 tick에서 계측 비중이 달라질 수 있으므로 동일 바이너리 profile 켬/끔 대조도 다시 확보한다. 목적에 따라 정지 전용·NPC 근거리 전투/ACK·복수 웨이브 시나리오를 추가하며 이번 검토에서 구현/성능 검증이 끝난 것으로 간주하지 않는다.

커밋 전 재검사 로그: `artifacts/logs/terrain-commit-20261013/`. 이전 Debug·실제 네트워크/미니언 검증은87번 기록과 hash가 같은 서버 소스를 근거로 유지하며 이번에 반복 실행하지 않았다. 기존 서버/참조 테스트 경고는 유지한다. HTML 실제 시각 검사·전체 GUI/운영 DB 검증은 추가 수행하지 않았다.

## 88. 프로파일링 변경 분리 커밋 준비 (2026-10-12)

- [x] 선택 활성화 tick 계측·분석 도구·기준 측정/과거 조사 문서만 스테이징하고 공간 분할 구현/검증/보고서는 제외
- [x] 공간 분할 없이 스테이징한 소스의 Debug/Release 서버 빌드·Core 각218검사/ABI110종·106상수·Python34검사 PASS, 작업 파일/기존 실행 파일 복원 확인
- [x] 사용자 미리보기 승인 후 프로파일링 35개 파일 커밋 `4673b396`·`origin/codex/game-tick-profiling` push 완료; 공간 분할 작업은 미커밋 보존

사용자 요청에 따라 계측 변경만 먼저 기록한다. CServer/GameUtil/MathUtil은 SHA-256을 검증한 개선 전 계측 snapshot으로 부분 스테이징하며 현재 공간 분할 작업 파일은 보존한다. 지형 인덱스 통계/API·전용 등가 테스트·교차 비교 도구·개선 보고서도 다음 개선 커밋 범위로 남긴다.

## 87. NavMesh·HeightMesh XZ 공간 분할 (2026-10-12)

- [x] 로드 완료 후 읽기 전용 격자 구축·두 메시 RayCast 연결
- [x] first-hit 순서·높이/스킬 차이·경계/중첩·비수직 Ray fallback 등가 검증
- [x] Debug/Release 빌드·Core/ABI·실제 이동/미니언 및 실패 경로 검증
- [x] 동일 seed·1/4/16매치 전후 교차 실측·보고서/흐름도 작성

86번의 계측과 기준 실행 파일을 보존한다. 이번 범위는 공간 검색이며 삼각형 판정 수식·게임 규칙·JobQueue·wire는 유지한다. 인덱스는 메시의 최종 순서를 보존하고 worker 시작 전에 구축하며 실행 중 변경하지 않는다. 품질 개선 중 지형 데이터/삼각형 판정 자체 변경은 별도 후속 과제로 분리한다. 커밋·푸시는 요청되지 않았다.

구현: 두 메시 공통 `TerrainSpatialIndex`의4단위 XZ 격자·CSR ID 목록과 전역 후보 순서 병합. 경계 패딩·불안정 면 전역 유지, 비수직/극단 Y·무효 인덱스의 선형 fallback. 기존 first-hit·높이 필터/보정·스킬 overload·miss 출력값을 유지한다. 인덱스에는 ID만 저장하며 조회 시 할당/잠금이 없다. startup 통계 복사본을 Server Lab API/원시 결과에 전달한다.

검증: 실제 메시 Nav5,743/Height127,756삼각형·고정 참조 Ray와27,904비교를 Debug/Release에서 각각 통과했다. Core 각1,471검사·ABI110종/106상수·Python35검사 PASS. Release 네트워크12종·보스4/5 매치·초기화 실패4종 및 Server Lab 통합8종(첫 미니언4개 이동 포함) PASS·정상 종료 자원0. 원시 자료 `artifacts/logs/terrain-spatial-20261012/`, `test-server-terrain-{Debug,Release}/`.

실측: 전후30회 중29회 성공(개선15/15, 기준14/15). 기준64명seed1에서 client4의 SC_LOGIN_INFO10초 timeout이 발생해 측정 구간이 없으며 원시 실패를 보존했다. 원인 미확정이고 timeout을 완화하거나 실패를 0으로 채우지 않았다. 성능은 같은 seed의 양쪽 성공 쌍만 사용했다(1/4매치5쌍씩,16매치4쌍). tick 실행 중앙값20.72→2.39/24.01→3.72/31.07→2.63ms, 게임 CPU7.58→1.65/31.02→6.48/49.59→14.73%, 갱신42.36→62.05/38.34→61.37/11.97→42.82Hz/매치다. 16매치에서도 미니언 Update4.561→4.526마리/tick, HeightRay 방문50,223→1,391개·NavRay3,090→334개/Ray로 감소했다. CPU 감소와 갱신 증가를 함께 확인했다.

범위: 동일 Release LOCAL_TEST·입장 소프트 제한 해제·양쪽 계측 켬·5초 워밍업/20초 측정/250ms 이동·정지·seed AB/BA·매회 재시작. 최대 수용량 검사가 아니다. 16매치의 예약 지연8.35ms와42.82Hz는 잔여 대기이며 원인 미확정이다. index capacity 저장량1.551MiB 추가·전체 프로세스/구축 임시 메모리와 구분. 종료 자원0·355개 frozen source/실행 파일/도구 hash·비교 중 fingerprint 불변·개선 실행 파일 복원을 확인했다. 자료는 `artifacts/logs/terrain-spatial-compare-20261012/`다.

[HTML 개선 보고서](../docs/development/TERRAIN_SPATIAL_INDEX.html)·[상세 근거](../docs/development/TERRAIN_SPATIAL_INDEX.md)·[흐름도](../docs/diagrams/terrain-spatial-index/README.md). Archify9/9·오류/경고0·PNG 그래프2개 직접 검토. 기존 local-file 보안 거절을 우회하지 않아 HTML 실제 시각/containment 검토는 미수행이다. 기존 서버 경고와 테스트에서 드러난 MathUtil/참조 코드의 C4100/C4189/C4505를 유지한다. 전체 GUI/DB/블록체인·근거리 전투/Astar·장시간·최대 수용량은 미검증이다.

- [ ] 지형 품질: 첫 hit와 근접 hit·계단/경사/겹친 지면의 기대값 확정 후 높이 판정 수식/보정 개선
- [ ] NavMesh·HeightMesh의 불안정/가늘거나 거의 수직인 면을 조사하고 전역 후보 축소·정적 계산 캐시 검토
- [ ] OBJ 입력 인덱스·잘못된 면·재로드의 원자적 교체/노드 수명 검증 보강
- [ ] `Triangle::ContainsEdge`가 메시 정점 없이 인자끼리/자기 자신을 비교하는 기존 구현을 수정하고 인접 관계·Astar 품질 검증
- [ ] 기준64명seed1 로그인 응답 timeout의 직접 원인 추적(이번 비교 기준14/15 성공, 개선15/15 성공)
- [ ] 공간 검색 개선 후 잔여 예약 지연·tick 밖 CPU·타이머/worker·송신/관측 비용 분리

## 86. 게임 서버 tick 로직 비용 분석 (2026-10-12)

- [x] 큐 실험 전체 복구 사본 보존·main 기반 새 브랜치 생성·기존 큐 후보와 실험 도구 제외
- [x] 게임 tick·이동/지형·NPC·송신/잠금의 선택 활성화 계측과 API/저장 결과 분석 도구 추가
- [x] Release 실제 1/4/16매치 반복 측정·계측 비용 대조·실패/종료 자원 확인
- [x] 지배 비용과 확인된 결함/후속 개선을 HTML 보고서·흐름도·작업 기록에 정리

새 브랜치는 `codex/game-tick-profiling`이며 기준은 main `3e34e71a`다. 로컬 `codex/jobqueue-throughput-optimization`을 삭제했다. 이전 미커밋 파일·binary patch·SHA-256 manifest는 Git 제외 `artifacts/logs/jobqueue-retired-20261012/`에 보관했다. 기존 큐 추가 개선 C++/테스트와 전용 비교/probe/보고서 생성 도구는 제외하고 보고서4개·첨부 다이어그램2종·인덱스·조사 기록만 보존한다. 원시 실측과 frozen 실행 파일은 기존 artifacts 경로에 유지한다. 큐 추가 개선을 현재 구현으로 설명하던 ServerCore 문서는 main으로 복원했다.

이번 단계는 실제 비용 분석과 필요한 계측이다. 게임 규칙·wire·15ms 예약·기존 FIFO/동시성 경계는 유지하고, 지형 검색/중복 송신의 실제 최적화는 분석 근거와 별도 검증 뒤 진행한다. 같은 perf-match 입력·입장 소프트 제한 해제·Release LOCAL_TEST·매회 재시작 조건을 사용한다. wall과 CPU cycle을 구분하며 cycle을 시간으로 변환하지 않는다.

결과: 실제20/20 PASS(1/4/16매치 profile 켬 각5회, 1매치 끔5회 교차). tick 실행 평균의 실행 간 중앙값20.27/23.96/31.03ms, 게임 프로세스 CPU7.65/30.90/49.62%, HeightRay17.02/20.63/28.34ms/tick·tick CPU cycle84.49/86.75/91.38%다. 1매치 높이 Ray의 미니언 비용14.56ms·플레이어2.43ms가 주요 구간이며, jobs는0.002~0.003ms/tick이다. HeightMesh127,756삼각형을 첫 hit까지 선형 방문하며 평균 방문 수35,799/39,739/50,261개/Ray와 미니언 Update3.74/3.95/4.56마리/tick의 차이도 확인했다. 다중 매치의 실행 시간 증가를 경합만으로 설명하지 않는다.

1매치 계측 끔→켬 tick20.34→20.27ms·CPU7.67→7.65%, 범위가 겹쳐 계측 비용0이나 개선으로 해석하지 않는다. 원본 main 대비 비활성 경로 비용 및 다중 매치의 계측 끔 대조는 미측정이다. 각 stage cycle_errors=0, API·저장 결과22구간 필드 확인·340개 소스/실행 파일/데이터 hash 유지·복구 snapshot 보존. 원시 결과는 `artifacts/logs/game-tick-profile-20261012/`다.

검증: Debug/Release 서버 빌드·Core 각218검사·ABI110종/106상수·Python33검사 PASS. Release 네트워크12종·보스4/5 4인 매치·초기화 실패4종 PASS. 성능/통합 정상 종료의 pending/sockets/leased=0 확인. 기존 C4065/C4101/C4244/C4305/C4309 경고 유지. Archify9/9·composition 오류0/경고0, 그래프 PNG 직접 점검. 기존 local-file 브라우저 보안 거절을 우회하지 않아 HTML 실제 시각/containment 검토는 미수행이다. 전체 GUI 플레이·운영 DB/블록체인·전투/Astar·장시간/최대 처리량은 미검증이며 Astar 호출0을 비용이 없다는 뜻으로 해석하지 않는다. 커밋/푸시 없음.

[HTML 분석 보고서](../docs/development/GAME_TICK_PROFILE.html)·[실측 근거](../docs/development/GAME_TICK_PROFILE.md)·[계측 도구](../tools/server_lab/tick_profile.py).

- [x] 기존 first-hit 순서/높이 필터/스킬 차이를 보존하는 XZ 공간 검색·동일 조건 전후 비교(87번)
- [ ] 정지 전용·근거리 전투/Astar·복수 웨이브 시나리오를 목적에 맞게 추가하고 상태 차이를 통제
- [ ] 지형 개선 후 잔여 CPU·tick 밖 IOCP/타이머·잠금 비용을 계측해 후속 우선순위 확정

## 85. 적은 매치 수의 tick 실행 시간 초과 확인 (2026-10-12)

- [x] 기존 1/4/16매치 원시 결과를 현재 집계 함수로 재계산하고 보존한 요약과 대조
- [x] tick 실행 시간과 예약 지연을 구분하고 최신 JobQueue 후보의 측정 범위 명시

`artifacts/logs/jobqueue-improvement/final-on/`의 게임 4/16/64명 각5회 원시 JSON을 `baseline.summarize_run`으로 재집계했다. 각 조건5/5 PASS이며 보존한 report.json과 실행 평균이 일치한다. 실행별 평균의 중앙값은 1매치20.342ms(19.732–21.121), 4매치23.542ms(22.859–26.247), 16매치31.493ms(31.090–34.598)다. 예약 지연 평균의 실행 간 중앙값은 각각8.318/10.830/69.684ms다. `TimeUtil::NextFrameTime()`은 현재 시각+15ms이며 1매치부터 실행 시간이 이 예약 간격을 초과한다. 매치 수 증가에 따른 대기 증가와 개별 tick 비용을 구분한다.

위 결과는 2026-10-11 FIFO/송신 개선 후 측정이며 최신 JobQueue 후보의 저부하 측정으로 해석하지 않는다. 84번 최신 비교는 64명·16매치 조건이다. 새 서버 실행·성능 측정·빌드·구현 수정·커밋/푸시는 수행하지 않았다. 세부 로직별 비용과 경합 원인은 추가 계측 전까지 미확정이다.

## 84. JobQueue 추가 개선의 실제 서버 성능 악화 원인 조사 (2026-10-12)

아래는 조사 당시 기록이다. 이후 사용자 요청으로 86번에서 큐 후보·전용 도구를 제외하고 보고서/원시 결과·복구 사본을 보존했다.

- [x] 기존 원시 실행·histogram·큐/서버 호출 경로 대조
- [x] 보존한 원본/개선 바이너리로 동일 seed의 실행 순서를 교차한 로비·게임 비교
- [x] 단일 스레드/1생산자·1소비자 비용·실제 tick 경로·측정 편차의 근거 분리
- [x] 확인된 결과와 미확정 원인을 결과 우선 보고서 및 후속 작업에 기록

직전 83번의 처리량 증가를 실제 서버 개선으로 간주하지 않는다. user가 실제 성능 악화를 지적했으므로 단위 벤치마크와 실제 호출 빈도/작업 크기의 차이, 새 미시작 수 atomic·두 번째 deque 비용, 1초 표본/2배 bucket p99의 해상도, 순차 매치 준비에 따른 NPC/timer 상태 차이를 조사한다. 원본과 개선 파일·실패 실행은 보존하며 추적을 위해 큐 구현을 임의 수정하거나 성능 실패를 timeout 완화로 덮지 않는다. 원인 조사와 필요한 측정 도구 확장 범위이며 커밋/푸시는 요청받지 않았다.

조사: 보존한 전후 Release 바이너리·64명·게임16매치×4명·계측 켬·입장 제한 해제·기존5초/20초/250ms 조건으로 seed 홀짝 AB/BA를 실행했다. 게임/로비 각5쌍 총20/20 PASS·종료 자원0·시작 전 실행 파일 복원 hash 일치다. 게임 CPU49.60→49.72%·평균 tick 실행30.76→30.65ms·tick 지연67.60→67.33ms·매치별 갱신12.10→12.14Hz로 과거 게임 p99 악화는 재현되지 않았다. 원본 추가 진단에서도 p99 상한262.144ms·낮은 갱신량이 있었고 기존 CPU 감소는 갱신량 감소를 동반했다. 과거 시스템 부하/성능 비율이 없어 정확한 원인은 미확정이다.

기존 1생산자·1소비자 벤치마크를2,000,000작업×5회 교차 재측정(40/40)하니 계측 끔 Five -8.1%·Snapshot -6.3% 처리량 감소가 재현됐다. 작은 묶음/자기 재등록의 단일 스레드 probe90/90과 별도 정렬 대조120/120을 실행했다. atomic 제거 대조는 계측 켬/끔 방향이 달라 직접 원인으로 입증되지 않았다. 큐/잠금/카운터64바이트 정렬 대조는 1생산자·1소비자 계측 끔143.139→122.076ns/job(-14.7%)로 메모리 배치/경합 비용이 유력하다. 어떤 필드의 false sharing인지·하드웨어 캐시 미스·실제 게임 악화까지는 확정하지 않는다. 대조군은 Git 제외 헤더/실행 파일만 생성했고 제품 큐는 수정하지 않았다.

게임 패킷 callback wall은 tick 합산 wall의 최대0.0017%로 측정됐으며 큐 잠금/atomic은 제외한 값이다. 이동/지형·NPC·송신이 큐 외부 tick에 있고 전후 모두15ms 목표를 초과한다. CMatch의 동일 이동 프레임 연속2회 송신은 기존 결함으로 확인했다. 단일 큐 처리량을 서버 성능으로 대체하거나 낮은 CPU를 개선으로 판정하지 않는다.

- [ ] 게임 tick의 이동/지형·NPC·송신·잠금 구간 비용/대기와 매치 나이·활성 NPC 수를 계측해 지배 비용 분해
- [ ] 캐시라인 배치/공유 카운터 경합을 필드별 대조·하드웨어 프로파일로 확인한 뒤 FIFO·Five/Snapshot·lost wakeup을 보존하는 후속 설계
- [ ] 동일 SendMovePacket 중복 송신을 독립 수정·검증하고 송신량/tick 비교
- [ ] 과거 게임 로그인 timeout과 큰 tick 악화의 직접 원인 추적; 이번 미재현을 결함 해소로 간주하지 않음

`compare.py`·PDH 시스템 표본·`analyze_compare.py`·큐 비용 probe/정렬 대조와 분석 회귀2개를 추가했다. Python31검사 PASS, 비용 probe의 Release 빌드 경고 없음. 서버/Core의 재빌드·별도 통합 재실행은 수행하지 않았으며 이전 기존 경고/검증 범위를 유지한다. 원시 결과는 `artifacts/logs/jobqueue-regression-20261012/`·`jobqueue-regression-cost-20261012/`·`jobqueue-regression-cacheline-20261012/`다.
[HTML 조사 보고서](../docs/development/JOBQUEUE_REGRESSION.html)·[상세 실측](../docs/development/JOBQUEUE_REGRESSION.md)·[도구 흐름도](../docs/diagrams/jobqueue-regression/README.md). 기존 보고서 수치는 보존하고 후속 판정 링크를 추가한다. Archify9/9·오류/경고0, 그래프 PNG 직접 검토. 기존 local-file 보안 거절을 우회하지 않아 HTML 실제 시각/containment 검토는 미수행이다. 운영 DB·전체 GUI 게임 플레이·장시간 전투·최대 처리량은 미검증이다.
최종 검사: Resource35개·후보 큐 헤더 hash 유지·양쪽 후보 바이너리 복원·실행 기록 없음 확인. 옵션 결합 smoke30/30으로 비원자 대조군이 threaded에서 제외됨을 확인했다. Python AST6파일·HTML5항목/그래프3개/근거 링크·UTF-8·C++ clang-format·git diff --check PASS. 원시 검증 요약은 조사 경로 verification.json에 보존한다.

## 82. IOCP 완료 처리 책임 분리 (2026-10-11)

- [x] Poll·완료/잠금 계약·두 서버 호출부와 기존 회귀 검사 확인
- [x] Poll의 대기/잠금/최종 완료와 연산별 처리 분리
- [x] AppEvent·오류/계측·callback 직렬성 등 분리 경계 회귀 확인
- [x] Debug/Release Core·ABI·Release 서버 빌드 및 실제 네트워크/종료 검증
- [x] 현재 코드 문서·Archify 완료 흐름도·실제 검증과 미검증 범위 기록

완료 종류는 고정된 IoOperation이므로 private switch와 처리 함수로 분리한다. 부분 송신 재발행·Disconnect 보류는 ContinuePolling, 최종 전달은 Deliver로 명시하며 Complete/오류 계측은 공통 경로에서 한 번 수행한다.
연결 callback 잠금 → 코어 상태 잠금 순서와 Completion의 callback guard 수명은 유지한다. 가상 핸들러·공개 API·패킷 형식·전송 배치 정책·m_deferred 의미는 변경하지 않는다. 목적은 책임 분리이며 성능 향상을 측정한 것으로 간주하지 않는다.

구현: GetDispatchMutex가 소켓/AppEvent key의 실행권을 조회하고 HandleCompletion의 switch가 Send·Disconnect·Accept 핸들러로 분류한다. Receive 성공 byte 계측과 AppEvent 통과는 분류 함수 안에 간결하게 유지한다. 오류 수·pending 감소·Completion 반환은 Poll의 공통 경로에 둔다.
검증: Test-ServerCore.ps1 Debug/Release 각 212검사·ABI 110종/106상수 PASS, Build.ps1 -Configuration Release -Module Servers PASS. 기존 부분 송신·배치 실패/취소·다중 worker callback/Disconnect 검사와 새 AppEvent key/byte/guard/계측·오류 1회 계측 검사가 통과했다.
Test-ServerCoreIntegration.ps1 Release PASS: 실제 네트워크 12종·보스 4/5의 4인 매칭/로딩·초기화 실패 4종, 각 종료 pending=0 sockets=0 leased=0. 결과는 artifacts/logs/test-server-core-{Debug,Release}/와 server-core-integration-Release/에 저장한다.
clang-format --dry-run --Werror(수정 C++ 3파일), UTF-8·Markdown 링크·git diff --check PASS. Release 서버 빌드의 기존 C4065/C4101/C4244/C4305/C4309 경고는 남아 있다.
기존 Archify workflow JSON/HTML을 갱신해 validate/deliver showcase 9/9·composition 오류/경고 0을 확인했다. 현재 hash/byte는 해당 README에 기록한다. 앞서 확인된 file URL 보안 제한을 우회하지 않아 실제 HTML 시각/containment 검증은 미실행이다.
새 성능 부하·Debug 서버 통합·전체 게임 플레이·운영 DB/블록체인·장시간 부하는 미검증이며 기존 JobQueue 성능 수치는 변경하지 않았다. 구현 당시에는 커밋·푸시하지 않았다. 후속 사용자 요청으로 관련 9개 파일을 검토·스테이징하고 정확한 메시지 미리보기를 준비한다. 미리보기 승인 후 main에 커밋·푸시한다.

## 81. 잔여 Serena 변경 검토·커밋 준비 (2026-10-11)

- [x] 잔여 3개 파일의 변경 범위·공용 실행 도구·기존 검증 근거 검토
- [x] NewWod 정적 환경·HTTP 연결과 등록된 9개 프로젝트의 세션/재연결·7개 프로젝트 심볼 재검증
- [x] PowerShell 구문·UTF-8·문서 링크·구조도 hash·staged diff 검증과 커밋 메시지 준비
- [ ] 정확한 미리보기 승인 후 main 커밋·푸시 및 원격 확인

실행 코드 변경은 scripts/Serena.ps1의 공용 초기화 mutex 대기를 60초에서 900초로 늘리는 것이다. 9개 프로젝트의 순차 초기화를 허용하며 개별 준비 제한 90초·숨김 실행·프로세스 소유권 검사·잠금 해제 경로는 유지한다.
docs/diagrams/serena-http/README.md와 70번에 공용 도구 적용·초기화 직렬화·기존 검증 근거를 정리했다. 사용자 공용 도구·다른 프로젝트 설정·로그·바로가기는 이 저장소 커밋 대상이 아니다.
이번 검토에서 Test-AgentEnvironment.ps1 -StaticOnly -SerenaHttp PASS(227소스), 9개 서버 상태/Startup 등록·독립 세션 2개·프로젝트 전환 거부·재연결 PASS를 확인했다. 언어 설정이 있는 7개 프로젝트의 실제 심볼 조회도 PASS이며 기존 빈 언어 목록 2개는 심볼 지원을 검증하지 않았다.
공용 Test-Projects.py --symbols 결과는 사용자 공용 verification.json에 저장한다. 기존 Unity/Dart 경고는 해소했다고 주장하지 않는다. 실제 Windows 재로그인·Codex UI 새 채팅·장시간 부하·게임 빌드는 이번 재검증 범위 밖이다.
커밋 대상은 Serena 관련 3개 파일이며 새 다이어그램이나 서버 구현 변경은 없다. 사용자 공통 commit 스킬에 따라 검토한 snapshot과 메시지를 먼저 제시하고 승인 후 커밋·푸시한다.

## 80. JobQueue 개선 커밋·푸시·main 병합 준비

- [x] 저장소/브랜치·원격 main·변경 범위와 기존 사용자 변경 확인
- [x] 커밋 대상 snapshot의 Core/ABI·서버 빌드·보고서/문서 검증과 메시지 미리보기
- [ ] 정확한 미리보기 승인 후 개선 변경 커밋
- [ ] codex/jobqueue-optimization 푸시·main fast-forward 병합·main 푸시와 원격 확인

사용자가 현재 작업의 커밋·푸시·머지를 요청했다. 대상은 JobQueue FIFO/ready 순환·연결별 전송 배치·SC_MATCH 전달 수정·회귀·보고서/구조도·이동 재설계 TODO다. 선행 텔리메트리 커밋 e41ae162도 함께 main에 포함한다.
별도 Serena.ps1·serena-http README와 tasks/todo.md의 Serena 확장 기록 변경은 이번 index에서 제외해 작업 트리에 보존한다. 로그·runtime·PNG/visual-check 생성물·사용자 공용 스킬은 커밋하지 않는다.
현재 origin/main은 199c1ab6이며 HEAD의 조상이다. 사용자 공통 commit 스킬이 정확한 staged snapshot과 메시지를 보여 준 뒤 명시적 승인을 받도록 요구하므로 이번 단계는 검증·스테이징·미리보기까지다. 승인 후 같은 snapshot으로 커밋하고, 원격 상태를 다시 확인해 병합한다.
이번 준비에서 Debug/Release Core 각 199검사·ABI 110종/106상수, Release 서버 빌드, Server Lab Python29, 보고서 원시 수치 48개/결정적 재생성/JS 모의 상호작용을 다시 확인했다. 새 성능 부하는 실행하지 않았으며 기존 LOCAL_TEST 120회 결과를 보존한다. 새 보고서와 완료 흐름도의 실제 브라우저 검증은 앞서 기록한 file URL 보안 제한으로 미완료다.

## 79. m_deferred 코드와 완료 처리 흐름 설명

- [x] Send 배치·취소·Disconnect 완료 재게시와 공통 Complete 경로 확인
- [x] 실제 코드에 맞춘 설명·Archify workflow JSON/HTML·문서 인덱스 작성
- [x] showcase validate/deliver 9/9·composition 오류/경고 0·hash·UTF-8·링크 검증
- [ ] 실제 HTML의 브라우저 시각/containment 검증: 기존 file URL 보안 차단을 우회하지 않아 미실행

docs/diagrams/iocp-completion-only/에 A 대표 native 완료와 B·C 개별 완료의 분기를 작성했다. true에서도 Complete와 결과 반환은 수행하며, 배치 처리/Disconnect 보류 판단만 생략한다. 원본 코드를 수정하거나 m_deferred 이름을 변경하지 않았다. 빌드·성능 검사는 재실행하지 않았다.

## 78. 임시 큐 대응 이후 클라이언트·서버 이동 방식 개선

- [x] 현재 대응의 임시 성격과 클라이언트·서버 공동 개선 범위 기록
- [ ] 로비·게임의 입력→서버 이동 계산→좌표 전송→클라이언트 표시 경로를 조사하고 예측/보간 적용 여부·갱신/전송 주기를 확인
- [ ] 임시 대응 검토: 이동 작업의 즉시 자기 재등록·중복 갱신 체인을 확인하고 주기적 예약/중복 예약 방지·제한된 연속 실행으로 큐 대기와 이동 갱신 공백 완화
- [ ] 공동 이동 설계 확정: 본인 캐릭터 입력 예측·서버 상태 대조/보정, 다른 캐릭터·NPC의 snapshot 보간을 후보로 검토하고 서버 권위·충돌 판정 유지
- [ ] 입력 순번·처리 완료 입력 식별·서버 tick 등 동기화 정보와 계산/전송 주기 설계, 시간 기준·재접속 초기화·급격한 위치 변경의 보정 규칙 정의
- [ ] 클라이언트·Lobby_Server·Game_Server·더미의 변경을 함께 적용하고 패킷 배치/packing·버전 호환·분할 수신·실패 경로 검증
- [ ] 입력→서버 적용/화면 반영 지연, 이동 갱신 간격, 좌표 오차·보정 빈도/크기, 큐 대기·tick·CPU·전송량 계측 추가 및 전후 비교
- [ ] 지속 이동·정지/재출발·급회전·지형/벽 충돌·재접속·다중 유저/매치와 지연·jitter·통신 정체 시나리오 추가, 합격 기준을 측정 전에 정의

현재 FIFO/ready 순환과 전송 배치 수정은 확인된 기아·출력 적체에 대한 임시 대응으로 유지한다. 이동 방식의 최종 개선이 완료된 것으로 보지 않으며, 이번 요청으로 구현을 추가하거나 되돌리지는 않는다.
사용자가 지적한 서버 계산 결과에 의존하는 클라이언트 표시는 큐/통신 지연을 체감 이동 지연으로 연결할 수 있다. 이 영향의 크기와 클라이언트의 기존 예측/보간 여부는 후속 조사 대상이며, 이미 확인한 포인터 주소 우선순위 기아·전송 적체와 구분한다. 클라이언트 예측을 도입해도 서버 작업 비용·공정성 문제는 별도로 개선한다.
77번의 유저 흐름 응답성 검토와 연계한다. 기존 4/16/64명·seed 1~5·계측 켬/끔의 조건은 보존하고, 새 이동 시나리오/네트워크 조건은 전후 동일하게 추가한다. 실제 클라이언트 화면 반영은 더미의 서버 좌표 수신만으로 검증했다고 간주하지 않는다.
서버가 최종 위치·속도 제한·충돌·게임 결과를 검증하는 권한을 유지한다. 설계 후보를 확정한 뒤 구조도/계약과 구현을 갱신하고, 결과·원인·해결·실측·추가 대응 순서의 보고서로 임시 대응과 최종 개선 효과를 구분한다.
이번 작업은 TODO 추가와 문서 형식 확인만 수행했다. 이동 구현·패킷·보고서·성능 결과는 변경하지 않았으며 빌드·부하 검사는 재실행하지 않았다.

## 77. 로비 FIFO 전환과 유저 흐름 응답성 검토

- [x] 유저별 큐·Five 예산·ready 순환·이동 자기 재등록과 기존 회귀 확인
- [x] 기능 순서 보장과 유저 체인의 재개 지연을 구분해 기존 실측·미측정 범위 검토

질문에 대한 검토이며 구현·측정·보고서는 변경하지 않았다. 큐는 유저별로 유지되고 한 차례 최대 5개를 실행하며, 남은 대상은 다른 ready 대상 뒤에서 재개한다. Five 예산 안에서는 새로 등록한 후속 작업도 앞선 작업 뒤에 실행 가능하지만, 예산 경계에서 같은 유저의 체인이 쉬는 시간은 생긴다. 기존 주소 우선순위는 이를 의도적으로 보장하는 정책이 아니며 작업/대상 기아 회귀가 있었다.
기능 60/60 통과는 응답성 보장이 아니다. 기존 로비 64명 패킷 대기 p99 bucket 상한 2,097.152ms·측정 종료 잔여 228개가 남아 있다. 유저별 이동 갱신 간격·명령 도착→적용 지연·ready 재개 지연을 분리한 검사는 미실행이다. 후속 후보는 유저 순서/직렬성을 유지하는 시간 예산·제한된 연속 실행과 주기적 이동 갱신/중복 예약 방지이며 적용 효과는 미검증이다.

## 76. JobQueue 보고서를 결과 우선 시각 보고서로 개편

- [x] 보고서 본문을 결과→원인→해결→실측→추가 대응의 다섯 항목으로 재구성
- [x] 원인/해결 코드와 핵심 비교 그래프·표를 작성하고 상세 근거는 링크로 분리
- [x] 재생성·원시 수치 대조·UTF-8/링크·그래프 이미지·JS 모의 상호작용 검증
- [ ] 새 HTML의 실제 브라우저 화면/상호작용 검증: 도구 보안 정책의 file URL 차단으로 미검증

write-engineering-report 공용 스킬을 적용한다. 서버 개선 코드·측정 원본·기존 기준 결과는 유지하며 보고서와 재생성 도구만 수정한다. 새 성능 검사와 커밋/push는 요청받지 않았다.
HTML을 결과 카드·짧은 원인/해결 코드·matplotlib SVG 6개·실측 6조건 표·후속 대응 5항목으로 개편했다. 전체 Markdown 본문을 HTML에 복제하지 않고 상세 근거로 연결한다. CPU·tick 미개선과 잔여 대기는 첫 결과에 표시했다.
독립 .runtime/report-venv에 matplotlib을 설치해 재생성했으며 HTML 자체에는 외부 의존성이 없다. 원시 비교 JSON 수치 48개·기능 통과 분모·실패/종료 자원, 결정적 재생성·SVG 6개·링크 10개·중복 ID 없음·JS 구문·선택 6상태·인쇄 호출·UTF-8·git diff --check PASS. 그래프 PNG를 직접 확인했다.
브라우저 도구가 file 프로토콜 접근을 정책상 거부해 새 레이아웃/넘침/브라우저 console은 확인하지 않았다. HTTP 서버나 다른 브라우저로 우회하지 않았다. 검증 기록은 artifacts/logs/jobqueue-improvement/revised-report-verification.json에 보존한다. 서버 빌드·성능 검사는 재실행하지 않았다.

## 75. 결과 우선·시각 자료 중심 공용 기술 보고서 스킬

- [x] 사용자 공용 skills에 보고서 작성 스킬과 UI 호출 정보 생성
- [x] 결과→원인→해결→실측→추가 결함 순서와 간결한 시각 자료 규칙 작성
- [x] 공식 skill validator·메타데이터·요구사항 검토

기존 JobQueue 보고서와 개선 코드는 수정하지 않는다. 새 스킬은 저장소 복제본 없이 사용자 공용 경로에 설치하며 커밋/push는 요청받지 않았다.
`%USERPROFILE%/.codex/skills/write-engineering-report/SKILL.md`와 agents/openai.yaml을 생성했다. 별도 형식 지정이 없으면 HTML을 기본으로 하며 결과 카드·짧은 원인/해결 코드 또는 그림·실측 비교·추가 대응 표를 사용한다.
장문 원문 복붙을 피하고 실측/가설·실패/미측정·악화 지표를 구분한다. 기존 보고서 자동 덮어쓰기나 추가 부하 검사/구현 변경을 보고서 작성 범위로 확대하지 않는다.
공식 quick_validate PASS, UI YAML/설명 길이/호출명·UTF-8·기본 자동 발견 설정을 확인했다. 실행 스크립트/템플릿은 추가하지 않아 서버 빌드나 성능 측정은 다시 실행하지 않았다.

## 74. 텔리메트리 선행 커밋 분리·개선 HTML 보고서

- [x] 계측/측정 도구·시나리오·기준 문서와 FIFO/전송 배치/매치 전달 개선을 index에서 분리
- [x] 분리한 계측 snapshot의 독립 빌드·회귀 및 개선 소스 복원 확인
- [x] 개선 실측 HTML·재생성 스크립트 작성·수치/링크·화면/상호작용 검증
- [x] staged diff·검사·정확한 메시지 미리보기와 명시적 승인 후 계측만 커밋 (e41ae162)

사용자는 텔리메트리 관측 코드/문서를 먼저 커밋하고 실제 개선 작업을 남기도록 요청했다. push 요청은 없다.
같은 파일의 혼합 변경은 별도 index blob으로 분리하며 전체 작업 트리 원본은 보존한다. Resource include·FIFO/ready·배치·매치 전달·해당 회귀/문서/HTML은 미커밋 개선 범위다.
별도 Serena 변경은 이번 계측 커밋에 넣지 않고 보존한다. commit 스킬의 정확한 snapshot 미리보기/승인 절차를 따른다.
분리한 계측만으로 Release Core164/ABI·서버 빌드 PASS. 혼합 C++ 7파일의 전체 작업 트리 bytes를 복원한 뒤 개선 Core199/ABI·서버 Release 빌드 PASS, Python29 PASS.
HTML은 Markdown 전체11표·6개 본문 절과 64명 지표6개를 포함한다. 실제 브라우저에서6개 선택/수치·1265px 가로 overflow0·console 오류/경고0 및 화면을 점검했다.
보고서/재생성 스크립트/개선 구조도와 73번 기록은 개선 변경으로 남긴다. 커밋 메시지와 staged fingerprint는 Git 제외 commit-split 경로에 보관하고 승인 뒤 일치 여부를 확인한다.
사용자 승인 후 텔리메트리만 e41ae162로 커밋했다. 당시 push는 수행하지 않았으며, 현재 개선의 커밋·push·main 병합은 80번에서 진행한다.

## 73. 로비 패킷 기아·다중 매치 응답 누락 원인 확인과 JobQueue 개선

- [x] 기준 소스/바이너리 보존·로비 및 게임 64명 실패 경로 진단·원인 재현
- [x] 원인별 최소 수정과 FIFO/공정성·직렬 실행·재등록·종료 계약 회귀 추가
- [x] Debug/Release Core/ABI·서버/실제 네트워크·Server Lab 검증
- [x] 동일 4/16/64명·seed 1~5·5초/20초/250ms·입장 제한 해제·계측 켬/끔 재측정
- [x] 실패 포함 전후 비교·계측/환경/소스 식별값·Archify·문서 기록

72번 기준 결과와 기존 미커밋 변경을 보존한다. 로비의 대상 우선순위·작업 우선순위 기아와 게임의 입력/상태 갱신/응답 수신 경로를 분리해 조사한다.
시나리오·판정·timeout을 완화해 결과를 통과시키지 않는다. 확인된 원인과 수정의 영향은 별도로 비교하며 커밋/push는 요청받지 않았다.

진단: 로비의 작업/대상 포인터 우선순위와 즉시 자기 재등록이 패킷을 굶겼다. 게임은 모든 매치 입력/좌표가 갱신됐으나 단일 프레임마다 worker 완료를 기다리는 전송에 수십만 context가 누적됐다.
FIFO 단독과 TCP_NODELAY 단독은 게임 64명 실패가 남고, 대기 프레임 묶음 전송 후 전체 이동/정지가 통과했다. SC_MATCH 직후 Disconnect로 전달이 취소되는 준비 EOF도 별도로 수정했다.
작업/ready FIFO·예산 뒤 대상 순환, 최대64 대기 프레임 배치와 부분 전송/오류/취소 개별 회수, API/결과 전송 대기·native 호출 관측을 추가했다.
Debug/Release 서버·Core199/ABI110종106상수·네트워크12종/보스4·5/초기화 실패4종·Python29·Server Lab API7/export 실패·정상 종료·에이전트 환경227소스 검증을 통과했다.

완료: 개선 후 계측 켬/끔 각30회가 모두 측정·기능 통과했다(기준18/60→개선60/60). 준비 EOF는 5→0회이며 모든 전후120회의 종료 자원/더미 연결 0을 확인했다.
계측 켬 로비64명 중앙값은 packets/s 16.899→250.702, 패킷 대기 p99 bucket 상한 33.554→2.097초, 측정 종료 미실행 패킷 5,769→228개다. 목표256/s와 대기0을 완전히 달성한 것은 아니다.
게임64명은 4명씩16매치이며 입력 처리율은 약256/s로 유지됐다. pending I/O 표본 peak 중앙값은 484,206.5→2,434, private bytes peak는 278.660→33.754MiB다.
게임64명 CPU 중앙값은 48.263→49.375%, tick 지연 p99는 131.072ms로 같아 CPU/tick까지 개선됐다고 해석하지 않는다.
원시120회 재계산, 개선 후 런타임/실행 파일37개·보존 기준36개·콘텐츠36개 hash 대조를 통과했다. 실패 실행도 비교에 보존하고 준비 실패는 별도로 구분했다.
같은 Metrics/벤치마크의 Concurrency.h만 바꾼 큐 단위 전후175회씩을 완료했다. 단일 생산자·32큐·burst는 개선됐고 일부 다중 생산자/소비자 경합은 저하돼 상세 표에 기록했다.
기존 큐의 FIFO 회귀 실패를 보존했고 FIFO 원본 복원 뒤 Debug/Release Core199/ABI를 다시 통과했다. 구조도9/9·4크기 containment·light/dark PNG 직접 점검·hash도 통과했다.
최종 UTF-8 56파일·Python AST8·프로젝트 XML2·문서 링크114·JS2·PowerShell4·diff 검사를 통과했다. 1265px 웹 화면의 전송 대기 표시·가로 overflow0·console 오류/경고0을 확인하고 검증용 서버/monitor/임시 탭을 종료했다.

후속 측정 과제: 로비 재등록 주기/예산·단일 content worker와 view/지형 비용, 게임 AI/충돌/전송 세부 시간·15ms 초과, 전체 출력 대기량 제한/backpressure를 각각 측정한다.
정상 처리 대상을 늘린 뒤 jobs/s·이동 간격을 기존 기아 상태와 단순 비교하지 않는다. 장시간/포화 요청률·매치 나이 동기화 시나리오는 별도 조건으로 추가한다.
[원인·전후 실측](../docs/development/JOBQUEUE_IMPROVEMENT.md)·[개선 흐름도](../docs/diagrams/jobqueue-improvement/README.md).


## 72. 실제 서버 JobQueue 최적화 전 기준 측정 (2026-10-11)

- [x] 로비/게임 입장 제어·실제 실행 경로·누락 지표 조사
- [x] LOCAL_TEST 측정용 입장 설정·더미 상한 해제, 작업 대기·tick 지연·패킷 처리 계측 추가
- [x] 실제 서버 로비 이동/정지·다중 매치 시나리오와 반복 측정/원시 결과 도구 추가
- [x] Release 부하별 반복 측정·실패/기아·CPU/부하 생성기 포화·종료 자원 확인
- [x] Core/ABI·Server Lab 회귀·실측 문서·Archify·작업 기록 정리

운영 JobQueue 자료구조·정책·worker 수·이동 재등록 방식은 최적화하지 않는다. 실제 IOCP/콘텐츠 경로를 유지한다.
사용자 승인에 따라 측정용 소프트 인원 제한을 해제한다. 매치 4인 규칙과 고정 저장 용량의 범위 검사는 유지하고 설정/용량을 결과에 기록한다.
71번의 기준 측정 및 기존 Serena 미커밋 변경을 보존한다. 커밋/push 요청은 없다.

완료: perf-lobby/perf-match와 Measure-ServerPerformance.ps1을 추가했다. 4/16/64명×5회×계측 켬/끔=60회를 실행해 55회 측정 구간·18회 기능 통과·42회 기능 실패를 보존했다.
양쪽 서버의 종료 pending=0 sockets=0 leased=0과 더미 연결 0은 60회 모두 확인했다. 계측 켬의 측정 구간 job 실패·I/O 오류 증가분은 0이었다.
로비 처리 packets/s 중앙값은 9.08/18.62/16.90, CPU는 16.63/16.21/17.65%였다. 미실행 패킷 대기의 전체 최대는 24.425초, 로비 15회 모두 이동/정지 실패였다.
게임 처리 packets/s는 16.02/64.03/255.55, CPU는 7.79/30.98/48.26%, tick 지연 p99 bucket 상한 중앙값은 32.768/32.768/131.072ms였다.
게임 64명은 측정 가능한 계측 켬 4회·끔 3회 모두 60/64명 이동 응답 누락, 준비 EOF는 켬 2회·끔 3회였다. 원인을 확정하거나 실패를 재시도로 대체하지 않았다.
정확한 조건·분포·CPU 대조·한계는 docs/development/SERVER_BASELINE.md에, 원시/요약/식별값은 artifacts/logs/server-baseline/20261011-final-on 및 final-off에 보존했다.
런타임 32개·데이터 36개 SHA-256 동일, 계측 끔 wrapper 단일 옵션 배열 수정과 당시 wrapper 사본을 분리 보존했다. 60행 원본과 요약을 재계산해 확인했다.
Release 서버 빌드, Core 162·ABI 110종/106상수(Debug/Release), Python 29, 실제 API 7회귀·export 실패 주입·종료 회수 PASS. 기존 C4065/C4101/C4244/C4305/C4309 경고 유지.
브라우저의 4인 성능 28판정·연결 0, 그래프 9개·대기 카드·1280px overflow 0·console 오류/경고 0 PASS. 검증용 프로세스는 종료했다.
Archify showcase 9/9·4크기 containment·light/dark PNG 직접 점검·hash, 정적 환경 227소스·PowerShell/Python/JS·UTF-8·링크·diff 검사 PASS.
Serena CNetworkMgr/TimerThread symbol 조회는 timeout되어 rg로 확인했다. 최대 TPS·장시간·전체 전투/GUI·운영 DB/블록체인·최적화/전후 비교는 미검증이다.

## 71. JobQueue 최적화 브랜치·현재 처리량·개선 방향 검토 (2026-10-11)

- [x] codex/jobqueue-optimization 브랜치 생성·기존 Serena 미커밋 변경 보존
- [x] 현재 JobQueue/Scheduler 실행 의미와 기준 소스 hash 고정·측정 도구 추가
- [x] Release 반복 측정: 정책·생산자/실행자·사전 할당 조건·혼합 작업·계측 비용·대기/공정성
- [x] 실제 결과와 개선 우선순위·계약/회귀 조건 문서화
- [x] Core/ABI·측정 결과 무결성·문서/프로젝트/UTF-8 검사

사용자 요청 범위는 브랜치 생성·현재 처리량 확인·개선 방향 검토다. 이번에는 기준 측정을 우선하고 운영 JobQueue 알고리즘은 변경하지 않는다.
새 측정 도구는 실제 Core 큐/스케줄러를 사용한다. 큐 단위 처리량과 서버 전체 처리량을 구분하며, 원시 결과는 Git 제외 artifacts/logs에 보관한다.

완료: Measure-JobQueue.ps1/Python과 ServerCore.Tests의 --jobqueue-benchmark 모드를 추가했다. 테스트 프로젝트 Release에 /O2를 명시했고 운영 큐/서버 빌드 설정은 유지했다.
백만 작업·35조건·5회=175회 PASS. 최종 결과는 artifacts/logs/jobqueue-baseline/20261011-101649에 있다.
단일 생산자/큐·계측 끔 중앙값은 Five 3.932M jobs/s, Snapshot 4.228M jobs/s다. 32 독립 큐는 21.929M/23.725M jobs/s이며 실제 ready Scheduler를 통하지 않는 단위 측정이다.
높은 주소 대상의 자기 재등록 100만 작업이 완료될 때까지 다른 대상이 실행되지 않은 현상을 10/10회 재현했다. 대기 중앙값은 Five 81.216ms/Snapshot 111.164ms다.
32 큐의 JobMetrics 켬 처리량 중앙값은 끔보다 약 50.5%/53.1% 감소했다. Scheduler 공정성→이동 주기→큐 경합→계측 비용→할당 측정 순서로 검토안을 작성했다.
상세 결과·편차·측정 경계·회귀 계약은 docs/development/JOBQUEUE_BASELINE.md, Archify JSON/HTML과 근거는 docs/diagrams/jobqueue-baseline에 기록했다.

실제 검증: Core 155검사·Protocol 110종/106상수 ABI 일치(Debug/Release), 175행/35조건/5회 무결성·요약 재계산·소스/실행 파일 SHA-256 일치 PASS.
잘못된 CLI 인자 5종 거부·상세 1,000작업 smoke PASS. UTF-8·Python AST·PowerShell 구문·문서 링크·git diff --check PASS.
반복 도구에 timeout/자식 실패를 주입한 35조건에서 전부 실패를 원시/요약에 남기고 최종 exit 1을 반환했다. 실제 성능 175회와 별도 진단이다.
신규 C++ clang-format dry-run·프로젝트/filter XML·신규 소스/도구/보고서 후행 공백 검사 PASS.
Test-AgentEnvironment.ps1 -StaticOnly PASS(새 벤치마크 포함 227소스). Archify showcase 9/9·composition 오류/경고 0·4크기 containment PASS, 최소/최대 light/dark PNG 4장 직접 점검.
최종 측정 빌드에서 C++ 경고는 없었으며 Git의 기존 LF→CRLF 안내는 남는다. 초기 준비 실행의 실패/조건 수정 로그는 최종 결과와 구분한다.
71번의 미검증 범위: 실제 서버 TPS/네트워크 부하·운영 DB/블록체인·개선 후 비교. 할당 횟수/bytes·최고 메모리·잠금별 대기·server tick은 이 큐 단위 계측에 없다. 이후 실제 서버의 제한 해제 부하·tick·CPU 기준은 72번에서 측정했다.
기존 Serena 변경은 보존했으며 커밋·push하지 않았다.

## 70. 다른 로컬 프로젝트에 Serena HTTP 확장 (2026-10-11)

- [x] Codex 로컬 프로젝트와 기존 Serena 프로젝트의 지침·설정·언어·포트 조사
- [x] 사용자 공용 실행 도구 설치·프로젝트별 HTTP URL/포트·활성 프로젝트 설정
- [x] 프로젝트별 로그인 자동 시작·다중 연결/재연결·프로젝트 전환 거부 검증
- [x] 포트 배정표·복구 방법·실제 검증과 미검증 범위 기록

대상은 NewWod(9120 유지), vafk_client(9121), vafk_server(9122), RunningApp(9123), WebPopup(9124), MazeRouge(9125), TradeApp(9126), TradeWebsite(9127), VampireAFK(9128)다.
기존 MCP의 다른 서버와 사용자 변경은 보존한다. 저장된 로컬 프로젝트/기존 Serena 등록에 없는 과거 작업 폴더와 원본 war_of_dimension은 수정하지 않는다.

완료: 사용자 공용 `%USERPROFILE%/.codex/tools/serena-http/`에 실행 도구·context·프로젝트 manifest·관리 안내·검사기를 설치했다.
9개 프로젝트의 Codex Serena 설정을 독립 HTTP URL로 전환하고 명시적인 프로젝트 경로로 서버를 실행했다. 모두 현재 실행 중이며 숨김 로그인 시작 바로가기를 등록했다.
기존 설정은 공용 도구의 `backups/20261011-092729/`에 보관했다. 기존 다른 MCP 설정과 Serena 프로젝트 언어 목록은 유지했다.
새 프로젝트 RunningApp은 설치된 Flutter와 같은 Dart SDK 3.13.4를 지정했고 WebPopup·MazeRouge·TradeApp·TradeWebsite는 실제 소스 언어로 구성했다.
NewWod의 로컬 실행 도구에도 동일한 초기화 개선을 반영했고 공용 도구는 NewWod 경로에 의존하지 않는다.

실제 검증: 9개 서버 각각 독립 세션 2개·올바른 활성 프로젝트·프로젝트 전환 거부·전체 연결 종료 후 재연결 PASS.
언어 설정이 있는 7개 프로젝트의 실제 소스 심볼 조회와 재연결 뒤 조회 PASS. 기존 languages: []인 vafk_server·VampireAFK는 활성 프로젝트와 세션 수명만 검증했다.
9개 프로젝트의 실제 작업 디렉터리에서 Codex CLI mcp get으로 streamable_http/전용 URL을 확인했다. 기존 전역 service_tier 문제는 조회에만 fast를 지정해 우회했으며 전역 설정은 변경하지 않았다.
9개 Startup 바로가기의 경로·포트·숨김 실행과 listener를 확인했다. WebPopup·TradeWebsite 바로가기를 동시에 실행해 두 서버의 준비 완료와 언어 서버 초기화 직렬화를 확인했다.
초기 동시 시작에서 발견한 TypeScript 언어 서버 패키지 설치 충돌을 공용 초기화 mutex와 언어 서버 준비 후 HTTP listener 개방으로 수정했다. 수정 후 실제 심볼 검사를 통과했다.
9개 서버의 순차 시작을 고려해 공용 초기화 순서 대기는 최대 15분, 개별 서버 준비는 기존 최대 90초로 설정했다.
Test-AgentEnvironment.ps1 -StaticOnly -SerenaHttp PASS(226소스 compilation database·NewWod HTTP 심볼 검사).
Archify showcase 9/9·composition 오류/경고 0·4크기 containment PASS, 최소/최대 크기의 light/dark PNG 4장 직접 점검. 현재 hash/byte는 구조도 README에 기록했다.
로그·검사 결과·백업은 사용자 공용 도구 또는 Git 제외 runtime/artifacts에 보관한다.
최종 UTF-8·Python AST·9개 프로젝트 TOML/YAML·PowerShell 구문·수정 범위 git diff --check PASS. Git 저장소가 아닌 TradeApp·TradeWebsite는 diff 검사 대상에서 제외했다.

기존 Unity C# 의존성/MSBuild 경고와 Dart 알림 미지원 경고는 남아 있으며 위 심볼 조회 성공과 구분한다.
실제 Windows 로그아웃/로그인과 Codex UI 새 채팅 연결·장시간 부하·게임/웹 서비스 빌드는 미검증이다. 기존 STDIO 채팅은 재연결 또는 새 채팅부터 URL 설정이 적용된다.
위 확장 적용 당시에는 커밋·push하지 않았다. 이후 잔여 변경의 검토·커밋 준비는 81번에 기록한다.

## 67. 프로젝트별 Serena HTTP 서버 재사용 (2026-10-11)

- [x] 프로젝트별 포트·활성 프로젝트를 고정하는 숨김 실행과 중복 시작 방지 구현
- [x] Codex HTTP 연결 전환·Windows 로그인 자동 시작·종료/복구 안내
- [x] 여러 MCP 세션의 동일 서버/활성 프로젝트·실패 경로·환경 검사 검증
- [x] 에이전트 환경 문서·Archify 구조도·실제 검증 결과 기록

기존 사용자 변경을 유지한다. NewWod의 Codex 설정만 전환하고 다른 저장소·Claude/Gemini 설정은 수정하지 않는다.
다른 프로젝트도 별도 포트와 명시적인 --project로 실행해 활성 프로젝트가 서로 바뀌지 않게 한다.

완료: NewWod Codex는 `127.0.0.1:9120/mcp`에 연결한다. `.serena/codex-http.yml`에서 activate_project를 제외한다.
Serena.ps1은 숨김 시작·중복 재사용·상태·소유한 프로세스 트리 종료·프로젝트별 Startup 바로가기 등록/해제를 제공한다.
현재 사용자 로그인 자동 시작을 등록했고 바로가기를 실제 실행해 서버 시작/HTTP 도구 호출을 확인했다. 서버를 실행 상태로 유지했다.
Serena 1.2.0 기본 HTTP의 세션 종료가 공유 agent/clangd를 정리하는 동작을 실제 재현해 serena_http.py에서 서버 수명으로 분리했다.
설치 패키지와 전역 Codex 설정은 변경하지 않았다. 내부 _set_mcp_tools 의존은 패키지 갱신 후 회귀 검사 대상이다.

실제 검증: 독립 MCP 세션 ID 2개·같은 활성 war_of_dimension·activate_project 미노출 및 직접 호출 거부·전체 연결 종료 후 재연결/실제 CServer 심볼 조회 PASS.
동일 PID 재사용·다른 프로젝트의 동일 포트 사용 거부·시작 시각 불일치 PID 종료 거부·소유 서버 트리 13개 종료·Startup 바로가기 재시작 PASS.
실행 기록의 JSON 날짜 자동 변환으로 생긴 비교 실패를 UTC ticks 비교로 수정했다. 최종 검사에서 같은 PID 재사용을 확인했다.
Test-AgentEnvironment.ps1 -StaticOnly -SerenaHttp PASS(226소스 compilation database 생성·경로 검증 및 HTTP 검사).
Codex CLI의 mcp get으로 streamable_http와 URL을 확인했다. 설치 CLI가 기존 전역 service_tier=default를 해석하지 못해 해당 조회에만 -c service_tier=fast를 사용했다. 설정 파일은 수정하지 않았다.
Archify 2.16 showcase 9/9·composition 오류/경고 0·4크기 containment PASS. 최소/최대 크기 light/dark PNG 4장 직접 점검했다.
JSON/HTML hash·byte와 재생성 방법은 구조도 README에 기록했고 자동 visualReview=pending과 별도 수동 점검을 구분한다.
로그·실행 기록·화면/자동 검사 receipt는 Git 제외 .runtime과 artifacts/logs/serena-http에 보관한다.
UTF-8·PowerShell/Python/TOML/YAML 구문·git diff --check PASS. Git의 기존 LF→CRLF 안내는 오류가 아니다.

기존 STDIO 채팅은 재연결 또는 새 채팅에서 새 설정을 읽어야 한다. 기존 채팅의 프로세스를 강제로 종료하지 않았다.
실제 Windows 로그아웃/로그인·Codex UI 새 채팅의 연결은 미검증이며 독립 MCP 클라이언트와 로그인 바로가기 실행으로 검사했다.
다른 실제 저장소의 추가 서버 등록·장시간 부하·게임 실행/빌드는 이번 변경 범위가 아니다. 커밋·push하지 않았다.

사용자 요청 순서대로 진행한다. 1~5는 초기 이전·개발 환경·커밋의 기록이고, 6은 현재 통합 솔루션·메시 공유 구현이다. 초기 커밋은 사용자 요청으로 GitHub에 업로드했으며 현재 구현의 추가 커밋·push는 별도 요청에 따라 진행한다.

## 1. 로컬 실행 설정

- [x] 기존 의존성 TBB 4.2.3.1 복원과 핵심 모듈 Debug x64 빌드 확인
- [x] 누락된 실행 리소스 확인 및 복원
- [x] 빌드·시작·중지·실행 점검 스크립트 구성
- [x] 로비·게임 서버 연결과 클라이언트 시작 확인

## 2. 에이전트 개발 환경

- [x] 다른 프로젝트의 공통 에이전트 지침·문서 인덱스·도구 설정 비교
- [x] AGENTS.md와 Claude/Gemini 진입점, 작업·문서 지침 구성
- [x] 로컬 코드 탐색 도구와 검증 명령 설정 및 점검

## 3. Visual Studio 2026

- [x] 프로젝트·솔루션을 v145/VS18 기준으로 명시적으로 이전
- [x] x64 Debug/Release 빌드 및 마이그레이션 후 실행 점검
- [x] 변경·검증·기존 경고 문서화

## 4. 파일 정리

- [x] 프로젝트 참조·런타임 사용·원본 복구 가능성을 근거로 파일 분류
- [x] 개인 IDE 파일·기존 빌드 산출물·중복 백업 등 확인된 불필요 파일 제거
- [x] 필요한 모델·텍스처·외부 라이브러리·실험 코드 보존 여부 기록
- [x] 정리 후 빌드·실행 점검
- [x] 최종 Git/LFS 및 초기 커밋 후보 확인

## 현재 확인

- 설치된 Visual Studio는 2026(18.10.12217.157), C++ toolset은 v145다. 초기 기준선 확인 후 프로젝트·솔루션을 이전했고 현재 빌드는 프로젝트의 v145 설정을 직접 사용한다.
- 원본은 `LOCAL_TEST`가 이미 켜져 있고 `WITH_DATABASE`는 꺼져 있다. 우선 원본의 DB 없는 로컬 개발 모드로 실행한다. 실제 DB 연동과 운영 배포 검증은 이 단계에 포함되지 않는다.
- TBB는 최초 기준선 복원에 사용했으나 실제 소스·링크·런타임에서 미사용임을 확인하여 정리했다. 현재 빌드는 TBB 패키지 복원 없이 통과한다.
- 정리 후 `scripts/Build.ps1 -Configuration Debug -Rebuild`, `scripts/Build.ps1 -Configuration Release -Rebuild`: 핵심 3개 모듈 각각 PASS. 루트 `NewWod.sln` 직접 Debug x64 빌드도 PASS.
- 정리 후 `scripts/Test-Local.ps1 -Configuration Debug`, `scripts/Test-Local.ps1 -Configuration Release`: listener, 게임 서버가 로비에 연결한 TCP 세션, 클라이언트 게임 창 확인 PASS. 점검 프로세스는 모두 중지했다.
- `scripts/Test-AgentEnvironment.ps1`: 220개 C++ 소스 compilation database와 경로 검사 PASS, Serena 핵심 3개 파일 심볼 조회 PASS, Archify doctor PASS. 현재 에이전트 세션의 MCP 재연결은 별도다.
- 핵심 비 UTF-8 텍스트 20개를 UTF-8로 변환했다. 실제 문자열과 게임 로직은 유지한다. 기존 narrowing·signedness 등 빌드 경고는 후속 리팩토링 대상으로 기록했다.
- 로컬 런타임 다이어그램의 schema·deliver 검증과 light/dark 화면 크기별 overflow 점검 PASS. 1440×900 light, 2048×1320 dark 결과를 직접 확인했다. 의존성 정리는 연결 구조를 바꾸지 않아 검증된 JSON/HTML을 재생성하지 않았다.
- 불필요 바이너리·압축 백업·개인 설정·빌드 캐시 20개 항목, 148.7MiB를 저장소 밖 `C:\GitFolder\NewWodCleanupBackup\20261004`에 보관했다. 필요한 HeightMesh를 복원하고 모델·텍스처·FMOD·외부 헤더·실험 코드와 자료는 보존했다.
- 최종 초기 커밋 후보는 17,500개 파일이다. 일반 Git blob 중 100MiB 초과 없음, LFS 11개 pointer·실제 에셋 SHA-256/크기·로컬 object 존재 확인 PASS. 생성 로그·캐시·compilation database와 정리 대상은 제외했다. 당시 원본 checkout은 clean이었고 새 저장소 main의 초기 커밋·remote·GitHub 업로드 전이었다. 후속 완료는 5번에 기록했다.
- `.gitignore`의 모델 `.obj` 예외를 일반 `*.obj` 규칙 뒤에 두어 신규 HeightMesh와 향후 필수 리소스가 실제로 추적되도록 확인했다. 설정·문서·스크립트 whitespace와 PowerShell 구문, 인코딩 변환 20개 파일의 원문 문자열 일치 확인 PASS.

## 5. 초기 커밋

- [x] 사용자 요청에 따라 기존 최신 코드·필수 에셋·개발 환경 전체를 초기 커밋 범위로 준비
- [x] 정확한 메시지·staged snapshot 미리보기 후 사용자 승인
- [x] 승인된 snapshot으로 초기 커밋 생성 및 작업 트리 확인

후속 사용자 요청으로 초기 커밋 `ed3428e`와 GitHub 공개 저장소 `Kimseongtae9911/war_of_dimension`의 main push를 완료했다. LFS 11개 원격 객체 및 로컬·원격 HEAD 일치, clean 작업 트리를 확인했다. 아래 snapshot 공백 검사는 초기 이전 당시의 기록이다.

초기 snapshot 전체의 `git diff --cached --check`는 원본·복사된 Archify 코드 6,286개 파일의 기존 공백 문제 80,500건으로 exit 2다. 원본 또는 복사된 패키지 외의 경로에서는 문제가 없고, 새 개발 환경·문서·스크립트 범위 검사는 PASS다. 초기 이전에서 원본·vendor 코드 전체를 공백 정규화하지 않는다. 결과는 Git에서 제외되는 `artifacts/logs/initial-commit-whitespace.txt`에 남겼다. 커밋 작성자 설정과 일반 staged blob의 100MiB 제한도 확인했다.

## 6. 통합 솔루션 및 메시 공유

- [x] 루트 통합 솔루션을 기본 빌드로 사용하고 서버 전용 solution filter 구성
- [x] 이름과 transform을 제외한 geometry 내용 기반 캐시 및 공유 소유권 구현
- [x] 스킨드 geometry 공유와 본·애니메이션 상태의 분리, UI/파티클 독립 유지
- [x] D3D12 공유·변경 데이터·device 경계·해제 순서 및 실제 에셋 중복 검증
- [x] 통합 Debug/Release 빌드·로컬 실행·Serena와 구조 문서 점검

기존 루트 솔루션에는 이미 핵심 3개 프로젝트가 포함되어 있었다. 개별 솔루션을 호출하던 빌드 경로를 루트로 통합하고 Servers·Client·Development 폴더, 서버 filter, 공유 실행 프로필을 추가했다. 파일·에셋의 디스크 위치와 개별 솔루션은 유지한다. Visual Studio 프로필 GUI 실행은 직접 점검하지 않았으며 서버 준비를 보장하는 Start-Local 절차를 함께 문서화했다.

프레임마다 geometry를 재생성하던 로더를 device·SHA-256·내용 길이 기반 캐시로 변경했다. weak_ptr 캐시, 객체의 공유 소유권, atomic 참조 수, 스킨드 base alias 소유권으로 해제 순서를 처리한다. 본 index/weight·bind-pose·프레임 연결은 공유하지 않는다. 메시 이름만 해시에서 제외하며 transform·material은 기존 프레임 상태를 유지한다. GPU 업로드 fence 동기화는 호출자가 담당한다.

- `Build.ps1 -Configuration Debug`, `Build.ps1 -Configuration Release`: 루트 솔루션의 핵심 3개 모듈 PASS. `Build.ps1 -Configuration Debug -Module Servers`: 서버 filter PASS.
- `Test-MeshSharing.ps1` Debug·Release: 실제 D3D12 WARP·하드웨어에서 12개 검사 PASS. created 8, reused 4, 종료 live 0. 동시 최초 로딩·자기 핸들 재지정·마지막 소유자·만료 후 재생성 포함.
- `Test-MeshSharing.ps1 -AuditAssets` Debug·Release: 실제 base geometry GPU 생성/재사용 PASS. LobbyScene_No 291→38개, Plane1 5,185→78개, ModularModel 724→698개. ModularModel의 이름이 다른 중복 26개 확인. 전체 장면 렌더링·FPS·실측 VRAM은 측정하지 않았다.
- `Test-Local.ps1` Debug·Release: listener·서버 간 연결·클라이언트 창 시작 PASS, 테스트 프로세스 중지. DB·로그인·전투 전체는 미검증이다.
- `Test-AgentEnvironment.ps1`: 222개 소스 compilation database·경로, Serena Game 5/Lobby 4/Client 18개 심볼, Archify doctor PASS.
- 메시 구조도: architecture showcase 9/9, 오류·경고 0, deliver PASS. 4개 화면 크기의 overflow 검사 PASS, 1440×900 light·2048×1320 dark 직접 시각 점검 PASS. 네트워크 연결은 변하지 않아 기존 local-runtime 구성도는 유지했다. 해시·재생성 근거는 `docs/diagrams/mesh-sharing/README.md`에 있다.
- 최종 PowerShell 9개 구문·프로필/filter JSON 경로·클라이언트 프로젝트 소스 존재·신규 파일 UTF-8/공백·변경 diff 공백 검사 PASS. HEAD와 origin/main은 초기 커밋 그대로이며 실행 중인 검증 프로세스는 0개다. 기존 빌드 경고는 이번 범위에서 일괄 수정하지 않았다.

이 단계는 로컬에서 검토한 뒤 후속 요청으로 7번과 함께 커밋·push를 완료했다.

## 7. 통합 솔루션 SLNX 전환

- [x] 공식 도구로 루트 NewWod.sln을 NewWod.slnx로 변환
- [x] 서버 filter와 빌드·실행 프로필·개발 문서 참조 갱신
- [x] SLNX 통합 Debug/Release 및 서버 filter 빌드, 설정 보존 확인
- [x] 변경된 snapshot으로 커밋 미리보기 갱신

사용자의 커밋·push 요청 이후 미리보기 승인을 기다리던 중 SLNX 전환 요청을 반영한다. 이전 미리보기의 staged tree는 더 이상 최종 snapshot이 아니다. 기존 메시 공유·네트워크·자원 수명 구조는 그대로라 검증된 다이어그램 JSON·HTML은 재생성하지 않는다.

`dotnet sln NewWod.sln migrate`로 변환한 뒤 `Build.ps1 -Configuration Debug`, `Build.ps1 -Configuration Release`, `Build.ps1 -Configuration Debug -Module Servers` PASS. 서버 filter 로그에 클라이언트가 포함되지 않는 것을 확인했다. XML의 프로젝트 3개·x64 플랫폼·개발 파일 경로를 기존 솔루션과 비교하고 launch JSON 내용 보존을 확인했다. 기존 루트 `.sln`은 `artifacts/logs/NewWod-before-slnx.sln`에 로컬 백업하고 제거했다. launch 파일은 같은 내용의 `.slnxLaunch`로 이전했다. 개별 모듈 솔루션은 보존한다. Visual Studio GUI의 프로필 실행은 여전히 미검증이다.

후속 사용자 승인으로 6~7은 커밋 bf7b4cee와 GitHub main push를 완료했다.

## 8. 클라이언트 메모리 분석 및 포트폴리오 기록

- [x] 인게임 실제 로딩 경로·영웅 모델/애니메이션/컨트롤러 소유권 확인
- [x] 에셋 CPU 배열·D3D12 버퍼 정렬·애니메이션 메모리 재현 계산 도구 구성
- [x] 개선 전 로딩 방식과 현재 방식의 동일 시나리오 메모리 측정
- [x] 영웅 1개 모델과 인게임 전체 결과, 절감 범위·남은 원인·재현 절차 문서화

개선 전 참조는 초기 커밋 ed3428e, 메시 공유 적용 기준은 bf7b4cee다. 전체 프로세스 실측·GPU segment usage·계산한 배열/버퍼 용량·직렬화 크기를 구분한다. 측정용 분기는 명시적 CLI 프로세스에서만 활성화하며 일반 실행 동작은 유지한다. 과거 로더를 현재 바이너리에 재현한 A/B 측정은 과거 commit의 바이너리 자체 측정과 구분해 표시한다.

2026-10-05 KST, RTX 4070 SUPER의 Release x64에서 각 3개 독립 프로세스로 A/B 측정 완료. 실제 Title→INGAME 자원 생성·GPU 완료 대기·기존 upload 해제 경로를 사용하고 네트워크/로딩 렌더 스레드/전투 프레임은 제외했다. 진입 시 private commit 중앙값 6,951.54→5,768.31MiB(17.02%), Working Set 2,867.15→2,105.12MiB(26.58%), DXGI LOCAL 4,069.55→3,662.88MiB(9.99%). 지표를 합산하지 않는다.

실제 모델 16종/18회 로딩을 끝까지 파싱했다. ModularModel 1회의 애니메이션 행렬은 6,947 keys×788 frames×64bytes=334.12MiB이며 이번 변경으로 줄지 않는다. 현재 영웅 슬롯의 model hierarchy는 2회 로딩, 컨트롤러는 local 중복 슬롯을 포함해 4개다. 첫 로딩 CPU 배열 계산은 377.64→376.99MiB, 두 번째 영웅 모델 로딩 구간의 private commit 증가량은 560.71→426.53MiB다. 화면의 영웅 수로 독립 모델 비용을 곱하지 않는다.

Release/Debug 클라이언트 빌드, 두 구성의 MeshSharing 12개 검사·실제 3개 모델 GPU 감사, 로컬 listener·서버 연결·클라이언트 창 시작 PASS. 빌드의 기존 narrowing/signedness 경고는 유지하며 전체 전투·FPS·장시간 누수·다른 boss·로그인에서 이어진 진입은 미검증이다. Serena Object.cpp 심볼 조회 PASS(194개), compilation database 223개 소스. 측정은 공유 구조를 바꾸지 않아 기존 Archify 구조도를 재생성하지 않았다.

문서: `docs/portfolio/CLIENT_MEMORY_OPTIMIZATION.md`. 공개용 수치 근거 JSON/CSV와 PNG/SVG 2종을 작성했고, 문제·선택한 설계·영웅 비용·재현 절차·후속 우선순위를 기록했다. 그래프는 matplotlib 3.11.2로 생성 후 직접 시각 점검했다. 일반 raw 로그와 패키지는 artifacts 아래에 있어 Git에서 제외된다. 이번 계측·문서 변경은 별도 요청 전까지 커밋·push하지 않는다.

최종 검증: 6회 완료/16종/18회 일치, Python 계산의 Plane1·ModularModel 레코드/unique 수와 네이티브 Release GPU 감사 일치, 영웅 행렬 공식·측정 소스 SHA-256 일치, parser의 잘린 token/음수 count 거부 PASS. 잘못된 측정 CLI mode는 exit 2로 종료 PASS. PowerShell 전체 구문, 프로젝트/filter 신규 파일 등록, 신규 텍스트 UTF-8·공백·크기, 포트폴리오 상대 링크, 변경 diff 공백 PASS. 실행 중인 검증 프로세스 0개다.

후속 맵 로딩 질문에 따라 동일 문서에 자원 목록을 추가했다. Plane1은 전체 hierarchy 5,594 frames/5,185 mesh records, 고유 geometry 78개, map DDS 2종의 픽셀 payload 합 8MiB다. object/material CB는 공유하지 않으며 작은 요청이 다수 committed buffer로 분리되는 비용을 구분해 기록했다. Scene 초기화는 미니언 12개·몬스터 7종/9개·boss·스킬 풀·파티클 texture 15종을 함께 만든다. Minotaur texture 픽셀 payload 240MiB, 파티클 객체당 300,000×32×2=18.31MiB 대형 DEFAULT payload를 확인했다. 실제 전체 파티클 수와 비용별 residency는 아직 계측하지 않았다. 기존 A/B raw checkpoint의 맵 로딩 구간 증가량만 추가 분석하고 기존 측정 근거는 변경하지 않았다. Serena Scene.cpp 79개 심볼 조회 PASS. 게임 로직 변경이 없는 문서/에셋 분석이므로 빌드·실행을 반복하지 않는다.

## 9. 현재 클라이언트 메모리의 구간별 분해

- [x] 맵 전체 생성·파티클 풀·캐릭터·UI/공통 자원의 checkpoint 추가
- [x] 실제 파티클 개수·버퍼 descriptor/할당 정보 수집 및 현재 Release 독립 프로세스 3회 측정
- [x] 구간 증가량의 합계/종료 시 감소와 실제 Working Set·DXGI 차이를 검증
- [x] 기존 근거를 보존하고 비용 분해·맵의 기여·최적화 우선순위 문서화

앞선 동일 시나리오의 5,768MiB private commit(약 5.63GiB)을 기준으로 비용을 분해한다. 사용자가 확인한 별도 UI의 메모리 항목을 직접 읽은 것은 아니다. 이 단계는 비용 분석이며 리소스 용량/로딩 정책 최적화는 적용하지 않는다. loading render thread/네트워크를 제외한 동일 시나리오에서 구간을 세분화하고 계측 분기 외 일반 동작은 유지한다.

Release 독립 프로세스 3회 평균 private commit 5,766.11MiB/Working Set 2,104.25MiB/LOCAL 3,662.88MiB. 파티클 131개(전체 스킬 풀 100+선택 효과 12+환경 19)의 대형 buffer payload 2,398.68MiB, 실제 descriptor의 할당 정보 합 2,407.13MiB. 파티클 구간 private commit 2,420.57MiB(41.98%)/LOCAL 2,408.15MiB/Working Set 7.60MiB를 확인했다. 현재 맵 전체 구간 142.32MiB(2.47%), 과거 동일 범위 비교 중앙값은 1,449.34→142.00MiB다. 8번 에셋 분석에서 미계측이던 파티클 수를 이번에 확정했다.

영웅 구간 private commit 1,088.22MiB/Working Set 998.02MiB 및 애니메이션 행렬 2회 중복 668.24MiB를 확인했다. NPC 구간 948.46MiB, 공통 자원 631.60MiB, UI/dissolve 300.97MiB. 기존 upload 해제 순회의 전용 컨테이너 누락을 후속 후보로 기록했다. 8192² shadow map은 논리 texel 용량 256MiB로 구분했다. 비용 구간 차이를 최종 retained heap 소유권이나 CPU/GPU 합산으로 표현하지 않는다.

Release/Debug 클라이언트 빌드, 양 구성의 MeshSharing 12개 검사·실제 3개 에셋 GPU 감사·일반 로컬 시작 PASS. Serena Scene.cpp 79개 심볼 PASS. 수집값의 byte 단위 합계, 3회 파티클 count/용량/stride 일치, 현재 소스 9개·보관 과거 소스 11개 및 실행 파일 SHA-256 일치, 보고서 재생성 일치, 입력 실패 경로 4종 PASS. 그래프 직접 점검 완료. 기존 A/B 근거를 보존하고 별도 JSON/CSV/PNG/SVG 및 `docs/portfolio/CLIENT_MEMORY_BREAKDOWN.md`로 기록했다. 계측은 자원 소유권·로딩 순서를 바꾸지 않아 기존 검증된 Archify 구조도를 재생성하지 않는다. 커밋·push는 진행하지 않았다.

## 10. 메시 공유 전후의 구간별 메모리 비교

- [x] 동일 세부 checkpoint 바이너리에서 legacy/shared 각 3회 교대 측정
- [x] 구간별 private commit·Working Set·DXGI 전후 통계 및 합계 검증
- [x] 기존 근거를 보존하고 비교 JSON/CSV·그래프·포트폴리오 문서 추가

사용자의 후속 요청에 따라 현재 바이너리의 기존 legacy geometry 분기로 메시 공유 전 로딩 방식을 재현한다. 과거 commit 실행 파일 자체를 측정했다고 표현하지 않는다. 네트워크·전투 제외 조건과 모델/파티클 시나리오를 같게 유지하고 새로운 최적화는 적용하지 않는다. 기존 현재 모드 3회와 별도 날짜의 과거 3회를 섞지 않고 이번에는 동일 바이너리로 양 모드를 교대 실행한다.

2026-10-05 KST, Release RTX 4070 SUPER에서 legacy/shared 각 3회 교대 실행 완료. 구간 private commit 평균은 파티클 2,419.96→2,422.04MiB, 맵 1,448.13→142.62MiB, 영웅 1,223.24→1,089.20MiB, NPC 949.93→950.18MiB였다. 파티클 131개·용량·stride·대형 buffer Width 및 할당 정보는 양 모드가 같았다. GPU 제출·대기·기존 upload 해제 구간 private commit −263.63→0.25MiB까지 포함해 최종 평균 6,941.19→5,770.25MiB, 순감소 1,170.94MiB(16.87%)를 확인했다. Working Set 2,859.86→2,104.74MiB(26.40%), LOCAL 4,069.55→3,662.88MiB(9.99%). 생성 구간 감소량을 최종 retained 비용 절감으로 단정하지 않는다.

`Build-ClientMemoryBreakdown.py --compare`로 모드별 3회 통계·구간 차이·최종 차이·JSON/CSV 및 PNG/SVG를 생성했다. 기존 최초 A/B와 현재 전용 근거는 보존했다. 실행 파일 SHA-256 cffa1503… 및 계측 소스 9개 일치, 6회 완료/교대 순서, 각 지표 byte 단위 합계/종료 값, 비교 차이 합계, 파티클 동일 시나리오 PASS. 보고서/전체 CLI 재생성 JSON 일치·CSV 11행, 비교 실패 입력 7종 및 충돌 summary CLI 거부 PASS. 기존 단일 모드 보고서 재생성과 과거 보관 소스 11개/실행 파일 해시 검증도 PASS. 상대 링크·UTF-8·공백·크기·그래프 직접 점검 PASS, 잔여 검증 프로세스 0개. 직전 검증과 같은 C++/바이너리이므로 게임 빌드는 반복하지 않았다. 자원 소유권·흐름 변경이 없어 기존 Archify 구조도를 유지한다. 포트폴리오 분석/인덱스와 개발 환경 재현 절차를 갱신했고 커밋·push는 하지 않았다.

## 11. 인게임 확정 외형으로 영웅 선택 파츠 로딩

- [x] 커스터마이징 수신 완료·인게임 진입 스냅샷 및 기본 외형 덮어쓰기 확인/수정
- [x] 선택 파츠의 geometry/skin/material만 생성하고 본·애니메이션 매핑 보존
- [x] 실제 에셋·남녀/서로 다른 외형·fallback·Debug/Release 검증 및 메모리 재측정
- [x] 변경 구조·포트폴리오 기록과 실제 인게임 렌더링 스크린샷 제공

로비/READY 편집 경로는 전체 모델을 유지한다. 인게임의 OtherClient 모델 계층은 기존 공유 구조이므로 참가 영웅의 파츠 합집합, 로컬 모델은 자신의 확정 파츠를 사용한다. 수신 미완료는 전체 로딩 fallback으로 안전하게 처리한다. 최초 A/B 실행 파일/계측 소스를 보존한 뒤 변경하고 기존 메모리 수치를 새 구현 결과로 재해석하지 않는다.

전체 788개 프레임·61개 clip·6,947개 keyframe을 보존하고 미선택 메시/skin/material 생성을 제외했다. 각 keyframe에서는 선택 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관한다. DDS 중복 참조의 소유 파츠가 빠진 경우 실제 DDS로 복구한다. 모델 생성 전/자기 자신 add packet도 기록하고 외형 스냅샷은 매치 중 불변이다. 인게임 기본 외형 덮어쓰기·F7 변경을 막았고 원본 `.bin`/프로토콜 형식은 유지했다.

실제 서버 연결 테스트 중 발견한 로비 `Job::Execute`의 실행 누락, 큐의 shared Job 소유권/API와 callable 제약, 테스트 전환의 배열 delete[]/슬롯 용량, Title의 ID 확정 전 READY 전환을 복구했다. Debug 감사의 정상 Direct2D 종료를 추가했다. Debug/Release 전체 빌드, HeroSelection 12개 검사와 원본 대비 행렬 1,576,969개 byte 일치, MeshSharing 12개 검사·실제 3종 GPU 감사, 일반 로컬 기동, 분할 패킷의 실제 로비→게임 전환 응답 PASS. compilation database 225개·Serena·에이전트 환경 PASS. 기존 signedness/narrowing 및 D3D12 initial state 경고는 남는다.

2026-10-05 Release RTX 4070 SUPER의 동일 최종 실행 파일에서 full/selected 각 3회 독립 프로세스 교대 측정 완료. 영웅 생성 구간 private commit 1,123.27→215.92MiB, 행렬 668.24→96.25MiB(85.60% 감소), 전체 진입 private commit 5,833.01→4,928.64MiB(904.36MiB/15.50% 감소), Working Set 2,133.17→1,291.35MiB, LOCAL 3,694.88→3,632.02MiB. 2회 모델 로딩의 스킨드 메시 1,440→91, 애니메이션 열 1,576→227. 파티클 131개는 동일하며 약 2.4GiB 구간은 남는다. Private/Working Set/DXGI를 합산하지 않는다. 외형 fixture·DDS 복구를 포함한 동일 바이너리 비교이므로 이전 geometry 공유 결과의 절대값과 직접 이어 붙이지 않는다.

최종 소스 18개와 실행 파일 SHA-256 `70f6b983225c74afe946e2fceac16f4596ee00482d0795ccb746768aed7d46f8` 일치·reference 사본 보존, 구간/차이의 byte 합계·파티클/모델 통계·JSON/CSV 재생성·잘못된 입력 8종 거부 PASS. 이전 메모리 근거 재생성과 보관 해시도 확인했다. `docs/portfolio/HERO_SELECTED_PARTS.md`·근거 JSON/CSV·PNG/SVG·Archify JSON/HTML을 작성했다. Archify showcase 9/9, 오류/경고 0, 네 해상도 및 light/dark 자동 검사와 직접 화면 점검 PASS.

DB 없는 실제 로컬 서버를 통한 Title 4번 영웅 테스트에서 TCP 8911 연결과 영웅·무기·맵·미니맵·스킬 UI 렌더링을 확인해 1922×1112 실제 창을 `docs/portfolio/evidence/figures/hero-selected-parts-ingame.jpg`로 보존했다. 캡처의 실제 JPEG 형식에 맞춰 확장자를 정리했으며 픽셀 데이터는 변환하지 않았다. 서버의 기존 CoolTime 0/MP Consumption 0 진단은 남고 전체 전투·네 사용자·DB/블록체인·매치 재진입은 미검증이다. 원본 단일 모델 파일 파싱 및 타인 모델의 기존 공유 계층 구조도 남는다. 개발/측정 프로세스는 중지했고 커밋·push는 하지 않았다.

## 12. 선택 외형의 애니메이션 변환 행렬 용어 정리

- [x] “선택한 외형 파츠와 공통 본·부모 프레임의 변환 행렬만 보관”으로 설명 통일
- [x] clip·keyframe·프레임 노드·행렬 보관 대상의 정의 및 기존/현재 구조 명시
- [x] 모든 61개 clip과 6,947개 keyframe 유지, 선택 스킬별 clip 로딩 미적용 명시
- [x] 구조도 JSON/HTML 문구 동기화, 문서 링크·UTF-8·공백 및 구조도 검증

사용자의 용어 정리 요청에 따라 포트폴리오·아키텍처·구조도 설명을 수정한다. 외형 번호 스냅샷과 선택 조건 포인터 전달, 파일 파싱과 보관 메모리 감소를 구분한다. 구현과 측정 근거는 변경하지 않아 게임 빌드·실행을 반복하지 않는다. 구조도의 모호한 “애니메이션 축소”는 “필요 행렬 보관”으로 수정하고 Archify 재검증 후 HTML을 갱신한다. 커밋·push는 진행하지 않는다.

검증 결과: 로더의 `keptFrameIndices`·행렬 복사·대상 프레임 포인터 매핑과 설명을 대조했다. Archify showcase validate/deliver 9/9·오류/경고 0, 네 해상도의 자동 검사 PASS. 새 HTML의 1440×900 light 및 2048×1320 dark를 직접 열어 레이블·카드 잘림/겹침이 없음을 확인했다. 문서 상대 링크·UTF-8·공백, JSON/HTML 전달 해시 PASS. 구현과 공개 측정 근거는 보존했다.

## 13. 메모리 분석·선택 외형 최적화 커밋 준비

- [x] 완료한 구현·계측·검증·포트폴리오 변경 및 신규 파일 검토
- [x] 해당 파일만 stage하고 검증·snapshot fingerprint·한글 메시지 미리보기 준비
- [x] 공통 commit 스킬에 따른 미리보기 승인 후 커밋 및 요청된 GitHub push

사용자의 커밋·push 요청에 따라 8~12번의 완료 작업을 함께 준비한다. 저장소는 `C:/GitFolder/NewWod`, 브랜치는 `main`, 원격은 `Kimseongtae9911/war_of_dimension`이다. `NewWod.slnx`의 로컬 `GitFolder/NewWod`에 의존하는 프로젝트 경로 변경은 이번 작업에서 제외하고 작업 트리에 보존한다. 실제 Debug/Release 빌드·GPU/행렬 검사·로컬 전환 검증과 현재 소스/바이너리 해시를 확인한 후 준비한 snapshot을 미리보기로 제시한다. 공통 스킬의 승인 전에는 커밋·push하지 않는다.

준비 검증: 관련 파일 59개만 stage했고 신규 파일은 최대 약 692KiB로 신규 대용량 LFS 등록 대상은 없다. 기존 메모리 근거와 영웅 전후 비교 재생성·원시 합계·실패 입력·현재 소스 18개 및 실행 파일 해시 확인 PASS. staged Python/JSON 구문·PowerShell 구문·UTF-8·문서 링크·신규 파일 및 변경 줄 공백 검사 PASS. 전체 기존 소스 줄까지 공백 검사를 확장했을 때 기존 GameFramework.cpp 공백이 감지되어 신규/변경 줄로 범위를 맞췄으며 기존 줄은 정규화하지 않았다. Archify 9/9·오류/경고 0 및 앞선 직접 시각 점검 결과를 확인했다. 원격 main을 fetch하여 HEAD와 일치함을 확인했다. 미리보기 승인 대상에서 제외한 변경은 `NewWod.slnx` 하나이며 신규 미추적 파일은 없다.

후속 미리보기 승인으로 커밋 ee60ac16과 GitHub main push를 완료했다. 제외한 `NewWod.slnx` 변경은 보존한다.

## 14. 선택 스킬에 필요한 파티클 버퍼 생성

- [x] 참가자 전체 선택 스킬·기본 공격·후속 스킬·타워 효과의 생성 의존 관계 확인
- [x] 선택 스킬 스냅샷과 필요한 종류의 파티클 풀만 생성, 수신 미완료 fallback 및 수명 보존
- [x] 실제 GPU 버퍼·스킬 의존 관계·실패 경로와 Debug/Release 검증
- [x] 동일 바이너리 전체/선택 풀 비교, 포트폴리오와 Archify 흐름도 기록

자신뿐 아니라 네 참가자의 선택 스킬 효과를 포함한다. 기본 공격·타워·환경 효과는 유지하며 기존 파티클 용량과 스킬 객체 풀 크기는 이번 범위에서 바꾸지 않는다. 서버가 자동 선택 결과보다 먼저 게임 시작을 전송하는 순서를 점검하고 선택 완료 정보로 인게임 자원을 생성하도록 한다. 기존 공개 측정 근거와 reference 소스/실행 파일을 보존하며 새로운 비교는 별도 시나리오/근거 파일에 기록한다. 원본 저장소와 제외된 로컬 SLNX 변경은 수정하지 않는다. 커밋·push는 별도 요청 전까지 진행하지 않는다.


검증 결과: Debug/Release 전체 빌드와 선택 파티클 검사 PASS. 종류별 100→45, 타인·기본 공격·타워·BigBang 후속·Archer 타이머·Programmer·중복 선택과 16개 미완료 슬롯, 잘못된 범위·스냅샷 고정·초기화 검사 PASS. 실제 D3D12 인게임 자원 생성·제출·대기 후 파티클 객체/텍스처/정보의 반복 해제도 두 구성에서 PASS. 서버의 보스 자동 선택 직업 4/5 정규화와 CS 20..39→SC 21..40 응답을 복구하고, 실제 네 TCP 참가자의 Ogre/Programmer Debug/Release 네 조합에서 수동 선택 보존·자동 선택 범위·궁극기·선택 16개가 게임 시작 전에 도착함·분할 READY header를 검증했다. wire 상수·field·packing은 유지한다.

최종 Release 실행 파일 SHA-256 b955f4289cf5caec4969decf374e3d7c82d7cd16ab460f8c7bd1e8829a0b96a0, 계측 소스 24개와 reference 사본 일치 PASS. 네 참가자의 고정 스킬 fixture와 geometry 공유·영웅 선택 외형을 양쪽에서 유지한 동일 바이너리 각 3회 교대 측정에서 파티클 127→72, 두 대형 버퍼 payload 2,325.44→1,318.36MiB, D3D12 allocation 2,333.625→1,323.000MiB였다. 최종 Private commit 4,857.28→3,837.25MiB(1,020.02MiB/21.00% 감소), Working Set 1,292.15→1,287.61MiB, DXGI LOCAL 3,558.49→2,547.44MiB. 지표는 합산하지 않는다. 이전 131개 시나리오와의 절대값 직접 비교도 하지 않는다. 원시 실행 근거·JSON/CSV·PNG/SVG와 선택 규칙/용어를 `docs/portfolio/PARTICLE_SELECTED_SKILLS.md`에 정리했다. byte 합계·모델 통계·반복 횟수·재생성·잘못된 측정 입력 9종 거부 PASS. 소스/바이너리는 `artifacts/logs/particle-selected-skills-verified`에 보존한다.

Release 기존 영웅 행렬 1,576,969개 일치·메시 공유 12개와 실제 3종 에셋 감사, Debug 일반 로컬 서버 연결·게임 창 기동 PASS. compilation database 227개·Serena 신규 선택 파일 색인·에이전트 환경 PASS. Archify showcase 9/9·오류/경고 0·네 해상도 자동 검사와 light/dark 직접 점검 PASS. 기존 signedness/narrowing·swprintf·D3D12 initial state 경고는 남는다. 모든 스킬의 전투 시각 검사·전체 매치 재진입·DB/블록체인은 미검증이고 기존 보스 일반 스킬 자동 선택의 중복 문제도 남는다. 스킬 객체 풀·원본 에셋·300,000개 용량은 유지한다. 실행 프로세스는 중지했고 로컬 SLNX 변경을 보존했으며 커밋·push는 하지 않았다.

## 15. 종류별 동시 효과용 파티클 5개의 필요성 검토

- [x] 서버 할당·재사용·패킷 ID와 클라이언트 버퍼 인덱싱 대조
- [x] 20종의 풀/시전자/고정 ID 분류와 다중 시전자·고갈 위험 확인
- [x] 실제 측정과 예상 절감량을 구분해 포트폴리오·정적 근거·인덱스 기록
- [x] 소스 해시·분류/계산·UTF-8·문서 링크·변경 줄 공백 검증

검토 요청에 따라 구현은 변경하지 않았다. 기존 5는 MAX_SKILL_OBJECT의 매치·종류별 서버 객체 풀 상한이며 실제 최적 동시 효과 수의 실측 근거는 없다. 14종은 첫 비활성 0~4번을 할당하므로 선택한 영웅 수로 클라이언트 벡터를 줄이면 중첩 투사체·높은 ID의 효과를 누락시킬 수 있다. StormArrow/BigBang 후속은 영웅 0~2, SCL은 단일 보스 0, 타워 투사체 풀은 PATH_NUM 4다. ArrowRain/DarknessRay는 모든 시전자에 0을 사용해 덮어쓰기·서로 다른 종료 시점의 표시 충돌 위험이 있다. 14종 풀 고갈 시 return 0과 호출부 -1 실패 검사 불일치도 확인했다.

현재 측정 fixture에서는 타워의 5번째 객체 한 개가 확실한 미사용 후보다. 적용 시 72→71개, 두 대형 DEFAULT allocation 18.375MiB 감소를 예상하지만 미구현·미측정이다. 20종의 현재 ID 주소 범위 합계 83은 기존 0번 충돌을 보존한 정적 계산이므로 정상 다중 영웅 권장 용량으로 제시하지 않는다. 5개 논리 슬롯을 유지하면서 실제 사용 시 GPU 버퍼를 생성하고 전투 활성 최대치·최대 ID·생존 시간·고갈 횟수를 계측하는 후속 개선을 제안했다.

검증: compile_commands.json 227개·Serena 상태를 확인하고 CGameMgr.cpp 37개 심볼 색인 PASS. 20종/14종 할당 자동 대조 및 현재 소스 10개 SHA-256, 기존 계측 소스 24개·Release 실행 파일 해시 보존, JSON 계산·구문·UTF-8·상대 링크·공백 검사 PASS. 기존 선택 흐름도를 현재 코드와 대조했고 구조·흐름 변경이 없어 Archify JSON/HTML 재생성은 하지 않았다. 실행 코드 변경이 없어 빌드·게임 실행을 반복하지 않았고 실제 전투 최대 중첩·최초 사용 생성 성능은 미검증이다. 로컬 SLNX 변경을 보존했으며 커밋·push는 하지 않았다.

## 16. 동일 파티클 자원의 공유 가능 범위 설명

- [x] CParticleObject의 공통 텍스처·난수·Shader 참조와 개별 CParticleMesh 생성 확인
- [x] Stream Output/Draw 상태 갱신·버퍼 교환·출력 카운터를 대조해 공유 제약 설명
- [x] 공통 버퍼 영역 할당·비활성 버퍼 재사용·동일 진행 상태의 다중 렌더링을 미구현 제안으로 기록

동일 종류의 정의·텍스처·Shader는 공유할 수 있고 현재 텍스처·난수 텍스처·Shader도 공유한다. 큰 두 버퍼는 효과별 입자 상태이므로 독립 효과끼리 같은 주소를 사용하면 상태 갱신·카운터·재시작 처리가 섞인다. 공통 공간의 별도 영역과 GPU 완료 후 재사용은 가능하지만 독립 입자 총 상태량은 남는다. 각 효과의 300,000개 예약을 그대로 두는 버퍼 통합만으로 큰 절감을 주장하지 않는다. 동일 시점·로컬 움직임을 재생하는 제한된 효과의 계산 결과 공유는 별도 Shader/렌더링 설계로 구분했다. 기존 풀 검토 문서에 설명을 추가했고 구현·패킷·측정 근거·Archify 산출물은 변경하지 않았다. UTF-8·링크·변경 줄 공백 확인 PASS. 실행 코드 변경이 없어 빌드·게임 실행을 반복하지 않았으며 새 설계의 실전 성능은 미검증이다.

## 17. 파티클 재사용의 조건별 절감량과 용량 초과 처리 검토

- [x] 현재 72개 중 스킬 45개·슬롯/환경 27개를 구분해 조건별 allocation 계산
- [x] 효과 수 부족·개별 입자 용량 부족·예산 상한·GPU 반환 시점의 처리 제안 정리
- [x] 기존 포화 시 재시작·통계 경로와 공식 D3D12 수명·출력 통계 근거 대조
- [x] 예상 근거 JSON·문서 링크·UTF-8·계산·소스 해시·변경 줄 공백 검증

스킬 버퍼만 재사용하고 쌍당 300,000개 용량을 유지할 때, 풀 유지량 9/18/27/45쌍의 예상 전체 대형 allocation은 661.500/826.875/992.250/1,323.000MiB이며 절감은 661.500/496.125/330.750/0MiB다. 예산 안의 실제 보유 수에는 잔존 입자·예비·fence 대기를 포함하며 평균 활성 수와 구분한다. 실전 동시 사용량 미측정이므로 특정 수치를 기대 성과로 확정하지 않았다. CPU Private/Working Set/DXGI 감소량으로 대입하지 않는다. 조건·byte 계산을 particle-buffer-reuse-estimates-20261005.json에 보존했다.

범위 밖 쓰기 대신 예산 안의 새 버퍼/영역 확보와 상태 전환, GPU fence 완료 후 반환을 제안했다. 초과 시 장식 입자 생성량 제한·핵심 공격 범위 표시 유지와 기록, 임시 증설의 구/신 동시 비용·통계 사후 검출의 한계를 명시한다. 기존 ParticlePostRender의 MAX_PARTICLES 도달 시 재시작과 비활성 SO 통계 경로를 확인했다. Microsoft 공식 출력 통계·fence 수명 관리·명시적 할당 자료를 대조했다. 소스/측정/실행 파일은 변경하지 않았다. 실행 코드·구조도 변경이 없어 빌드·플레이 테스트·Archify 재생성은 반복하지 않았고 실제 재사용 성능·포화 시 시각 품질은 미검증이다. 구현·커밋·push는 하지 않았다.

## 18. 모든 파티클의 공용 GPU 버퍼 재사용·확장·종료 해제

- [x] 스킬·슬롯·환경 효과에 공용 풀 연결, 활성 효과별 독립 상태 유지
- [x] 빈 버퍼 재사용·부족 시 추가 할당·GPU fence 완료 후 반환
- [x] 개별 입자 용량 초과 검출·확장과 상태 복사, 실패 시 기존 상태 보존
- [x] 게임 종료·장면 전환·반복 해제와 실제 GPU 성장/재사용/격리 검사
- [x] Debug/Release 빌드·회귀·동일 조건 메모리 비교 및 포트폴리오·Archify 기록

사용자 요청에 따라 전체 파티클에 적용한다. 네트워크 ID와 효과별 시뮬레이션 상태는 유지하고, 같은 저장 형식의 두 대형 버퍼를 공용 풀에서 임대한다. 비활성 효과의 버퍼는 마지막 GPU fence 이후 재사용하며 부족하면 확장한다. 기존 측정 원본/reference는 보존하고 새 비교는 별도 파일에 기록한다. 로컬 SLNX 변경은 보존하며 커밋·push는 진행하지 않는다.

검증 결과: 스킬·타워·선택 슬롯·점프·코인·장벽의 다섯 생성 경로를 장면 공용 풀에 연결했다. 효과 객체/서버 ID는 유지하고 최초 표시 시 두 대형 버퍼를 임대한다. 비활성 효과를 먼저 반환하고 stride·용량이 맞는 GPU 완료 블록을 재사용한다. 없으면 추가 생성하며 개별 입자 포화는 다음 프레임에 더 큰 블록으로 유효 상태를 복사한다. 장면 전환·앱 종료 및 ReleaseParticles는 GPU 완료 후 임대를 반환하고 확장분·유휴분을 전부 해제한다. 이전 cap 도달 시 전체 재시작을 제거했다. 사후 통계이므로 초과 프레임의 미기록 입자는 복구하지 않는 한계를 명시했다.

Debug/Release 클라이언트 빌드·실제 WARP GPU 검사 PASS. 동시 임대/종류별 상태 격리, gate로 GPU를 멈춘 미완료 fence의 재사용 방지, 풀 2→3쌍 확장, 4→12개 입자 용량과 GPU 상태 보존, 새 종류 seed/counter 초기화, 두 번 생성·종료 후 잔여 쌍 0, 범위 초과 거부를 확인했다. GPU 오류 0·경고 각 12개이며 실제 장치 OOM은 유발하지 않았다. Debug 실제 게임 Shader의 고정 재생·반복 ReleaseParticles도 PASS. 선택 스킬 두 구성, Release 메시 공유 12개/실제 3종 에셋 및 영웅 행렬 1,576,969개, Debug 로컬 서버/TCP/게임 창 기동 PASS. compilation database 229개와 Serena 공용 풀 13개 심볼 색인·에이전트 환경 PASS.

Release 실행 파일 ced83cfa0ae04e77b58bffae130da6e716398f573ee06976f8a9123c8c557fac에서 dedicated/pooled 각 3회 교대 측정. 합성 활성 순서 4→19→8→0→18을 각 2프레임 재생해 논리 객체 72개 유지, 실제 대형 버퍼 72→19쌍·누적 재사용 30회, allocation 1,323.000→349.125MiB(973.875MiB/73.61% 감소), Private commit 3,838.65→2,778.36MiB(1,060.29MiB/27.62%), Working Set 1,289.75→1,289.69MiB, DXGI LOCAL 2,548.22→1,574.34MiB를 확인했다. 실제 전투 최대치로 해석하거나 다른 메모리 지표를 합산하지 않는다. 계측 소스 28개·추가 Shader/테스트 5개와 실행 파일 reference, 원시 실행·JSON/CSV·PNG/SVG를 보존했다. 결과 재생성·byte 합계·입력 변조 6종 거부·Debug/Release 현재 소스/실행 파일 해시 PASS.

포트폴리오 PARTICLE_BUFFER_POOL.md와 인덱스·현재 아키텍처·설정을 갱신하고 이전 수치/제안은 당시 기록으로 명시했다. 새 공용 풀 수명도와 선택 흐름도는 Archify 2.16 showcase 9/9·오류/경고 0, 네 해상도 자동 검사 및 두 크기 light/dark 직접 점검 PASS. JSON/HTML 해시·UTF-8·문서 상대 링크·PowerShell/Python 구문·공백 검사 PASS. 기존 빌드 경고와 GPU 경고는 남는다. 실전 장시간 전투·전체 매치 재진입·DB/블록체인 및 메모리 압박 시 품질 조절은 미검증이다. 기존 서버 효과 ID 충돌·풀 고갈 반환은 변경하지 않았다. 로컬 SLNX 변경을 보존했으며 커밋·push하지 않았다.

## 19. 파티클 최적화 커밋·push 준비

- [x] 선택 스킬 구성·공용 GPU 풀·검증·측정 문서의 완료 변경 검토
- [x] 현재 계측 소스와 Release 실행 파일 해시 및 공개 결과 재생성 확인
- [ ] 공통 commit 스킬의 미리보기 승인 후 커밋 및 GitHub push

사용자의 커밋·push 요청에 따라 14~18번 작업을 함께 준비한다. 저장소는 `C:/GitFolder/NewWod`, 브랜치는 `main`이다. 원격 fetch 후 HEAD와 origin/main 일치를 확인했다. 기존 `NewWod.slnx`의 로컬 프로젝트 경로 변경은 제외하고 작업 트리에 보존한다. 앞선 Debug/Release 빌드·GPU 수명·회귀 검증 결과를 확인했고 계측 소스 28개 및 Release 바이너리의 해시가 그대로임을 확인했다. GPU 오류 0·경고 각 12개와 실전 최대 동시 효과 수 미검증 범위는 유지한다. 정확한 staged snapshot과 한글 메시지를 제시한 뒤 승인받아 커밋·push한다.

준비 검증: 작업 파일 56개를 명시적으로 stage했다. 공개 측정 JSON/CSV 재생성·소스/실행 파일 해시·구조도 전달 해시·UTF-8·JSON/Python/PowerShell 구문 검사 PASS. staged 공백 검사에서 새 matplotlib SVG의 경로 줄 끝 공백을 발견해 정리했으며 XML 요소·속성·텍스트의 의미가 동일함을 확인했다. 실행 소스와 측정값은 변경하지 않았다. 제외된 변경은 NewWod.slnx 하나다.

## 20. 공용 풀 적용 후 클라이언트 메모리 구성 비율

- [x] 최신 pooled 3회 원시 checkpoint와 현재 소스 28개·Release 실행 파일 해시 확인
- [x] Private commit 구간별 순증가 비율 및 로딩 완료/고정 재생 후 총량 집계
- [x] 포트폴리오·JSON/CSV·PNG/SVG·재생성 스크립트 기록 및 이전 분석의 시점 명시
- [x] 구간/지표 byte 합계·비율 합계·잘못된 입력 4종·그림 직접 점검

커밋 38bb9ee7이 GitHub main에 반영된 이후 구성 비율을 요청받았다. 기존 최종 측정의 pooled 3회 자료를 재집계했으며 재실행/실전 최대치 측정으로 주장하지 않는다. Private 2,778.36MiB 중 미니언·몬스터·보스 34.16%, 공통 자원 22.71%, UI 10.72%, 파티클 고정 재생 9.71%, 영웅 7.74%, 하늘 6.92%, 맵 6.37%다. 파티클 준비/객체/재생 합계는 10.38%이고 별도 GPU allocation과 합산하지 않는다. 순증가량을 최종 자원 소유량으로 해석하지 않는다. 현재 런타임 소스 변경이 없어 기존 검증을 유지하고 빌드·전투 실행은 반복하지 않았다. NewWod.slnx를 보존하며 이번 집계는 커밋·push하지 않았다.

## 21. 미니언·몬스터 자원 공통화 가능성 검토

- [x] compilation database와 Serena 상태 확인, 모델 로딩·인스턴스·애니메이션 소유권 조사
- [x] 최신 3회 실측의 미니언/몬스터/보스 구간 및 에셋·DDS 비용 분해
- [x] 기존 공유 범위·추가 공통화·용량 감소 후보와 예상 효과/위험 문서화
- [x] 근거 해시·계산·문서 링크 및 기존 Archify 구조도와 설명 대조

사용자의 검토 요청으로 구현은 변경하지 않는다. 미니언 반복 생성과 몬스터 동일 모델 반복 사용, 종류 간 동일 DDS와 geometry, 개별 pose/track 및 GPU 상수 버퍼의 경계를 확인한다. 실제 현재 자원 공유와 설계 제안을 구분하며 기존 최종 측정·reference 및 로컬 SLNX 변경을 보존한다.

검토 결과: 모델은 미니언 1회→12개, 일반 몬스터 7종 1회씩→9개(Chest/Beholder 각 2개)로 이미 공유한다. 최신 pooled 3회 실측 구간을 미니언 Private 28.55MiB·일반 몬스터 836.86MiB·Ogre 83.63MiB로 나누었다. 9개 모델과 DDS 25개를 파싱·해시 확인했고 geometry 26개는 모두 다른 내용이다. 일반 몬스터 clip 행렬은 16.20MiB, 비압축 텍스처 payload는 400MiB이며 Minotaur 240MiB/로더 Private 증가 488.69MiB가 큰 비용이다.

NPC upload 해제 순회 누락(일반 몬스터 texture upload 후보 400MiB), 모델 사이 동일 DDS 2장의 중복 8MiB, 300개 미니언 object CB의 arena 예약량 18.75→0.125MiB, 공유 material CB 반복 생성의 포인터 덮어쓰기를 확인했다. Minotaur의 동일 해상도 BC7 단일 mip 예시는 240→60MiB이며 아직 에셋 변경·품질/감소 실측은 하지 않았다. 후보의 메모리 지표·중첩·GPU 완료 조건을 문서화하고 공유 clip과 개별 pose를 분리하는 방향을 제안했다. compilation database 229개·Serena Object.cpp 194 심볼, 기존 Archify showcase 9/9·오류/경고 0, 현재 소스/에셋/원시 근거 해시·계산·UTF-8·문서 링크 검사 PASS. 실행 소스·에셋·구조도가 그대로여서 게임 빌드/전투 실행·HTML 재생성은 반복하지 않았다. 커밋·push하지 않았다.

## 22. 모델 DDS 공용 자원 적용 및 upload·압축·상수 버퍼 설명

- [x] 모델 material의 DDS resource를 device·정규화 경로·용도별로 공유하고 binding은 개별 유지
- [x] GPU 초기 업로드 순서·반복 해제·마지막 소유자·잘못된 DDS 및 실제 Chest/Beholder 검사
- [x] Debug/Release 빌드·관련 회귀·공용 자원 수명과 실제 Shader 고정 재생 검증
- [x] 업로드 해제 누락 범위·BC7 및 상수 buffer arena 용어, 문서·Archify JSON/HTML 기록

사용자는 검토 2번의 구현을 요청했고 1·3·4번은 설명을 요청했다. 조기 upload 해제 순회·에셋 압축·상수 arena는 이번에 적용하지 않는다. 모델 DDS의 공유 자원과 wrapper/binding 수명을 분리하고 기존 측정 근거·reference·로컬 SLNX 변경을 보존한다.

적용 결과: SharedDdsTexture가 device·정규 경로·resource 용도별 DEFAULT/UPLOAD를 공유하고 weak cache로 마지막 소유자 수명을 유지한다. material별 wrapper/binding 저장은 독립, 같은 Scene SRV 힙의 읽기 전용 descriptor는 재사용하며 힙 교체 시 캐시를 비운다. CTexture 복사의 소유권·binding과 남은 upload 종료 회수를 보완했다. NPC 조기 해제 순회·압축·arena는 미적용이다.

검증: Debug/Release Client 빌드 및 DDS 12개 범주 PASS. 실제 Chest/Beholder 4개 texture 참조→2개 GPU resource, DEFAULT/UPLOAD 각각 8MiB 중복 제거, GPU 첫 mip pixel readback 일치·upload 반복 해제·새 Scene 힙·복사 wrapper 마지막 소유자·live DDS=0·GPU errors=0. 기존 모델 UPLOAD 초기 상태 경고 1328은 16개씩 기록했다. Release 메시 12개/실제 모델 3종 audit·영웅 행렬 1,576,969개 비교 PASS. 실제 Shader 고정 재생 1회와 particle 종료 PASS(Private 2,656.47MiB, WS 1,229.85MiB, LOCAL 1,518.34MiB). 반복 A/B 성과로 사용하지 않는다. 에이전트 환경 231개 소스·Serena 3개 핵심 파일·Archify doctor PASS. DDS 구조도 showcase 9/9·오류/경고 0, 4해상도 containment 및 1440 light/2048 dark 직접 점검 PASS. 기존 signedness/narrowing 등 경고 유지. 실제 멀티플레이 전투·압축 화질·pose 분리·NPC 조기 upload 정리·arena는 미검증. 구현/근거 JSON·문서·해시/링크 검사 기록, 기존 baseline·로컬 SLNX 변경 보존. 커밋·push하지 않았다.

## 23. NPC 임시 upload 회수·텍스처 BC7·상수 버퍼 arena 적용

- [x] 실제 게임 Shader의 고정 카메라 몬스터 전후 캡처 경로와 변경 전 화면·메모리·소스·에셋 보존
- [x] GPU 완료 후 누락된 NPC/타인/스킬의 임시 upload 순회와 반복 해제 검증
- [x] NPC 객체 상수 버퍼를 frame별 독립 영역의 확장 가능한 arena에 배치, 공유 material CB 생성 멱등화
- [x] 몬스터 DDS를 해상도·채널 해석·색 공간 유지 BC7로 변환하고 동일 조건 후 화면·픽셀/품질·실측 비교
- [x] Debug/Release·GPU 수명/실패 경로·관련 회귀·로컬 실행 및 문서·근거·Archify JSON/HTML 기록

사용자가 1·3·4번의 작업과 3번 변경 전후 몬스터 스크린샷을 요청했다. 실제 에셋/Shader의 화면을 먼저 보존하고 동일 fixture로 비교한다. 기존 원본 저장소와 선행 작업의 기준선은 보존하며 커밋·push는 이번 요청 범위가 아니다.

적용 결과: GPU 초기 copy fence 이후 minion/monster/OtherClient/tower/skill/static skill model/particle texture를 순회해 임시 upload를 회수한다. 지속적으로 갱신하는 CB·particle counter는 유지한다. 공유 material CB 생성은 멱등으로 변경했다. 미니언 300·일반 몬스터 508개 논리 객체 CB를 2프레임 256byte draw 영역으로 통합하고 부족하면 64KiB 페이지를 확장한다. 기존 CB 예약 50.5→0.5MiB, 정상 종료·확장분 마지막 소유자 해제 후 page 0을 확인했다. Ogre·영웅 CB와 개별 pose/clip은 유지한다.

에셋: Microsoft DirectXTex may2026 고정 도구/SHA 검증, BC7 `-bc x`로 24 DDS를 동일 해상도/단일 mip/UNORM/채널 배치에 변환했다. 기존 R8 Metallic 1장은 압축 이득이 없어 원본 유지. 25개 고유 NPC DDS의 실제 DEFAULT allocation 441.25→113.3125MiB(327.9375MiB 감소), Minotaur 240→60MiB. DDS 해시·헤더·payload 검증 후 LFS 등록. GPU backbuffer 원본 PNG 9종 앞·뒤 전후 36장과 동일 카메라 근거를 공개 포트폴리오에 보존했다. 캡처 양쪽은 1·4번 적용 후라 BC7 시각 차이만 비교한다. 실제 클라이언트 Shader의 고정 idle 모델 화면이며 네트워크 매치/맵 UI 화면이 아니다. 전후 직접 점검, 25 DDS RGBA와 18 화면 foreground RGB MSE/PSNR/최대 오차 기록. 모든 조명·거리·애니메이션의 무손실 동일 화질로 주장하지 않는다.

Release 최종 EXE `64c50c4fa1617b904f4109088e3db8affc2be3ed32d9aada6c4d6fcb78dbda0e`, 비교 EXE `2622d1df0d6a5841e2238b777cbd46cb69d13a910bcfd97fe05a72c58f6ef356`. 동일 선택 외형/직업/스킬/pooled 합성 순서를 전후 각각 3회 순차 측정했다. 마지막 ingame_ready 중앙값 Private 2,656.35→1,964.46MiB(691.89MiB/26.05% 감소), WS 1,233.63→897.06MiB, LOCAL 1,518.3438→1,190.4062MiB. 파티클 19쌍/재사용 30회 양쪽 일치, 공유 DDS upload 28개/125.3125MiB가 복사 완료 후 0개/0byte. 전체 UPLOAD 0을 의미하지 않고 지표·중복 절감은 합산하지 않는다. 초기 5.8GiB·공용 풀 2,778MiB·DDS 기능 검사 1회 근거는 당시 자료로 유지한다.

검증: Debug/Release Client 빌드·arena native 10범주·DDS native 12범주 PASS. 600 draw 성장·GPU readback 값 보존·프레임 격리·미완료 gate fence 대기·잘못된 입력/상태 거부·scope 복구·material 1회 생성·종료 page/resource 0 확인. 실제 모델 렌더/반복 upload 정리/OnDestroy 두 구성 PASS. 기존 UI 종료의 중복 ReleaseWrappedResources를 제거하고 Flush 후 GPU 완료를 기다려 device removed 오류를 보완했다. 최종 Debug 초기화 GPU 오류 0·기존 state ignored 1328 경고 1,511개, 캡처/종료 오류·경고 0, removedReason 0. DDS native 기존 1328 경고 16개씩과 particle pool 기존 경고 12개, 기존 compiler 경고는 유지한다.

Release 메시 12범주·실제 모델 3종 audit, 영웅 행렬 1,576,969개 일치, 선택 파티클 및 공용 풀 성장/종료 회귀 PASS. Release 로컬 TCP 서버 연결·클라이언트 창 기동과 234개 소스 compilation database·Serena 3파일·Archify doctor PASS. 구조도 showcase 9/9·오류/경고 0 및 4해상도 containment, 1440 light/2048 dark 직접 점검 PASS. 구현/측정/변환/검증/품질/36 PNG 해시·JSON/CSV 집계·재현은 NPC_MEMORY_OPTIMIZATION.md에 기록했다. 실제 장치 OOM·장시간 네트워크 전투·전체 매치 재진입·운영 DB/블록체인은 미검증이다. 로컬 SLNX 변경과 원본 저장소를 보존했으며 커밋·push하지 않았다.

최종 문서·산출물 검사: Playwright/실제 Chrome에서 갤러리 9모델×2시점의 선택 18조합·전후 원본 PNG 36개 1920×1080 로드·링크 변경·페이지 오류 0·가로 overflow 0 PASS. 갤러리 실제 화면도 직접 점검했다. 문서 상대 링크 151개, JSON/Python/PowerShell 구문, 추가 소스·EXE·에셋·PNG SHA, 수정 C++ 19개 strict UTF-8, project XML/source 경로, Git diff 공백 검사 PASS. SLNX SHA f3447f2a3a56e5a04b4cae2d11f7e5ccef60bc0ee116713eed3c48156d4f2562 보존. 실행 프로세스는 중지했고 임시 검사/캡처 sidecar는 Git 제외를 확인했다.

## 24. DDS 공유·NPC 메모리 최적화 커밋·push 준비

- [x] 작업 범위·원격 main·최신 소스/실행 파일/에셋/스크린샷 해시·측정 근거 검토
- [x] 한글 커밋 메시지·정확한 staged snapshot 미리보기 준비, SLNX 기존 수정 제외
- [x] 공통 commit 스킬의 미리보기 승인 후 커밋 및 GitHub main push

사용자가 완료 작업의 커밋·push와 메모리 현황 표를 요청했다. 20~23번의 메모리 비율 분석·NPC 후보 검토·DDS 공유·임시 upload 회수·BC7·객체 arena·전후 화면/문서/검증을 함께 준비한다. 저장소는 C:/GitFolder/NewWod, 브랜치는 main이며 fetch 후 HEAD와 origin/main은 38bb9ee76645b9f7e6cea16cab37d4613c4bb005로 일치했다. 최신 소스/EXE/25 DDS/36 PNG 해시·집계와 151개 문서 링크 검사를 통과했고 직전 Debug/Release 빌드·native GPU·회귀·로컬 실행 결과를 확인했다. 기존 경고와 전체 네트워크 전투 미검증 범위는 유지한다. NewWod.slnx는 제외·보존하고, 실행 로그/바이너리/생성 DB/임시 검사는 Git에 포함하지 않는다. 메시지와 staged fingerprint를 제시한 뒤 승인받아 커밋·push한다.

Stage 검토에서 원본 R8 Metallic의 새 LFS 속성에 맞게 해당 파일만 --renormalize했다. 실제 DDS bytes/SHA는 06e84337b5412950b78efa4950466958376a75615d60e507fe3b7a0b0cac7110으로 그대로이고 Git index만 LFS pointer로 전환한다. 압축 24장과 원본 유지 1장 모두 staged LFS pointer·내용 해시/크기를 검증한다. 공개 측정 JSON/CSV 재생성·현재 소스/바이너리/에셋/PNG와 추가 소스 해시 PASS, staged 공백 검사 PASS. 기존 실행 검증 이후 기능 변경은 없으므로 게임 테스트를 반복하지 않는다.

NPC 수명도 v2 JSON은 검증/전달된 원본이 CRLF인 반면 Git의 기본 정규화가 LF로 바꾸는 것을 확인했다. 해당 JSON 하나에 -text를 지정해 frozen bytes와 receipt SHA-256을 그대로 stage한다. 원본 JSON/HTML은 수정하지 않았으며 두 DDS/NPC 구조도의 staged bytes가 전달 당시 SHA와 일치하는지 검사한다.

완료: 사용자 승인 후 076d2692cb843f6263425f66d108983e3bf7e3f9를 생성하고 GitHub main에 push했다. LFS 25개 업로드 및 ls-remote의 동일 commit을 확인했다. 기존 NewWod.slnx 변경은 제외·보존했다.

## 25. 기동·Title·렌더링·음원 메모리 구간 설명

- [x] 최신 공개 측정 3회의 초기화 checkpoint를 다시 집계하고 실제 생성 코드와 대조
- [x] Title 화면 자원과 공통 렌더링·음원·shadow map의 측정 범위를 구분

기존 632.22MiB 항목은 scene_setup_ready까지의 누적 Private Bytes다. 3회 평균 순증가를 나누면 framework 준비까지 121.3385MiB, Title/렌더링/UI 생성 127.4922MiB, FMOD 초기화·지정 Sound 디렉터리 로딩 125.8451MiB, 후속 BGM 재생/GPU 제출·정리/shadow map/장면 전환 준비 257.5456MiB다. 각 자원의 최종 보유량을 직접 계측한 값으로 해석하지 않는다. Title 화면은 배경 및 SignIn/SignUp DDS와 UI 객체를 생성하며 비용은 두 번째 구간에 포함된다. 마지막 구간의 주요 생성 자원은 8192×8192·32bit shadow map(논리 texel 256MiB)이며 Title 이미지 단독 비용으로 부르지 않는다. GameFramework.cpp, Scene.cpp, Shader.cpp, UILayer.cpp, SoundManager.cpp, ShadowMap.cpp/h와 measurements.json을 대조했다. 실행 코드 변경이 없어 빌드·게임 재실행은 반복하지 않았다.

## 26. 장면별 자원·음원 및 공통 렌더링 최적화 후보 검토

- [x] Title 종료 시 객체·재질·텍스처·GPU 복사 버퍼의 참조/해제 경로 확인
- [x] FMOD 장면별·선택 스킬별 로딩과 BGM streaming 가능성, 파일/호출 범위 조사
- [x] shadow map·G-buffer·skybox·인게임 UI의 비용과 후보 우선순위 문서화
- [x] 코드·측정 근거·문서 링크·에이전트 색인 검증 및 기존 구조도 대조

검토 결과: Title UI 3개와 player/camera는 장면 전환의 반환 경로가 있으며 DDS payload 합계 4.207703MiB다. 기동 632.22MiB 전체를 Title 잔존량으로 해석하지 않는다. 음원은 전환 시 Stop_All만 호출해 상주한다. 포함된 FMOD 2.00.01을 NOSOUND로 조사하여 142개 생성 성공, PCM 122.86MiB 중 BGM 86.48MiB, 비인게임 BGM 75.44MiB를 확인했다. BGM 4곡 stream 생성 성공, 인게임 BGM 재생 전 FMOD 증가 126.18KiB, 독립 조사 종료 FMOD 0byte. 실제 재생·루프 안정성은 미검증이다. 중복 Jump.wav의 emplace 실패 시 반환 누락 경로도 확인했다.

UI shader/dissolve의 임시 upload 조기 순회 누락 및 UI ReleaseUploadBuffers의 빈 부모 호출을 확인했다. skybox payload 96→24MiB BC7 후보, 승패 UI 각각 25.43MiB·Speed 41.66MiB·Blood 8.06MiB, shadow map 256→64MiB/4096 후보, 위치 RT 31.64→15.82MiB 형식 후보와 화질 검증 조건을 기록했다. GPU/PCM payload·allocator·Private Bytes를 합산하지 않는다. scripts/Measure-StartupResources.py와 공개 JSON, STARTUP_RESOURCE_OPTIMIZATION_REVIEW.md 및 문서 인덱스를 추가했다. 현재 구현은 변경하지 않으며 UI upload→BGM→skybox/UI→shadow/G-buffer 순서로 제안한다.

검증: 234개 compilation source/에이전트 환경·Serena GameFramework 36/Shader 208/SoundManager 14 심볼·Archify doctor PASS. 공개 근거의 DLL/142 음원/9 DDS/14 source SHA, 그룹/PCM 합계와 스크립트 구문·문서 상대 링크·Git 공백 검사를 확인했다. 기존 DDS/arena 구조도와 범위를 대조했고 구현 구조가 그대로라 frozen JSON/HTML을 재생성하지 않았다. 게임 빌드/전투를 반복하지 않았고 실제 Title 전환 live 자원·audio playback·압축/렌더링 품질은 미검증이다. 원본 저장소·SLNX 기존 변경을 보존하며 이번 검토는 커밋·push하지 않았다.

## 27. Shadow map 유지와 위치 RT/UI 동적 로딩 부작용 설명

- [x] 사용자 지시로 shadow map 8192 해상도 유지, 후보 문서 갱신
- [x] 현재 world position의 조명·shadow 소비 경로와 FP16 간격 계산 확인
- [x] UI 이벤트/생성 경로와 즉시 효과·결과 UI의 동적 로딩 적합성 구분

Shader의 world position 저장→DeferredLighting/shadow transform 경로와 Microsoft의 half 형식을 대조했다. FP16의 좌표 구간별 간격/최근접 오차를 계산하고, 8192 shadow를 유지해도 위치 정밀도 변경이 그림자/조명에 영향을 줄 수 있음을 기록했다. 서버 충돌/실제 transform과 구분하며 위치 RT는 미적용이다. 승패 UI는 필요한 한 장의 비동기 준비 후보, Speed/Blood는 최초 스킬/피격에 즉시 필요하므로 사전 준비 권장이다. 파일 I/O·GPU upload/fence·게시 단계와 cold/warm cache/최대 frame time 검증을 구분했다. 정확한 UI 동적 로딩 시간은 미측정이며 수치를 추정 성과로 제시하지 않는다. 문서/지시만 갱신, 실행 코드·구조도가 그대로라 게임 빌드·rendering 검증·Archify 재생성을 반복하지 않았다.
## 28. 위치 G-buffer 유지와 UI BC7 사전 로딩 절감량 계산

- [x] 사용자 지시로 6번 위치 RT 형식 변경도 제외하고 5번 shadow map 유지와 함께 문서 반영
- [x] INGAME UI DDS의 실제 역할별 생성 범위·헤더·payload 및 BC7 패딩 포함 예상량 계산
- [x] BC7 압축 상태의 GPU 상주·샘플링과 사전 로딩, 손실 압축·UV·실측 한계 문서화

현재 INGAME UI 고유 DDS 21개 중 역할별 20개가 생성되며 모두 32bit RGBA 계열 단일 mip이다. BC7 payload는 ceil(width/4)×ceil(height/4)×16으로 계산했고 4배수 패딩·UV/atlas 보존이 필요하다. 대형 UI 네 장 100.57→25.17MiB(75.40MiB 절감), 플레이어 20종 142.29→35.62MiB(106.67MiB 절감), 보스 20종 138.02→34.55MiB(103.47MiB 절감) 예상이다. 네 장은 20종의 부분집합이며 합산하지 않는다. 모든 대상의 BC7 화질 채택을 가정한 DDS/texel payload 계산으로 GPU actual allocation·전체 Private Bytes 절감과 구분한다. 압축 DDS를 그대로 업로드해 GPU에서 압축 블록으로 상주하고 샘플링 시 하드웨어가 해석하는 경로를 기존 helper/loader와 Microsoft 설명으로 확인했다. 사전 로딩은 첫 표시의 파일 로딩을 피하며 실제 로딩 시간은 미측정이다.

근거 JSON의 21 DDS/Shader SHA·헤더·byte 계산·역할별 합계, 문서 링크·공백 검사를 완료했다. 구현/에셋·구조 흐름 변경 없이 문서와 계산 근거만 추가해 빌드·게임·Archify 재생성은 반복하지 않았다. BC7 변환·화질·메모리 실측은 미적용이며 원본 저장소·기존 SLNX를 보존했다. 커밋·push하지 않았다.

## 29. UI BC7 압축과 사전 로딩 적용

- [x] 원본 DDS·코드·실행 파일 보존과 동일 fixture 전 메모리 측정
- [x] 픽셀·wrap 경계·atlas를 보존한 BC7 변환 및 패딩 UV 전달
- [x] 실제 UI GPU 렌더링 전후 비교·역할별 allocation·실패/수명 검사
- [x] Debug/Release 빌드·로컬 실행·후 메모리 실측·품질 및 포트폴리오/Archify 기록

사용자가 UI 압축 후 사전 로딩 적용을 요청했다. 그림자 8192와 위치 FP32는 유지한다. 결과 UI를 포함한 기존 생성 시점은 유지하며 다른 음원/skybox/upload 회수 후보는 이번 범위에 포함하지 않는다. 기존 SLNX 수정 및 앞선 검토 자료를 보존하고 원본 저장소는 수정하지 않는다. 커밋·push는 별도 요청 전까지 진행하지 않는다.

적용 결과: 고유 21개 RGBA32 UI DDS를 단일 mip BC7_UNORM으로 변환했다. 원본 내용 픽셀을 보존하고 비4배수 11개에 1px wrap gutter 및 4배수 패딩을 추가했다. DDS reserved1의 UIB7/v1 내용 영역을 기존 파일 bytes에서 검사하여 CTexture/독립 UI mesh 상수로 전달한다. atlas/progress 계산 후 frac(uv)×scale+offset, Speed blur의 각 샘플에도 적용하며 일반 DDS/blur는 identity를 유지한다. 기존 장면 사전 생성/사용 시점은 그대로다. DDS load/내용 계약/upload 실패의 자원은 RAII로 반환한다. 8192 shadow와 FP32 위치 RT는 실제 resource 검사·원본 source hash 대조로 유지 확인했다. UI upload 조기 회수·음원·skybox 등 다른 제안은 미적용이다.

실측: 같은 Release EXE e8c0949a0c22a9cb0824bc770592b7dd31329891051f1fb4b0bb891d54c3f462로 원본 에셋 3회→BC7 에셋 3회를 순차 실행했다. Private 중앙값 1,964.52→1,756.43MiB(208.09MiB/10.59% 감소), WS 897.17→790.44MiB, DXGI LOCAL 1,190.40625→1,084.71875MiB. 파티클 19쌍/재사용 30회·직업/스킬/외형 fixture는 같다. payload 플레이어 142.29→35.68MiB·보스 138.02→34.61MiB, 실제 DEFAULT 각각 147.625→41.9375MiB·143.4375→40.5625MiB, 남은 UPLOAD 각각 143.4375→36.875MiB·139.125→35.8125MiB다. 예상 대비 wrap gutter 비용을 구분하고 지표를 합산하지 않는다.

검증: Debug/Release Client 빌드 및 두 역할 실제 UI 16화면·20 texture·9개 내용 계약 오류 거부·copy 참조/UV 보존·GPU 완료 후 OnDestroy의 DEFAULT/UPLOAD 40개 잔여 참조 0 PASS. 역할별 전후 GPU PNG 64개와 전체 스킬 칸/아이템/게이지 5단계/Speed 4프레임/밝고 어두운 alpha 화면을 보존했다. 원본 픽셀/atlas 동일 배치·내용 영역 보존을 검사하고 실제 화면 및 국소 최대 오차를 직접 비교했다. DDS 최소 채널 PSNR 37.08dB, 화면 foreground 최소 41.34dB/최대 오차87(255단계)을 기록하며 무손실 품질로 주장하지 않는다.

기존 경고: Debug 초기화 ID1328 플레이어1511/보스1435, 추가 비교 grid 이후78/50이며 종료 진단에 같은 누적 경고가 남는다. 오류/removedReason 0. DDS 회귀 기존1328 경고16개씩과 Shader.cpp 기존 C4267은 유지한다. Release 메시12범주/실제3모델·DDS Debug/Release 회귀·일반 로컬 연결/클라 창 기동 PASS. 에이전트235 source·Serena capture9심볼 및 Archify 최종v2 showcase9/9·4해상도 containment·1440light/2048dark 직접 점검 PASS. 첫 구조도 overflow 후보는 로컬 artifacts에 보존하고 현재 후보를 새 v2로 전달했다. 갤러리32선택/64개1920×1080 원본 이미지·링크·페이지 오류0·가로 overflow0 PASS. 공개 JSON/CSV/변환·근거 스크립트 및 포트폴리오/인덱스/개발/구조 문서를 갱신했다.

반복 매치 재진입·장시간 네트워크 전투·모든 해상도/DPI·모든 Twinkle 시각·실제 OOM·운영 DB/블록체인은 미검증이다. 정상 종료 검사도 해당 UI 자원 범위이며 전체 누수0으로 보고하지 않는다. 원본 저장소 및 기존 SLNX SHA f3447f2a3a56e5a04b4cae2d11f7e5ccef60bc0ee116713eed3c48156d4f2562를 보존했다. 커밋·push하지 않았다.

## 30. UI 압축 작업 커밋 준비와 하늘·dissolve 후속 검토

- [x] 완료한 UI 압축·앞선 후보 검토 자료의 diff·LFS·근거 해시를 확인하고 커밋 미리보기 준비
- [x] Space 큐브맵 6면·mip·채널과 dissolve의 실제 형식·사용 채널·기존 측정 구간 확인
- [x] 추가 절감 후보·수명·화질/효과 부작용·검증 조건을 별도 포트폴리오 근거로 기록

사용자가 현재 작업 커밋·push와 후속 검토를 요청했다. 원본 저장소와 기존 NewWod.slnx 변경은 제외·보존한다. 실행 코드·에셋은 추가 변경하지 않고 검토 자료를 작성한다. 공통 commit 스킬은 준비된 메시지·staged snapshot의 미리보기 승인 후 커밋을 요구하므로, 승인 전에는 커밋·push하지 않는다. HEAD/origin/main은 fetch 후 076d2692cb843f6263425f66d108983e3bf7e3f9로 일치했다.

검토: Space.dds는 BGRA32 2048×2048 6면/단일 mip 96MiB·전체 alpha255. 같은 해상도 BC7 단일 mip 24MiB로 payload 72MiB 절감 후보이며 6면 경계/반짝임·밝기 판정/회전 레이어를 비교해야 한다. 하늘 upload는 현재 fence 후 이미 반환한다. dissolve.dds는 BC1 1024×1024 11 mip, payload 699,064byte(0.666679MiB)로 기존에 압축됐다. BC4는 절감0, R8는 약0.666654MiB 증가, 512 BC1은0.5MiB 절감으로 우선순위가 낮다. dissolve는 공통 t26 한 장·객체 상태 상수이며 UI shader 소비자가 아니다. ingame_ui_dissolve_ready는 UI 생성과 dissolve 합친 구간으로, 기존 각3회 Private 순증가 중앙값297.48→88.19MiB·LOCAL148.5117→42.8242MiB를 확인했다. 개별 DDS 크기로 해석하지 않는다.

UI upload 실제 잔존 플레이어36.8750/보스35.8125MiB와 dissolve staging의 fence 후 반환 후보를 기록했다. dissolve 개별 upload allocation은 새로 계측하지 않았으며 Private 절감으로 단정하지 않는다. dissolve 일반 draw sample 생략·Speed 225 sample blur는 GPU 시간 후보로 구분한다. SharedDdsTexture cube 지원·weak cache와 순차 Scene 반환을 확인했고 상주 cache는 최종 메모리 절감이 아니다. 새 검토 문서·독립 근거 JSON/읽기 전용 스크립트와 인덱스를 추가했다.

검증: 에이전트235 source PASS, Serena Framework36/Object195심볼 PASS. DDS6면/mip payload·전체 alpha·후보 byte 계산·기존6실행 구간 대조 PASS. 현재 UI23소스/21DDS/64PNG/동일Release EXE/역할별native GPU·수명 결과 및 frozen JSON/HTML 해시 PASS, 새 검토 근거13소스/2DDS 해시·Python구문·109문서링크·Git공백 PASS. 직전 Debug/Release UI빌드·실제렌더링/종료·DDS/메시/로컬실행 결과를 유지하며 새 기능 변경이 없어 반복하지 않았다. 기존 C4267·GPU1328 경고와 전체 네트워크 전투 미검증 범위는 유지한다. 현재 구조/흐름 변경이 없어 기존 Archify 수명도 대조만 수행했고 재생성하지 않았다. 원본·SLNX 보존. 커밋·push는 미리보기 승인 대기다.

완료: 2026-10-06 사용자 승인 후 fingerprint cd8832a598d76fe398aa122079b08e15edd49405와 정확한 메시지를 재확인해 e85eba42958acfdaff5713eb188d7959b39b2fe8를 생성했다. 126파일·85 LFS pointer/내용 검증 범위를 커밋했고 GitHub main push 성공, 신규 고유 LFS59개/55MB 업로드 및 ls-remote의 동일 HEAD를 확인했다. 기존 NewWod.slnx만 남겨 보존했다.

## 31. 단계별 리팩토링 최종 보고서

- [x] 승인된 UI 압축·후속 검토 snapshot 커밋 및 GitHub main push
- [x] 개발 기반·원인 분석·공유 자원·영웅·파티클·NPC·UI를 큰 항목별로 종합
- [x] 각 단계의 전후 측정 조건·통계 기준·검증·미적용 후보와 기존 보고서 근거 연결
- [x] 최종 보고서·인덱스·근거 수치/링크 검산 및 기존 사용자 변경 보존

사용자는 기존 작업 커밋·push 후 단계별 보고서를 바탕으로 최종 정리를 요청했다. 승인된 snapshot은 먼저 push했고 최종 보고서는 별도 문서로 작성한다. 기존 단계별 보고서/측정/사진을 덮어쓰지 않고 현재 구현과 역사 자료·제안을 구분한다. 코드·자원 구조는 변경하지 않으므로 빌드/게임을 반복하거나 frozen 구조도를 재생성하지 않는다.

완료: docs/portfolio/FINAL_REFACTORING_REPORT.md에 8개 큰 항목으로 문제·진행 작업·결과/검증·한계·기존 상세 보고서 링크를 정리했다. 초기 이전·개발/에이전트 환경·VS2026/SLNX·정리, 병목 계측, geometry/DDS 공유, 선택 외형/변환 행렬, 선택 파티클/공용 GPU 풀, NPC upload/BC7/상수 arena, UI BC7/UV, 검증·미적용 후속 후보를 종합했다. 각단계 통계/fixture가 다른 절감량은 합산하지 않으며 최신 오프라인 Private 중앙값1,756.43MiB·WS790.44MiB·LOCAL1,084.72MiB를 실전 peak와 구분한다. 19개 기존 보고서와 11개 데이터셋 해시·6개 Private 비교·최신3회 중앙값·GPU allocation 예시·6개 커밋을 독립 JSON 근거로 보존했고 상대 링크108개와 8개 항목 구조를 확인했다. docs/INDEX.md와 포트폴리오 인덱스를 갱신했다.

최종 검증은 종합 수치·출처/보고서 해시·byte 차이·기존 전후 통계·미적용 하늘/BC4/R8 계산·문서 링크·UTF-8·Git공백 검사다. 승인된 커밋e85eba42958acfdaff5713eb188d7959b39b2fe8와 GitHub main 일치, 원본/사용자SLNX 보존을 확인했다. 문서 추가 이후 새 빌드·게임·측정은 수행하지 않았다. 최종 보고서·근거·인덱스·이번 작업 기록은 push 후 추가한 로컬 문서이며 승인된 이전snapshot의 커밋에는 포함하지 않았다.


## 32. 테스트 편의성 목적의 최종 보고서 재작성

- [x] 4개 클라이언트 동시 테스트의 메모리 제약과 작업 목적 명시
- [x] 1번에서 저장소 이전·실행 가능 상태 확보 내용을 제거하고 에이전트 개발 환경으로 한정
- [x] 병목 측정·적용 항목 3~7에 결과물 요약, 8번은 미적용 조사와 예상 절감량만 정리
- [x] 종합 근거 JSON·수치·8개 항목·문서 링크·UTF-8·사용자 변경 보존 검증

사용자 요청에 따라 기존 단계별 기록은 유지하고 최종 보고서만 목적·구성을 재작성한다. 오프라인 단일 클라이언트 측정과 실제 게임 클라이언트 4개 동시 실행을 구분한다. 미적용 후보의 절감량은 allocation/payload/PCM/논리 GPU texel 기준을 명시하고 중첩 대안은 합산하지 않는다. 이번 범위는 문서 변경이며 구현·측정·구조도는 변경하지 않고 빌드/게임 실행을 반복하지 않는다. 커밋·push는 요청되지 않았다.

완료: 1번은 공통 지침·도구 설정·compilation database·Serena/clangd·빌드/검증 진입점·Archify 근거 관리로 한정했다. 2번 병목 측정, 3~7 적용한 공유/영웅/파티클/NPC/UI 항목에 결과물 요약을 추가했다. 8번의 종합 검증 표·일반 코드 구조 후보는 제거하고 하늘·UI/dissolve·음원·Title·후속 메모리 후보와 사용자 제외 후보만 남겼다. 하늘 BC7 72MiB, UI upload 플레이어36.8750/보스35.8125MiB, 비인게임 BGM PCM75.44MiB 등 계산 근거와 중첩/미산정 구분을 포함했다.

검증: 기존 6개 단계 Private 비교·최신 3회 중앙값 검산, 16개 출처 문서/11개 데이터셋 해시, 21개 미적용 후보의 수치·미산정 근거, 8개 항목/결과물/4개 동시 테스트 미검증 표기, 상대 링크104개 PASS. 보고서와 종합 JSON 해시·UTF-8·공백·표 구조 및 Git 공백 검사 PASS. GitHub main과 기준 커밋e85eba42 일치, 사용자 NewWod.slnx SHA-256 불변을 확인했다. 이번 변경은 로컬 문서에만 있으며 새 빌드·게임 측정·커밋·push는 수행하지 않았다.

## 33. 병목 측정 결과표 추가

- [x] 기존 비용 분해 자료에서 자원 생성 구간 10개의 측정 기준·지표 확인
- [x] 최종 보고서 2번에 Private Bytes·비중·Working Set·DXGI LOCAL 결과표와 최종 평균 추가
- [x] 원시 JSON·표 수치·합계·종합 근거 해시·문서 링크·사용자 변경 보존 검증

표는 2026-10-05 메시 공유 직후, 영웅·파티클·NPC·UI 최적화 이전의 독립 프로세스 3회 평균이다. 당시 병목 분석 기준선을 최신 최적화 후 구성 비율과 구분하고, checkpoint 순증가량과 지표별 합계 조건을 명시한다. 기존 측정 자료를 재사용하는 문서 변경이며 게임 측정·빌드·커밋·push는 수행하지 않는다.

검증: 표의 10개 구간·40개 수치와 최종 평균을 원시 JSON에 대조하고, 반올림 전 3개 지표의 구간 합계가 최종 평균과 일치함을 확인했다. 종합 근거에 당시 baseline의 범위·통계·구간/합계를 추가하고 보고서 SHA-256 갱신, 105개 상대 링크·UTF-8·공백·기존 8개 항목 검증 PASS. 사용자 NewWod.slnx SHA-256과 기준 commit/remote 일치를 유지했다. Git 공백 검사 통과, 작업 기록의 LF→CRLF 안내는 기존 저장소 정규화 안내다.

## 34. 적용 항목 하단에 메모리 감소량 추가

- [x] 전체 성과의 6개 비교를 적용 항목 3~7에 대응
- [x] 각 항목 하단에 Private Bytes 감소량·감소율·전후 값·통계/비교 조건 추가
- [x] 전체 성과 표·원시 데이터와 일치 및 보고서 해시·링크·기존 사용자 변경 보존 검증

사용자 요청에 따라 3~7번 하단에 메모리 감소량 요약을 추가한다. 5번의 선택 파티클과 공용 GPU 풀은 별도 비교로 표시하고 합산하지 않는다. 3번의 geometry 측정에 모델 DDS 추가 절감을 포함한 것으로 해석하지 않는다. 문서 변경만 진행하며 새 측정·빌드·커밋·push는 수행하지 않는다.

검증: 5개 적용 항목 하단의 6개 감소량·감소율·전후 값을 원시 JSON과 대조하고 평균/중앙값 표시를 확인했다. 종합 근거에 해당 항목 번호·하단 요약 포함 여부 및 갱신한 보고서 SHA-256을 기록했다. 기존 2번 병목 표·8개 항목·105개 상대 링크·UTF-8·공백 검증 PASS. 사용자 NewWod.slnx SHA-256과 기준 commit/remote 일치 유지. Git 공백 검사 통과, 작업 기록 LF→CRLF 정규화 안내만 남는다.


## 35. 최종 보고서 문장과 줄바꿈 정리

- [x] 긴 문장·중복 설명·불필요한 수식어 축약
- [x] 측정값·검증·한계·참조 링크를 분리하고 긴 표 셀에 줄바꿈 추가
- [x] 기존 수치·측정 조건·자료 링크 보존 및 근거 해시·문서 형식 검증

기존 8개 항목과 2번 결과표·3~7번 감소량을 유지한다. 이번 범위는 최종 보고서의 가독성 개선이다. 단일 클라이언트 오프라인 측정과 미검증 4개 동시 테스트, 적용/미적용 상태 및 지표 구분은 유지한다. 구현·측정·구조도·커밋·push는 변경하지 않는다.

완료: 52개 문구를 다듬고 반복 성과 설명을 감소량 요약으로 모았다. 최신 측정값·검증·한계는 짧은 문단/목록으로 나눴으며 참조 링크는 각각 표시한다. 표의 긴 셀과 여러 설명이 이어지는 줄에는 렌더링 줄바꿈을 추가했다.

검증: 수정 전 수치 토큰·자료 링크 전체 보존, 원시 데이터의 6개 성과/2번 병목 표/21개 후보 대조, 8개 항목·5개 감소량 요약·105개 상대 링크 PASS. 굵은 표시·표 열·UTF-8·공백·긴 일반 문장 분리 및 갱신한 보고서 해시를 확인했다. 사용자 NewWod.slnx SHA-256과 기준 commit/remote는 유지됐다. 새 빌드·게임 측정·커밋·push는 수행하지 않았다.

## 36. 에이전트 구성 파일·인덱스 구조 첨부

- [x] 실제 진입 문서·MCP/Serena 설정·문서 인덱스·검증 스크립트 경로 확인
- [x] 최종 보고서 1번에 디렉터리 트리·파일 역할·문서 확인 순서 추가
- [x] 첨부 경로·링크·생성 파일 제외·근거 해시·기존 항목/수치 보존 검증

전체 저장소 대신 에이전트 환경 관련 파일을 발췌한다. CLAUDE/GEMINI의 공통 지침 연결과 compile_commands.json의 로컬 생성/Git 제외를 명시한다. 설정 내용·개인 경로는 첨부하지 않으며 파일 역할만 설명한다. 문서 변경으로 구현·측정·구조도·커밋·push는 변경하지 않는다.

검증: 트리의 파일·디렉터리 경로를 계층대로 파싱해 실제 존재와 대조했다. 구성 파일 25개의 목록과 생성 파일 제외·문서 확인 순서를 종합 근거에 기록하고 보고서 SHA-256을 갱신했다. 기존 6개 성과·2번 결과표·21개 미적용 후보·8개 항목·5개 감소량 요약 및 상대 링크109개 PASS. UTF-8·공백·사용자 NewWod.slnx SHA-256·기준 commit/remote 일치를 확인했다. 구현 변경이 없어 빌드·게임 측정을 반복하지 않았으며 커밋·push는 수행하지 않았다.

## 37. 전체 감소량과 항목별 합산 추가

- [x] 최초 geometry 공유 전·최신 UI 적용 후 Private 중앙값 차이와 감소율 계산
- [x] 전체 성과에 전체 감소 표·항목별 단순 합계 및 비교 조건 추가
- [x] 원시 byte 계산·단순 합계·보고서 해시·기존 수치/구조·사용자 변경 보존 검증

최초/최신 3회 중앙값 차이와 6개 단계 절감량의 산술 합을 구분한다. 시나리오·바이너리·재생 조건이 다른 두 측정값 차이를 동일 조건의 전체 A/B로 표현하지 않는다. 반올림 전 byte로 계산하며 신규 실행·측정·커밋·push는 수행하지 않는다.

검증: 최초7289212928byte·최신1841754112byte의 차이5447458816byte = 5,195.10MiB/약5.07GiB, 감소율74.73%를 확인했다. 6개 단계의 반올림 전 절감량 합은5,067.88MiB/약4.95GiB이며 전체 실측 감소량으로 사용하지 않는 참고 합계로 명시했다. 종합 근거에 최초/최신 출처·통계·비교 범위·단순 합계를 기록하고 보고서 해시를 갱신했다. 기존 6개 비교·2번 결과표·21개 후보·8개 항목·109개 링크·표 구조·UTF-8·공백 검증 PASS. 사용자 SLNX와 기준 commit/remote 유지, 새 빌드·게임 측정·커밋·push는 수행하지 않았다.

## 38. 최종 보고서 커밋·push 준비

- [x] 보고서·근거·인덱스·작업 기록의 5개 파일로 범위 확정
- [x] 측정 수치·출처/보고서 해시·문서 링크·UTF-8·공백 검증
- [x] 사용자 SLNX 변경 제외 및 한글 커밋 미리보기 준비

사용자가 커밋·push를 요청했다. 공통 commit 스킬의 2단계 절차로 준비하며, 미리보기 승인 후 동일 staged snapshot을 커밋하고 origin/main에 push한다. 문서 변경만 포함하고 기존 NewWod.slnx 변경은 보존한다. 새 게임 실행·측정·빌드는 수행하지 않는다.

완료: 승인된 fingerprint 0d2acb41d3c5275a5cd4fcd95d26b02d895ef448와 메시지로 86f83a94f76e7218c4cd2f34d992f9d2d02174f1을 생성·push했다. GitHub main과 로컬 HEAD 일치, 문서 5개 반영과 기존 사용자 SLNX 변경만 남았음을 확인했다.

## 39. 서버 리팩토링 1단계 ServerCore 구현 계획

- [x] 두 서버의 동일 파일·의미 차이·의존성·프로젝트 설정 조사
- [x] ServerCore 범위·프로토콜/콘텐츠 경계·자원 수명·단계별 구현/검증 계획 작성
- [x] 1~6단계 순차 로드맵과 후속 새 더미 도구의 연결 지점 기록
- [x] 미구현 설계 Archify JSON/HTML·출처 및 문서 검증

사용자 요청은 1단계 구현 계획 작성이다. 현재 서버 소스와 동작은 변경하지 않는다. 2~4단계의 신규 시나리오 더미·agent/GUI 제어·월드 관찰, 5단계 상세 서버 문서화, 6단계 콘텐츠 데이터화 완료는 순차 후속 작업으로 구분한다. 원본 저장소와 기존 사용자 SLNX 변경을 보존하고 커밋·push는 수행하지 않는다.

검증: 최상위 동일 이름 35쌍 중 4쌍의 byte 동일·31쌍의 차이를 확인하고 소스 SHA-256·줄 위치를 근거 JSON에 기록했다. 로비 5개/게임 시작 크기 budget, 다른 OP_TYPE·풀·스레드 및 4개 수신 경로의 경계를 계획에 반영했다. 계획은 1-A~1-H 구현·검증 단위와 완료 기준으로 작성했다. compilation database 235개/서버171개, 누락 경로0개; Serena로 두 CNetworkMgr·CClient를 인덱싱(22/14/24/9 symbols)했고 Test-AgentEnvironment.ps1 -StaticOnly PASS. Archify 출처3개·showcase9/9 오류0/경고0, desktop4크기 containment와 light/dark4화면 직접 확인 PASS. 정상 종료·패킷·부하 테스트는 구현 후 수행할 계획이며 이번에는 서버 빌드·구동·DB 검증을 실행하지 않았다.

마무리: 조사 대상 89개 소스·설정의 SHA-256, 문서 상대 링크73개, frozen JSON/HTML 해시·byte, UTF-8·공백을 확인했다. 문서별 정규화 예외만 .gitattributes에 추가했고 서버/클라이언트 소스·프로젝트·개발 스크립트는 변경하지 않았다. 사용자 SLNX SHA-256과 기준 HEAD를 보존했다.

## 40. ServerCore 공통 기반 구현

- [x] 기존 빌드·LOCAL_TEST·패킷 ABI 기준 확보
- [x] 공통 정적 라이브러리·Protocol·주소·진단·Job·풀 추출
- [x] 공유 송수신·IOCP·프레임·오류·자원 수명과 서버 어댑터 연결
- [x] 정상 종료·회귀 테스트·Debug/Release·에이전트 환경 검증
- [x] 실제 구현 문서·Archify 구조도·정리 근거 작성

사용자가 1단계 구현을 요청했다. 계획의 1-A~1-H를 순차 적용한다. 새 더미 제품·GUI·월드 관찰·콘텐츠 데이터화는 다음 단계이며 이번에 함께 구현하지 않는다. 기존 사용자 SLNX 경로를 보존하며 프로젝트만 추가하고 커밋·push는 하지 않는다. 기준 빌드를 막던 LFS 와일드카드 사전 검사를 실제 Git LFS 파일 목록으로 수정한다.

완료: ServerCore 정적 라이브러리와 테스트를 추가했다. Winsock·소켓·IOCP·부분 송신·프레임·Job·풀·스레드 회수·진단을 공통화했다. Protocol은 Shared로 이동하고 이전 경로에 forwarding header를 남겼다. 서버별 OP_TYPE·content handler·작업 budget과 도메인 풀은 어댑터에 유지한다. 중복 `.cpp` 12개의 compile/filter 참조를 제거하고 복구 commit과 대체 구현을 기록했다.

수명: 연결별 callback 직렬화, 송신 완료 후 풀 반환, Disconnect 이전 native I/O 배출, 접속 세대에 따른 지연 패킷·사용자 상태 타이머 검사, 생산자 → IOCP → 콘텐츠 순서의 정상 종료를 적용했다. 부분 초기화/worker 생성 실패도 회수한다. 기존 싱글턴 자기 재해제·보스 시작 위치·로딩 재예약·몬스터 배치 범위·비활성 미니언 navigation·NPC 이벤트 대상·패킷 fall-through·누락 외형 파일의 무한 읽기 결함을 회귀 검사에 필요한 범위에서 수정했다.

검증: 서버/Core/tests Debug·Release 빌드, 구성별 Core 93항목·Protocol 구조체110종/상수106개, 네트워크12종, 보스4/5의 4인 매칭·전원 로딩 완료·첫 NPC 알림 PASS. 통합7시나리오에는 리소스 누락2종·로비 미실행·배타 포트 충돌의 실패 종료4종을 포함한다. 정상/실패 종료에서 pending·sockets·leased0을 확인했다. 마지막 패킷 분기·범위 수정 후 두 구성의 빌드/Core/전체 통합 검사를 다시 통과했다.

추가 확인: Client Debug 빌드와 실제 게임 창 시작, Release 서버 간 연결 smoke, Test-AgentEnvironment 전체와 Core Net.cpp Serena51 symbols PASS. compilation database226개·누락0개. 실제 구현 Archify showcase9/9 오류0/경고0·4viewport containment·4PNG 직접 확인 PASS. 구조도 frozen byte/해시·소스 UTF-8·문서 링크·삭제 파일 참조·기존 사용자 SLNX3경로를 확인하고 구현/정리 문서·인덱스·소스/검증 근거를 작성했다.

한계: 기존 서버/클라이언트 컴파일 경고를 유지한다. DB 없는 LOCAL_TEST이며 운영 DB·블록체인·전투 전체·장시간 부하·4개 렌더링 클라이언트는 미검증이다. NPC/매치/스킬 전체 타이머 수명과 ExtraPos/Look CSV 파싱은 후속 범위다. CPU/메모리 개선량은 측정하지 않았다. 기준 HEAD 유지, 커밋·push는 수행하지 않았다.

## 41. ServerCore 커밋·push 준비

- [x] 계획·구현·테스트·문서·구조도와 프로젝트 추가를 커밋 범위로 확정
- [x] 기존 사용자 SLNX 경로/ID와 이에 맞춘 로컬 filter 경로는 작업 트리에 보존
- [x] 소스 해시·문서 링크·UTF-8·공백·staged 프로젝트/filter 정합성 검사
- [x] 한글 메시지와 staged fingerprint를 준비하고 승인 절차 적용

사용자가 커밋·push를 요청했다. 공통 commit 스킬에 따라 미리보기를 준비한다.
승인 후 동일 snapshot을 커밋하고 origin/main으로 push한다.
SLNX/filter에는 Core/test 프로젝트 추가만 stage하고 기존 상대 경로를 유지한다.
사용자의 기존 긴 SLNX 경로·ID와 로컬 filter 호환 경로는 미커밋으로 보존한다.
직전 Debug/Release 검증 이후 구현 변경은 없어 게임 검사를 반복하지 않는다.

## 42. 클라이언트 장면 로딩 전후 비교

- [x] 메모리 최적화 이전/현재 코드·에셋과 동일한 측정 조건 확보
- [x] Title→Lobby, Lobby→Ready→Ingame, Ingame→Lobby를 실제 장면 로더로 반복 측정
- [x] 독립 프로세스 교대 실행·GPU 완료·로딩 화면 포함 여부와 결과 검증
- [x] 최종 보고서에 세 구간 결과표만 추가하고 근거 해시 갱신

사용자는 후속 서버 작업 전에 클라이언트 로딩 시간을 비교하도록 요청했다.
최적화 전 ed3428e9와 현재 d02737a5의 격리된 Release 빌드를 비교한다.
로그인/매칭/선택 대기는 제외하고 실제 장면 자원 준비 시간을 측정한다.
로비→인게임에는 READY 장면 생성도 포함한다. 보고서에는 최종 결과와 짧은 측정 조건만 기록한다.
사용자 솔루션 경로 변경과 원본 저장소를 보존하고 커밋·push는 하지 않는다.

완료: 두 commit의 Release x64를 artifacts 아래 격리 빌드했다. 개선 전 UI 21개·NPC 텍스처 25개를 당시 Git/LFS 내용으로 복원했다. 동일한 측정 코드와 외형·스킬·1920×1080 조건으로 예열 각 1회 제외, 독립 프로세스를 전후 교대로 각 5회 실행했다. 기존 메모리 profiler를 사용하지 않아 정상 로딩 화면 worker와 장면 자원 로더를 유지했다.

검증: 측정 10회·예열 2회 모두 정상 완료했다. 장면 전환부터 BGM 선택·목적지 첫 프레임 GPU fence까지 측정하고 GPU 오류·stderr가 없음을 확인했다. 로비→인게임은 각 실행의 Lobby→Ready·Ready→Ingame을 합산한 뒤 중앙값을 계산했다. 중앙값은 타이틀→로비 8.92→7.10초, 로비→인게임 40.17→15.55초, 인게임→로비 14.77→8.06초다. 원시 결과는 Git 제외 경로에 보존하고 최종 수치·바이너리/측정 코드/원시 결과 해시를 근거 JSON에 기록했다.

범위: 캐시를 비우지 않은 오프라인 자원 로딩 비교다. 로그인·매칭·선택·서버 응답 대기, 전투 전체·4클라이언트 동시 실행은 측정하지 않았다. 제품 소스는 변경하지 않았고 기존 메모리 수치를 재측정하지 않았다.

## 43. 로딩 측정 보고서 커밋·push 준비

- [x] 최종 보고서·측정 근거·인덱스·작업 기록과 솔루션/filter 7개 파일을 커밋 범위로 확정
- [x] 10회 측정·예열 2회 결과, 중앙값·해시·기존 메모리 수치·문서 링크 81개 재검증
- [x] 솔루션/filter 경로를 저장소 기준 상대 경로로 정리하고 기존 프로젝트 ID 유지
- [x] 프로젝트 5개·서버 filter 4개·실행 프로필 참조를 검증하고 한글 메시지 미리보기 갱신

사용자가 커밋·push를 요청했다. 공통 commit 스킬에 따라 미리보기 승인 후 동일 staged snapshot을 커밋하고 origin/main에 push한다. 제품 소스 변경이 없어 빌드·게임 측정을 반복하지 않는다. 공백 검사는 통과했고 Git의 기존 LF→CRLF 안내를 확인했다.

범위 갱신: 사용자가 솔루션/filter도 함께 커밋하도록 요청했다. 긴 `../../../../../../../../GitFolder/NewWod/` 접두어를 제거하고 기존 5개 프로젝트 ID를 유지했다. 경로 정리 전후 실제 대상 파일이 동일함을 확인했다. 수정 전 파일은 Git 제외 artifacts에 보관했다. 새 범위와 메시지의 미리보기 승인 전에는 커밋하지 않는다.

추가 검증: 솔루션의 프로젝트 ID 5개와 실제 ProjectGuid, 서버 filter의 4개 참조, 실행 프로필의 경로가 일치한다. VS2026 MSBuild의 ValidateSolutionConfiguration을 전체 솔루션과 서버 filter에서 Debug/x64로 통과했다. C++ 재빌드·게임 실행과 GUI 재로드 재현은 수행하지 않았다.

## 44. ServerCore 파일 표시·코드 가독성 정리

- [x] Core 공용 헤더 등록 누락을 수정하고 구현/헤더/테스트 filter 구성
- [x] ServerCore의 C++ 구현·헤더·테스트에 함수 간 빈 줄·다중 행 if·마지막 return 분리 적용
- [x] 공통 포맷 설정·작업 지침에 사용자 형식 규칙 반영
- [x] 소스 토큰/프로젝트 참조·Debug/Release 서버 빌드·Core/ABI 검사와 에이전트 환경 검증

사용자는 ServerCore 파일 누락과 코드 가독성을 지적했다. Core 프로젝트는 Net.cpp만 등록했고 공용 헤더 4개와 filters가 누락돼 있었다. 공용 헤더·구현과 별도 테스트 프로젝트의 소스를 표시하고 ServerCore 소스 7개를 포맷한다. 로직·패킷·동시성·자원 수명은 유지한다. 기존 서버/클라이언트 전체나 외부 코드를 일괄 포맷하지 않는다. 커밋·push는 요청 시 별도로 진행한다.

완료: Core의 헤더 4개를 ClInclude에 등록하고 Core/test에 filters를 추가했다. Core 소스 5개와 테스트 소스 2개, 포맷 설정을 소스·헤더·개발 설정으로 구분했다. 프로젝트 XML도 여러 줄로 정리했다. 루트 .clang-format과 공통 지침에 사용자 형식 규칙을 반영하고 구현/개발 문서를 갱신했다.

검증: 7개 소스의 UTF-8·C++ 토큰 동일·포맷 반복 안정성과 clang-format 22.1.3 dry-run PASS. 프로젝트/filter 누락·중복 없음, 기존 빌드 설정과 ProjectReference 동일, MSBuild 평가에서 헤더 4개 확인. 최종 소스 상태에서 서버 Debug/Release 빌드와 구성별 Core 93항목·ABI 구조체 110종/상수 106개 PASS. Test-AgentEnvironment -StaticOnly와 compilation database 226개, Serena Net.cpp 51 symbols PASS. 문서 상대 링크 21개·공백 검사 PASS.

범위: 기존 narrowing·signedness 등 컴파일 경고는 유지했다. GUI 표시·재로드, 이번 변경의 서버 통합/전투/운영 DB 검사는 실행하지 않았다. 소스 토큰이 같고 패킷 ABI 검사를 통과했으며 이전 통합 검증 기록은 그대로 보존했다. .slnx/.slnf·기존 서버 어댑터·클라이언트·외부 코드는 수정하지 않았다. 커밋·push는 수행하지 않았다.

## 45. if 분기 뒤 간격 추가

- [x] ServerCore의 if/else 분기 묶음 뒤에 다음 문장과 구분하는 빈 줄 하나 추가
- [x] 중첩 if·중괄호 없는 분기·else 연결과 기존 마지막 return 간격 보존
- [x] 공통 지침·포맷 설정·관련 문서 갱신 및 토큰 동일·포맷 검사

사용자의 추가 형식 규칙을 기존 ServerCore 포맷 작업에 반영한다. else 연결과 닫는 중괄호 직전은 제외하고 다음 문장이 있을 때 빈 줄을 추가한다. 공백만 변경하며 이전 검증 이후 빌드·게임 실행은 반복하지 않는다. 커밋·push는 수행하지 않는다.

완료: Concurrency.h·Session.h·Net.cpp·CoreTests.cpp의 분기 뒤에 빈 줄 60개를 추가했다. 긴 조건의 줄 나눔도 포맷 기준으로 유지했다. 공통 지침과 포맷 설정·관련 문서에 후속 규칙을 반영했다.

검증: 중첩·중괄호 없는 if, dangling else, 분기 연결·범위 끝의 간격 검사 PASS. C++ 소스 7개 토큰 동일·포맷 반복 안정성·clang-format dry-run, 기존 프로젝트/filter와 빌드 설정 보존·문서 상대 링크 21개·공백 검사 PASS. 이전 단계의 Debug/Release 빌드·Core/ABI 검증 기록을 유지했고 이번 공백 변경에 빌드·게임 실행은 반복하지 않았다.

## 46. 자동 줄 나눔 기준 250자 적용

- [x] clang-format ColumnLimit을 120에서 250으로 변경하고 공통 지침에 반영
- [x] ServerCore 소스의 긴 식·인자 목록을 다시 포맷하고 함수/if/return 간격 유지
- [x] 소스 토큰 동일·250자 초과 행·포맷 반복 안정성·공백 검사

사용자는 지나친 자동 줄 나눔을 줄이고 250자를 초과할 때 나누도록 요청했다. 기존 함수·분기 구조와 빈 줄은 유지하며 공백만 정리한다. 빌드·게임 실행 및 커밋·push는 진행하지 않는다.

완료: ColumnLimit 250과 공통 지침·개발 문서를 반영했다. ServerCore C++ 소스 7개를 검사하고 Session.h·Net.cpp·CoreTests.cpp·ProtocolAbi.cpp의 식/인자 목록 줄 나눔을 정리했다. 토큰 동일·250자 초과 행 0개·함수/분기/return 간격 유지·포맷 반복 안정성·clang-format dry-run PASS. 기존 프로젝트/filter와 빌드 설정, 문서 상대 링크 21개·공백 검사도 통과했다. 기능 변경이 없어 빌드·게임 실행은 반복하지 않았다.

## 47. while 간격·연결된 if 중괄호 보완

- [x] while 뒤 빈 줄 규칙을 공통 지침과 ServerCore에 적용
- [x] 연결된 if/else-if/else의 단일 문장 본문도 중괄호로 감싸기
- [x] 기존 문장·250자·함수/분기/return 간격 보존과 포맷·Core/ABI 검증

사용자의 추가 형식 규칙을 적용한다. 연결된 분기의 단일 함수 호출 2곳에 중괄호를 추가하고 while 뒤에 다음 문장이 있을 때 빈 줄을 둔다. 변수 선언·자원 수명·분기 연결을 변경하지 않는다. 이전 작업은 보존하고 커밋·push는 수행하지 않는다.

완료: Net.cpp의 Disconnect 오류 분기와 Concurrency.h의 풀 소유 등록 분기를 중괄호로 감쌌다. 연결된 나머지 분기는 기존 중괄호를 유지했다. Net.cpp·CoreTests.cpp의 while 뒤 8곳에 빈 줄을 추가하고 공통 지침·포맷 설정·문서를 갱신했다.

검증: 기존 토큰은 추가 중괄호 2쌍 외에 동일하고 감싼 본문은 각각 단일 함수 호출이다. else 연결·do/while 경계·닫는 중괄호 직전 검사, 250자·함수/if/return 간격·포맷 반복 안정성·clang-format dry-run PASS. 프로젝트/filter·기존 빌드 설정·문서 링크 21개·공백 검사 PASS. Core/test Debug·Release 빌드와 각 93항목·패킷 ABI 110종/106상수 PASS. 이번 변경에서 전체 서버/클라이언트 재빌드·GUI·통합/전투 검사는 실행하지 않았고 기존 경고를 숨기거나 비활성화하지 않았다.

## 48. 빌드 오류·멤버 변수 접미사 확인

- [x] VS2026 전체 Debug x64 빌드와 실제 IDE 버전 확인
- [x] VS2022에서 v145 도구 누락 오류 재현 및 해결 명령 문서화
- [x] ServerCore의 멤버 변수 접미사 확인

사용자는 빌드 오류 확인과 변수 뒤 `_`의 의미를 요청했다.
기존 포맷 작업을 보존하고 실제 빌드와 설치된 도구를 비교했다.

검증: `Build.ps1 -Configuration Debug`로 전체 솔루션 빌드 PASS.
Compilation database 226개를 생성했다. 기존 변환·미사용 변수 등의 경고는 남아 있다.
NewWod를 연 IDE 프로세스는 VS2022였다.
VS2022 MSBuild로 Core의 `PrepareForBuild`를 실행해 `MSB8020`(v145 도구 누락)을 재현했다.
VS2026으로 명시적으로 여는 명령을 마이그레이션 문서에 추가했다.

`stop_`, `failure_`, `threads_` 등은 클래스 멤버를 인자·지역 변수와 구분하는 이름 규칙이다.
C++ 문법상 특별한 기능은 없으며 기존 `m_` 방식과 혼재한다.
변수 이름·제품 소스·toolset은 변경하지 않았다.
GUI 빌드·게임 실행은 검증하지 않았고 커밋·push는 수행하지 않았다.

## 49. 멤버 변수·함수 인자 이름 규칙 확정

- [x] 멤버 `m_`·함수 인자 `_` 접두어를 공통 지침에 기록
- [x] C++ 예약 식별자 조건 확인 및 개발 문서에 근거 연결
- [x] 추가 결정 대상과 실제 적용 범위 구분

사용자가 제시한 접두어 규칙을 확정했다.
함수 인자는 `_parameter`처럼 소문자로 시작하고 `__`를 포함하지 않는다.
전역 이름에는 함수 인자 규칙을 적용하지 않는다.
지역 변수·상수·정적 멤버의 세부 표기와 기존 코드 변경 범위는 아직 확정하지 않았다.

검증: C++ 표준 초안의 예약 식별자 조항을 확인했다.
문서 링크 대상·UTF-8·공백 검사를 수행했다.
제품 소스의 이름 변경·빌드·게임 실행·커밋·push는 수행하지 않았다.

## 50. 기존 서버 코드의 멤버·인자 이름 통일

- [x] 빌드 대상의 자체 코드와 외부·프로토콜 계약 범위 분류
- [x] 멤버 `m_`·함수 인자 `_` 규칙으로 선언과 참조 변경
- [x] 식별자 외 변경·누락·예약 이름 검사
- [x] 서버 Debug/Release 빌드·Core/ABI·통합 검사

사용자가 기존 코드에도 확정된 접두어 규칙을 적용하도록 요청했다.
후속 지시에 따라 대상은 ServerCore·로비 서버·게임 서버로 한정한다.
클라이언트·Shared 공통 프로토콜·외부 코드는 변경하지 않는다.
이전 작업을 보존하며 클래스 멤버·함수 인자의 이름만 변경한다.
지역 변수·함수·타입·전역 상수는 별도 변경하지 않는다.
외부 코드·패킷·파일 형식의 필드는 계약을 확인하고 보존한다.
커밋·push는 별도 요청 시 진행한다.

완료: 서버 파일 311개에 멤버 선언 216곳·함수 인자 선언 2,912곳과 사용처를 반영했다.
총 9,924곳의 식별자를 변경했다. 정적 멤버·내부 상태 구조체 필드도 `m_`로 구분한다.
자동 도구가 누락한 `offsetof` 참조를 보완하고 `_stat`와 매크로 이름 충돌을 피하도록 `_statValue`를 사용했다.
줄 끝 공백이 있던 파일 97개도 정리했다. 기존 식별자 이외의 C++ 토큰·문자열은 보존했다.
CSV reflection에 사용하는 `tabledata.h` 필드는 유지했다.
백업·수집 목록·검증 해시는 Git 제외 `artifacts/naming-refactor`에 보관했다.

검증: 서버 Debug/Release 빌드 PASS.
각 구성에서 Core 93항목·패킷 ABI 110종/상수 106개 PASS.
네트워크 12개 경로·4참가자 보스 4/5 선택과 게임 진입·정상 종료를 두 구성에서 확인했다.
초기화 실패 4종에서도 종료 코드 1과 pending/sockets/leased=0을 확인했다.
변경 대상 재조사에서 계약 예외 외 추가 이름 변경 후보 0개를 확인했다.
UTF-8·식별자/공백 외 변경 없음·보호 경로 변경 없음·공백 검사 PASS.
Test-AgentEnvironment -StaticOnly와 compilation database 226개 PASS.

한계: Clang 분석에서는 MSVC가 허용하는 기존 상속 싱글턴 정적 멤버 정의 13곳의 진단이 남았다.
해당 정의와 로직을 변경하지 않았으며 실제 v145 빌드·회귀 검사는 통과했다.
기존 변환·미사용 변수 등의 컴파일 경고는 유지했다.
운영 DB·블록체인·GUI 플레이 전체는 이번 검증 범위가 아니다.
커밋·push는 수행하지 않았다.

## 51. 서버 이름·포맷 정리 커밋·push 준비

- [x] 서버 소스·Core 프로젝트/filter·포맷 설정·관련 문서의 커밋 범위 검토
- [x] 현재 소스 해시와 기존 Debug/Release Core·ABI·통합 검사 결과 재확인
- [x] UTF-8·문서 링크·프로젝트/filter·staged 공백 검사 및 미리보기 준비
- [ ] 미리보기 승인 후 동일 snapshot 커밋 및 origin/main push

사용자가 커밋·push를 요청했다. 공통 commit 스킬의 미리보기 승인 절차를 적용한다.
소스는 마지막 검증 이후 변경되지 않아 빌드·서버 실행을 반복하지 않는다.
클라이언트·공통 프로토콜·외부 코드·빌드 산출물·로그는 커밋 범위에서 제외한다.
정확한 메시지와 staged fingerprint는 Git 제외 artifacts에 저장한다.

## 52. 서버에 남은 공통 세션·클라이언트·작업 기반 추출

- [x] 지정 파일·사용처·기존 ServerCore 계약·작업 트리 확인
- [x] SocketUtil·Session·Listener 및 공통 I/O context 구현을 ServerCore로 이동
- [x] CClient 공통 수신·disconnect 기반과 다형적 패킷 전달 확장점 적용
- [x] 로비 작업 스케줄러·큐 상태와 공통 Job 구현 소유권 정리
- [x] Debug/Release 서버 빌드·Core/ABI·네트워크·실패/종료 회귀 검사
- [x] 구조 문서·Archify JSON/HTML·정리 근거·실제 결과 갱신

사용자가 지정한 SockAddr·SocketUtil·TCPSocket·CClient·Job·JobQueue와 주변 중복을 검토한다.
기존 staged 이름·포맷 변경을 보존한다. ServerCore는 서버 콘텐츠·Protocol에 의존하지 않고,
템플릿으로 I/O context·풀 정책을 주입하며 CClient는 가상 함수로 패킷 전달을 확장한다.
패킷 ABI·서버 권위·로비 Five/게임 Snapshot budget·세대 검사·종료 순서를 유지한다.
커밋·push는 수행하지 않는다.


완료: Session·Listener·SocketUtil·OverlapEx 공통 구현을 ServerCore로 이동했다.
CClient는 BasicClient의 공통 Receive/Disconnect를 상속하고 패킷 검증·전달·상태 변경을 가상 함수로 구현한다.
로비 작업 큐·atomic 등록 상태·스케줄러는 JobTarget/JobScheduler가 소유하며 사용자 정리는 로비 callback에 남겼다.
SockAddr·Job·LogUtil은 이미 별칭만 있어 유지했다. SocketUtil.cpp 2개와 프로젝트/filter 항목을 제거했다.
게임 accept 세션 풀은 Resource::m_acceptSessionPool로 이동했다. 같은 이름의 자체 소스 28쌍과 주변 사용처를 검토했다.

검증: Debug/Release 서버·Core·테스트 빌드 PASS. 구성별 Core 114항목·패킷 ABI 110종/106상수 PASS.
네트워크 12종·4참가자 보스 4/5의 매칭·로딩 완료·첫 NPC 알림과 정상 종료 PASS.
초기화 실패 4종도 종료 코드 1과 pending/sockets/leased=0 PASS. 신규 테스트 21항목은 수신 확장·세대·거부·큐 재예약·폐기를 포함한다.
Test-AgentEnvironment -StaticOnly·compilation database 225개·UTF-8·프로젝트/filter 참조·공백 검사 PASS.
구조도 architecture showcase 9/9(오류·경고 0), 데스크톱 4크기 light/dark containment PASS, PNG 4장 직접 확인.
일반 텍스트의 줄 끝과 수정 영역 간격만 마지막에 정리했으며, 검증 이후 실행 로직은 바꾸지 않았다.

기존 변환·미사용 변수 등의 컴파일 경고는 유지했다.
운영 DB·블록체인·실제 GUI 전투 전체·장시간 부하는 미검증이다.
main 예외 처리·CServer worker 조합은 후속 공통화 후보로 기록했다.
기존 staged 변경을 보존했고 커밋·push는 수행하지 않았다.


## 53. JobQueue 생성자 정책 지정으로 단순화

- [x] 정책 래퍼 타입 제거와 두 서버 JobQueue 별칭 통일
- [x] JobQueue 생성자 정책 인자 필수화·로비 Five/게임 Snapshot 명시
- [x] Debug/Release 서버 빌드·기존 Core/ABI 검사·공백 확인
- [x] 현재 구현 설명과 성능 개선·비교 측정 후속 작업 기록

사용자가 불필요한 정책 래퍼를 제거하고 JobQueue 생성자 인자로 정책을 지정하도록 요청했다.
로비 CClient의 JobTarget(Five)은 유지하고 Zone(Five)·게임 CMatch(Snapshot)·테스트 큐 정책을 명시했다.
큐 자료구조·실행 직렬화·처리 개수·패킷 순서·스케줄러 수명은 이번에 바꾸지 않는다.
현재 구조도의 컴포넌트·호출·자원 수명에 변화가 없어 Archify JSON/HTML은 재생성하지 않는다.

완료: 정책 래퍼와 기본 정책 인자를 제거했다. 두 서버는 같은 JobQueue 타입을 사용하고 모든 생성 위치에서 정책을 전달한다.
Debug/Release 서버·Core·테스트 빌드 및 각 Core 114항목·ABI 110종/106상수 PASS.
UTF-8·clang-format·공백 검사 PASS. 기존 컴파일 경고는 유지했다.
실행 방식이 같아 네트워크/GUI 검사를 반복하지 않았다. 이번에는 성능 측정을 하지 않았으며 커밋·push도 수행하지 않았다.

## 54. JobQueue 성능 측정·개선·동일 조건 비교 (임시 개선과 전후 측정 완료, 이동 방식 재설계 후속)

- [x] 개선 전에 현재 방식의 실행 의미와 기준 소스 hash를 고정
- [x] 재현 가능한 큐 단위 측정 도구와 부하 조건을 준비하고 현재 방식 성능 측정
- [x] 실제 로비 이동/정지·다중 게임 매치 기준 성능과 계측 끔 CPU 대조 (72번)
- [x] 측정에서 확인한 병목을 근거로 개선안을 결정·구현하고 기능/동시성 회귀 검사 (73번)
- [x] 기준 측정과 같은 조건으로 개선 후 성능을 측정하고 원시 결과·비교표·판단 근거 기록 (73번)
- [ ] 클라이언트·서버 이동 방식 재설계와 유저 흐름 응답성 개선 (77·78번)

개선 전 구현은 PPL concurrent_priority_queue<shared_ptr<IJob>>에 작업을 저장하고,
실행 mutex를 잡은 상태에서 Five 또는 Snapshot 개수만큼 callback을 실행한다.
작업별 shared_ptr/Job 생성 비용, 큐 연산·실행 mutex 경합, size 조회 비용,
Five/Snapshot의 처리량·긴 작업 점유·대상 간 공정성과 스케줄러 재등록/깨우기 비용을 조사한다.
shared_ptr 비교로 정렬하므로 FIFO나 예약 시간순 실행은 현재 보장하지 않는다.
순서·예약 시간·budget·종료 중 폐기 정책은 먼저 계약을 정하고 개선 시 호환 영향을 구분한다.

측정 계획:

- 동일한 Windows/x64 Release·도구 버전·CPU·worker/producer 수·전원 설정·입력 seed를 기록한다.
  워밍업·측정 시간·반복 횟수와 대상 큐/작업 수·작업 비용·도착 패턴도 전후 동일하게 고정한다.
- 빈 큐/idle, 단일 producer, 여러 producer의 경합, 순간 burst, 지속 부하,
  짧은/긴 callback 혼합, 여러 대상 큐, callback 내부 재등록을 포함한다.
  로비 Five와 게임 Snapshot은 별도로 비교하고 큐 자체 비용과 실제 작업 시간을 분리한다.
- 처리량(jobs/s), enqueue→실행 시작 지연(p50/p95/p99), 실행 완료 지연,
  queue depth/최대 대기, CPU 시간·idle CPU, 할당 횟수/bytes·메모리 사용량,
  대상별 대기·기아 여부를 기록한다. 가능하면 lock 대기·깨우기/재예약 횟수도 수집한다.
- 단위 측정과 LOCAL_TEST 서버 부하 검사를 나누고, 동일 계측을 전후 사용해 계측 비용을 구분한다.
  반복 결과의 중앙값·분산과 부하별 tradeoff를 함께 기록하며 결과를 미리 개선으로 단정하지 않는다.
- 개선 전 기준 결과를 확보한 뒤 자료구조·할당/소유권·batch/budget·잠금 범위를 검토한다.
  개선 후에도 작업 누락/중복·직렬화·재등록·세대 검사·disconnect 폐기·정상 종료를 검증한다.
  기존 Core/ABI·네트워크 회귀 결과와 성능 수치를 구분하고 운영 DB·블록체인 검증으로 확대 해석하지 않는다.

2026-10-11에 71번에서 큐 단위 기준, 72번에서 실제 서버 60회 기준 측정을 수행했고, 73번에서 임시 개선과 동일 조건 전후 비교를 완료했다. 잔여 지연과 이동 방식 재설계는 77·78번에 남긴다.
생성 로그·대용량 원시 결과는 Git 제외 artifacts에 저장하고 재현 명령·요약·소스 hash를 문서에 남긴다.

2026-10-10 후속 관찰: Server Lab 첫 구현에서 선택 활성화된 작업 카운터·실행 시간·서버 CPU/메모리/송수신 시계열을 추가했다.
이는 재현 조건을 고정한 기준 벤치마크가 아니었다. 이후 큐 단위 기준은 71번, 실제 서버 기준은 72번, 임시 개선·전후 비교는 73번에서 수행했다.
로비 ProcessUpdate의 즉시 자기 재등록 중 이동/정지 요청 timeout을 실제 관찰했다.
주소 비교 우선순위와 반복 업데이트의 대상/작업 공정성을 기준 측정에 반드시 포함한다.


## 55. Resource 공통화·참조 기호 위치 통일

- [x] 양쪽 Resource·풀 사용처·소유권·Serena/compilation database 확인
- [x] 공통 I/O 자원·세션/소켓 풀 구현을 ServerCore로 이동하고 서버별 구성만 유지
- [x] ReferenceAlignment: Left 설정·서버 자체 코드의 참조 선언 공백만 변경
- [x] 자원 구성/반환 검사·Debug/Release 빌드·Core/ABI·통합 회귀
- [x] 문서·정리 근거·Archify JSON/HTML·실제 결과 갱신

사용자는 Resource 중복 정리와 SocketResource& _other 형태의 참조 기호 위치를 요청했다.
I/O context 풀 접근은 공통 기반, 세션 shared_ptr/네이티브 소켓 재사용은 타입별 구성으로 분리한다.
기존 game accept/reuse 풀의 분리·lobby 시간순 소켓 풀·IOCP native 소켓 소유권을 유지한다.
일반 서버 자체 .h/.cpp의 &/&& 참조 선언 공백만 바꾸며 다른 포맷·이름·실행 로직은 보존한다.
클라이언트·Shared Protocol·외부 코드는 제외한다. JobQueue 성능 측정 후속 작업은 미착수 상태로 유지한다.
커밋·push는 수행하지 않는다.

완료: ServerCore/Resource.h에 공통 구현을 두고 양쪽 Resource.h는 구성 별칭만 유지한다.
Resource.cpp 2개와 프로젝트/filters 등록을 제거했으며 삭제 전 파일은 Git 제외 artifacts에 보존했다.
참조 선언은 기존 소스 10개 파일의 공백 183곳을 수정했고 C++ 토큰이 동일함을 확인했다.
새 ResourceTests.cpp는 context 풀 공유/격리·임대 중 Clear 거부·반환 후 재사용,
accept/reuse 세션 풀 분리·shared_ptr 소유·소켓 시간순 반환 등 12항목을 추가한다.
Debug/Release 서버 빌드·각 Core 126항목·Protocol ABI 110종/106상수 PASS.
각 구성에서 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종 PASS.
모든 통합 시나리오 종료에서 pending/sockets/leased=0을 확인했다.
에이전트 환경 정적 검사 PASS, compilation database 224개 소스.
UTF-8·프로젝트/filters 일치·clang-format·공백 검사 PASS.
Archify architecture showcase 9/9·오류/경고 0, 4크기·light/dark containment PASS 및 PNG 4장 직접 확인.
기존 형 변환·미사용 변수 경고는 유지했고 운영 DB·블록체인·실제 GUI 전투 전체·장시간 부하는 미검증이다.
성능 측정·커밋·push는 수행하지 않았다.

## 56. NetworkRuntime 명명·호환 래퍼·별칭 헤더 정리

- [x] TransportHost를 NetworkRuntime으로 변경하고 Start/Stop/Get으로 API 통일
- [x] SockAddr·NetworkRuntime using 선언을 두 서버 pch.h에 모으고 별칭 전용 헤더 4개 제거
- [x] 사용처·프로젝트/filters·문서 정리와 Debug/Release 빌드·Core/ABI·서버 회귀

NetworkRuntime 선언은 Winsock·IOCP를 정의하는 Net.h에 두며 Session.h의 서비스 접근도 새 이름을 사용한다.
단순 전달 Startup/Cleanup/Runtime과 사용처 없는 호환 오류 helper를 제거한다.
풀 주입·기본 포트·accept ID 기록을 담당하는 어댑터는 유지한다.
두 서버 pch.h는 현재 PrecompiledHeader=NotUsing인 공통 include 헤더다.
별칭은 서버 도메인 헤더보다 앞에 선언하고 Core는 서버 pch.h에 의존하지 않는다.
Archify 지침에 따라 현재 구조도를 대조했다. 표시된 모듈·연결·자원 수명이 동일하고
변경되는 클래스/호환 API는 구조도에 표시되지 않아 JSON/HTML은 재생성하지 않는다.
새 evidence JSON은 만들지 않으며 과거 검증 기록은 당시 snapshot으로 보존한다.
JobQueue 성능 측정·커밋·push는 수행하지 않는다.

완료: NetworkRuntime 선언을 Net.h로 이동하고 두 서버 호출부와 Core의 풀 어댑터를 새 API로 통일했다.
Startup/Cleanup/Runtime 전달 함수와 사용처 없는 GetLastError/PrintError를 제거했다.
SockAddr.h·SocketUtil.h 4개, 관련 include·프로젝트/filters 항목을 제거하고 pch.h에 using 선언을 모았다.
삭제 직전 파일은 artifacts/runtime-cleanup/baseline에 보존했으며 정리 문서에 복구 근거를 기록했다.
Debug/Release 서버 빌드·각 Core 126항목·ABI 110종/106상수 PASS.
각 구성의 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종 PASS.
통합 시나리오 각 7개에서 pending/sockets/leased=0 확인.
에이전트 환경 정적 검사 PASS, compilation database 224개 소스.
Core clang-format·UTF-8·프로젝트/filters 및 참조 파일 존재·삭제 include 잔여 없음·문서 링크·git diff --check PASS.
기존 C4065/C4101/C4244/C4305/C4309 경고는 로그에 유지했다.
운영 DB·블록체인·실제 GUI 전투 전체·장시간 부하 및 JobQueue 성능 측정은 미검증/미착수다.
새 evidence JSON·테스트 소스는 추가하지 않았고 커밋·push는 수행하지 않았다.

## 57. 작업·로그·자원 별칭 헤더를 pch.h에 통합

- [x] 두 서버 자체 헤더의 별칭 전용 파일 조사·PacketJobQueue 역할 확인
- [x] Job/JobQueue/LogUtil/OverlapEx/Resource 별칭을 pch.h로 이동하고 헤더 10개·등록·include 제거
- [x] 로비 PacketJobQueue 빈 파생 클래스를 JobScheduler 별칭으로 변경·전방 선언 수정
- [x] Debug/Release 빌드·Core/ABI·통합 회귀·삭제 참조 및 문서 확인

로비 PacketJobQueue는 대상 클라이언트 큐를 선택하는 Core JobScheduler의 기존 이름이다.
로비 worker는 Five budget·남은 작업 재등록·disconnect 정리를 사용하고,
게임은 IOCP 타이머의 매치 갱신 경로에서 CMatch의 Snapshot 큐를 처리한다.
이 실행 모델·동시성·소유권을 유지하고 선언 위치와 불필요한 빈 파생 타입만 정리한다.
OP_TYPE 정의 → Core 별칭/Resource 구성 → Global.h/도메인 헤더 순서로 의존성을 유지한다.
함수/클래스와 함께 있는 내부 별칭·TCPSocket 정책 어댑터·프로토콜 forwarding header·외부 코드는 유지한다.
Archify 지침과 구조도를 대조했으며 표시된 모듈·호출·수명 변화가 없어 JSON/HTML은 재생성하지 않는다.
삭제 전 파일은 Git 제외 artifacts/alias-cleanup/baseline에 보존하고 정리 문서에 근거를 남긴다.
새 evidence JSON·테스트 소스·성능 측정·커밋·push는 추가/실행하지 않는다.

완료: 두 서버 자체 헤더 177개를 조사했고 별칭 전용 10개를 제거했다. 남은 자체 헤더 167개에 별칭 전용 파일이 없음을 확인했다.
기존 전역 IJob/Job 및 wod_server 내부 JobQueue·로그·I/O context·풀 구성의 namespace를 유지했다.
PacketJobQueue는 로비 pch.h의 JobScheduler 별칭으로 바꾸고 Global.h의 class 지정자를 제거했다.
Core 구현·예산 정책·세대 검사·풀 소유·worker 종료 순서는 유지했다.
Debug/Release 서버 빌드·각 Core 126항목·ABI 110종/106상수 PASS.
각 구성의 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종 PASS.
각 7개 통합 시나리오에서 pending/sockets/leased=0 확인.
에이전트 환경 정적 검사 PASS, compilation database 224개 소스.
UTF-8·pch 선언 순서·프로젝트/filters 일치 및 참조 파일 존재·삭제 include/타입 잔여 없음·문서 링크·git diff --check PASS.
기존 C4065/C4101/C4244/C4305/C4309 경고는 로그에 유지했다.
운영 DB·블록체인·실제 GUI 전투 전체·장시간 부하는 미검증이고 JobQueue 성능 측정은 미착수다.
새 evidence JSON·테스트 소스·커밋·push는 추가/실행하지 않았다.

## 58. GameObject 기반·공통 위치/방향/속도 상태 추출

- [x] GameObject/CMoveObject와 로비 Transform/Physic 상태·동작·사용처 조사
- [x] Core의 타입 주입 GameObject/MoveObject 및 공통 상태를 추출하고 양쪽에 적용
- [x] DirectX 충돌 변환·타워/넥서스·로비 이동/시야 및 SetLook 차이 보존
- [x] 의미 있는 상태/가상 확장/충돌 위임 검사와 Debug/Release·ABI·통합 회귀
- [x] 구현/정리 문서·Archify JSON/HTML·검증 결과 기록

ObjectPosition·ObjectOrientation·ObjectMotion을 게임 객체와 로비 컴포넌트가 공유한다.
Core GameObject/MoveObject는 Geometry 타입 정책으로 벡터/box/matrix와 충돌 변환을 주입받는다.
Core는 DirectX·서버 pch·Protocol·게임 singleton에 의존하지 않는다.
GameObject.h/cpp의 타워·넥서스는 게임 서버에 유지하며 파일을 통째로 이동하지 않는다.
로비 SetLook 단순 대입과 게임 SetLook의 right 재계산, vec2 위치 변경 시 높이 보존을 유지한다.
ID 기본값은 기존 미초기화 상태 대신 -1로 명시한다. 기존의 유효 ID 지정 경로는 유지한다.
새 evidence JSON·성능 측정·커밋·push는 추가/실행하지 않는다.

완료: Core GameObject.h와 GameObjectTests.cpp를 등록했고 게임 기반 2종 및 로비 컴포넌트 2종에 적용했다.
DirectX 충돌 변환 문장 토큰은 이전 구현과 동일하며 타워/넥서스의 선언·구현 suffix도 그대로임을 비교했다.
GameObject.h/cpp는 콘텐츠·충돌 정책이 있어 유지하고 새 별칭 전용 헤더는 만들지 않았다.
새 검사 14항목: 기본 ID/매치/노드, 위치 높이 보존·3축 대입, 서버별 SetLook,
속도·객체별 상태 독립, 충돌 정책 인자·출력/미변경 행렬 항목, 가상 함수/소멸 경로.
Debug/Release 서버 빌드·각 Core 140항목·ABI 110종/106상수 PASS.
각 구성의 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종 PASS.
각 7개 통합 시나리오에서 pending/sockets/leased=0 확인.
에이전트 환경 정적 검사 PASS, compilation database 225개 소스.
신규 Core/로비 헤더 및 검사 clang-format·UTF-8·프로젝트/filters 일치·참조 파일·문서 링크·git diff --check PASS.
Archify architecture showcase 9/9·오류/경고 0, 4크기 containment·light/dark PASS, PNG 4장 직접 확인.
JSON/HTML 해시·byte를 delivery receipt 및 README와 일치시켰고 한국어 본문/영어 고정 Viewer UI를 유지했다.
기존 C4065/C4101/C4244/C4305/C4309 경고는 로그에 유지했다. 새 GameObject/컴포넌트 헤더와 검사에는 경고가 없었다.
운영 DB·블록체인·실제 GUI 전투 전체·장시간 부하는 미검증이며 JobQueue 성능 측정도 미착수다.
추가 evidence JSON·커밋·push는 생성/실행하지 않았다.

## 59. BasicClient를 SessionHandler로 명명 정리

- [x] 클래스·헤더·양쪽 CClient 상속·검사·프로젝트/filters 사용처 갱신
- [x] 기존 구현과 이름 외 토큰 동일성·삭제 참조·문서 확인
- [x] Debug/Release 서버 빌드·Core/ABI·환경 정적 검사

BasicClient는 클라이언트 상태를 소유하지 않고 Session의 수신 프레임 처리·disconnect를 조율한다.
실제 소켓 I/O를 수행하는 BasicSession과 구분해 SessionHandler로 명명한다.
Client.h는 SessionHandler.h로 이동하며 옛 이름의 별칭/forwarding header를 남기지 않는다.
소유권·가상 확장점·처리 순서와 동시성 경계는 유지하고 새 테스트 소스는 추가하지 않는다.
구조도에는 BasicClient/Client.h가 표시되지 않고 모듈·호출·수명 변화가 없어 Archify JSON/HTML은 재생성하지 않는다.
기존 Core 검사에 수신 분할/연속·검증/전달 실패·세대 검사가 있어 해당 검사를 재실행한다.
네트워크/매칭 통합은 앞선 GameObject 추출 결과로 보존하고 명명 정리에서는 반복하지 않는다.
새 evidence JSON·성능 측정·커밋·push는 추가/실행하지 않는다.

완료: SessionHandler.h로 이동하고 클래스·양쪽 CClient 상속·기존 검사 및 프로젝트/filters 참조를 갱신했다.
이름/경로 치환 외 Core 헤더와 코드 사용처는 변경 전 byte와 일치한다.
Server 소스·프로젝트에 BasicClient 및 ServerCore/Client.h 참조가 남지 않음을 확인했다.
Debug/Release 서버 빌드·각 Core 140항목·ABI 110종/106상수 PASS.
에이전트 환경 정적 검사 PASS, compilation database 225개 소스.
clang-format·UTF-8·프로젝트/filters 일치·참조 파일 존재·git diff --check PASS.
기존 C4065/C4101/C4244/C4305/C4309 경고는 유지되며 명명 변경으로 새 경고는 발생하지 않았다.
네트워크/매칭 통합은 이번 명명 정리에서 재실행하지 않았다. 운영 DB·블록체인·실제 GUI 전투 전체·장시간 부하도 미검증이다.
JobQueue 성능 측정·추가 evidence JSON·커밋·push는 수행하지 않았다.

## 60. 현재 변경 전체 커밋·push

- [x] 저장소·브랜치·전체 변경과 기존 검증 결과 확인
- [x] 전체 변경 스테이징·차이 검사·한글 커밋 메시지 및 snapshot 준비

사용자가 현재 변경 전체의 커밋과 push를 요청했다. Git 제외 artifacts·빌드·로그는 포함하지 않는다.
commit 스킬의 명시된 2단계 절차에 따라 스테이징한 변경과 정확한 메시지를 먼저 제시한다.
미리보기 승인 후 커밋·origin/main push를 진행하고, 실제 결과는 Git 이력·원격 commit·작업 트리와 완료 보고로 확인한다.
직전 검증: Debug/Release 서버 빌드·각 Core 140항목·ABI 110종/106상수·환경 정적 검사 PASS.
GameObject 추출 시 실행한 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종도 양쪽 구성 PASS.
이후 SessionHandler 명명 정리에서는 Core/ABI·서버 빌드를 재검증했고 통합 검사는 반복하지 않았다.
기존 C4065/C4101/C4244/C4305/C4309 경고는 유지되며 운영 DB·블록체인·전체 GUI 전투·장시간 부하는 미검증이다.

## 61. 시나리오 더미·실시간 로컬 서버 관측 첫 구현

- [x] 기존 Stress_Test·CDummyClient·회귀 harness·Core 계측 지점 조사
- [x] 새 시나리오 runner·단계별 assertion/timeout/취소·JSON/CSV 결과 구현
- [x] 선택 활성화 서버 계측·별도 snapshot 채널·프로세스 자원/네트워크/작업 지표 구현
- [x] 로컬 웹 대시보드·시나리오 실행/중지·시계열/실패 단계·결과 다운로드 구현
- [x] 실패/분할 수신/취소/재실행 및 실제 Debug/Release 서버와 도구 통합 검증
- [x] 사용 문서·Archify JSON/HTML·실제 검증 및 한계 기록

사용자는 설계안에 그치지 않고 새 더미와 로컬 웹 대시보드의 첫 구현을 선택했다.
기존 Stress_Test·빈 CDummyClient를 기반으로 확장하지 않고 새 도구를 만든다.
첫 범위는 실제 로그인·4인 매칭·직업/스킬 선택·게임 진입·이동/정지·스킬 응답 검증이다.
Python 표준 라이브러리 runner는 Shared/Protocol ABI manifest로 지원하는 wire codec을 검증한다.
서버 관측은 명시적으로 켜고 게임 wire와 별도 로컬 snapshot을 사용한다. 지표 수집은 게임 worker에서 파일/HTTP I/O를 하지 않는다.
화면의 위치는 더미가 실제 수신한 플레이어 상태이며 전체 서버 월드 snapshot으로 표시하지 않는다.
JobQueue 알고리즘 개선/전후 성능 측정, 운영 DB·블록체인·전투 전체 검증, 커밋·push는 이번 범위에 포함하지 않는다.

구현: Python 표준 라이브러리 CLI·웹 공용 runner, 3개 시나리오, 역할별 행동·seed·실행 조건·32인 상한을 추가했다.
현재 로그인·매칭·선택·READY·로딩·이동·정지·기본 공격 응답 및 로비 재접속을 검사한다.
게임 4인 그룹 준비는 순차, 준비된 그룹 행동은 병렬/순차 선택이다.
실행 중지 후 모든 비동기 작업·소켓을 정리하며 실패/취소 결과도 보관한다.
Core TelemetryReporter가 선택 활성화된 JobMetrics·Windows 프로세스·네트워크 snapshot을 별도 스레드에서 내보낸다.
웹은 1초 갱신 그래프·수집 신선도·단계 지연·자신의 X/Z 위치·타임라인·최근 결과 다운로드를 제공한다.
계측 부하/순간 카운터의 근사값·RTT와 다른 단계 지연·전체 월드가 아닌 위치 관측을 사용 문서에 명시했다.

실제 검증 중 로비 IOCP와 content worker의 중복 ClientReset으로 배열 assertion을 재현했다.
정리는 content worker에만 예약하고 JobTarget의 한 번 정리 claim을 추가했다. 재등록·재사용 claim 회귀 3항목을 추가했다.
Core Debug/Release 각 151항목(기존 140+계측 8+정리 3)·ABI 110종/106상수 PASS.
Debug/Release 서버 빌드·기존 네트워크 12종·보스 4/5 매칭·초기화 실패 4종 PASS.
각 구성 7개 기존 통합 시나리오에서 pending/sockets/leased=0을 확인했다.
Python 12검사에서 ABI 불일치·잘못된 시나리오·분할/합침·잘못된 프레임/EOF·timeout·이동 응답 5,000개·취소·수집 중단/재시작·로컬 HTTP 제한을 검증했다.
Server Lab Debug/Release API 5시나리오(4인 로비·4/8인 게임·중복 거부 및 실행 중 취소·취소 후 로비 재실행) PASS.
게임 4/8인 행동 판정은 각각 48/96개 통과. 결과 JSON의 서버 시계열과 CSV·정상 최종 표본 및 종료 잔여 자원 0을 확인했다.
계측 I/O 실패 주입도 Debug/Release PASS: 로비 로그인·재접속 및 양쪽 서버 정상 종료가 가능하고 export 오류는 프로세스당 한 번 기록했다.
브라우저에서 실제 실행·중지와 그래프/타임라인·성공 상태를 확인했다. 최종 화면 검증은 아래 후속 결과에 기록한다.
환경 정적 검사 PASS, compilation database 226개 소스. 기존 C4065/C4101/C4244/C4305/C4309 경고는 유지한다.
Archify showcase 9/9·오류/경고 0, 4크기 containment 및 light/dark PNG 4장 직접 확인 PASS.
server-lab의 최종 JSON/HTML hash·byte는 delivery.json과 README에 기록하고 텍스트 정규화를 제외했다.
이번 작업에서 만든 미구현 초안 docs/diagrams/server-test-plan은 최종 server-lab 구조도로 대체해 제거했다.
초안은 미추적 생성물로 코드/외부 참조가 없고, 최종 JSON·HTML·재현 절차가 복구 근거다. 원본 저장소는 수정하지 않았다.
추가 architecture/evidence JSON·커밋·push는 만들거나 실행하지 않았다.

최종 브라우저 검증: Release 로비 이동의 정지 실패 단계 표시 → 게임 4인 재실행 48/48 통과·연결 0 확인.
완료된 최신 run_id의 JSON/CSV 링크 자동 선택·실제 CSV 다운로드, 그래프 6개·현재 973px 화면의 가로 overflow 0·console 오류/경고 0 확인.
화면 캡처는 artifacts/logs/server-lab/dashboard.png에 두며 Git 제외다.
Python 자동 탐색은 Windows 설치 alias를 건너뛰고 설치된 런타임을 재사용한다. 기본 인자의 Release 통합도 PASS.
Core clang-format·엄격 UTF-8·프로젝트/filters·solution 참조·JS/PowerShell 구문·문서 링크·JSON/HTML 해시·git diff --check PASS.

## 62. Server Lab 후속 관측·서버 동작 개선

- [ ] 72번에서 재현한 로비 이동/정지 실패·idle busy loop·작업/대상별 기아의 원인별 개선
- [ ] 게임 참가자 종료 뒤 매치 업데이트가 닫힌 세션에 계속 전송하는 경로·매치 종료/타이머 수명 검토
- [x] enqueue→실행 대기·tick 지연 histogram·미실행 패킷 나이·계측 끔 CPU 대조·빌드/환경/소스 hash 기록 (72번)
- [ ] 대상별 큐 공정성·lock 대기·게임 tick 내부 비용·매치별 입력/응답/전송 대기 관측 확장
- [ ] 64명 게임의 60명 이동 응답 누락과 match-transfer 준비 EOF 원인 조사·회귀 추가 (72번)
- [ ] 웹 일정의 첫 8그룹 고정 연결을 서버 실제 매치 수로 확장 (64명 성능·더미 검증은 16매치 전체 포함)
- [ ] 로비 최초 Move의 m_updateTime 초기값으로 발생하는 과대 간격/누적 histogram overflow 처리 검토
- [ ] 서버 소유 스레드에서 접속자/매치/NPC 지표·월드 snapshot을 내보내고 관측 화면 확장
- [x] NPC ADD 종류의 `UNIQUE_DRAGON` 고정·MOVE/REMOVE의 `MINION` 기본값과 실제 종족 계약 정리 (65번)
- [ ] NPC 근처에 타깃을 배치하는 전투 시나리오·AI/충돌/애니메이션 완료 ACK 검증 (일반 몬스터의 타깃 부재 대기와 66번의 미니언 이동 오류를 구분)
- [ ] 피해·명중·쿨다운 거부·사망/부활·승패·게임 재접속 assertion 및 장시간/최대 동시 접속 검증

첫 구현에서 로비 병렬 이동/정지는 실제 timeout을 기록한다. 기본 로비 생명주기 검사와 별도 진단 시나리오로 제공한다.
게임 4/8인 행동은 통과했지만 참가자 종료 후 I/O 오류 누적 증가도 관찰해 정상 플레이 전체 통과로 해석하지 않는다.
첫 구현 당시에는 계측을 켠 실행에서만 얻은 관찰이었다. 이후 72번에서 계측 켬/끔 기준을 측정했으며 JobQueue의 개선 전후 비교는 아직 아니다.

## 63. Server Lab 실제 지형·클라이언트 위치 탭

- [x] 서버 HeightMesh/NavMesh 경로·좌표 확인 및 고정 맵 API·높이 영상 캐시 구현
- [x] 지표/지형 탭·동일 좌표 위치 겹침·맵/매치 선택·확대/이동 구현
- [x] 지형 파서/투영/HTTP 실패 경로와 실제 에셋·브라우저·더미 동작 검증
- [x] 사용 문서·Archify 구조도·실제 검증과 한계 갱신

서버가 읽는 로비 HeightMesh2.obj/NavMeshData2.obj와 게임 HeightMesh.obj/NavMeshData3.obj를 사용한다.
원본 월드 X/Z를 보존하고 지형의 가장 높은 표면을 높이색으로 표시한다. GUI 텍스처·전체 월드 snapshot은 범위 밖이다.

완료: terrain.py가 전체 HeightMesh 삼각형을 512² PNG로 투영하고 원본 NavMesh X/Z 삼각형·bounds·SHA-256을 고정 API로 제공한다.
로비 124,834면·NavMesh 445면, 게임 127,756면·NavMesh 5,743면을 확인했다. 원본 메시·서버 코드·wire는 변경하지 않았다.
monitor/브라우저 캐시로 매초 지형을 재전송하거나 파싱하지 않는다. 지표/지형 탭·자동/수동 맵·매치 그룹·좌표 표·확대/이동·종료 위치 표시를 구현했다.
Python 16검사(기존 12+지형 4) PASS: 실제 에셋·인덱스/LFS/비정상 좌표·면·높이 보간/겹침/+Z·PNG alpha/CRC·캐시·HTTP allowlist/누락 실패.
실제 Release monitor/API 7검사(기존 5+양쪽 지형) PASS. 4/8인 게임 48/96 assertion, 로그인·재접속·중복 거부·취소 후 재실행·JSON/CSV PASS.
브라우저에서 양쪽 지형·자동 게임 전환·8명→매치별 4명 필터·확대·드래그·NavMesh 토글·키보드 탭 전환·4인 48/48 결과 및 연결 0을 확인했다.
973px 화면 가로 overflow 없음·console 오류/경고 없음. 최종 지형 화면은 artifacts/logs/server-lab/terrain-dashboard.png(Git 제외)에 저장했다.
Archify architecture showcase 9/9·오류/경고 0, 4크기 containment와 light/dark 4장 직접 점검 PASS. 새 receipt/hash는 README에 반영했다.
JS 구문·엄격 UTF-8·solution 파일 참조·delivery byte/hash·git diff --check PASS.
검증 뒤 기존 게임 매치의 종료 후 전송 누적과 pending/메모리 증가가 다시 관찰되어 양쪽 서버만 정상 종료/재시작했다. monitor의 마지막 실행·위치는 보존했다.
서버 정상 종료/재시작 스크립트 성공과 재시작 후 idle 관측을 확인했다. 이번 재시작의 이전 종료 로그는 시작 시 교체되어 별도 자원 0 증거로 보존하지 않았다. 관련 서버 개선은 62번으로 유지한다.
이번 범위는 Python/웹/문서이므로 C++ 빌드·Debug 서버 통합은 반복하지 않았다. 지형 영상은 충돌 판정 검증·3D 텍스처·NPC/전체 월드 관측·장시간 부하를 대체하지 않는다.
추가 architecture/evidence JSON·커밋·push는 실행하지 않았다.

## 64. Server Lab 더미 이동·NPC 관측

- [x] NPC wire·ID·매치 경계와 실제 전송 경로 확인
- [x] 제한된 수신 관측 저장소·더미/NPC 궤적·방향/HP/공격/제거·별도 월드 API 구현
- [x] 지형 위 실시간 이동·객체 선택/추적·매치/레이어 필터·관측 상태 표시 구현
- [x] codec/중복/재접속/분할/잘못된 응답·실제 서버/API/브라우저 검증
- [x] 사용 문서·Archify 원본/산출물·검증 결과/한계 갱신

기존 게임 wire에서 웹 더미가 수신하는 정보를 시각화한다. 서버 FSM 전체/공격 목표는 패킷에 없으므로 추정해 확정 상태로 표시하지 않는다.
지표 갱신과 월드 갱신을 분리하고, 매치별 대표 수신자를 선택해 중복 관측을 합친다. 입력/패킷/트레일/객체/사건 저장은 상한을 둔다.

완료(2026-10-11 KST): observation.py가 더미 자신의 위치/방향/스킬 및 대표 수신자의 NPC 생성·이동·HP·공격·제거를 보관한다.
매치별 NPC ID를 분리하고 대표 연결 종료 시 다음 수신자로 전환한다. ADD 종류를 유지하고 제거 뒤 지연 MOVE로 재생성하지 않는다.
궤적은 객체당 120점·최소 100ms 간격, NPC는 그룹당 128개, 사건은 100개로 제한한다. 종료 JSON에 제한된 world를 추가했다.
별도 /api/world를 지형 탭에서 0.2초마다 요청하고 0.2초 위치 보간·최근 15초 궤적·방향·공격 원·HP·객체 선택/추적·레이어/매치 필터를 구현했다.
지표 1초 갱신은 유지한다. 비활성 탭은 월드 요청/애니메이션을 중단하고 지형 배경은 캐시한다. 기본 world-watch 시나리오는 약 40초다.

Python 23검사(기존 16+월드 7) PASS. ABI 필드/ADD 순서·매치 분리/중복·대표 전환·이동/공격/HP/제거/재생성·저장 상한·snapshot 분리,
길이/NaN/ID 거부·맵 전환 초기화·TCP 분할 및 NPC MOVE 5,000개 뒤 로그인 응답 보존을 확인했다.
실제 Release API 7검사 PASS: 양쪽 지형, 4인 로비, 4/8인 게임 48/96단계, 중복 거부·취소·재실행·JSON/CSV·NPC 그룹 분리·종료 후 대표 해제.
짧은 기본 시나리오의 NPC 이동 발생은 보장할 수 없어 통과 조건으로 강제하지 않는다. 실제 위치 수신과 궤적 보관을 검사하고 NPC 이동 기능은 패킷 주입 단위 검사로 확인했다.
브라우저 world-watch 4인 52/52·8인 104/104 통과·연결 0, 더미 8명/NPC 26개→각 매치 더미 4명/NPC 13개 표시를 확인했다.
더미 이동 궤적·NPC HP/생성·객체 추적·레이어 전환·종료 상태·973px 가로 overflow 0·console 오류/경고 0 확인.
관측에서 NPC 좌표 변화·공격 응답은 없었으며 서버의 일반 몬스터 ADD 종류 고정도 확인했다. 일반 몬스터의 타깃 부재 대기는 정상일 수 있으나, 미니언 경로 이동 정지는 별도 오류임을 66번에서 확인·수정했다. 종류 전송은 65번에서 수정했으며 근처 타깃 전투/피해·NPC 애니메이션 ACK는 62번 후속 범위다.
실제 관측/화면/통합/종료 로그는 artifacts/logs/server-lab/world-*에 저장하며 Git 제외다.
검증 뒤 기존 종료 매치 전송 누적을 피하도록 서버만 정상 재시작했다. 두 종료 모두 pending=0 sockets=0 leased=0 로그를 보존하고 재시작 후 I/O 오류 0·idle 관측을 확인했다.
Archify showcase 9/9·오류/경고 0, 4크기 containment·light/dark 4장 직접 점검·delivery SHA/byte 일치 PASS.
JS 구문·엄격 UTF-8·solution 참조·문서 링크·git diff --check PASS. Python/웹 변경으로 C++ 빌드·Debug 서버 실행은 반복하지 않았다.
운영 DB·블록체인·실제 전체 전투/GUI 애니메이션·전체 월드/리플레이·장시간 부하는 미검증이다. 커밋·push하지 않았다.

## 65. 로비 NPC·종류별 표시·서버 일정 타이머

- [x] 로비 고정 NPC 원본·게임 NPC 종류·웨이브/게이트 일정과 실행 경로 확인
- [x] 로비 NPC 위치와 종류별 기호/색상/이름/범례 표시
- [x] 게임 NPC 종류 전송 수정·기존 매치 업데이트에서 타이머 snapshot 발행·별도 reporter I/O 구현
- [x] 매치별 게임 경과·다음 웨이브 검사/예약 스폰·자기장 게이트 해제 카운트다운·오래된 관측/종료 표시
- [x] 단위·서버 빌드/실제 매치·브라우저 검증 및 문서/구조도 갱신

사용자 설명에 따라 근처 타깃이 없던 일반 몬스터의 이동/공격 미발생은 정상일 수 있는 조건으로 정정한다. 이후 미니언의 경로 이동 정지는 별도 서버 오류임을 확인해 66번에서 수정했다.
게임 규칙·패킷 배치는 유지하고, 표시할 상태는 기존 매치 업데이트 실행 경로가 복사한 값과 로비 클라이언트 원본에서 얻는다.
예약 스폰·웨이브 검사와 실제 생성 완료를 구분하며 수집 지연/종료 뒤에는 타이머를 계속 진행시키지 않는다.

완료(2026-10-11 KST): 로비 서버에는 활성 NPC/broadcast가 없고 클라이언트 Scene.h의 고정 배치 4명이 실제 원본임을 확인했다.
상점 N5·경매 N6·블록체인 N7·꾸미기 N8을 원본 좌표/SHA와 함께 표시하며 고정 출처·HP/공격 미수신을 구분한다.
게임 NPC 8종은 실제 ENpcType→NPC_TYPE으로 ADD/MOVE/REMOVE에 전송한다. wire 상수·크기·packing은 유지한다.
지도에 종류별 모양·색상·약어와 범례, 선택 목록/표의 이름을 적용했다.
기존 IOCP worker의 CMatch 업데이트 경로에서 1초마다 값 복사본을 MatchTelemetry에 발행하고 reporter는 잠금 보호 복사본만 직렬화한다.
개별 예약 시각/수집 주기는 atomic, 복사본 발행/읽기는 mutex로 보호한다. 기존 매치 실행 구조/동시성 경계 전체를 재설계하지 않았다.
로그인 이름 byte 4개의 정확한 집합으로 현재 실행의 그룹과 서버 match ID를 연결한다. 식별용 key는 브라우저/결과에서 제외한다.
현재 CSV는 38초 웨이브 검사·2/4/6/8초 개별 예약·활성 상한 12마리이며 게임 시간 180초 게이트 기준은 CGameMgr의 공유 상수에서 발행한다.
표시에서 웨이브 검사·실제 예약·서버 해제 상태를 구분하고 수집 지연/연결 해제/실행·매치·서버 종료 뒤에는 마지막 표본에서 고정한다.
190초 관측하는 timeline-watch 시나리오와 선택적인 integration.py --timeline 검사를 추가했다.

Debug/Release 서버 빌드 PASS(기존 타입 변환·미사용 변수 등의 경고 유지). Core 각 155항목·ABI 110종/106상수 PASS.
Debug 기존 네트워크 12종·보스 4/5의 4인 매칭·초기화 실패 4종 회귀 PASS.
Python 26검사 PASS: 로비 원본 4좌표/누락·개수·NaN 거부, 정확한 그룹 연결·분리·잘못된 일정/중복 슬롯·지연/종료/서버 재시작 고정,
게이트 서버 상태·수용 상한을 확인했다. C++ 검사는 복사본 분리·동시 발행/읽기·제어 문자 이름 byte·매치 재등록 초기화를 포함한다.
Release 실제 API 8검사 PASS: 기존 7개 회귀에 NPC 종류 0~7/매치별 일정 확인과 190초 일정 관측을 추가했다.
실제 미니언 예약→4/8/12마리 활성화와 게임 180,175ms 시점 fence=false를 확인했다. 종료 타이머 live=false와 JSON/CSV 저장도 확인했다.
종료 양쪽 pending=0 sockets=0 leased=0 로그는 artifacts/logs/server-lab/npc-integration-stop-*.log에 보존했다.
브라우저에서 로비 4종 고정 배치·게임 8종 범례/기호·실시간 예약/카운트다운, 973px 가로 overflow 0·console 오류/경고 0을 확인했다.
브라우저의 실제 180초 이후 `해제됨 · 서버 확인` 화면은 artifacts/logs/server-lab/npc-timers.png에 저장했다(Git 제외).
브라우저 일정 시나리오도 32/32·연결 0으로 완료했다. 양쪽 종료 자원 0 로그(npc-browser-stop-*.log)를 보존하고 서버만 재시작해 idle·I/O 오류 0을 확인했다. monitor의 마지막 지도/일정은 유지했다.
Archify showcase 9/9·오류/경고 0, 4크기 containment·light/dark PNG 4장 직접 점검·JSON/HTML byte/hash PASS.
사용/구조 문서·인덱스·구조도/receipt 갱신, UTF-8 54개 파일·문서 링크·JS 구문·환경 정적 검사(226소스)·git diff --check PASS.
NPC 근처 타깃 전투/애니메이션 ACK·피해/명중·전체 GUI 플레이·운영 DB/블록체인·최대 32인 부하/성능 전후 비교는 미검증이다.
기존 종료 매치의 전송 누적 개선은 62번에 남긴다. 추가 architecture/evidence JSON·커밋·push는 실행하지 않았다.

## 66. 미니언 이동 정지 원인 및 회귀 검증

- [x] 스폰 이후 서버 이동/수신 좌표와 CSV→런타임 데이터 경로 조사 및 수정 전 재현
- [x] 최대 이동 속도 유실 수정·실제 미니언 이동 회귀 추가
- [x] 서버 빌드·수정 전후 실제 이동/경로 진행·웹 표시·관련 회귀 검증
- [x] 사용 문서와 실제 검증/미검증 범위 기록

미니언은 근처 플레이어가 없어도 경로를 따라 건물로 이동해야 한다. 일반 몬스터의 타깃 부재 대기와 구분한다.
기존 Server Lab 변경을 유지하며 원인과 직접 관련된 데이터 초기화만 우선 수정한다. 커밋·push는 요청받지 않았다.

원인: npc_info.csv의 미니언 MaxSpeed는 4이고 reflection loader도 읽지만, NpcCsv 생성자의 수동 필드 복사에서 누락해 런타임 값은 0이었다.
CMinion/CMonster가 이 값을 m_maxVelXZ로 사용하므로 이동 속도가 0으로 제한되었다. NpcCsv의 기반 tabledata::NpcInfo 전체 복사로 원본 필드 유실을 방지했다.
버프/좌표 문자열 파싱·이동 알고리즘·충돌·전투/애니메이션 ACK·wire 형식은 변경하지 않았다. 일반 몬스터에도 같은 속도 수정이 적용된다.
이전 NPC 정지 전체를 타깃 부재로 간주하던 설명을 수정하고 일반 몬스터의 조건부 대기와 미니언의 자율 경로 이동을 구분했다.

실제 수정 전 Release에서 N0가 스폰 후 약 19초간 (4.26, 3.50)에 머무르고 trail 1점인 것을 재현했다(minion-before.json).
수정 후 첫 웨이브 N0~N3는 약 112~134 units 이동했고 모두 120점 궤적을 저장했다. N0는 첫 경유지(-44, 4.166667)를 지나 최종 관측 (-129.76, 1.50)까지 진행했다.
브라우저에서 네 미니언의 순차 이동·궤적·좌표/이동 관측과 객체 추적을 확인했다. 화면은 artifacts/logs/server-lab/minion-movement.png(Git 제외)에 보존했다.
integration.py --minions는 기존 7개 회귀 뒤 첫 웨이브 4마리의 유한한 좌표·4 units 초과 이동·종료/저장을 검사한다.
Release 실제 통합 8개 PASS, Debug/Release 서버 빌드 PASS. 기존 타입 변환·미사용 변수 등의 경고는 유지한다.
검증 결과/수정 전 관측은 artifacts/logs/server-lab/minion-*.json에 저장한다.
Python 26검사·UTF-8·git diff --check PASS. 검증 뒤 두 서버의 pending=0 sockets=0 leased=0 로그를 보존하고 서버만 재시작했다. monitor의 마지막 궤적은 유지하고 새 서버 idle·I/O 오류 0을 확인했다.
전체 경로 완주·모든 경로 선택·전투/공격 애니메이션 ACK·재스폰 초기화 전체·운영 DB/블록체인·성능/최대 부하는 이번 검증 범위가 아니다.
구조·스케줄링 경계 변경 없이 데이터 초기화 오류를 수정했으며 추가 다이어그램/evidence JSON·커밋·push는 만들지 않았다.
## 68. Server Lab 커밋 준비·에이전트 API 접근 확인 (2026-10-11)

- [x] Server Lab·계측·NPC 표시/일정·미니언 이동 수정과 별도 Serena HTTP 변경 범위 구분
- [x] 커밋 전 Python 26검사·Release Core 155검사·ABI 110종/106상수·diff 검사
- [x] 에이전트용 API 목록·PowerShell 조회/실행 예제·관측 한계 문서화
- [x] 임시 로컬 HTTP 서버에서 지표/월드/시나리오/양쪽 지형·저장 결과 조회 확인

사용자가 커밋·push를 미리 승인했다. Server Lab 관련 변경만 커밋 대상으로 하며 별도 Serena HTTP 설정/스크립트/구조도와 67번 기록은 작업 트리에 보존한다.
이전 Debug/Release 서버 빌드·실제 미니언 이동 8개 통합·190초 일정 검증 결과를 확인했으며 이번 커밋 준비에서 다시 실행했다고 주장하지 않는다.
API 조회는 같은 PC에서 실행 중인 monitor에 가능하다. 커밋 준비 시 8790은 중지 상태였으므로 임시 포트의 같은 HTTP handler로 읽기만 검사했다.
실제 저장 결과에서 NPC 13개·매치 일정 1개를 읽었고 현재 계측 파일은 stale로 확인했다. 임시 HTTP 서버는 종료했다.
전체 서버 월드·AI FSM/타깃·명중·외부 참가자 위치·전체 시간 월드 리플레이는 현재 관측 범위 밖이다.
로그·실행 결과·스크린샷은 Git 제외 상태를 유지한다. 기존 컴파일 경고·운영 DB/블록체인 및 전체 전투 미검증 범위도 유지한다.

## 69. 에이전트 서버 성능 검사·계측/시나리오 확장 지침 (2026-10-11)

- [x] 기존 Server Lab API·지표·시나리오·보존 상한과 후속 성능 과제 확인
- [x] 필요한 정보의 직접 조회와 부족한 계측의 도구/API/결과 확장 기준 문서화
- [x] 검사 목적에 맞는 시나리오 추가·검증 및 동일 조건 전후 측정 지침 작성
- [x] 작업 지침/문서 인덱스·Server Lab 연결, UTF-8·링크·diff 검사

docs/guides/SERVER_PERFORMANCE.md를 추가했다. API 원본/저장 결과 조회, 정보 누락 시 소유 경로의 안전한 계측과 재사용 가능한 조회/저장 확장을 기본으로 한다.
JobQueue 대기/공정성·tick 지연·실제 수용 부하·부하 생성기 포화·종료 후 비용·장시간 보존의 현재 미구현 항목을 구분했다.
성능 검사마다 시나리오 적합성을 확인하고 필요한 행동/부하/시간이 없으면 시나리오와 runner를 보완하도록 했다.
동일 입력/계측·워밍업/측정/정리 구간·반복·실패율/꼬리 지연·계측 비용·원시 결과를 기록하며 기능 회귀를 벤치마크로 해석하지 않는다.
이번 작업은 문서 지침 추가다. 새 계측/시나리오 구현·성능 측정·게임 실행·빌드/회귀 재실행·커밋/push는 수행하지 않았다.
기존 Serena HTTP 관련 미커밋 변경은 보존했다. 구조/실행 흐름을 변경하지 않아 다이어그램은 갱신하지 않았다.
