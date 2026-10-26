# 샘플 콘텐츠 가져오기와 인계 노트

## 현재 상태 (2026-10-27, 작업 #65 이후)

- 재구성(#65)으로 샘플 클래스가 Ds 접두사로 바뀌었다: `UDsSampleAnimInstance`·`ADsSampleEnemy`·`UDsSampleContent`(`Source/DrSample/DsSampleContent.h`). `Tools/build_sample_content.py`도 Ds 클래스를 쓰도록 고쳤고, 부모가 바뀐 에셋(플레이어·적·게임 모드 BP, ABP, DataAsset 2개, 맵)은 지우고 스크립트로 다시 만들었다. 입력 액션·몽타주는 클래스 참조가 없어 유지했다.
- 테이블은 새 스키마로 다시 구웠다(`Content/Data`: DA_GameAssets·DA_EffectDefs·DA_BossAttack·DA_BossPhase·DA_EffectExtensions).
- `DrSample.PIE.BasicCombat` 통과(이동, 공격 명중·피격, 적 추적·반격·플레이어 피격, 적 사망). 전체 자동화 70/70(당시).
- 아래 #64 기록의 `DrSampleContent`·`ADrSampleEnemy`·`DrTestFixture.h` 같은 이름은 #64 당시 이름이다. 지금 이름은 위와 같고, 테스트 픽스처는 샘플 `Source/DrSample/Tests/DsTestFixture.h`, 기반 `Plugins/DrFramework/Source/DrFramework/Tests/DrTestWorld.h`·`DrTableTestFixture.h`다.
- 이후(같은 날) Abilities·캐릭터 골격·상태 효과·GameAssets·EffectDefs도 샘플로 옮겼다. 기반 ASC·어빌리티·캐릭터는 Ds 클래스와 합쳤고, 테이블 에셋은 Ds 클래스로 `-Force` 재굽기했다. 샘플 BP는 재생성 없이 그대로 동작했다. DrTableSystem은 git 서브모듈이 됐다. 전체 자동화 72/72.
- 저장소 분리(10/27): 이 저장소(DrSample)는 샘플 프로젝트이고 DrFramework는 플러그인 전용 저장소의 서브모듈이다. 프로젝트 파일은 `DrSample.uproject`.
- 남은 일: 렌더링 캡처(셰이더 작업 폴더 권한 문제), 피격 방향 4종 선택(#849), 메뉴·사운드·FX 콘텐츠.

## 작업 #64 기록

2026-10-27 Bridge 메시지 #855로 **#64 최종 범위는 Epic 콘텐츠 가져오기 도구까지만** 축소됐다. 이 메시지를 보고 직전 수신함 확인에서 읽기 전에 생성한 샘플 소스·에셋은 지우지 않고 작업트리에 보존한다. 후속 #65 재구성 후 이름·모듈 경계·BP 부모를 다시 맞춰야 하므로 현재 샘플을 최종 배포본으로 취급하지 않는다. 커밋·브랜치 변경은 하지 않았다.

### 완료한 가져오기 도구

- Tools/import_epic_content.ps1: Lyra 원본 읽기 전용, 목록 경로 검증, 원래 패키지 경로 복사, 에디터 정리, 입력·출력 해시 상태 저장. 상태는 Saved/import_epic_content-state.json으로 Git에서 제외된다. 정리 스크립트/DrSample DLL이 바뀌면 상태를 무효화한다.
- Tools/prepare_epic_content.py: 선택 루트에서 AssetRegistry 하드·소프트 패키지 의존성 폐쇄를 산출하거나 고정 목록을 정리한다. Tools/epic_content_manifest.txt 115개.
- DrSampleContent의 CleanAnimation/CleanPhysics는 가져오기 도구가 사용하는 에디터 보조 함수다. #65에서 샘플 클래스 이름을 바꾸더라도 이 Python 호출과 DLL 참조를 함께 갱신해야 한다.
- 제외 루트: Content/Characters, Content/Effects, Content/PhysicsMaterials. Git 커밋 후보에 Epic 원본 0개.
- 정리 참조: 로드 불가/게임 BP/LyraGame 노티파이, AnimationModifiers 사용자 데이터(발소리 수정자 경로 포함), PA_Mannequin의 Lyra 전용 PhysicalMaterial(PhysMat_Player_WeakSpot). 복사본만 저장한다.
- 빈 상태 115개 재설치 성공. 즉시 재실행 시 SHA256 차이 0개. 첫 정리 시 원본 Lyra 클래스가 없다는 로드 경고는 있으나 커맨드릿 오류 0개다.

### 이미 생성한 샘플 — 보존·후속 검토 대상

- Source/DrSample/DrSampleContent.h/.cpp: UDrSampleAnimInstance(Speed), ADrSampleEnemy(피격 연출), UDrSampleContent(애님 그래프 생성·Epic 정리).
- Source/DrSample/DrSampleBasicCombatTest.cpp: DrSample.PIE.BasicCombat. DrSample.Build.cs의 런타임/에디터 링크 의존성 추가.
- Content/DrSample 23개: 플레이어·적·게임 모드 BP 3개, ABP 1개, 몽타주 5개, DataAsset 2개, 입력 액션 10개·매핑 1개, 맵 1개.
- Content/Data 4개: DrTableBake의 BossAttack/BossPhase/EffectDefs/GameAssets.
- Design/Tables/{GameAssets,BossTables}.xlsx: 샘플 경로와 Swing 두 행. 스키마·생성 C++는 의미 변경 없음.
- Config/DefaultEngine.ini·DefaultGame.ini: 샘플 맵·게임 모드와 AssetRoot=/Game/Data.
- Plugins/DrFramework/Source/DrFramework/Tests/DrTestFixture.h: 단위 테스트가 게임 전투 수치에 의존하지 않도록 기본값을 잠시 적용하고 종료 시 복구.
- Tools/build_sample_content.py: 에셋 재생성. README와 FRAMEWORK_FOR_AI의 샘플 절은 이 중간 상태의 연결을 기록한다.

### 프레임워크 후보 기능과 후속 확인

현재 ADrBossCharacter의 일반 피해 처리는 페이즈 검사만 하며 일반 피격 연출이 없다. 공용 전투 구조를 바꾸지 않기 위해 ADrSampleEnemy의 일반 피해 델리게이트에서 피격 몽타주를 재생했다(#849 A 승인). 현 구현은 앞 방향만 연결되어 있어 **#849의 앞/뒤/좌/우 선택은 아직 미반영**이다. #855 축소에 따라 더 구현하지 않고 후속 작업에 넘긴다. 현재 사망한 적은 피격 몽타주를 재생하지 않는다. 보스 공격·사망은 기존 시퀀스 경로다.

추가 검토: 실제 키보드 WASD modifier 저장 상태, NavMeshBoundsVolume의 브러시/내비 데이터와 렌더링 장면을 확인한다. NullRHI 이동 테스트는 EnhancedInput의 Move 액션을 주입하며 데스크톱 키를 주입하지 않는다.

### 실행 증거

- DrSampleEditor Win64 Development: Result: Succeeded (Saved/task64-build-final.log).
- Automation List; RunTests DrFramework+DrSample: 65/65 Success, Fail 0 (Saved/task64-tests-pass.log). PIE의 a 이동, b 플레이어 공격·적 피격, c 적 추적·공격·플레이어 피격, d 적 사망 통과.
- Saved/task64-import-verification.txt: 빈 상태 설치 후 재실행 SHA256 차이 0개.
- 렌더링 시도는 셰이더 작업 폴더 C:/Users/ldnjin/UnrealShaderWorkingDir 쓰기 거부로 종료됐다. PNG 미생성. Saved/task64-render.log. **최종 축소 범위에서는 렌더링 검증 대상 제외**이며 후속 작업에서 샌드박스 밖 대행이 필요하다. 우회하지 않았다.
- 제안 커밋 제목: `Add reproducible Lyra content importer`. 포함 범위와 나머지 보존 파일의 커밋 분리는 Claude가 결정한다.
