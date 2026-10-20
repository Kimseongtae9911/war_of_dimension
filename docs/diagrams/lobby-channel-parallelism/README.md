# 로비 채널 병렬 실행 소유권

## 현재 구현

2026-10-19 명칭 정리: 이전 `Inbox`는 현재 코드에서 `SessionWorkQueue`다. 멤버 `m_workQueue`, 정리 함수 `ClearWorkQueue/ClearWorkQueues`를 사용한다. 큐의 순서·잠금·실행 경계는 유지한다.

[구현 내용·검증](../../development/LOBBY_CHANNEL_PARALLELISM.md) · [구현 구조도](implementation.html) · [구현 JSON](implementation.architecture.json)

유저 SessionWorkQueue는 선두 한 작업만 채널/전역 큐에 발행하고, 완료 후 소유 채널을 다시 읽어 다음 작업을 발행한다. 공통 worker pool의 일반 채널 callback은 공유 barrier, 기존 전역 기능은 배타 barrier에서 실행한다. 아래의 이전 제안과 달리 별도 routeEpoch/비동기 전역 결과 서비스는 도입하지 않았다. 그림은 주 실행 경로이며 신규 접속/서버 메시지가 전역 큐에 직접 들어가는 보조 경로는 상세 구현 문서에 설명한다.

근거: [LobbyDispatcher](../../../Server/Lobby_Server/LobbyDispatcher.h), [CClient](../../../Server/Lobby_Server/CClient.cpp), [CServer](../../../Server/Lobby_Server/CServer.cpp), [핸들러 분류](../../../Server/Lobby_Server/CPacketMgr.cpp).

### 구현 의도와 선택 근거

**채널 큐는 공유 상태의 실행 소유권을, SessionWorkQueue는 여러 큐를 오가는 한 유저의 입력 순서를 보호한다.** 전역 배타 barrier는 채널 간 상태 변경의 겹침을 막으며 큐 사이 FIFO를 대신하지 않는다. 그림의 하단 카드에도 선택 이유와 전역 작업의 비용을 표시했다.

[상세 설계 선택 이유](../../development/LOBBY_CHANNEL_PARALLELISM.md#설계-선택-이유)에서 각 선택의 문제·대안의 한계·보장 규칙·비용·코드 근거, `이동1→채널 변경→이동2` 실패 예시, SessionWorkQueue mutex와 실행 잠금의 차이를 설명한다. 공통 worker/채널 큐의 채택을 성능 개선의 증명으로 설명하지 않으며 [밀집 재측정](../../development/LOBBY_CHANNEL_PARALLELISM.md#채널당-인원-증가-재측정)의 CPU·송신 적체·정지 실패도 함께 확인한다.

2026-10-19 명칭 정리 반영: Archify 2.16.0, `architecture`, showcase 9/9·오류/경고0. JSON 3438 bytes / SHA-256 `9009afd2160a69eea830c53abad9d97b9493a5f362d0584658e75b775d158727`, HTML 706492 bytes / SHA-256 `74d79913884e0d3aea548ba6c4ab2710f1c90fd2f0f17202b8283ba6490f46f5`. receipt는 Git 제외 `artifacts/logs/lobby-work-queue-name-{validation,delivery}-20261019.json`이다. JSON은 최종 검증/전달 후 수정하지 않았다. 이전2026-10-18 및 설명 카드 보완 receipt는 당시 버전의 근거로 보존한다.

현재 정리는 코드 명칭·문서·표시 이름을 변경했으며 서버 동작·이전 제안도·실측 결과는 바꾸지 않았다. 본문은 한글이며 고정 Viewer UI와 `<html lang>`는 지원 기본값인 영어다. 기존 로컬 파일 접근 제한을 우회하지 않아 `visual-check`와 실제 브라우저 배치/가독성 검사는 미수행이다. 자동9항목 검증은 시각 검증을 대신하지 않는다.

이 디렉터리의 JSON/HTML은 `.gitattributes`에서 `-text`로 관리한다. 검증 후 고정한 원본 바이트와 SHA-256이 Git 줄바꿈 변환으로 달라지지 않도록 하며, stage의 blob도 전달 receipt와 대조한다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/lobby-channel-parallelism/implementation.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/lobby-channel-parallelism/implementation.architecture.json docs/diagrams/lobby-channel-parallelism/implementation.html --quality showcase --json
```

## 이전 검토안

2026-10-18 · **미구현 제안**. 현재 전역 로직 worker 1개를 표현한 구현 구조도가 아니다.

- [검토 내용·기존 결함·검증 계획](../../development/LOBBY_CHANNEL_CONCURRENCY.md)
- [제안 구조도 HTML](proposal.html)
- [수정 원본 JSON](proposal.architecture.json)

같은 채널은 한 번에 하나의 worker가 처리하고, 서로 다른 채널은 제한된 공통 worker pool에서 병렬 실행한다. IOCP·타이머·가입/종료도 라우팅 경계를 통과한다. 정적 지형은 공유하며, 채널 간 매칭/이동은 전역 조정자와 채널 소유자 사이의 메시지로 처리한다. 그림은 주 소유권 경로만 표현하며 모든 응답 왕복/실패 절차를 나열하지 않는다.

근거는 [CServer::Run](../../../Server/Lobby_Server/CServer.cpp), [CUserMgr](../../../Server/Lobby_Server/CUserMgr.cpp), [CClient](../../../Server/Lobby_Server/CClient.cpp), [CViewList](../../../Server/Lobby_Server/CViewList.cpp), [CMatchMgr](../../../Server/Lobby_Server/CMatchMgr.cpp), [Core 큐](../../../Server/ServerCore/include/ServerCore/Concurrency.h)다. 새 구성요소 이름은 제안 역할이며 현재 클래스의 구현 사실을 뜻하지 않는다.

Archify 2.16.0으로 `architecture`, `showcase` 검증/전달을 완료했다. 9/9 검사 통과, composition 오류/경고 0이다. 원본은 최종 검증 뒤 변경하지 않았다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate architecture docs/diagrams/lobby-channel-parallelism/proposal.architecture.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver architecture docs/diagrams/lobby-channel-parallelism/proposal.architecture.json docs/diagrams/lobby-channel-parallelism/proposal.html --quality showcase --json
```

| 산출물 | bytes | SHA-256 |
|---|---:|---|
| JSON | 2471 | `6ffd5d443778d9191ca9298bc239b92dfb9ed71fa95027d34b8f9921d119a207` |
| HTML | 708682 | `f14e5b77d7fe784fe38223d536f4380a6507795c8efcf2e99fc63624b7adb3b3` |

receipt는 Git 제외 `artifacts/logs/lobby-channel-diagram-{validation,receipt}-20261018.json`에 보존한다. 이전 로컬 파일 접근 제한을 우회하지 않아 실제 브라우저 배치/가독성 검사는 미수행이다. 본문은 한글이며 renderer의 고정 Viewer UI와 `<html lang>`는 지원 기본값인 영어다.
