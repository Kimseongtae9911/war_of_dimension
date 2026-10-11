# m_deferred와 전송 완료 처리

[흐름도 HTML](iocp-completion-only.workflow.html) · [수정 원본 JSON](iocp-completion-only.workflow.json)

`m_deferred`는 Send 배치 처리 또는 Disconnect 보류 판단을 건너뛰고 개별 완료 처리로 진행하도록 하는 표시다. `true`여도 `Complete()`와 완료 결과 반환은 실행한다. 이름과 달리 앞으로 처리를 미루라는 명령이 아니다.

흐름도는 A·B·C의 배치 전송이 모두 끝난 성공 경로를 보여준다. A·B·C는 각 원래 프레임의 `IoContext`다. 실제 native 완료는 A가 받는다. 배치 처리에서 A·B·C를 전송 목록에서 제거한 뒤 B·C에 `true`를 설정하고 `PostQueuedCompletionStatus`로 개별 알림을 게시한다. A는 현재 Poll에서 완료되고, B·C는 각각 후속 Poll에서 배치 분기를 건너뛰고 완료된다. worker들의 정확한 실행/회수 시점 순서를 고정한 그림은 아니다.

## 코드와 연결

- [선언](../../../Server/ServerCore/include/ServerCore/Net.h:118): `bool m_deferred = false;`
- [Send 핸들러](../../../Server/ServerCore/src/Net.cpp:446): `HandleCompletion`이 Send를 분류하며 `HandleSendCompletion`은 `m_deferred`가 true면 배치 처리 없이 Deliver를 반환한다.
- [B·C 완료 게시](../../../Server/ServerCore/src/Net.cpp:485): 대표 context 이외의 항목에 true·동일 오류 결과를 설정하고 개별 완료를 게시한다.
- [공통 완료](../../../Server/ServerCore/src/Net.cpp:563): `Complete(*context)`가 pending을 감소시키고 Poll이 호출자에게 완료 결과를 반환한다. 객체 풀 회수는 호출자의 처리 경로에서 수행한다.

부분 전송은 배치의 남은 bytes부터 재발행하며, 전체 성공/오류가 결정되기 전에는 위 완료 분배를 수행하지 않는다. true가 없으면 B·C의 알림이 대표 배치 완료로 다시 처리되어 `send completion without active batch` 검사를 실패하거나 다음 배치와 충돌할 수 있다.

## 다른 용도

| 경우 | true를 설정하는 이유 |
|---|---|
| 발행 전 대기 전송 취소 | 이미 전송 목록에서 제거한 항목을 배치 완료로 다시 처리하지 않고 취소 완료만 전달 |
| Disconnect 완료 재게시 | 다른 pending I/O가 정리된 뒤 보류했던 종료 결과를 최종 완료로 전달 |

근거: [Disconnect 대기 전송 취소](../../../Server/ServerCore/src/Net.cpp:344), [보류 종료 재게시](../../../Server/ServerCore/src/Net.cpp:396), [Disconnect 핸들러](../../../Server/ServerCore/src/Net.cpp:502), [Close 취소](../../../Server/ServerCore/src/Net.cpp:591).
모든 내부 게시 알림이 true인 것은 아니다. 예를 들어 실제 전송 발행의 즉시 실패는 배치 오류 처리가 필요하므로 false로 게시될 수 있다.

## Poll과 연산별 처리 분리

`Poll`은 완료 대기 → callback 실행권 확보 → 상태 잠금 → `HandleCompletion` 호출 → 최종 오류 계측/`Complete`/결과 반환을 담당한다. 연결 callback 잠금을 기다릴 때 코어 상태 잠금을 보유하지 않는다. 반환한 Completion은 호출자의 callback 실행이 끝날 때까지 실행권을 유지한다.

| 처리 위치 | 책임 |
|---|---|
| `GetDispatchMutex` | 소켓 또는 AppEvent key의 callback 잠금 조회 |
| `HandleCompletion` | 고정된 IoOperation을 switch로 분류; Receive 성공 byte 계측, AppEvent 통과 |
| `HandleSendCompletion` | 배치/부분 송신·오류 분배·다음 전송 발행 |
| `HandleDisconnectCompletion` | 다른 pending I/O가 남은 종료 결과 보류 |
| `HandleAcceptCompletion` | 성공한 AcceptEx의 SO_UPDATE_ACCEPT_CONTEXT 설정 |

부분 송신 재발행 또는 Disconnect 보류는 `ContinuePolling`을 반환해 그 context의 최종 완료를 아직 전달하지 않는다. `Deliver`일 때만 Poll의 공통 최종 완료 경로로 진행한다. 가상 핸들러·동적 등록·공개 API를 추가하지 않았으며 이 책임 분리의 처리량 효과는 측정하지 않았다.

## 생성과 검증

Archify 2.16 · workflow v2 · showcase. 작성 내용은 한글이며 고정 Viewer UI와 HTML lang은 영어 fallback이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/iocp-completion-only/iocp-completion-only.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/iocp-completion-only/iocp-completion-only.workflow.json docs/diagrams/iocp-completion-only/iocp-completion-only.workflow.html --quality showcase --json
```

최종 validate/deliver: 9/9 검사 PASS, composition 오류/경고 0. JSON SHA-256 `740a49ad5be22fa0e916407769365872ecbcd72b41815befcd0bc7d75850e6c5` (3,875 bytes), HTML SHA-256 `61e1453b0aae0b8d7e9e93a482e67db72ce56b326ea9b40aba49c6f7fc9d83e3` (710,277 bytes).
브라우저 도구의 file URL 접근 제한을 우회하지 않아 실제 HTML 화면·4크기 containment·light/dark 시각 점검은 미실행이다. 자동 SVG 검사 통과를 실제 화면 검증으로 간주하지 않는다.
초판은 설명만 작성했으며 이후 Poll 책임 분리를 구현했다. 실제 빌드·회귀 결과는 tasks/todo.md의 IOCP 완료 처리 책임 분리 항목에 기록한다. 기존 JobQueue 성능 측정 결과는 변경하지 않는다.
