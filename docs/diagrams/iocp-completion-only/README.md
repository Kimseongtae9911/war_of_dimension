# m_deferred와 전송 완료 처리

[흐름도 HTML](iocp-completion-only.workflow.html) · [수정 원본 JSON](iocp-completion-only.workflow.json)

`m_deferred`는 Send 배치 처리 또는 Disconnect 보류 판단을 건너뛰고 개별 완료 처리로 진행하도록 하는 표시다. `true`여도 `Complete()`와 완료 결과 반환은 실행한다. 이름과 달리 앞으로 처리를 미루라는 명령이 아니다.

흐름도는 A·B·C의 배치 전송이 모두 끝난 성공 경로를 보여준다. A·B·C는 각 원래 프레임의 `IoContext`다. 실제 native 완료는 A가 받는다. 배치 처리에서 A·B·C를 전송 목록에서 제거한 뒤 B·C에 `true`를 설정하고 `PostQueuedCompletionStatus`로 개별 알림을 게시한다. A는 현재 Poll에서 완료되고, B·C는 각각 후속 Poll에서 배치 분기를 건너뛰고 완료된다. worker들의 정확한 실행/회수 시점 순서를 고정한 그림은 아니다.

## 코드와 연결

- [선언](../../../Server/ServerCore/include/ServerCore/Net.h:118): `bool m_deferred = false;`
- [Send 분기](../../../Server/ServerCore/src/Net.cpp:457): `Send && !context->m_deferred`일 때만 배치 처리를 수행한다.
- [B·C 완료 게시](../../../Server/ServerCore/src/Net.cpp:489): 대표 context 이외의 항목에 true·동일 오류 결과를 설정하고 개별 완료를 게시한다.
- [공통 완료](../../../Server/ServerCore/src/Net.cpp:526): `Complete(*context)`가 pending을 감소시키고 Poll이 호출자에게 완료 결과를 반환한다. 객체 풀 회수는 호출자의 처리 경로에서 수행한다.

부분 전송은 배치의 남은 bytes부터 재발행하며, 전체 성공/오류가 결정되기 전에는 위 완료 분배를 수행하지 않는다. true가 없으면 B·C의 알림이 대표 배치 완료로 다시 처리되어 `send completion without active batch` 검사를 실패하거나 다음 배치와 충돌할 수 있다.

## 다른 용도

| 경우 | true를 설정하는 이유 |
|---|---|
| 발행 전 대기 전송 취소 | 이미 전송 목록에서 제거한 항목을 배치 완료로 다시 처리하지 않고 취소 완료만 전달 |
| Disconnect 완료 재게시 | 다른 pending I/O가 정리된 뒤 보류했던 종료 결과를 최종 완료로 전달 |

근거: [Disconnect 대기 전송 취소](../../../Server/ServerCore/src/Net.cpp:344), [보류 종료 재게시](../../../Server/ServerCore/src/Net.cpp:396), [Disconnect 보류 분기](../../../Server/ServerCore/src/Net.cpp:506), [Close 취소](../../../Server/ServerCore/src/Net.cpp:550).
모든 내부 게시 알림이 true인 것은 아니다. 예를 들어 실제 전송 발행의 즉시 실패는 배치 오류 처리가 필요하므로 false로 게시될 수 있다.

## 생성과 검증

Archify 2.16 · workflow v2 · showcase. 작성 내용은 한글이며 고정 Viewer UI와 HTML lang은 영어 fallback이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/iocp-completion-only/iocp-completion-only.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/iocp-completion-only/iocp-completion-only.workflow.json docs/diagrams/iocp-completion-only/iocp-completion-only.workflow.html --quality showcase --json
```

최종 validate/deliver: 9/9 검사 PASS, composition 오류/경고 0. JSON SHA-256 `5ed57c97f6fbc6bc368b643aa328281a1e984a5f93d3addf6a74f2ae6ada1997` (3,803 bytes), HTML SHA-256 `4a595bc77345901c126c3de6b33966fa06ae0fe38ef2b7cac29b03392170bdc5` (710,137 bytes).
브라우저 도구의 file URL 접근 제한을 우회하지 않아 실제 HTML 화면·4크기 containment·light/dark 시각 점검은 미실행이다. 자동 SVG 검사 통과를 실제 화면 검증으로 간주하지 않는다.
서버 구현·성능 결과는 변경하지 않았으며 빌드·부하 검사는 재실행하지 않았다.
