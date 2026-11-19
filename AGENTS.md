# DrSample 작업 규칙 (AI 에이전트 공통)

- 이 저장소는 DrFramework의 **샘플 프로젝트(참고 구현)**다. 기반 코드는 서브모듈 `Plugins/DrFramework`(DrFramework 저장소), UI 흐름·부품은 서브모듈 `Plugins/DrUIKit`(DrUIKit 저장소)에 있다. 기반을 바꿔야 하면 DrFramework 저장소에서 고치고(그쪽 AGENTS.md 규칙), 여기서는 서브모듈 커밋만 올린다. 서브모듈 안을 직접 고쳐 커밋하지 않는다.
- **엔진: UE 5.9 기준, 5.8 하위호환 유지.** 5.8에 없는 API는 `#include "Misc/EngineVersionComparison.h"` 후 `#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 9, 0)`로 나누고, 5.9에서 지원 중단된 API는 5.8에도 있는 대체 API로 바꾼다. 빌드·테스트는 5.9로 하고, 5.8 엔진이 있으면 5.8로도 확인한다. **에셋(.uasset·.umap)은 5.8 에디터로 저장한다** — 5.9에서 다시 저장한 에셋은 5.8에서 열리지 않는다. 5.9 에디터·커맨드릿으로 에셋을 만들거나 고쳐야 하면 먼저 사용자와 상의한다.
- **구조 변경은 같은 작업에서 문서에 반영한다.** 샘플 클래스·모듈·데이터 흐름·테이블 스키마·이름 규칙·설정이 바뀌면 `Docs/SAMPLE_DESIGN.md`와 `README.md`를 함께 고친다.
- 장르를 제한하는 표현을 쓰지 않는다. 전투·보스 등은 "예시 구현"으로 표기한다.
- 샘플 접두사는 `Ds`, 테스트는 `DrSample.*`. 기반 이름(`Dr`)은 바꾸지 않는다.
- Epic 콘텐츠(Lyra 등)는 저장소에 넣지 않는다. `Tools/import_epic_content.ps1`이 원래 패키지 경로(`Content/Characters` 등, gitignore)로 복사한다.
- `TableData/Generated`는 손으로 고치지 않는다. 스키마(`Source/DrSample/Tables`)를 고치고 `Tools/gen_tables.ps1`로 다시 만든다. DrTableSystem은 서브모듈 `Plugins/DrTableSystem`이다.
- 사람에게 보이는 글은 한국어로 쓴다.
