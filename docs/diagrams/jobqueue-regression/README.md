# JobQueue 성능 원인 조사 도구의 실행 흐름

> 과거 실험의 흐름도다. 2026-10-12 큐 후보와 전용 도구는 작업 트리에서 제외했으며 현재 구현을 나타내지 않는다. JSON/HTML은 당시 검증된 원본으로 보존한다.

[JSON](compare.workflow.json) · [HTML](compare.workflow.html) · [조사 보고서](../../development/JOBQUEUE_REGRESSION.html).

`compare.py`는 보존한 전후 실행 파일을 seed 홀짝에 따라 AB/BA로 교대하고 매번 서버를 재시작한다. 기존 `baseline.one_run`의 실제 시나리오·기능 판정·종료 검사를 그대로 사용한다. 별도 스레드가 Windows PDH 시스템 부하를 1초마다 수집하고, 종료 시 원래 실행 파일을 복원해 SHA-256을 확인한다. 실패 결과도 보존한다.

`analyze_compare.py`는 측정 구간 내부 reporter 표본의 instance·중복 sequence를 확인해 완료량·tick·callback 시간과 시스템 표본을 비교한다. 성공한 전후 짝만 성능 변화율에 포함하고 실패 실행 수는 별도로 남긴다. CPU 감소만으로 개선을 판정하지 않는다.

`probe_queue_cost.py`는 별도 프로세스에서 작은 묶음·Five 자기 재등록을 측정한다. 선택적 atomic 제거 대조군은 단일 스레드의 비용 분리만을 위한 것으로 동시성 안전성이 없으며 서버에서 사용하지 않는다. 실제 서버 측정과 동시에 실행하지 않는다.
추가 정렬 대조는 같은 atomic/메모리 순서를 유지한 채 queue·mutex·카운터의 캐시라인 배치를 바꾸며 1생산자·1소비자 조건도 지원한다. 그림의 단일 스레드 비용 분리 외에 수행하는 별도 진단이다.

근거: [교차 실행](../../../tools/server_lab/compare.py), [시스템 부하](../../../tools/server_lab/system_load.py), [원시 결과 분석](../../../tools/server_lab/analyze_compare.py), [비용 대조](../../../tools/server_lab/probe_queue_cost.py), [C++ 비용 측정](../../../tools/server_lab/queue_cost_probe.cpp).

Archify 2.16 validate/deliver showcase 9/9·composition 오류/경고 0. 한글 설명과 노드를 사용하며 고정 Viewer UI·HTML lang은 영어 fallback이다. 기존 local-file 브라우저 보안 거절을 우회하지 않아 실제 HTML 가독성·desktop containment 검토는 미수행이다.

```powershell
node .agents/skills/archify/bin/archify.mjs validate workflow docs/diagrams/jobqueue-regression/compare.workflow.json --quality showcase --json
node .agents/skills/archify/bin/archify.mjs deliver workflow docs/diagrams/jobqueue-regression/compare.workflow.json docs/diagrams/jobqueue-regression/compare.workflow.html --quality showcase --json
```

JSON SHA-256 `70c931566e9c59e197e3a9eec4dba68f5f2e04bb849b8d86821d8a2a373c200d` (2,836 bytes).
HTML SHA-256 `7061dfa9b55b3cf11bf7dbdd6a157f2ce05f20053c04f2ca83770a4ec7c9dba7` (710,468 bytes).
