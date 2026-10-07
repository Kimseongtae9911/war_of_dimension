# War Of Dimension 리팩토링 최종 보고서

작성일: **2026-10-06 KST** · 로딩 실측 추가: **2026-10-07 KST**

대상: Windows C++20·DirectX 12/FMOD 클라이언트, IOCP 로비/게임 서버.

목적은 **멀티플레이 테스트 편의성 개선**이다.<br>
클라이언트의 높은 메모리 사용량으로 **4개 클라이언트 동시 실행이 불가능**했다.

에이전트 개발 환경을 구축하고, 테스트 환경 개선의 하나로 메모리 병목을 측정·최적화했다.

보고서는 **에이전트 환경 → 병목 측정 → 최적화 5개 항목 → 미적용 후보**로 구성한다.

최신 메모리 측정은 단일 클라이언트의 오프라인 인게임 고정 재생 결과다.<br>
**최적화 후 실제 게임 클라이언트 4개 동시 실행·전투는 아직 검증하지 않았다.**

메모리 최적화 구현 기준은 커밋 [`e85eba42`](https://github.com/Kimseongtae9911/war_of_dimension/commit/e85eba42958acfdaff5713eb188d7959b39b2fe8)다.<br>
메모리 수치는 기존 보고서와 측정 자료를 재정리했다.<br>
장면 로딩 속도는 최적화 이전과 현재 빌드를 새로 측정했다.

## 전체 성과와 수치 해석

에이전트 개발 환경을 구성하고 geometry·외형·파티클·텍스처·임시 버퍼·상수 버퍼를 최적화했다.

최신 오프라인 고정 재생 측정의 중앙값은 다음과 같다.

- **Private Bytes: 1,756.43MiB**
- **Working Set: 790.44MiB**
- **DXGI LOCAL usage: 1,084.72MiB**

전투 최대 사용량과 프레임 시간은 미측정이다.<br>
단계별 조건·통계가 다르므로 항목별 합계와 최초·최신 측정값 차이를 구분한다.

| 개선 항목 | Private Bytes 전 → 후 | 감소량 | 해당 비교 조건 |
|---|---:|---:|---|
| 동일 geometry 공유 | 6,951.54 → 5,768.31MiB | 1,183.22MiB / 17.02% | 동일 바이너리의 이전 로더 재현/공유 경로, 각 3회 중앙값 |
| 영웅 선택 외형·변환 행렬 | 5,833.01 → 4,928.64MiB | 904.36MiB / 15.50% | 동일 바이너리의 전체/선택 파츠, 각 3회 평균 |
| 선택 스킬 파티클 종류 제한 | 4,857.28 → 3,837.25MiB | 1,020.02MiB / 21.00% | 동일 바이너리의 전체/선택 종류, 각 3회 평균 |
| 전체 파티클 공용 GPU 풀 | 3,838.65 → 2,778.36MiB | 1,060.29MiB / 27.62% | 동일 바이너리·동일 Shader 고정 재생, 각 3회 평균 |
| NPC upload·BC7·상수 arena | 2,656.35 → 1,964.46MiB | 691.89MiB / 26.05% | 변경 전/후 별도 바이너리, 같은 fixture 각 3회 중앙값 |
| UI BC7·사전 로딩 유지 | 1,964.52 → **1,756.43MiB** | **208.09MiB / 10.59%** | 동일 바이너리·에셋만 변경, 각 3회 중앙값 |

### 전체 감소량

**최초·최신 Private Bytes 측정값은 5,195.10MiB(약 5.07GiB), 74.73% 감소했다.**

| 비교 | 최초 측정 | 최신 측정 | 감소량 | 감소율 |
|---|---:|---:|---:|---:|
| 전체 클라이언트 Private Bytes | 6,951.54MiB | 1,756.43MiB | **5,195.10MiB** | **74.73%** |

최초 값은 geometry 공유 전 로더 재현, 최신 값은 UI BC7 적용 후 결과다.<br>
둘 다 각 3회 중앙값이다. 시나리오·바이너리·재생 조건이 달라 동일 조건의 전체 A/B는 아니다.

**항목별 절감량의 단순 합계는 5,067.88MiB(약 4.95GiB)**다.<br>
이 합계는 서로 다른 단계의 계산값을 더한 참고치다. 전체 실측 감소량으로 사용하지 않는다.

수치는 다음 기준으로 해석한다.

- MiB = 1,048,576byte. 감소량은 반올림 전 byte로 계산했다.
- `private commit`과 `Private Bytes`는 같은 프로세스 private 지표다.
- Working Set·DXGI usage·GPU allocation·DDS payload는 별도 지표다. 서로 합산하지 않는다.
- checkpoint 순증가량은 자원별 최종 소유량이 아니다.

출처·통계·문서 해시는 [종합 근거 JSON](evidence/final-report-20261006.json)에 기록했다.<br>
과거 수치와 제안은 기존 보고서에 보존했다.

### 장면 로딩 속도

메모리 최적화 전 `ed3428e9`와 적용 후 `d02737a5`를 비교했다.
Release x64·1920×1080·RTX 4070 SUPER, 예열 제외 각 5회 중앙값이다.

| 구간 | 개선 전 | 개선 후 | 시간 변화 | 변화율 |
|---|---:|---:|---:|---:|
| 타이틀→로비 | 8.92초 | 7.10초 | **1.82초 단축** | **20.35% 감소** |
| 로비→인게임 | 40.17초 | 15.55초 | **24.63초 단축** | **61.30% 감소** |
| 인게임→로비 | 14.77초 | 8.06초 | **6.71초 단축** | **45.43% 감소** |

장면 전환 시작부터 로딩 화면 종료·첫 목적지 프레임의 GPU 완료까지 측정했다.<br>
로비→인게임은 READY 로딩을 포함한다. 로그인·매칭·선택 대기는 제외했다.<br>
오프라인·숨김 창에서 독립 클라이언트를 교대 실행했다. cold boot 측정은 아니다.<br>
시간 변화·변화율은 반올림 전 값으로 계산했다.

[최종 측정 근거](evidence/client-loading-20261007.json)

## 1. 에이전트 기반 개발 환경 구축

### 진행한 작업

에이전트별 작업 기준을 통일하고 코드 탐색·검증 절차를 구성했다.

| 작업 | 적용 내용 | 결과물 |
|---|---|---|
| 공통 작업 지침 | 정책 우선순위·자원 소유권·작업/검증 기준 통합 | `AGENTS.md`, 도구별 진입 문서, 작업·문서 지침 인덱스 |
| 에이전트 도구 설정 | Codex·Claude Code·Gemini/Antigravity의 Serena 연결 설정 구성 | `.codex/config.toml`, `.mcp.json`, `.gemini/settings.json`, `.serena/project.yml` |
| C++ 코드 탐색 | 소스·C++20·MSVC/SDK 설정으로 compilation database 생성<br>Serena/clangd 심볼 탐색 구성 | 생성 스크립트·235개 source 검증<br>생성 파일은 Git 제외 |
| 공통 빌드·테스트 진입점 | VS2026/v145·`NewWod.slnx`·서버 filter<br>공통 로컬 실행·검증 명령 | `Build/Start/Stop/Test-Local`<br>`Test-AgentEnvironment`, `Test-MeshSharing`<br>개발 지침 |
| 구조와 근거 관리 | 구조·수명 변경을 Archify와 문서로 설명하고 단계별 작업·검증 근거 유지 | Archify 스킬·JSON/HTML 구조도, `tasks/todo.md`, 포트폴리오 보고서·근거 데이터 |

### 구성 파일과 문서 인덱스

에이전트 환경 관련 파일만 추린 구조다.

```text
NewWod/
├── AGENTS.md                          # 공통 작업 규칙
├── CLAUDE.md                          # Claude 진입점
├── GEMINI.md                          # Gemini/Antigravity 진입점
├── .codex/
│   └── config.toml                    # Codex MCP 설정
├── .mcp.json                          # Claude MCP 설정
├── .gemini/
│   └── settings.json                  # Gemini/Antigravity MCP 설정
├── .serena/
│   └── project.yml                    # C++ 색인·clangd 설정
├── .agents/skills/archify/
│   └── SKILL.md                       # 구조 문서 작성 스킬
├── docs/
│   ├── AGENTS.md                      # 문서 작성 규칙
│   ├── INDEX.md                       # 영역별 문서 인덱스
│   ├── guides/
│   │   ├── INDEX.md                   # 작업별 필수 지침 인덱스
│   │   └── archify.md                 # 구조도 작성 지침
│   ├── development/
│   │   ├── AGENTS.md                  # 에이전트 환경 설명
│   │   └── SETUP.md                   # 빌드·로컬 실행 절차
│   ├── architecture/
│   │   └── OVERVIEW.md                # 모듈·자원 소유권 개요
│   ├── diagrams/                      # 구조도 JSON·HTML·근거
│   └── portfolio/
│       ├── README.md                  # 단계별 보고서 인덱스
│       ├── FINAL_REFACTORING_REPORT.md # 최종 보고서
│       └── evidence/                  # 측정·화면·검증 근거
├── scripts/
│   ├── New-CompilationDatabase.ps1    # C++ 분석 설정 생성
│   ├── Test-AgentEnvironment.ps1      # 에이전트 환경 검사
│   ├── Build.ps1                      # 빌드
│   ├── Start-Local.ps1                # 로컬 프로세스 실행
│   ├── Stop-Local.ps1                 # 로컬 프로세스 종료
│   ├── Test-Local.ps1                 # 로컬 실행 검사
│   └── Test-MeshSharing.ps1           # 메시 공유 검사
├── tasks/
│   └── todo.md                        # 작업·검증 기록
└── compile_commands.json             # 로컬 생성, Git 제외
```

문서는 다음 순서로 확인한다.

1. [AGENTS.md](../../AGENTS.md): 공통 규칙 확인.
2. [작업 지침 인덱스](../guides/INDEX.md): 작업에 필요한 지침 선택.
3. [문서 인덱스](../INDEX.md): 영향 영역의 문서 확인.
4. 구현·테스트 확인 후 [tasks/todo.md](../../tasks/todo.md)에 계획·검증 기록.

`CLAUDE.md`·`GEMINI.md`는 공통 지침으로 연결한다. 정책을 각각 복제하지 않는다.<br>
소스·toolset·SDK 변경 시 `compile_commands.json`을 다시 생성한다.

### 결과와 검증

**결과물:** 공통 지침·도구 설정·코드 탐색·검증 스크립트·구조 문서.<br>
메모리 최적화에 이 환경을 사용했다.

검증: compilation database **235 source**, Serena 심볼 조회, Archify doctor 통과.

로컬 실행은 `LOCAL_TEST`·DB 없는 모드다. 운영 DB·블록체인은 미검증이다.

**상세 자료:**

- [에이전트 환경](../development/AGENTS.md)
- [공통 개발 명령](../development/SETUP.md)
- [VS2026 설정](../development/VS2026.md)
- [아키텍처 개요](../architecture/OVERVIEW.md)
- [작업 기록](../../tasks/todo.md)

## 2. 메모리 병목을 측정

### 해결한 문제

클라이언트 하나가 약 5.8GB를 사용했지만 병목 자원을 알 수 없었다.<br>
CPU 배열·GPU 할당·프로세스 측정값을 구분해 개선 순서를 정해야 했다.

### 진행한 작업

- Title→INGAME의 실제 자원 생성 경로에 checkpoint를 추가하고 Private Bytes·Working Set·DXGI segment usage를 수집했다.
- 같은 바이너리에서 전후 비교 모드를 제공했다.<br>
  로더·외형 파츠·파티클 종류·버퍼 풀을 각각 비교한다.
- 실제 `.bin` geometry·skin·clip·keyframe과 DDS header/mip/payload를 파싱하고, `GetResourceAllocationInfo`로 GPU 할당량을 조사했다.
- 각 모드를 3회 측정하고 JSON/CSV·그래프를 생성했다.<br>
  합계·시나리오·반복 번호·완료 checkpoint·해시를 검사하고 잘못된 입력을 거부한다.
- 맵 geometry와 맵 전체 생성 구간, 영웅 모델 1회 비용과 화면의 영웅 수, 임시 upload와 최종 잔존 자원을 분리했다.

### 측정 결과와 확인한 원인

측정 시점: **2026-10-05 메시 공유 직후, 영웅·파티클·NPC·UI 최적화 전**.

조건: Release x64/RTX 4070 SUPER, 오프라인 Title→INGAME.<br>
동일 시나리오의 독립 프로세스 **3회 평균**이다.

| 자원 생성 구간 | Private Bytes 증가 | 최종 Private 대비 비중 | Working Set 증가 | DXGI LOCAL 증가 |
|---|---:|---:|---:|---:|
| 파티클 객체·대형 버퍼 | **2,420.57MiB** | **41.98%** | 7.60MiB | 2,408.15MiB |
| 영웅 모델 2회·컨트롤러 4개 | **1,088.22MiB** | **18.87%** | 998.02MiB | 82.96MiB |
| 미니언·몬스터·Ogre boss | **948.46MiB** | **16.45%** | 489.88MiB | 453.98MiB |
| 기동·Title·공통 자원 | 631.60MiB | 10.95% | 214.48MiB | 438.64MiB |
| 인게임 UI·dissolve | 300.97MiB | 5.22% | 147.29MiB | 148.51MiB |
| 하늘 cubemap | 192.51MiB | 3.34% | 96.31MiB | 96.00MiB |
| 맵 전체 생성 | 142.32MiB | 2.47% | 126.92MiB | 17.80MiB |
| 스킬 모델·객체·billboard | 27.52MiB | 0.48% | 16.04MiB | 10.84MiB |
| 파티클 텍스처·공통 설정 | 13.68MiB | 0.24% | 7.43MiB | 6.00MiB |
| GPU 제출·대기·upload 해제 | 0.25MiB | 0.00% | 0.26MiB | 0.00MiB |
| **최종 평균** | **5,766.11MiB** | **100%** | **2,104.25MiB** | **3,662.88MiB** |

각 행은 checkpoint **순증가량**이다. 초기화 행에는 프로세스 기동 기준값을 포함했다.<br>
반올림 전 구간 합계는 최종 값과 일치한다. 서로 다른 지표는 합산하지 않는다.

이 표는 당시 병목 분석 기준선이다. 최신 메모리 구성 비율은 아니다.<br>
원시 checkpoint와 통계는 [측정 JSON](evidence/client-memory-breakdown-20261005.json)에 기록했다.

파티클·영웅·NPC의 Private 증가 비중이 가장 컸다. 맵은 메시 공유 후 2.47%로 줄어, 이 세 영역을 다음 개선 대상으로 선정했다.

별도 전후 비교에서 맵 생성 Private 증가는 **1,448.13→142.62MiB**로 줄었다.<br>
맵 중복을 줄인 뒤 파티클 선할당과 영웅 외형·행렬이 다음 병목으로 드러났다.

최종 절감량에는 생성 구간뿐 아니라 진입 끝의 upload 반환도 반영했다.

계측은 CLI로 실행하며 네트워크·loading render thread를 생략한다.<br>
전투 peak·FPS와 자원별 heap 소유량은 미측정이다.

**결과물:** checkpoint 계측·전후 비교 모드·에셋/할당량 분석 도구·JSON/CSV·그래프.<br>
파티클·영웅·NPC를 우선 개선할 근거를 확보했다.

**기존 보고서:**

- [최초 A/B·맵/영웅 계산](CLIENT_MEMORY_OPTIMIZATION.md)
- [5.8GB 비용 분해·메시 공유 전후 구간 비교](CLIENT_MEMORY_BREAKDOWN.md)
- [공용 풀 당시 비율](PARTICLE_BUFFER_POOL.md#최적화-후-클라이언트-메모리-구성-비율)

## 3. 공통 geometry와 모델 DDS 자원 공유

### 해결한 문제

같은 geometry를 위치·이름이 다른 프레임마다 CPU 배열과 GPU 버퍼로 만들었다.<br>
모델 사이에서 같은 DDS를 읽어도 texture/upload를 다시 생성했다.<br>
공유하면서 개별 transform·material·본·재생 상태와 수명을 유지해야 했다.

### 진행한 작업

| 자원 | 공유 기준·범위 | 개별로 유지한 것 |
|---|---|---|
| 정적 geometry | device·geometry 내용 SHA-256·직렬화 길이. 메시 이름은 제외하며 동일 CPU 배열·정점/인덱스 buffer 재사용 | 프레임 이름·transform·material |
| 스킨드 base geometry | 정점·속성·subset/index의 읽기 전용 base 재사용 | skin wrapper·본 index/weight·bind-pose·본 연결·컨트롤러 상태 |
| 모델 DDS | device·정규 파일 경로·resource 용도. DEFAULT texture·upload 및 같은 Scene 힙의 SRV 재사용 | material wrapper·root parameter·binding 저장 |

캐시는 `weak_ptr`로 관리하며 마지막 소유자가 해제되면 자원도 반환된다.<br>
가변 UI·파티클 메시는 캐시에서 제외했다. DDS는 픽셀 내용 대신 파일 경로로 공유한다.

동시 로딩·device 경계·copy fence·반복 upload 반환·마지막 소유자 해제를 검사했다.

### 결과와 검증

| 실제 모델 | 기존 메시 레코드 | 고유 GPU geometry |
|---|---:|---:|
| LobbyScene_No | 291 | 38 |
| Plane1 인게임 맵 | 5,185 | 78 |
| ModularModel 영웅 | 724 | 698 |

ModularModel에서 이름이 다른 중복 geometry 26개를 확인했다.<br>
같은 바이너리의 최초 A/B에서 Private 중앙값은 **1,183.22MiB 감소**했다.<br>
geometry 공유 12범주와 실제 모델 3종 GPU audit를 통과했다.

Chest·Beholder의 texture 참조 4개를 고유 DDS 2개로 공유했다.

- 압축 전 표본 DEFAULT/UPLOAD: 각각 **16→8MiB**.
- BC7 적용 후 같은 표본의 중복 절감: 각각 **2MiB**.

기준 에셋이 다르므로 두 절감량을 합산하지 않는다.<br>
DDS 공유 12범주·readback·binding 독립·Scene 힙 교체·종료 live 0을 검사했다.

**결과물:** geometry 캐시·`SetSharedMesh`·DDS 캐시·GPU/수명 검사·구조도.<br>
객체 상태는 유지하고 메시·텍스처의 중복 생성을 줄였다.

### 메모리 감소량

동일 geometry 공유의 Private Bytes는 **1,183.22MiB / 17.02% 감소**했다.<br>
이전 로더 재현 **6,951.54MiB → 공유 경로 5,768.31MiB**, 동일 바이너리 각 3회 중앙값 기준이다.<br>
모델 DDS 공유의 추가 효과는 이 비교에 포함하지 않는다.

**기존 보고서:**

- [메시 공유 구현·소유권](../architecture/MESH_SHARING.md)
- [메모리 A/B](CLIENT_MEMORY_OPTIMIZATION.md)
- [모델 DDS 공유](NPC_RESOURCE_SHARING_REVIEW.md#후속-적용-모델-dds-공유)
- [메시 구조도](../diagrams/mesh-sharing/README.md)
- [DDS 수명도](../diagrams/dds-sharing/README.md)

## 4. 확정 외형을 이용한 영웅 메모리 최적화

### 해결한 문제

외형 확정 후에도 모든 파츠와 788개 프레임의 keyframe 행렬을 보관했다.<br>
모델은 영웅마다 읽지 않고 타인용·로컬용으로 2회 로딩했다.

### 진행한 작업

- READY에서 수신한 외형 번호를 저장하고 인게임 진입 시 mutex로 확정 스냅샷을 만든다. 미수신이면 기존 전체 모델로 fallback하고 다음 매치에서 초기화한다.
- 타인 계층은 세 영웅의 선택 파츠 합집합, 로컬 계층은 자신의 선택 파츠로 GPU/CPU geometry·skin wrapper·material 생성을 제한한다.
- **모든 61개 clip, 6,947개 keyframe, 전체 788개 프레임 계층을 유지**하면서 선택 외형 파츠와 공통 본·부모의 변환 행렬만 보관한다.
- 미선택 파츠의 원본 레코드는 경계 검증과 함께 건너뛰고, 제외된 texture 소유 파츠에 대한 중복 참조를 실제 DDS에서 복구한다.
- 실제 진입을 막던 로비 `Job::Execute` 호출 누락과 서버 응답 전 Title 장면 전환을 복구했다. 패킷 필드·packing은 유지했다.

외형 번호로 만든 `ModelPartSelection`을 동기 로더에 전달한다.<br>
파일 읽기량과 재생 시간축은 유지하고 파츠·행렬 보관량을 줄였다.

로비 모델 포인터 전달이나 선택 스킬 clip만 로딩하는 방식은 아니다.

### 결과와 검증

| 인게임 2회 로딩 fixture | 전체 | 선택 |
|---|---:|---:|
| 스킨드 메시 | 1,440 | 91 |
| keyframe당 행렬 보관 대상의 합계 | 1,576 | 227 |
| 변환 행렬 payload | 668.24MiB | 96.25MiB |

행렬 payload는 **571.99MiB/85.60%** 감소했다.

- **1,576,969개 행렬**이 원본과 byte 단위로 일치했다.
- 본 링크·재질·clip 길이·fallback·Reset·손상 레코드 거부를 검사했다.
- DB 없는 Title 테스트에서 TCP 8910→8911 전환과 영웅·무기·맵·UI를 촬영했다.

실제 네 사용자 전투는 미검증이다.

**결과물:** 외형 스냅샷·선택 파츠 로더·변환 행렬 저장·원본 비교 검사·진입 화면.<br>
모든 clip을 유지하고 미선택 외형의 비용을 줄였다.

### 메모리 감소량

선택 외형·변환 행렬 최적화의 Private Bytes는 **904.36MiB / 15.50% 감소**했다.<br>
전체 파츠 **5,833.01MiB → 선택 파츠 4,928.64MiB**, 동일 바이너리 각 3회 평균 기준이다.

**기존 보고서:**

- [용어·선택 파츠·행렬·실측·실제 화면](HERO_SELECTED_PARTS.md)
- [선택 흐름도](../diagrams/hero-selection/README.md)

## 5. 선택 스킬 파티클과 전체 효과 GPU 버퍼 풀

### 해결한 문제

모든 스킬 종류의 5개 객체에 300,000입자 용량의 버퍼를 선할당했다.<br>
버퍼 한 쌍의 payload는 **18.31MiB**, 실제 allocation은 **18.375MiB**였다.

동시 효과는 입자 상태가 달라 별도 버퍼 영역이 필요하다.

### 진행한 작업

| 단계 | 적용 내용 |
|---|---|
| 선택 스킬로 필요 종류 판별 | 직업·선택 스킬 합집합 + 기본 공격·타워·후속/타이머 효과<br>선택 고정·범위 검사·미완료 시 전체 종류 fallback |
| 서버 전송 순서 보완 | 자동 선택 결과를 `SC_GAME_START` 전에 전송<br>보스 자동/수동 응답 번호 통일. 패킷 계약 유지 |
| 효과 공용 GPU 풀 | 스킬뿐 아니라 슬롯 버프/이동·점프·코인·장벽도 같은 장면 풀에서 최초 표시 시 버퍼 쌍 임대 |
| 재사용과 동시 격리 | 활성 효과는 별도 블록 사용<br>비활성 블록은 fence 후 재임대, seed/counter 초기화 |
| 부족 시 확장 | 유휴 쌍 부족 시 새 쌍 추가. 입자 출력 포화 시 다음 프레임에 용량 증가와 실제 기록된 상태 복사 |
| 종료·실패 수명 | 부분 할당 RAII 회수, 확장 실패 시 기존 상태 유지, GPU 완료 후 최초·확장·유휴 블록 전체 반환 및 반복 종료 검사 |

### 결과와 검증

선택 종류 제한 결과는 다음과 같다.

- 객체: **127→72개**. 종류별 45개·선택 슬롯 8개·환경 19개.
- 대형 GPU allocation: **2,333.625→1,323.000MiB**, **1,010.625MiB 감소**.

이전 측정의 131개는 다른 선택 슬롯 조건이므로 직접 비교하지 않는다.

공용 풀은 **논리 객체 72개를 유지**하고 동일 Shader를 10프레임 재생해 비교했다.

- 대형 버퍼: **72→19쌍**.
- allocation: **1,323.000→349.125MiB**, **973.875MiB/73.61% 감소**.
- 종류 간 재사용: 30회. 마지막 활성 18쌍·유휴 1쌍, 종료 시 모두 반환.

Debug/Release 실제 GPU에서 동시 임대 격리·미완료 fence 재사용 방지·성장·상태 보존·종료 잔여 쌍 0을 검사했다.<br>
네 TCP 클라이언트의 Ogre/Programmer 자동/수동 선택·분할 READY·게임 시작 전 선택 수신도 확인했다.

**한계:**

- 19쌍은 고정 재생 결과다. 실전 상한이 아니다.
- 포화 프레임에서 누락된 입자는 다음 프레임 확장으로 복구할 수 없다.
- 유휴 풀은 게임 종료까지 유지하며 자동 축소하지 않는다.
- 실제 OOM·최대 중첩·장시간 전투는 미검증이다. 서버 효과 ID/풀 고갈 문제는 남아 있다.

**결과물:** 선택 스킬 판별·공용 GPU 풀·fence 재사용·확장·종료 반환·GPU 검사.<br>
동시 효과는 분리하고 비활성 버퍼는 종류 간 재사용한다.

### 메모리 감소량

- **선택 스킬 파티클 종류 제한: Private Bytes 1,020.02MiB / 21.00% 감소.**<br>
  전체 종류 4,857.28MiB → 선택 종류 3,837.25MiB, 동일 바이너리 각 3회 평균 기준이다.
- **전체 파티클 공용 GPU 풀: Private Bytes 1,060.29MiB / 27.62% 감소.**<br>
  개별 버퍼 3,838.65MiB → 공용 풀 2,778.36MiB, 동일 바이너리·동일 Shader 고정 재생 각 3회 평균 기준이다.

두 단계의 비교 기준이 다르므로 감소량을 합산하지 않는다. 위 Private Bytes 절감과 대형 GPU buffer allocation 절감도 서로 합산하지 않는다.

**기존 보고서:**

- [선택 파티클](PARTICLE_SELECTED_SKILLS.md)
- [5개 풀 필요성 검토](PARTICLE_POOL_CAPACITY_REVIEW.md)
- [공용 GPU 풀](PARTICLE_BUFFER_POOL.md)
- [선택 흐름](../diagrams/particle-selection/README.md)
- [버퍼 풀 수명](../diagrams/particle-buffer-pool/README.md)

## 6. 미니언·몬스터·보스 자원과 상수 버퍼 최적화

### 해결한 문제

NPC 모델·clip·geometry는 이미 공유 중이었다.<br>
남은 병목은 upload 미회수·비압축 텍스처·객체 상수별 64KiB 할당이었다.<br>
개별 pose·위치는 유지해야 했다.

### 진행한 작업

- GPU copy 완료 뒤 미니언·몬스터뿐 아니라 타인·타워 공격·스킬 객체/모델·파티클 텍스처까지 upload 반환 순회를 보완했다.<br>
  DEFAULT geometry·texture와 매 프레임 상수는 유지했다.
- NPC 고유 DDS 25장 중 비압축 24장을 같은 해상도·단일 mip·UNORM 해석의 BC7으로 변환했다. 이미 R8인 Metallic 한 장은 원본 그대로 유지했다.
- 미니언 12개와 일반 몬스터 9개의 계층에 있는 **808개 논리 객체 상수 영역**을 256byte 정렬·2프레임 `ObjectConstantArena`로 배치했다.<br>
  draw마다 새 주소와 값을 기록하고 마지막 제출 fence 완료 후 재사용한다.
- capacity 부족 시 64KiB 페이지를 추가하고, 장면 종료에 확장 페이지까지 반환했다. material CB 반복 생성으로 공유 포인터를 덮어쓰는 경로도 보완했다.
- 종료 검증에서 발견한 11-on-12 wrapped resource 중복 반환을 제거하고 Flush 후 GPU 완료를 기다리도록 했다.

### 결과와 검증

| NPC 자원 | 변경 전 | 변경 후 | 감소 |
|---|---:|---:|---:|
| 고유 DDS 25장 payload/실제 DEFAULT 합계 | 441.2500MiB | 113.3125MiB | 327.9375MiB |
| Minotaur 몸통·도끼 6장 payload, 위 합계에 포함 | 240MiB | 60MiB | 180MiB |
| 객체 상수 committed allocation | 50.5MiB | 0.5MiB | 50.0MiB |

공용 DDS upload **28개/125.3125MiB**를 회수해 해당 cache의 upload를 0개로 줄였다.<br>
클라이언트의 모든 UPLOAD를 해제한 것은 아니다.<br>
압축·회수 대상이 겹치므로 위 자원 절감을 Private 감소량에 더하지 않는다.

모델 DDS 공유는 전후 양쪽에 이미 적용되어 있다. 전체 Private 감소량은 아래에 정리했다.

실제 Deferred 셰이더로 **9종 앞/뒤 전후 36개 PNG**를 비교했다.<br>
Minotaur를 포함해 큰 파츠·형태 누락은 보이지 않았다.

- BC7 품질: DDS 최소 채널 PSNR 33.20dB, 화면 foreground 최소 33.32dB.
- arena: 성장·frame 격리·readback·fence 대기·종료 live page 0 검사.
- upload 회수 후 렌더링 확인.

BC7은 손실 압축이다. 모든 거리·조명·포즈·전투는 미검증이다.

**결과물:** upload 반환·BC7 DDS 24장·`ObjectConstantArena`·9종 전후 화면·측정 자료.<br>
개별 pose·위치를 유지하고 임시 자원·할당 낭비를 줄였다.

### 메모리 감소량

NPC upload 회수·BC7·상수 arena의 Private Bytes는 **691.89MiB / 26.05% 감소**했다.<br>
변경 전 **2,656.35MiB → 변경 후 1,964.46MiB**, 별도 바이너리·같은 fixture 각 3회 중앙값 기준이다.

**기존 보고서:**

- [NPC 공통화 검토·DDS 공유](NPC_RESOURCE_SHARING_REVIEW.md)
- [NPC 최종 구현·실측](NPC_MEMORY_OPTIMIZATION.md)
- [9종 전후 갤러리](evidence/npc-memory-20261005/gallery.html)
- [상수 수명도](../diagrams/npc-memory/README.md)

## 7. 인게임 UI BC7 압축과 사전 로딩 유지

### 해결한 문제

UI는 비압축 32bit DDS로 승패·상점·스킬·게이지·미니맵·피격/가속 효과를 사전 로딩했다.<br>
첫 표시 지연을 피하기 위해 로딩 시점은 유지하고 텍스처를 압축했다.

### 진행한 작업

- 플레이어/보스 전체 고유 DDS **21장**을 단일 mip `BC7_UNORM`으로 변환했다. 한 역할에서 실제 생성하는 고유 texture는 **20종**이다.
- 4의 배수가 아닌 **11장**은 resize 대신 내용 픽셀을 그대로 두고 1px wrap gutter와 4배수 패딩을 추가했다.
- DDS `UIB7/v1` 내용 영역을 읽어 texture→독립 UI mesh 상수로 전달하고 atlas/progress 계산 뒤 UV scale/offset을 적용했다.<br>
  Speed blur의 각 샘플에도 보정했다.
- BC7 블록을 GPU에 그대로 로딩하고 기존 UI 생성·표시 시점을 유지했다. 내용 영역·version·형식·offset·mip 오류 및 생성 실패의 반환 경로를 검증했다.
- shadow map **8192**와 위치 G-buffer **R32G32B32A32_FLOAT**를 실제 resource 형식·원본 소스 해시로 유지 확인했다.

### 결과와 검증

| UI 자원 지표 | 변경 전 | 변경 후 | 감소 |
|---|---:|---:|---:|
| 플레이어 20종 DDS payload | 142.29MiB | 35.68MiB | 106.61MiB |
| 플레이어 실제 DEFAULT allocation | 147.6250MiB | 41.9375MiB | 105.6875MiB |
| 플레이어 남은 UPLOAD allocation | 143.4375MiB | 36.8750MiB | 106.5625MiB |
| 보스 실제 DEFAULT allocation | 143.4375MiB | 40.5625MiB | 102.8750MiB |

동일 Release EXE·외형/스킬·파티클 19쌍/재사용 30회 조건에서 에셋만 바꿨다.<br>
플레이어 전체 프로세스를 각 3회 측정했다. 보스 전체 메모리는 별도 3회 비교하지 않았다.

플레이어/보스 각 16화면의 전후 **64개 PNG/32쌍**을 남겼다.

- 대상: HUD·상점·승패·투명 효과·Speed 4프레임·스킬/아이템 칸·5단계 게이지.
- 품질: foreground 최소 PSNR 41.34dB, 최대 RGB 오차 87/255.
- 큰 atlas·글자 누락은 보이지 않았다. 무손실 압축은 아니며 모든 시간·DPI는 미검증이다.

Debug/Release 빌드와 실제 역할별 렌더링·내용 계약 오류 9개 거부·복사 참조/UV·GPU 완료 후 UI DEFAULT/UPLOAD 40개 종료 잔여 참조 0을 확인했다.<br>
UI upload 조기 회수는 아직 미적용이며 압축으로 그 크기를 줄인 것이다.

기존 경고와 검사 결과는 다음과 같다.

- compiler signedness/narrowing, `Shader.cpp` C4267 경고 유지.
- D3D12 초기화 ID1328: 플레이어 1,511개·보스 1,435개.
- 검사 범위의 GPU error/device removed: 0. 전체 게임 플레이는 미검증.

**결과물:** BC7 DDS 21장·내용 영역 metadata·UV 보정·GPU/수명 검사·전후 화면 32쌍.<br>
기존 사전 로딩 시점을 유지했다.

### 메모리 감소량

UI BC7 압축의 Private Bytes는 **208.09MiB / 10.59% 감소**했다.<br>
기존 에셋 **1,964.52MiB → 압축 에셋 1,756.43MiB**, 동일 바이너리·에셋만 변경한 플레이어 역할 각 3회 중앙값 기준이다.

**기존 보고서:**

- [UI 최종 구현·실측·품질](UI_TEXTURE_COMPRESSION.md)
- [전후 화면 갤러리](evidence/ui-bc7-20261005/gallery.html)
- [로딩·UV 구조도](../diagrams/ui-bc7/README.md)

## 8. 조사 내용과 적용하지 않은 후보

아래 후보는 **미적용 또는 적용 제외** 항목이다.<br>
예상 절감량은 클라이언트 1개 기준의 자원 계산값이다. Private Bytes 실측값이 아니다.<br>
같은 자원의 대안·중첩 범위는 합산하지 않는다.

### 하늘 큐브맵

현재 하늘은 비압축 2048²×6면·단일 mip로 **96MiB payload**를 사용한다. alpha는 모두 255였다.

| 미적용 후보 | 예상 payload | 예상 절감 | 조사 결과·남은 조건 |
|---|---:|---:|---|
| 같은 해상도 BC7·단일 mip | 24MiB | **72MiB** | 6면 경계·별·밝기 threshold·반짝임·시간 회전의 품질 비교 필요 |
| 같은 해상도 BC7·전체 12 mip | 약 32MiB | **약 64MiB** | mip를 추가하는 별도 대안. 샘플링·화질 검증 필요 |
| 같은 해상도 BC1·단일 mip | 12MiB | **84MiB** | 작은 별과 threshold 변화 위험 |
| 1024² BC7·단일 mip | 6MiB | **90MiB** | 해상도 감소에 따른 세부 표현 손실 위험 |

네 행은 서로 대안이다. 하늘 upload는 이미 fence 후 반환하므로 추가 절감에 포함하지 않는다.<br>
Scene 간 캐시는 최종 인게임 상주량을 줄이지 않는다.

### UI 임시 버퍼와 dissolve

| 미적용 후보 | 예상 절감·지표 | 조사 결과·남은 조건 |
|---|---|---|
| UI upload 조기 반환 | **플레이어 36.8750MiB / 보스 35.8125MiB allocation** | BC7 적용 후에도 남은 staging buffer. 복사 완료 후 모든 참조 반환과 재진입·실패 경로 검증 필요 |
| dissolve upload 조기 반환 | **미산정** | Framework 소유 자원의 반환 순회 누락 확인. 독립 upload allocation은 아직 측정하지 않음 |
| dissolve BC4·동일 해상도/mip | **0MiB payload** | 현재 BC1 1024²·11 mip의 0.666679MiB와 크기가 같음 |
| dissolve R8·동일 해상도/mip | **절감 없음, 약 0.666654MiB 증가** | 약 1.333333MiB로 커지므로 메모리 절감 목적에 부합하지 않음 |
| dissolve BC1·512²/10 mip | **0.50MiB payload** | 약 0.166679MiB로 감소. 절감이 작고 소멸 경계 품질 비교 필요 |
| 비소멸 draw의 dissolve sample 생략 | **텍스처 상주량 절감 0MiB** | GPU 시간 후보. compiled shader와 실제 GPU 시간은 미측정 |

`ingame_ui_dissolve_ready`는 UI와 캐릭터 소멸 마스크 생성을 합친 checkpoint다.<br>
UI shader가 dissolve 마스크를 사용하는 구조는 아니다.<br>
위 UI upload 절감은 이미 적용한 UI 압축 절감과 중복 계산하지 않는다.

### 음원·Title·추가 메모리 후보

| 미적용 후보 | 예상 절감·지표 | 조사 결과·남은 조건 |
|---|---|---|
| 인게임에서 Title/Lobby/Ready BGM 제외 | **75.44MiB PCM** | 전체 142개 음원의 PCM 122.86MiB 중 비인게임 BGM.<br>모든 효과음 유지 시 PCM 약 47.42MiB.<br>장면 전환·channel 수명 검증 필요 |
| BGM 4곡 streaming | **최대 86.48MiB PCM 선적재 회피** | FMOD NOSOUND에서 stream 생성 확인. 실제 stream buffer가 남으며 playback·loop·전환은 미검증. 위 75.44MiB와 중첩 |
| 선택 스킬 음원만 로딩·중복 음원 반환 | **미산정** | 스킬 이벤트 의존성 조사와 `Jump.wav` 중복 삽입 실패의 소유권 보완 필요 |
| Title 전용 자원 추가 회수 | **추가 절감 미확인** | Title DDS 3장 payload 4.21MiB와 UI/player/camera는 장면 전환 반환 경로가 이미 있음.<br>기동 약 632MiB 전체를 Title 잔존량으로 보지 않음 |
| 렌더 타깃 생성을 인게임까지 지연 | **최종 인게임 절감 0MiB** | G-buffer 6장 약 71.19MiB와 shadow 256MiB의 생성 시점을 늦추는 후보. 기동 peak 영향과 인게임 상주량 감소를 구분 |
| 영웅 불변 clip 추가 공유·선택 clip 로딩 | **미산정** | 선택 외형 적용 후 기준으로 clip 의존성·개별 pose·공유 범위를 재조사해야 함 |
| 입자 초기 용량 조정·유휴 풀 축소 | **미산정** | 실제 전투 peak·동시 효과 수와 fence 안전성에 따라 달라짐. 고정 재생의 19쌍을 실전 상한으로 보지 않음 |
| Speed blur 셰이더 변경 | **미산정, GPU 시간 중심 후보** | 코드상 225 sample/픽셀. 별도 pass는 렌더 타깃 메모리를 늘릴 수 있으며 실제 GPU 시간은 미측정 |

PCM 계산은 FMOD 음원 정보에 근거하며 Private Bytes 감소량을 뜻하지 않는다. streaming 조사도 재생 전 NOSOUND 조건이므로 실제 오디오 동작 검증을 대체하지 않는다.

### 사용자 지시로 제외한 후보

| 적용 제외 후보 | 계산상 예상 절감 | 제외 이유 |
|---|---:|---|
| shadow map 8192² → 4096² | **192MiB 논리 GPU texel**: 256 → 64MiB | 그림자 품질 유지. 현재 8192 유지 |
| 위치 G-buffer FP32 → FP16 | **약 15.82MiB 논리 GPU texel**: 31.64 → 15.82MiB, 1920×1080 기준 | 위치 정밀도와 그림자 등의 영향으로 제외. 현재 `R32G32B32A32_FLOAT` 유지 |

**조사 결과물:** DDS·alpha 검사, GPU allocation/수명 조사, 음원 PCM/stream 조사, 후보 계산 JSON.<br>
절감량을 계산하지 못한 후보는 미산정으로 표시했다.

**상세 자료:**

- [기동·Title·음원·렌더링 조사](STARTUP_RESOURCE_OPTIMIZATION_REVIEW.md)
- [하늘·UI/dissolve 조사](SKYBOX_DISSOLVE_OPTIMIZATION_REVIEW.md)
- [계산과 출처 근거](evidence/final-report-20261006.json)
