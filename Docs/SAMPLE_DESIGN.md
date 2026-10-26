# DrSample 샘플 설계 설명서 — DrFramework 참고 구현 (UE 5.8.2)

이 문서는 [DrFramework](https://github.com/ldnjin81/DrFramework) 기반을 상속해 게임을 만드는 참고 구현(샘플 프로젝트)의 재현 명세다. 기반 자체는 [설계 명세서](https://github.com/ldnjin81/DrFramework/blob/main/Docs/FRAMEWORK_FOR_AI.md)를 따른다. 의존 방향은 `DrSample → DrFramework`, `DrSample → DrTableSystem`이며 두 플러그인은 git 서브모듈(`Plugins/DrFramework`, `Plugins/DrTableSystem`)이다. 게임은 이 샘플을 포함하지 않고 참고만 한다. 샘플 타입·파일·범주는 Ds 접두사, 테스트는 DrSample을 사용한다.

## 기반에서 확장한 지점

| 기반 클래스 | 샘플 파생 | 재정의/추가 동작 |
| --- | --- | --- |
| UDrGameInstance | UDsGameInstance | GetBootSteps·ExecuteBootStep·OnBootFinished: 연결→Ping→로그인→프로필 |
| ADrGameMode | ADsGameMode | 기본 캐릭터·컨트롤러 지정, GameAssets.Player 로드 |
| UDrGameUserSettings | UDsGameUserSettings | SFX·Voice·Music 저장·기본값 |
| ACharacter(엔진) | ADsCharacter | ASC 하위 객체·IAbilitySystemInterface, 전투 속성·입력·카메라·메시·FX |
| UDrMenuWidget | UDsMenuWidget → 제목/일시정지/설정 메뉴 | RebuildWidget·BuildMenu: 글래스 레이아웃·버튼·문자열·음량 슬라이더 |
| UDrHUDWidget | UDsCombatHUDWidget | RebuildWidget·NativeTick: 전투 게이지·표시 모델·잔상 |
| ADrPlayerController | ADsPlayerController | OnHUDCreated·OnPawnChanged·CanOpenPauseMenu·RequestRestart: 전투 HUD·사망 중 차단·체크포인트 |
| UDrContentBase | UDsPlayerProfileContent | 메시지 ID별 프로필 응답·변경 알림 |
| UDrSmokeSubsystem | UDsSmokeSubsystem | title·travel·player·boss·tables·assets·hud·pause_open·settings·pause_close 콜백 등록 |
| UDrPerfSubsystem | UDsPerfSubsystem | 카메라 경로·구간 샘플 수집, 기반 Summarize로 frame/game/gpu 평균·p95·최댓값 계산 |

## 상태 효과·ASC·어빌리티 (샘플 소유)

기반 플러그인에는 GAS 코드가 없다. 아래 체계는 샘플의 `UDsAbilitySystemComponent`·`UDsGameplayAbility`·`ADsCharacter`가 한 클래스씩으로 구현한다(이전의 Dr 기반 + Ds 파생 2단을 합침). 다른 게임은 이 구현을 복사해 시작하거나 자기 방식을 쓴다.


UDsAbilitySystemComponent는 매 틱 활성 UDsGameplayAbility::AbilityTick, 한시 효과·주기 피해, 쿨다운을 진행한다. 효과는 태그별 한시/영구 맵에 저장하며 같은 태그에는 한시 효과가 우선한다. 이동·받는 피해·주는 피해 배수는 각 효과의 곱이다. Refresh는 남은 시간을 갱신하고 Accumulate는 StackBonus 길이까지 단계를 올린다. 확대 비율은 `1+StackBonus[단계-1]/100`이며 배수는 1에서의 차이를 확대한다. 재적용은 주기 위상을 초기화하지 않는다. 만료 시각과 같은 주기 피해는 포함한다. 유효하지 않은 태그 또는 0 이하 지속시간은 적용하지 않는다.

| 연결부 | 기본 동작 | 샘플 구현 |
| --- | --- | --- |
| ApplyDirectDamage | 아무 동작 없음 | 주기·즉시 피해를 게임 속성에 적용 |
| GetCurrentHealth | 0 | 현재 값 제공 |
| OnEffectApplied | 아무 동작 없음 | 게임별 추가 효과 처리 |
| IsShieldEffect | false | 흡수 풀로 쓸 효과 선택 |
| ApplyEffectById | 공통 EffectDefs 조회·대상 선택 | 필요할 때 재정의 |
| UDsGameplayAbility::AbilityTick | 아무 동작 없음 | 시간 구간 진행 |
| ADsCharacter::InitializeAbilitySystem | ASC ActorInfo 초기화 | 속성·어빌리티 초기화 추가 |

흡수 풀은 선택된 효과의 Magnitude로 초기화하고 소진되면 해당 효과를 해제한다. UDsGameplayAbility는 InstancedPerActor·LocalPredicted이고 CooldownTag가 있으면 발동을 막는다. CommitCooldown은 ASC의 틱 기반 쿨다운을 시작한다. 게임 자원 비용과 동작 상태 태그는 게임 파생 어빌리티가 구현한다.

ADsCharacter는 기본 ASC 하위 객체 AbilitySystem을 생성·복제 설정하고 IAbilitySystemInterface를 구현한다. ASC 하위 객체는 UDsAbilitySystemComponent로 직접 생성한다.


테이블: 샘플이 GameAssets·EffectDefs·BossPhase·BossAttack·EffectExtensions를 모두 소유한다(`Source/DrSample/Tables/{Schemas,Enums,Data}`, `--prefix Ds`, 등록은 `TableData/DsTables.cpp` 한 곳). 에셋 경로 조회 도우미는 `DsGameAssets::Path`다.

## 모듈과 데이터 경계

- `Source/DrSample/Abilities`, `Characters`, `Combat`, `Boss`, `Encounter`, `Audio`, `Core`, `UI`, `Game`, `Debug`가 예시 동작을 소유한다.
- `Tables/Schemas`의 GameAssets·EffectDefs·BossAttack·BossPhase·EffectExtensions 스키마와 `Tables/Enums`(EffectApplyTo·EffectCategory·EffectStackRule·BossAttackKind), `Tables/Data` 원본 데이터는 모두 샘플 소유다. `Tools/gen_tables.ps1`이 `--prefix Ds`로 한 번 생성하고 `TableData/DsTables.cpp`가 등록한다. DrTableSystem은 git 서브모듈 `Plugins/DrTableSystem`(v0.10.0)이다.
- EffectDefs에는 포이즈 같은 전투 전용 칸이 없다. `DsEffectExtensions`는 같은 효과 Id에 대한 ProcPoiseDamage를 소유한다. 기반 런타임 효과의 DefinitionId를 통해 OnEffectApplied에서 조회하고, Accumulate 중첩 보너스에 따라 포이즈 피해도 확대한다.
- BossAttack.Effect는 EffectDefs의 정수 Id다. 0은 효과 없음이다.
- 네트워크 메시지는 `DsMessages`의 FName 상수(Ping·Login·RequestPlayerProfile·ServerPush_ProfileUpdated)다. 부팅 Connect 단계가 가짜 서버의 로그인·프로필 처리기를 등록한다. 기반 네트워크는 로그인 구조체를 모른다.
- CommonUI의 클래스·입력·맵은 `Config/DefaultGame.ini`의 UDrUISettings, 콘텐츠 목록은 UDrContentSettings에서 설정한다. 기반 컨트롤러는 샘플 GameAssets 키를 조회하지 않는다.
- `DsSmokeTest`, `DsPerfCapture`, `DsAutoNetSmoke`는 샘플 실행 옵션이다. 스모크는 타이틀 맵과 콘텐츠 준비가 전제다.

## 재현과 콘텐츠

1. 기반 플러그인 둘을 활성화하고 DrSample 모듈을 추가한다. 모듈 의존성은 DrSample.Build.cs를 기준으로 한다.
2. 테이블 생성 후 DrSampleEditor를 빌드한다. 게임 인스턴스·사용자 설정·게임 모드는 각각 DsGameInstance·DsGameUserSettings·DsGameMode로 지정한다.
3. 테이블은 `Tools/gen_tables.ps1`(Ds 한 번 생성, 행 구조체 `DRSAMPLE_API` 내보내기 포함) 뒤 DrSampleEditor를 빌드하고 `-run=DrTableBake -Input=%TEMP%/DrSample-DrTable/sample/client -Out=/Game/Data`로 굽는다. 클래스 이름이 바뀌었으면 `-Force`.
4. 샘플 에셋은 `Tools/build_sample_content.py`가 만든다. 플레이어 BP 부모는 `ADsCharacter`, 적은 `ADsSampleEnemy`, 애님 BP는 `UDsSampleAnimInstance`, 게임 모드 BP는 `ADsGameMode`, 전투 데이터는 `UDsPlayerCombatData`·`UDsBossData`다. 스크립트는 이미 있는 에셋을 재사용하므로, 부모 클래스를 바꿀 때는 Characters·Combat·Maps 폴더와 ABP·게임 모드 BP를 지우고 다시 실행한다(#65에서 이 방식으로 Dr→Ds 부모를 갱신했다. CoreRedirects는 쓰지 않는다).
5. Lyra 가져오기 도구는 기존 설치 경로를 보존하며, #64 도구와 SAMPLE_NOTES.md를 따른다. Epic 원본을 저장소에 넣지 않는다.
6. 자동화는 `Automation List; Automation RunTests DrFramework+DrSample`로 실행한다(72개: 기반 5, 샘플 67). `DrSample.PIE.BasicCombat`은 `L_DrSample`에서 이동→공격 명중→적 추적·반격→사망을 검증하며 맵이 없으면 경고 후 건너뛴다. 재구성 전 64개의 새 이름은 Docs/task65-test-map.csv다.

## 기존 예시 구현의 상세 명세와 수치

아래는 이전 통합 명세의 상세 동작·수치·테스트 기대값을 보존한 이식 자료다. **경로·모듈 배치(예: `Plugins/DrFramework/...`의 테이블·GAS 코드, `Design/Tables`, `--prefix Dr`)는 재구성 전 기준이라 지금과 다르다. 경로와 소유는 위 절이 우선한다.** 클래스 이름은 이동한 Ds 이름으로 갱신했다. 위의 경계·상속·설정 설명이 우선하며, 아래의 이전 부팅·컨트롤러·디버그 연결 및 기반/샘플 위치 표기는 위 구조로 대체한다. 

### 이전 통합 명세 원문 (예시 구현 상세)

> 용도: 언리얼 엔진으로 게임을 새로 시작할 때 AI 에이전트가 따르는 게임 코드 설계.
> 프레임워크 플러그인은 `Dr`, 게임 모듈은 자기 접두사를 쓴다. `<Game>` = 소비 게임 모듈 이름. 수치는 권장 시작값. 사람용 설명은 `../README.md`.
> 범위: 게임 런타임 코드만. 작업 방식·협업·도구는 다루지 않는다.

---

## 적용 범위

- 장르 무관 공통 기반: 부팅·네트워크·가짜 서버·정적 테이블·콘텐츠 캐시·UI·설정·엔카운터·오디오·디버그/검증.
- 포함된 예시 구현: 근접 전투·GAS 속성과 효과·캐릭터·락온·타격감·보스·전투 FX. 필요 없으면 빼거나 바꿔 쓴다. 아래의 보스·아레나·근접 전투 수치는 예시 구현의 재현 기준이며 프레임워크를 사용하는 모든 게임의 요구 사항은 아니다.
- 사람용 시스템 개요·관계도·표는 저장소 루트 README.md를 따른다. 클래스와 코드 이름은 유지한다.

## 플러그인 배포·재현 명세 (1단계)

프레임워크 플러그인은 **Dr**, 게임 모듈은 자기 접두사를 쓴다. 아래의 UDr/ADr/FDr/EDr는 플러그인 소유 API이며 게임별 복제 대상이 아니다. `<Game>`은 소비 게임 모듈 이름이다.

| 경계 | 소유 내용 | 소비 게임이 할 일 |
| --- | --- | --- |
| Plugins/DrFramework | GAS·전투·캐릭터·보스·엔카운터·네트워크·UI·오디오 런타임, 생성 테이블 타입 | DrFramework 모듈에 의존하고 ADsCharacter/ADsBossCharacter/ADsGameMode/ADsPlayerController/UDsGameInstance 등을 상속·설정한다 |
| Plugins/DrTableSystem | DrTableRuntime 조회와 DrTableEditor 굽기 도구(0.10.0) | 이미 생성된 타입을 다시 중복 생성하지 않는다 |
| Source/DrSample | 모듈 등록과 빌드 타깃만 있는 검증 호스트 | 게임 코드를 넣지 않는다. 이 샘플에는 맵·BP·에셋이 없다 |
| Plugins/DrFramework/Tables/Schemas, Enums | 프레임워크 테이블 스키마·열거형 | 플러그인 API로 취급한다. 스키마 변경 뒤 C++를 재생성하고 소비 게임 데이터를 다시 굽는다 |
| Design/Tables | 게임 소유 데이터 엑셀(샘플 BossTables.xlsx, DrTables.xlsx, GameAssets.xlsx) | 자기 게임 경로와 밸런스로 채운다. GameAssets의 Player/PlayerCombat/CombatFX/Boss는 현재 경로가 비어 있다 |

**다른 엔진에서 재현하는 순서**
1. 최소 UE C++ 프로젝트에 Plugins/DrFramework와 Plugins/DrTableSystem을 복사한다. .uproject에서 DrFramework·DrTableSystem을 활성화하고 EngineAssociation을 대상 엔진에 맞춘다. .Target.cs는 UE 5.8 기준 BuildSettingsVersion.V7·IncludeOrderVersion.Latest이며 다른 버전에서는 해당 엔진에서 지원하는 값으로 바꾼다.
2. 게임 Build.cs의 PublicDependencyModuleNames에 DrFramework를 추가한다. DrFramework.Build.cs는 원본 런타임 모듈 의존성을 유지하며 PublicIncludePaths.Add(ModuleDirectory)로 `Characters/DsCharacter.h` 같은 경로를 노출한다. 모듈은 IMPLEMENT_MODULE, 게임 호스트는 IMPLEMENT_PRIMARY_GAME_MODULE을 쓴다.
3. DrFramework.uplugin은 DrTableSystem, GameplayAbilities, EnhancedInput, CommonUI, ModelViewViewModel, AudioModulation, Metasound, Niagara, EngineCameras, PoseSearch, Chooser, MotionWarping을 활성화한다. 실제 C++ 링크 모듈은 Build.cs가 기준이다. UnrealEd·AssetRegistry는 에디터 빌드에서만 의존한다.
4. Config/DefaultEngine.ini의 GameInstanceClass=/Script/DrFramework.DsGameInstance, GameUserSettingsClassName=/Script/DrFramework.DsGameUserSettings와 CommonGameViewportClient를 설정한다. EnhancedInput 클래스와 CommonInput의 Enhanced Input 지원은 샘플 Config를 따른다. 맵·InputData·ControllerData·Cook 목록은 게임 콘텐츠를 만든 뒤 게임이 설정한다.
5. DrTable 0.10.0 실행 파일 경로를 지정하여 `powershell -File Tools/gen_tables.ps1 -DrTable <drtable.exe>`를 실행한다. `--prefix Dr --ue-plugin`으로 Plugins/DrFramework/Source/DrFramework/TableData/Generated에 타입·등록 코드를 만든다. 스키마/열거형은 플러그인, 데이터는 게임 경로를 입력한다. DrTables의 참조 수식 링크도 이 플러그인 스키마를 가리킨다.
6. 등록 진입점은 `DRTABLE_AUTO_REGISTER(DrGeneratedTables::RegisterAll<UDrTableSubsystem>)`이며 DrTables.cpp 한 곳에서만 호출한다. 샘플은 DrTableBake로 만든 /Game/Data를 DefaultGame.ini의 DrTableSettings.AssetRoot로 지정한다. 소비 게임도 별도 굽기 단계에서 생성한 에셋 폴더와 ExtraAssets를 설정한다.
7. GameAssets 경로와 BP/DataAsset/몽타주·애니메이션·Niagara·오디오·입력을 게임이 공급한다. 보스는 `BossAssetKey`(EditDefaultsOnly, 기본 Boss) 또는 BossData 속성으로 게임별 데이터를 지정한다. UDsBossData의 기본 BossName/DisplayName은 Boss/보스이다. 구체적인 영웅·적 이름은 게임 데이터에 둔다.
8. `Build.bat DrSampleEditor Win64 Development -project=<DrSample.uproject> -WaitMutex -UBARootDir=<쓰기 가능한 경로>`로 빌드한다. 에디터가 종료된 환경에서 `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation List; RunTests DrFramework+DrSample" -TestExit="Automation Test Queue Empty" -nullrhi -unattended -log`로 테스트한다. Automation 뒤 세미콜론 내부는 RunTests이며 Automation을 반복하지 않는다. 실행 개수가 0이면 성공으로 보지 않는다.

콘솔은 Ds.Net.Ping/Ds.Profile.Request/Ds.Tables.Reload/Ds.Net.SetLatency, 명령줄은 -DsAutoNetSmoke/-DsSmokeTest/-DsPerfCapture/-DsCaptureAudio처럼 샘플 접두사 Ds를 쓴다(기반 플러그인에는 콘솔 명령·명령줄 플래그가 없다). 액터 태그는 Ds.Arena.Door·Ds.Boss·Ds.Checkpoint.Gate·Ds.Player.Start다. State.*, Ability.*, Data.* 게임플레이 태그는 샘플(`Core/DsGameplayTags`) 소유다. 이전 이름과 새 이름은 rename_map.csv를 본다.

## 0. 설계 원칙 (모든 시스템에 적용)
1. **게임 규칙의 시간은 틱 누적으로 진행한다.** 어빌리티 타격 구간·보스 패턴·경직·리스폰·상태 효과 지속과 주기 피해·쿨다운·기력 재생 지연은 모두 `Elapsed += DeltaTime`(또는 남은 시간 −= DeltaTime)으로 잰다. 애니메이션 노티파이나 `FTimerManager`에 맡기지 않는다 → 헤드리스 테스트가 결정적이다. 타이머는 HUD 연출(잔상·페이드) 같은 표현에만 쓴다.
2. **구간 판정은 겹침으로 한다.** `if (Prev < End && Now >= Start)` — 틱이 길어도 판정 구간을 건너뛰지 않는다.
3. **표현이 없어도 규칙은 돈다.** 몽타주·메시·FX가 없으면 대체 경로(대시, 판정만)로 동작한다. nullrhi에서도 모든 규칙이 테스트된다.
4. **수치는 데이터, 규칙은 코드.** 밸런스는 DrTable(엑셀) 또는 DataAsset. 코드 상수는 공식 계수와 기본값만.
5. **이벤트로 연결한다.** 속성 세트 → 네이티브 델리게이트 → 캐릭터/엔카운터/HUD/오디오. 위젯·오디오가 게임 규칙을 직접 바꾸지 않는다.
6. **테스트 이음새를 남긴다.** 테이블 주입, 난수 시드, 테이블 서브시스템 override, `#if WITH_AUTOMATION_TESTS` 접근자, 입력 진입점 함수.
7. **엔진 최신 방식**: GAS, Enhanced Input, CommonUI, MVVM, GASP 로코모션(PoseSearch·Chooser·MotionWarping), MetaSound + AudioModulation, Niagara, `FInstancedStruct`.

## 1. 수명 계층과 소유 관계
```
UEngine
 └ UDrTableSubsystem        (엔진 서브시스템, 플러그인) — 정적 테이블 등록·로드·조회. 에디터·게임 전체에서 하나
UGameInstance (UDsGameInstance)            — 부팅 흐름
 ├ UDrNetworkSubsystem      (GI 서브시스템) — 요청/응답, 지연 큐, 연결 상태
 │   └ UDrServerEndpoint ← UDrFakeServer   — 서버 구현(교체 가능)
 ├ UDrContentSubsystem      (GI)           — UDrContentBase 파생 캐시(PlayerProfile)
 ├ UDsPerfSubsystem / UDsSmokeSubsystem (GI, Debug/ 폴더, 명령줄 플래그로만 동작) — 패키지 검증
UWorld
 ├ UDsEncounterSubsystem    (Tickable World) — 체크포인트·문·사망·승리
 ├ UDsHitFeedbackSubsystem  (Tickable World) — 히트스톱·셰이크·플래시
 └ UDsAudioSubsystem        (Tickable World) — 음악 상태·보스/피격/환경 소리·채널 음량
ADsPlayerController — UDrUIRootLayout(Game/Menu 스택), HUD ViewModel·Widget, 일시정지·설정
ADsCharacter        — ASC + AttributeSet, LockOn, CombatFX, 카메라, Manny 표시 메시
ADsBossCharacter    — ASC + AttributeSet, BossBrain, CombatFX, LockOnTarget
```
- 규칙: GI 범위 = 맵 전환을 넘어 유지되는 것. World 범위 = 한 판의 상태. 컴포넌트 = 액터 고유 상태.
- World 서브시스템(엔카운터·타격감·오디오)은 `DoesSupportWorldType`으로 Game/PIE만 허용한다(에디터 월드 제외).

### 1-1. 상속 (부모 → 자식, [A] = Abstract)
```
UGameInstance            → UDsGameInstance
UGameInstanceSubsystem   → UDrNetworkSubsystem, UDrContentSubsystem, UDsPerfSubsystem, UDsSmokeSubsystem
UTickableWorldSubsystem  → UDsEncounterSubsystem, UDsHitFeedbackSubsystem, UDsAudioSubsystem
AGameModeBase            → ADsGameMode, ADsTitleGameMode
UGameUserSettings        → UDsGameUserSettings
UEngineSubsystem         → UDrTableSubsystem (DrTableSystem 플러그인, 정적 테이블)
UObject                  → UDrServerEndpoint[A] → UDrFakeServer
                         → UDrContentBase[A]    → UDsPlayerProfileContent
ACharacter + IAbilitySystemInterface → ADsCharacter, ADsBossCharacter
APlayerController        → ADsPlayerController
UActorComponent          → UDsBossBrainComponent, UDsLockOnComponent, UDsLockOnTargetComponent, UDsCombatFXComponent
UAbilitySystemComponent  → UDsAbilitySystemComponent
UAttributeSet            → UDsAttributeSet
UGameplayEffect          → UDsGameplayEffect_Damage
UGameplayAbility         → UDsGameplayAbility[A] → _MeleeAttack[A] → _AttackLight, _AttackHeavy
                                                → _Dodge, _Guard, _ApplyEffect
UDataAsset               → UDsPlayerCombatData, UDsBossData, UDsCombatFeelData, UDsCombatFXData, UDsAudioData
UCameraShakeBase         → UDsLightHitShake, UDsHeavyHitShake
UUserWidget              → UDrUIRootLayout
UCommonActivatableWidget → UDsCombatHUDWidget, UDsMenuWidget[A] → UDsTitleMenuWidget, UDsPauseMenuWidget, UDsSettingsMenuWidget
UCommonButtonBase        → UDsGlassButton      (UCommonButtonStyle → UDsGlassButtonStyle)
UMVVMViewModelBase       → UDsCombatHUDViewModel
값 구조체: FDrMeleeStage, FDrResolvedEffect, FDr<Table>Row(생성), FDrNetPacket/Response, FDrLatencyProfile,
          FDrAttackFX, FDrHitFeedbackState, FDsHUDDamageTrail(비 USTRUCT), FDsStatFormula(정적 공식)
```
규칙: 엔진 클래스를 한 단계만 상속한다. 공통 동작이 3개 이상 파생에 필요할 때만 Abstract 기반을 하나 둔다.

### 1-2. 멤버 관계 (own = UPROPERTY 소유·생성, weak = TWeakObjectPtr, data = Soft/TSubclassOf/Get()/행 사본)
```
ADsCharacter        own: UDsAbilitySystemComponent, UDsAttributeSet, UDsLockOnComponent, UDsCombatFXComponent,
                         USpringArmComponent→UCameraComponent, 표시 USkeletalMeshComponent
                    data: UDsPlayerCombatData::Get()(스탯·이동·경직), TArray<TSubclassOf<UDsGameplayAbility>> DefaultAbilities, IMC×2 + UInputAction들, 피격·사망 몽타주
ADsBossCharacter    own: UDsAbilitySystemComponent, UDsAttributeSet, UDsBossBrainComponent, UDsCombatFXComponent, UDsLockOnTargetComponent
                    data: TSoftObjectPtr<UDsBossData> BossData(기본값·애니 이름·스탯), TMap<FName, UAnimSequence*> Animations
UDsBossBrainComponent        data: TArray<FDsBossPhaseRow>, TArray<FDsBossAttackRow> (주입 가능)   weak: Target, HitActors
                    own: FRandomStream, TMap<int32,double> CooldownUntil
UDsAbilitySystemComponent own: 부여된 어빌리티 스펙, TMap<Tag,FDrResolvedEffect> Timed/Permanent, StackCounts, ShieldPool
                    data: UDrTableSubsystem(효과 조회)
UDsGameplayAbility_MeleeAttack data: TArray<FDrMeleeStage>(몽타주 Soft)     _Dodge data: TArray<TSoftObjectPtr<UAnimMontage>>(8)
UDsLockOnComponent  data: UDsCombatFeelData   weak: UDsLockOnTargetComponent
UDsHitFeedbackSubsystem data: UDsCombatFeelData   own: TMap<TWeakObjectPtr<AActor>, FDrHitFeedbackState>
UDsCombatFXComponent data: UDsCombatFXData::Get()  own: UNiagaraComponent들, UDecalComponent+MID, UPointLightComponent, FDrGhostPose[]
UDsEncounterSubsystem weak: ADsCharacter, ADsBossCharacter, Checkpoint/Door/Start AActor
UDsAudioSubsystem   data: UDsAudioData   own: Music/Door/Environment UAudioComponent   weak: Boss, Encounter(의존성 초기화), 피해 이벤트를 구독한 플레이어·보스
ADsPlayerController own: UDrUIRootLayout(→GameStack, MenuStack), UDsCombatHUDViewModel, UDsCombatHUDWidget, UDsSettingsMenuWidget, 일시정지 IMC/IA
UDsCombatHUDViewModel weak: Player, PlayerASC, BossASC, Encounter
UDsCombatHUDWidget  own: ViewModel 참조, 패널·텍스트, 게이지 MID, FDsHUDDamageTrail×2
UDrNetworkSubsystem own: UDrServerEndpoint(ServerEndpointClass로 생성), PendingRequests, ReceiveQueue
UDrFakeServer       own: TMap<FString,FInstancedStruct> Store, ReceiveQueue
UDrContentSubsystem own: UDsPlayerProfileContent, TArray<UDrContentBase*>   (UDrContentBase.Owner → 서브시스템 역참조)
```

### 1-3. 이벤트 (알림 → 구독)
```
UDsAttributeSet.OnDamaged/OnOutOfHealth/OnPoiseBroken/OnParried → ADsCharacter, ADsBossCharacter, UDsEncounterSubsystem; OnDamaged → UDsAudioSubsystem(피격 소리)
ASC.GetGameplayAttributeValueChangeDelegate(...)               → UDsCombatHUDViewModel
UDsEncounterSubsystem.OnStateChanged / OnActorsChanged         → UDsCombatHUDViewModel, UDsAudioSubsystem
UDsEncounterSubsystem.OnDoorMoving                             → UDsAudioSubsystem(문 소리)
UDsLockOnComponent.OnTargetChanged                             → UDsCombatHUDViewModel
UDrServerEndpoint.OnPacketToClient / OnEndpointConnectionChanged → UDrNetworkSubsystem
UDrNetworkSubsystem.OnConnectionStateChanged                   → UDsGameInstance(부팅)
UDrNetworkSubsystem.OnServerPush                               → UDrContentSubsystem → UDrContentBase.HandleServerPacket
UDsPlayerProfileContent.OnProfileChanged                       → 프로필 UI
APlayerController.OnPossessedPawnChanged                       → ViewModel 재바인딩
직접 호출은 규칙 → 표현 한 방향만: Ability/BossBrain → CombatFXComponent, HitFeedbackSubsystem, AudioSubsystem(보스 공격 소리·페이즈). 엔카운터·속성 세트는 오디오를 부르지 않고 이벤트만 낸다.
```

## 2. 부팅·동적 데이터 (서버형 단방향)
흐름: `FakeServer → ServerEndpoint.OnPacketToClient → NetworkSubsystem(ReceiveQueue, 도착시각 정렬) → Dispatch → (요청 콜백 | OnServerPush) → ContentSubsystem → ContentBase.HandleServerPacket`.
- `FDrNetPacket { Type, RequestId, FInstancedStruct Payload, SendTimeSeconds, Result }`.
- `EDrNetMessageType { None, Ping, Login, RequestPlayerProfile, ServerPush_ProfileUpdated }`, `EDrNetResult { Ok, Timeout, Error, NotConnected, InvalidRequest }`, `EDrConnectionState { Disconnected, Connecting, Connected, Reconnecting }`.
- `SendRequest<PayloadT>(Type, Payload, Callback, Timeout=5s)` → RequestId. 만료되면 Timeout으로 콜백.
- `FDrLatencyProfile { BaseLatency .08, Jitter .03, FailureRate 0 }`를 클라·서버 양쪽에 둔다. 도착 시각은 직전 도착보다 앞서지 않게 해 순서를 보존한다.
- 틱은 `FTSTicker`(월드 없이 동작).
- 부팅(`UDsGameInstance::StartBootFlow`): 테이블 로드 → Connect → Connected 시 Login → 세션 토큰 → RequestPlayerProfile → `bBootCompleted`.
- `-DrAutoNetSmoke`: 단계마다 `[DrAutoNetSmoke] PASS|FAIL <Step>` 출력, 30초 타임아웃.
- 콘솔: `Dr.Net.Ping`, `Dr.Net.SetLatency <base> <jitter> <failRate>`, `Dr.Profile.Request`, `Dr.Tables.Reload`.
- **새 메시지 추가 체크리스트**: enum 값 → 페이로드 USTRUCT → FakeServer `ProcessRequest` 분기 → 클라 콘텐츠의 `GetHandledMessageTypes`/`HandleServerPacket` → 자동화 테스트(지연·실패율 포함).

## 3. 정적 데이터 (DrTable 플러그인)
- 도구: **DrTableSystem 0.10.0** — https://github.com/ldnjin81/DrTableSystem (설치·문법은 저장소 README / README.ko.md, `docs/ko/manual.md`가 기준).
  - 구성: `drtable` CLI(`build`, `check`, `diff`, `l10n`, `new`), `drtable-gui`, 언리얼 플러그인(`DrTableRuntime`, `DrTableEditor`).
  - 설치: GitHub Releases의 OS별 실행 파일(윈도우 `x86_64-pc-windows-msvc`)을 `Tools/DrTable/`에 넣고, 플러그인 폴더 `unreal/DrTableSystem`을 `Plugins/DrTableSystem`으로 복사한다. 생성기와 플러그인은 같은 버전으로 맞춘다.
  - `drtable build --input <데이터> --schema <스키마> --out-cpp Plugins/DrFramework/Source/DrFramework/TableData/Generated --out-client <클라 JSON> --out-server <서버 JSON> --prefix Dr --ue-plugin` → `drtable check --client … --server …`(참조 무결성) → 에디터 빌드 → 굽기 커맨드릿(바뀐 테이블만, `-Verify`는 CI용, `path` 칸이 없는 에셋을 가리키면 칸 위치와 함께 실패).
- 원본: 스키마(`Plugins/DrFramework/Tables/Schemas/*.schema.xlsx`, `Plugins/DrFramework/Tables/Enums/*.enum.xlsx`, 프레임워크 소유)와 데이터 엑셀(`Design/Tables/*.xlsx`, 기획 소유)을 분리한다. 생성 코드는 스키마에만 의존하므로 데이터 수정은 코드를 바꾸지 않는다. 필드 범위(all/client/server)로 클라·서버 JSON이 나뉜다.
- 타입 표기: `ID<int32>`, `SubKey<name>`(보조 키 → `FindByX`), `Ref<Table>`/`Ref<Table.Col>`(참조 무결성 검사 + `GetX()` 접근자), `fixed<100>`(정수 저장, `XScale` 상수로 나눔), `duration`(→ `FTimespan`), `path`(→ `FSoftObjectPath`, 타입이 필요하면 `TSoftObjectPtr<T>(Path)`, 굽기 때 에셋 존재 검사), 열거형 시트(`Enums/*.enum.xlsx` → `UENUM EDr*`), 배열 `T[]`/`T[N]`(데이터는 `Name[0]`, `Name[1]` 열 또는 쉼표 칸, 행은 `Name_Start/Num` + 테이블 풀 → `GetName()`이 `TConstArrayView`). 정확한 문법은 저장소 manual이 기준.
- 생성물: `Plugins/DrFramework/Source/DrFramework/TableData/Generated/FDr<Table>Row`(+`Find(Id)`, `GetAll()`, 참조 접근자), `DrTableRegistration.h`. **손으로 고치지 않는다.**
- 런타임 구조(0.9.0부터 별도 레지스트리 없음, 0.10.0 기준)
  | 역할 | 클래스 |
  |---|---|
  | 등록·로드·조회 | `UDrTableSubsystem`(UEngineSubsystem). `Get()`, `RegisterAutoTables()`, `LoadAllTables()`, `FindRowByKey/FindRowsBySubKey/GetRows/GetArray`, `SetOverride/ClearOverride`, `AreAllTablesLoaded()`(등록 1개 이상 + 전부 로드, 시작 점검·스모크용) |
  | 테이블 한 장 | `UDr<Table>Table`(생성, `UDrTableAssetBase`=`UPrimaryDataAsset` 파생). 행 배열·키 인덱스·스키마/내용 해시 |
  | 행 | `FDr<Table>Row`(생성). `Find(Id)`, `FindBy<SubKey>`, `GetAll()`, `Get<Ref>()` → 내부에서 `UDrTableSubsystem::Get()` 사용 |
  | 설정 | `UDrTableSettings`(프로젝트 설정 → 플러그인 → DrTable): `AssetRoot`(기본 `/Game/Data`), `ExtraAssets`, `bLoadOnFirstUse`, `bWarnOnContentMismatch` |
  | 굽기 | `UDrTableBakeCommandlet`(`-run=DrTableBake`, `-Verify`/`-Force`). 매 실행(`-Verify`·변경 없는 테이블 포함) `path` 값과 배열 원소를 에셋 레지스트리로 검사, 실패 시 기존 에셋 보존. 빈 값과 BP 클래스 경로(`…_C`)는 허용 |
  | 쿡 | `DrTableEditor`가 `UE::Cook::FDelegates::ModifyCook`으로 `AssetRoot` 아래 테이블·문자열 에셋과 `ExtraAssets`를 쿡에 넣는다. `path` 열 에셋은 구운 테이블의 소프트 참조로 함께 쿡 |
- 등록: 생성 코드를 가진 모듈의 .cpp 하나에
  ```cpp
  #include "DrTableSubsystem.h"
  #include "TableData/Generated/DrTableRegistration.h"
  DRTABLE_AUTO_REGISTER(DrGeneratedTables::RegisterAll<UDrTableSubsystem>);
  ```
  생성된 `RegisterAll(TTables& Tables)`가 테이블마다 `Tables.Register<Row, Asset>(…)`를 부른다.
- 테스트: `NewObject<UDrTableSubsystem>()`으로 독립 인스턴스를 만들고 `Register` → `ExtraAssets` → `LoadAllTables` 후 `SetOverride`, 끝나면 `ClearOverride`.
- 패키지: 테이블용 설정은 따로 없다. `DirectoriesToAlwaysCook`에 테이블 폴더(`/Game/Data`)를 넣지 않는다(0.9.0 이전 우회책, 있으면 지운다). 패키지 스모크의 `tables` 단계는 `AreAllTablesLoaded()`로 판정한다.
- 콘솔: `DrTable.Status`(등록·로드 행 수), `DrTable.Reload`(다시 로드, 이전 행 포인터 무효).
- 문자열: `UDrStringTables`/`UDrStringSubsystem`(언어 선택·비동기 로드·`FindText(Table, Key)`), 위젯은 `UDrLocalizedTextBlock`. 문자열 테이블이 없는 프로젝트는 매니페스트(`DA_DrStrings`)가 없으면 로드를 시도하지 않아 시작 경고가 없다.
- 처음 만들 기본 테이블
  - `BossPhase { Id, Boss, Phase, EnterHpRate fixed<100>, PatternSet Ref<BossAttack.PatternSet>, MoveSpeedRate fixed<100> }`
  - `BossAttack { Id, PatternSet SubKey, Kind EDsBossAttackKind{Swing,Dash,Slam,Area,Siphon}, Anim, Weight, Cooldown, Damage, StaminaDamage, Effect Ref<EffectDefs>, TelegraphTime, Duration, ActiveStart, ActiveEnd, PoiseDamage, HitRadius, HitForwardOffset, MinRange, MaxRange, MoveDistance, Heal }`
  - `EffectDefs { Id, Category, ApplyTo{Caster,…}, Duration, DamagePerTick, Period, Magnitude, MoveSpeed/DamageTaken/DamageDealt Multiplier, StackBonus[0..2], StateTag, TriggerTag, ProcInstantDamageCurHpPct, ProcPoiseDamage, StackRule{Refresh,Accumulate}, DisplayName, Icon }`

## 4. 게임플레이 태그 (`Core/DsGameplayTags`, 네이티브 선언)
- 상태: `State.Dead`, `State.Invulnerable`, `State.Groggy`, `State.Attacking`, `State.Guarding`, `State.Parrying`, `State.Dodging`, `State.Staggered`
- 버프/디버프: `State.Buff.{Enrage,Guard,Shield}`, `State.Debuff.{Burn,Chill,Sunder,Rot,Bleed,Crush}`
- 어빌리티 식별: `Ability.Attack.Light`, `Ability.Attack.Heavy`, `Ability.Guard`, `Ability.Dodge`
- SetByCaller: `Data.Damage`, `Data.PoiseDamage`
- 상태 태그는 **루즈 태그**(`SetLooseState`)로 켜고 끈다. GE 기간 태그를 쓰지 않는다(틱 기반 진행과 맞추기 위해).

## 5. 속성 (`UDsAttributeSet`)
| 묶음 | 속성 |
|---|---|
| 바이탈 | Health, MaxHealth, Stamina, MaxStamina, StaminaRegenRate |
| 포이즈 | Poise, MaxPoise, PoiseRegenRate |
| 전투 | AttackPower, Defense, ElementalResist |
| 특성 | Strength, Dexterity, Vitality, Spirit, Will |
| 메타(비복제) | IncomingDamage, IncomingPoiseDamage |
- 초기화: 데이터 에셋의 `FDrStatBlock`(기본값 + 특성)을 `UDsAttributeSet::InitFromStats`로 적용한다. 공식으로 최대치를 파생하고 현재치를 최대치로 채운다. 플레이어 기본값: MaxHealth 100, MaxStamina 100, StaminaRegen 25/s, MaxPoise 100, PoiseRegen 20/s, AttackPower 20, Defense 5, 특성 0. 보스: MaxHealth 2000, PoiseRegen 0(그로기 종료 시 한 번에 채움).
- 특성 → 스탯 공식 `FDsStatFormula`: MaxHealth = Base + 12·Vit, Defense = Base + 1·Vit, MaxStamina = Base + 4·Spi, StaminaRegen = Base + .8·Dex, AttackPower = Base + 2·Str, MaxPoise = Base + 3·Str, ElementalResist = clamp(2·Will, 0, 80)%.
- 이벤트(네이티브 멀티캐스트, UPROPERTY 아님): `OnDamaged(Damage, Instigator, Causer)`, `OnOutOfHealth`, `OnPoiseBroken`, `OnParried`.

## 6. 피해 파이프라인 (`PostGameplayEffectExecute`)
`UDsGameplayEffect_Damage`(C++ 정의 즉시형 GE, SetByCaller `Data.Damage`/`Data.PoiseDamage` → 메타 속성)를 `ASC::ApplyDamage(TargetASC, Damage, Poise, Causer)`로 적용.
IncomingDamage 처리 순서:
1. Health ≤ 0이면 무시(사망 이벤트 중복 방지).
2. `State.Invulnerable`이면 전부 차단.
3. 공격자가 **전방 반구**일 때: `State.Parrying` → 피해 0, `OnParried`, 공격자에게 포이즈 피해 `Guard.ParryPoiseReturn(40)`. `State.Guarding` → 비용 = 원피해×`Guard.GuardStaminaRate(.5)`; 스태미나 충분하면 소모하고 피해×`Guard.GuardDamageRate(.3)`(값은 `UDsPlayerCombatData`), 부족하면 막기 붕괴(`OnPoiseBroken`, 전액).
4. 방어: `max(Raw − Defense, Raw × 5%)`.
5. × 활성 효과의 받는 피해 배수 곱 → 보호막 풀 흡수(`AbsorbWithShield`).
6. 스태미나 재생 정지(`StaminaRegenDelayAfterHit` 1.5초). Health 감소, 오디오 `PlayDamage`, `OnDamaged`, 0이 되는 전이에서만 `OnOutOfHealth`.
IncomingPoiseDamage: 무적이거나 정면 쳐내기/막기 중이면 무시. Poise ≤ 0이 되면 MaxPoise로 다시 채우고 `OnPoiseBroken`.

## 7. ASC (`UDsAbilitySystemComponent`)
- `GetShouldTick()`은 항상 true. 틱에서 **활성 Dr 어빌리티의 `AbilityTick` 구동 → 한시 효과 시계 → 쿨다운 → 기력 재생 지연 → 스태미나·포이즈 재생** 순서로 진행한다.
- 스태미나: `SpendStamina` → 남은 지연을 `StaminaRegenDelayAfterSpend(1s)`로, `PushStaminaRegenDelay(sec)`는 더 긴 쪽으로. 모두 틱에서 줄어든다.
- 효과: `ApplyEffectById(Id, Target, Causer, Tables?)` → `DrEffects::Resolve(Row)` → `ApplyStatusEffect`. 한시 효과는 태그별 시계 `{Remaining, SinceTick}`: 재적용하면 Remaining만 새로, 주기 위상은 유지; 틱에서 `min(dt, Remaining)`만큼 위상을 더해 주기마다 피해(만료와 같은 시각 포함) 후 Remaining ≤ 0이면 해제. 상시 효과(`Add/RemovePermanentEffect`, 장비용)는 따로 저장.
- 쿨다운: `StartCooldown(Tag, Duration)`이 루즈 태그를 켜고 틱에서 끈다. `GetCooldownRemaining(Tag)`.
- 중첩: `Refresh`는 지속시간만 갱신, `Accumulate`는 단계 증가 + `StackBonus[단계-1]`% 보너스를 주 수치에 반영(`GetStackCount`).
- 배수 집계: `GetMoveSpeed/IncomingDamage/OutgoingDamageMultiplier()` = 활성 효과 곱(같은 태그면 한시 효과 우선).
- 즉발: `ProcInstantDamageCurHpPct`(현재 HP 비율 피해), `ProcPoiseDamage`. 보호막 = `Magnitude`만큼 흡수 풀.
- 기타: `TryActivateAbilityByTag`, `ApplyHeal`, `TakePoiseDamage`(생명력 없이 포이즈만).

## 8. 어빌리티
### 8-1. 기반 `UDsGameplayAbility`(Abstract)
- `StaminaCost`(가상 `GetStaminaCost()` — 데이터에서 읽는 어빌리티가 재정의), `CooldownDuration` + `CooldownTag`(ASC `StartCooldown`), `GetCooldownRemaining`.
- **수치는 `UDsPlayerCombatData`(DA_PlayerCombat)에서 읽는다**: 근접 타 배열(`LightStages`/`HeavyStages`, 파생이 `GetStages()`로 고름)·판정 구, 회피(`Dodge`), 막기(`Guard`). 코드의 값은 에셋이 없을 때의 기본값이다.
- `CanActivateAbility`: 스태미나 확인 + `BlockDuringCombatStates()`(Dead·Staggered·Attacking·Dodging 등 중 발동 금지).
- `AbilityTick(dt)`: ASC가 호출. 모든 구간은 여기서 잰다.
- `ApplyConstantMove(Dir, Distance, Duration, EndSpeed)`: 루트 모션 없는 전진·대시.
- `PlayAbilityMontage(SoftPtr, PlayRate) → UAnimMontage*`: 표시 메시가 있는 캐릭터일 때만 동기 로드·재생, 실패 시 nullptr → 호출자가 폴백.
### 8-2. 근접 `UDsGameplayAbility_MeleeAttack`(Abstract)
- `TArray<FDrMeleeStage> { Montage, Duration, ActiveStart, ActiveEnd, DamageRate, PoiseDamage, StaminaCost, LungeDistance }`.
- 판정: 전방 `HitForwardOffset(130)` 지점 반경 `HitRadius(90)` 구 `OverlapMultiByObjectType(Pawn)`, 한 타에 대상당 1회(`HitActors`). 피해 = AttackPower × DamageRate.
- 콤보: 진행 중 입력 → `BufferNextInput`; 현재 타의 ActiveEnd 이후 다음 타 시작(스태미나 부족이면 현재 타 끝까지).
- 피해가 실제로 들어간 경우에만 `HitFeedback.PlayHit(Light|Heavy)`. 판정 구간 동안 `CombatFX.SetSword(true)`.
- `AttackLight` 3타: (1.67s, .50–.84, ×1.0, 포이즈20) / (1.40s, .42–.70, ×1.1, 20) / (1.87s, .56–.94, ×1.3, 30), 각 스태미나 12, 전진 80cm.
- `AttackHeavy` 1타: 3.03s, 1.36–1.82, ×2.2, 포이즈 60, 스태미나 30, 전진 0(모션이 약 2.6m 이동 → 캡슐이 따라감).
### 8-3. 회피 `UDsGameplayAbility_Dodge`
- 방향 = 이동 입력(없으면 뒤). 무적 `.05–.3s`(`State.Invulnerable`), `State.Dodging`.
- 표시 메시 있음: 8방향 `RollMontages`(0=앞, 시계방향 45°, `SelectRollIndex` = atan2/45) × `RollPlayRate 1.6`, 모션이 캡슐 이동을 이끈다, 지속 = 모션 길이/속도.
- 표시 메시 없음: `ApplyConstantMove(450cm, .35s)` 대시. 취소되면 몽타주 정지.
### 8-4. 막기 `UDsGameplayAbility_Guard`
- 길이 1.13s. 앞 `.3s` = `State.Parrying`, 이후 `State.Guarding`. 경감은 속성 세트가 태그를 보고 처리(6장).
### 8-5. `UDsGameplayAbility_ApplyEffect`
- 게임플레이 이벤트 페이로드의 Target에 `EffectId` 적용(데이터 주도 스킬의 기본형).

## 9. 플레이어 캐릭터 (`ADsCharacter`)
- GASP 로코모션: Walk 200 / Run 500 / Sprint 700(데이터), 웅크리기, 착지 속도 기록, `FillAnimationProperties`(CustomThunk로 애님 BP 구조체 채움).
- 입력: 로코모션 IMC + 전투 IMC(더 높은 우선순위). **입력 진입점 함수** `InputAttackLight/Heavy/Guard/Dodge/LockOn()`을 입력·AI·테스트가 공통으로 쓴다.
- 초기화: 처음 한 번 `InitFromStats(UDsPlayerCombatData::Get()->Stats)`와 이동 속도 갱신. 어빌리티 부여: `DefaultAbilities`를 `PossessedBy`/`OnRep_Controller`에서 1회.
- 표시 메시: 캡슐 메시와 별도인 Manny 표시 메시(`MannyMesh`) + 의상 보조 메시가 포즈를 따름(`FollowVisibleMeshPose`).
- 전투 모션: `PlayCombatMontage(Montage, bHoldLastPose, PlayRate)`. 애님 BP에 슬롯이 없으면 단일 애니 재생으로 덮고 `PoseOverrideRemaining = 길이/속도` 후 복구. **모션 따라가기**: `CombatMotionBone(pelvis)`의 수평 이동을 샘플링해 캡슐을 옮기고 메시를 캡슐 위로 되돌린다(루트 모션이 꺼진 팩도 이동이 맞음).
- 피격: 앞/뒤 HitReact 몽타주, 포이즈 붕괴 → `State.Staggered` `StaggerDuration(.8s, 데이터)`, 사망 → `State.Dead` + DeathMontage. 전투 동작·경직·사망 중 이동 입력 차단.
- `Revive(Transform)`: 상태 태그 초기화, 속성 회복, 위치 이동. `FellOutOfWorld`도 사망 처리.

## 10. 보스
### 10-1. `ADsBossCharacter`
- 플레이어와 **같은 GAS 피해 경로**. 원본 메시·스켈레톤·애니를 그대로 쓰고 `PlaySequence(Name, bLoop)`로 단일 애니 재생(이름 → `UAnimSequence` 맵, 누락 애니는 경고 후 건너뜀).
- 수치는 `UDsBossData`(보스마다 DA, 캐릭터의 `BossData` 소프트 참조, 없으면 클래스 기본값): 이름(BossPhase.Boss와 매칭)·표시 이름, `FDrStatBlock`(MaxHealth 2000, MaxPoise 100, PoiseRegen 0), Move 350, Aggro 2500, Groggy 2.67s, PhaseTransition 2s, Recover .6s, TurnRate 180°/s, AttackFacingAngle 60°, 애니 이름(Idle·Jog_Fwd·Knock_back·Death_front).
- `ResetBoss(Home)`: 체력·포이즈·상태·두뇌 초기화.
### 10-2. `UDsBossBrainComponent` (권한 측 틱 상태기계)
`EDrBossState { Idle, Approach, Attack, Recover, Groggy, PhaseTransition, Dead }`
- Idle → Approach: `bEngageAllowed`(엔카운터가 켬) && 대상 생존 && 거리 ≤ Aggro.
- Approach: 대상으로 회전, `PickAttack(거리)` 성공 시 Attack, 아니면 정면일 때 전진(속도 = Base × 페이즈 MoveSpeedRate × 효과 배수).
- `PickAttack`: 현재 페이즈 PatternSet && Weight > 0 && Min ≤ 거리 ≤ Max && 쿨다운 경과 && (정면이 아니면 Area/Siphon만) → **가중치 추첨**(`FRandomStream`, 시드 주입 가능). 선택은 쿨다운을 소비하지 않고 시작할 때 소비.
- Attack 타임라인: `[0, Telegraph)` 대상 추적 회전 + 예고 FX 진행 → Telegraph 시점 방향 고정, `StartMove(MoveDistance)` → `ActiveStart`에 임팩트 FX·공격 소리(Slam은 `PlaySlam`) → `[ActiveStart, ActiveEnd)` 판정(대상당 1회 피해·포이즈. 실제로 피해가 들어가면 타격감 Boss, 대상 스태미나 −StaminaDamage, Effect 적용, 보스 Heal) → `Duration` 후 Recover.
- Recover(.6s) → Approach. 포이즈 붕괴 → Groggy(2.67s, 끝나면 포이즈 회복). PhaseTransition 동안은 `State.Invulnerable`(전환 연출 중 피해 없음). HP 비율 ≤ 다음 페이즈 `EnterHpRate`면 `PendingPhase`; Attack/Groggy가 아닐 때 PhaseTransition(2s) → 페이즈 교체, 오디오 페이즈 2, CombatFX 2페이즈 루프.
- 테스트 이음새: `InjectTables(Phases, Attacks)`, `SetRandomSeed`, `SetTarget`. 테이블이 없으면 1회 경고.

## 11. 락온·타격감·FX
- `UDsCombatFeelData`(DataAsset, `Get()`): 락온 획득 2500 / 해제 3000 / 가림 유예 2s / 획득 반각 60° / 전환 임계 .7·쿨다운 .3s, 카메라 보간 7·암 +60(근접 350 이내 +150)·측면 오프셋 70·피치 바이어스 −8(−35~10). 히트스톱 Light .06 / Heavy .12 / Boss .1s, 정지 시간배율 .08, 플래시 .1s, 셰이크 Light .5 / Heavy 1.0, 오버레이 머티리얼.
- `UDsLockOnComponent`: `Toggle/Clear/SwitchTarget(dir)/LookInput`. 잠금 시 회전 설정·암 길이·소켓 오프셋을 저장했다가 해제 시 복구. `OnTargetChanged`.
- `UDsLockOnTargetComponent`: `bLockable`, 소켓 `spine_03`, 소켓 없으면 가슴 오프셋(0,0,50).
- `UDsHitFeedbackSubsystem`: 액터별 `CustomTimeDilation` 정지 + 머티리얼 오버레이 플래시 + 카메라 셰이크. 복구는 **월드 실제 시간**으로(정지된 액터 시간에 의존하지 않음). `PlayHit(Attacker, Victim, Light|Heavy|Boss)`, `PlaySlam(Source)`(거리 감쇠 셰이크), `ResetActor`.
- `UDsCombatFXData`(DataAsset, `Get()`): `TMap<int32 BossAttackId, FDrAttackFX{Telegraph, Impact, Socket, Scale}>`, 대시(바람·블러·먼지), 검광 좌/우, 플레이어·보스 임팩트, 예고 바닥 원 **디퍼드 데칼**(스칼라 `Progress` 0~1), 페이즈 버스트/루프, 잔상 머티리얼(간격 .2s, 수명 .45s), 임팩트 점광원(6000cd, 650, .35s, 주황).
- `UDsCombatFXComponent`: `SetDash/SetSword/BeginTelegraph/AdvanceTelegraph/EndTelegraph/PlayAttackImpact/EnterPhaseTwo/PlayHit/Reset`. 잔상은 `UPoseableMeshComponent` 스냅샷. 모든 FX는 Transient로 생성·파괴(수명 관리 일원화).

## 12. 엔카운터 (`UDsEncounterSubsystem`)
`EDrEncounterState { Idle, Fighting, PlayerDead, Victory }`, `OnStateChanged(Old, New)`, `OnActorsChanged`.
- `OnWorldBeginPlay`에서 액터 태그로 찾아 `BindActors(Player, Boss, Checkpoint, Door, Start)`: `Dr.Boss`, `Dr.Checkpoint.Gate`, `Dr.Arena.Door`, `Dr.Player.Start`(플레이어는 첫 컨트롤러의 폰). 보스·체크포인트가 없으면 비활성.
- 체크포인트: 반경 400 안에 들어오면 활성 → 리스폰 지점이 체크포인트로 바뀜.
- 아레나 진입: 문 기준 아레나 방향으로 `max(300, 문 반두께 + 캡슐 반경 + 1)` 넘어가면 보스 교전 허용 + Fighting + 문 닫힘(1s 보간, 문 이동 소리).
- 플레이어 사망 → PlayerDead → 3s 후 보스 `ResetBoss(Home)`·교전 금지, 플레이어 `Revive`, 문 열림, Idle. 보스 사망 → Victory.
- `RestartCheckpoint()`(메뉴 "체크포인트에서 다시"): 대기 없이 즉시 리스폰.

## 13. 오디오 (`UDsAudioSubsystem` + `UDsAudioData`)
- `UDsAudioData`(하드 참조로 패키지 포함): Whooshes, Impacts, BossAttacks, Music, Distant, Wind, Fire, Slide, Footsteps, PlayerVoices, BossVoices, ControlBus 배열, 보스/환경 감쇠.
- `EDsAudioChannel { SFX, Voice, Music }` → AudioModulation 컨트롤 버스. `SetChannelVolume`는 `UDsGameUserSettings`(Config=GameUserSettings)에 저장 → 맵 전환·재실행 유지.
- 상태 연동(이벤트 구독): `Initialize`에서 엔카운터를 의존성으로 초기화하고 `OnStateChanged`→`EncounterChanged`(음악 시작/정지/승리), `OnActorsChanged`→플레이어·보스 `OnDamaged` 구독→`PlayDamage`, `OnDoorMoving`→`MoveDoor`. 보스 두뇌는 `SetPhase`·`PlayBossAttack(Kind)`를 직접 부른다(규칙→표현). 환경 루프·원거리 효과음·발소리는 오디오 틱.
- 몽타주 사운드 노티파이(휘두름·접촉·기합)는 에디터 라이브러리 함수가 몽타주 이름 규칙으로 재생성.

## 14. UI (CommonUI + MVVM, 리퀴드 글라스)
- `ADsPlayerController`가 `UDrUIRootLayout`(GameStack, MenuStack) 소유. `PushGame/PushMenu/PopMenu/ClearMenus`.
- 일시정지: 전용 입력 컨텍스트, `OpenPauseMenu/Close/Toggle`, `OpenSettings`, `BackMenu`, `StartGame`, `ReturnToTitle`, `RestartCheckpoint`. 메뉴 입력 모드 전환(`ApplyInputMode`). 사망 중에는 메뉴 차단.
- `UDsMenuWidget`(Abstract, `UCommonActivatableWidget`): 입력 구성·첫 포커스·뒤로가기·키 미리보기 공통, 파생은 `BuildMenu(VerticalBox)`만 구현. Title(뒤로가기 무시)·Pause·Settings(SFX/Voice/Music 슬라이더).
- `UDsGlassButton`(`UCommonButtonBase`) + `UDsGlassButtonStyle`: 키보드·패드 포커스와 마우스 호버를 같은 유리 림 MID로 표시.
- HUD: `UDsCombatHUDViewModel`(`UMVVMViewModelBase`, FieldNotify) — PlayerHealth/Max, PlayerStamina/Max, BossVisible, BossDisplayName, BossHealthRate, EncounterState, LockOnTarget + 계산 필드 HealthRate/StaminaRate/IsPlayerDead/IsVictory. **GAS 속성 변경 델리게이트와 엔카운터 이벤트만 구독**.
- `UDsCombatHUDWidget`: 네이티브로 레이아웃 생성(`RebuildWidget`; 파이썬에 WidgetTree 편집이 노출되지 않기 때문). 게이지는 머티리얼 MID, 락온 마커, 결과 패널(사망/승리) 페이드.
- `FDsHUDDamageTrail`: 피해 시 목표만 내리고 .4s 대기 후 .3s 동안 따라감, 회복은 즉시.
- `ADsTitleGameMode`: 플레이어 없이 진입로 시네 카메라(천천히 흔들림) + 타이틀 메뉴.

## 15. 검증 지원 코드
- `DsTestFixture::FWorldFixture`: 오디오·내비·AI 없는 경량 Game 월드(고유 이름). `WarmUpTimers`(`++GFrameCounter` + `Tick(0)`).
- 테스트 이름: `<Game>.<영역>.<하위|PIE>.<항목>`(항상 4단계, 17장). 보스 1종 버티컬 슬라이스 기준 규모 감: 약 90개(현재 91) — Boss ~18, Combat ~15, Effects ~10, Encounter ~10, Characters ~8, UI ~8, Framework ~6, FX ~6, 그 밖(Audio, Levels, Playthrough, Visual) ~9.
- 패키지 검증 서브시스템(명령줄로만 활성): `UDsSmokeSubsystem`(실제 메뉴·맵 전환 단계별 PASS/FAIL·캡처), `UDsPerfSubsystem`(고정 60초 카메라 경로 4구간, frame/game/gpu p95).

## 16. 확장 레시피
- **새 플레이어 기술**: `UDsGameplayAbility` 파생 → 수치는 `UDsPlayerCombatData`에 필드 추가 → 비용(`GetStaminaCost`)·쿨다운·태그 → `ActivateAbility`에서 루즈 상태 켜기 → `AbilityTick`에서 구간 판정 → `EndAbility`에서 정리 → 몽타주 실패 시 폴백 → `DefaultAbilities` 등록 → 입력 진입점 → 테스트(헤드리스 + PIE).
- **새 보스 공격**: BossAttack 행 추가(PatternSet·Kind·시간·범위·가중치) → 필요하면 `CombatFXData.BossAttacks[Id]` → 오디오 Kind 매핑 → BossBrain 테스트에 `InjectTables`로 시나리오.
- **새 상태 효과**: 태그 선언 → EffectDefs 행(StateTag·배수·DoT·StackRule) → 사용처에서 `ApplyEffectById` → HUD/FX 표시가 필요하면 ViewModel 필드.
- **새 보스**: `UDsBossData` 에셋 하나(이름·스탯·애니 이름·기본값) + 보스 BP의 `BossData` 지정 + BossPhase/BossAttack 행 + 레벨 태그. 행동이 다를 때만 `ADsBossCharacter`/두뇌를 파생한다.
- **새 메뉴**: `UDsMenuWidget` 파생 `BuildMenu` → 컨트롤러에서 `PushMenu` → 첫 포커스·뒤로가기 확인.

## 17. 네이밍 규칙 (코드·에셋·도구·문서 공통)
목표: 이름만 보고 종류·위치·역할을 알 수 있게 한다. 규칙을 바꾸면 기존 이름도 한 번에 옮긴다(CoreRedirects + 에셋 이름 이전 스크립트).

### 17-1. C++
| 대상 | 규칙 | 예 |
|---|---|---|
| 접두사 | U(UObject)·A(Actor)·F(struct)·E(enum)·I(interface) + `Dr` | `UDsAttributeSet`, `FDrMeleeStage` |
| 약어 | 모두 대문자 | `HUD`, `UI`, `FX`, `PIE`, `AI` (`UDsCombatHUDWidget`) |
| 파일 | 파일 이름 = 클래스 이름(접두 글자 제외). **UCLASS는 파일당 하나** | `DsCombatHUDWidget.h` |
| 같이 둬도 되는 것 | 그 클래스만 쓰는 작은 USTRUCT·UENUM·델리게이트 | `FDrMeleeStage` in MeleeAttack 헤더 |
| 접미사 | `…Subsystem`, `…Component`, `…Data`(DataAsset), `…Widget`, `…MenuWidget`, `…ViewModel`, `…Settings`, `…Library`(BlueprintFunctionLibrary) | `UDsBossBrainComponent` |
| GAS | `UDsGameplayAbility_<동작>`, `UDrGameplayEffect_<효과>` | `UDsGameplayAbility_Dodge` |
| 델리게이트 | `FDr<주어><사건>Delegate`(시그니처가 같아도 사건마다 따로), 멤버 `On<사건>`, 처리 함수 `Handle<사건>` | `FDrLockOnChangedDelegate OnTargetChanged`, `FDrPoiseBrokenDelegate OnPoiseBroken` |
| 변수 | bool은 `b…`, 입력 매개변수가 멤버를 가리면 `In…` | `bLocked`, `InRadius` |
| 폴더 | 기능 영역(Game, Core, Network, Backend, Content, Abilities/Combat, Abilities/Effects, Characters, Boss, Combat, Encounter, Audio, UI, Debug, Tests). include는 모듈 루트 기준 | `"Game/DsPlayerController.h"` |
| 태그 | `State.*`, `Ability.*`, `Data.*` / C++ 상수 `DsTags::State_Dodging` | |
| 콘솔 명령 | `Dr.<영역>.<동작>` | `Dr.Net.Ping` |
| 테스트 | 파일 `Dr<영역>[<하위>][PIE]Test.cpp`(파일 하나에 한 영역·PIE 여부만), 이름 `<Game>.<영역>.<하위|PIE>.<항목>`, 여러 파일이 쓰는 픽스처는 `Tests/Dr<영역>TestFixture.h` | `Game.Combat.Dodge.RollDirections` |
| 이름 변경 | 클래스 이름을 바꾸면 `[CoreRedirects] +ClassRedirects`를 같이 넣는다 | |

### 17-2. 에셋 (`/Game/Dr/<영역>/<종류 접두사>_<본문>`)
- 본문은 밑줄로 나눈 PascalCase 토큰, 약어 대문자. **에셋 이름에 프로젝트 이름(Dr)을 넣지 않는다**(폴더가 이미 나타낸다).
- 외부 팩은 원본 경로(`/Game/<팩>`)를 유지한다. 프로젝트가 만든 사본은 `/Game/Dr/` 아래 두고, 여러 팩의 사본이 섞이면 원본 팩 폴더로 나눈다(`FX/Materials/<팩>/M_…`). 해시 꼬리로 구분하지 않는다.
- 샘플에서 따라온 기반 에셋(버스·서브믹스 등)은 출처 토큰을 남긴다(`CB_AncientOne_SFX`).

| 접두사 | 종류 | 접두사 | 종류 |
|---|---|---|---|
| `L_` | 맵 | `BP_` / `WBP_` / `ABP_` | 블루프린트 / 위젯 / 애님 BP |
| `A_` | 애니 시퀀스 | `AM_` / `BS_` / `AO_` | 몽타주 / 블렌드 스페이스 / 에임 오프셋 |
| `SKM_` / `SK_` / `PA_` | 스켈레탈 메시 / 스켈레톤 / 피직스 에셋 | `SM_` | 스태틱 메시 |
| `M_` / `MI_` / `MF_` / `MPC_` | 머티리얼 / 인스턴스 / 함수 / 파라미터 컬렉션 | `T_` (+`_D`·`_N`·`_MR`) | 텍스처(디퓨즈·노멀·금속거칠기) |
| `NS_` / `NE_` / `ET_` | 나이아가라 시스템 / 이미터 / 이펙트 타입 | `DA_` / `DT_` / `ST_` | 데이터 에셋 / 데이터 테이블 / 문자열 테이블 |
| `S_` / `SC_` / `MS_` / `MSP_` | 사운드 웨이브 / 큐 / 메타사운드 / 메타사운드 패치 | `ATT_` / `SCON_` / `SCL_` | 감쇠 / 동시 재생 / 사운드 클래스 |
| `SMX_` / `SEP_` / `IR_` | 서브믹스 / 서브믹스 이펙트 프리셋 / 임펄스 응답 | `CB_` / `CBM_` / `MPT_` / `MPR_` | 컨트롤 버스 / 버스 믹스 / 모듈레이션 패치 / 파라미터 |
| `IA_` / `IMC_` | 입력 액션 / 매핑 컨텍스트 | `IK_` / `RTG_` | IK 리그 / 리타기터 |
| `MHC_` | 메타휴먼 캐릭터 | `CF_` / `CLC_` | 커브 |
- 규칙은 `Tools/xx_naming.py` 한 곳에 두고 생성 스크립트와 이전 스크립트가 같이 쓴다. 생성 스크립트는 처음부터 규칙 이름으로 만든다(배치 리타깃처럼 도구가 접두를 붙이면 바로 규칙 이름으로 옮긴다).
- 이름 이전(렌더링 에디터, 한 프로세스): ① 대상 전부를 `AssetTools.rename_assets` **한 번에**(하나씩 옮기면 앞서 옮긴 참조처를 레지스트리가 옛 경로로 기억해 참조가 끊긴다) → ② 더티 패키지 저장 → ③ `scan_paths_synchronous` 재스캔 → ④ 남은 리다이렉터의 참조처(외부 액터·다른 맵)를 `load_package` 후 다시 저장 → ⑤ 재스캔 후 참조처가 없는 리다이렉터만 삭제 → ⑥ `/Game` 전체 참조 누락 0 확인 → ⑦ 코드·스크립트·ini의 경로 문자열을 대응표로 치환. `IAssetTools::FixupReferencers`는 5.8 무인 실행에서 단언 오류·확인창 크래시가 나서 쓰지 않는다. 사용 중인 에셋(ini가 부르는 CommonUI 데이터 등)은 ini 경로를 먼저 바꾼다. 위젯 BP 저장은 썸네일을 그리므로 플레이어 없이도 위젯이 만들어져야 한다(WidgetTree로 생성). 대소문자만 다른 이름 변경은 하지 않는다(Windows·VCS).

### 17-3. 도구·문서
- 에디터 스크립트 `Tools/task_<주제>.py`(멱등, 머리 docstring에 실행 명령), 공유 모듈 `Tools/xx_<주제>.py`, 셸 진입점 `<동사>_<대상>.cmd/.ps1`(snake_case). 작업 번호를 파일 이름에 넣지 않는다.
- 문서 `Docs/<분류>/UPPER_SNAKE.md`.

## 18. UE 5.8 C++ 함정
1. 각 .cpp 첫 include는 자기 헤더(IWYU). 서브폴더 include는 모듈 루트 기준.
2. `*.Target.cs`에 namespace 금지, `BuildSettingsVersion.V7`.
3. UCLASS BlueprintNativeEvent는 `Obj->Foo()`로 호출(`Execute_`는 인터페이스 전용).
4. UFUNCTION 파라미터명이 멤버명을 가리면 UHT 오류 → `InX`.
5. reparent 후 헤더 touch → 재빌드. 파이썬에서 고칠 필드는 `EditAnywhere`.
6. ASC 기본 구현은 태스크가 없으면 틱을 끈다 → `GetShouldTick()` 재정의.
7. 타이머 테스트는 `WarmUpTimers` 필요. 판정은 타이머 대신 틱 누적.


---

# 부록 A. 부트스트랩 — 빈 프로젝트에서 같은 뼈대 세우기

## A-1. 출발 프로젝트
- DrSample은 콘텐츠 없는 최소 C++ 프로젝트에서 시작한다. 소비 게임이 로코모션 표현을 추가할 때 **Epic "Game Animation Sample"(GASP) UE 5.8판**을 연결한다. 샘플 에셋은 이 저장소에 포함하지 않는다.
- 코드가 경로로 기대는 샘플 에셋(이동하거나 이름을 바꾸지 않는다):
  | 경로 | 역할 |
  |---|---|
  | `/Game/Blueprints/SandboxCharacter_CMC_ABP` | 소스 애님 BP(Motion Matching, CMC판). 캡슐용 숨김 메시에 붙인다 |
  | `/Game/Blueprints/BPI_SandboxCharacter_Pawn` | 애님 BP가 폰에서 속성을 읽는 인터페이스 |
  | `/Game/Blueprints/RetargetedCharacters/ABP_GenericRetarget` | 표시 메시 런타임 리타깃(소스 메시 포즈 복사) |
  | `/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin` | 소스 메시(숨김) |
  | `/Game/Characters/UE5_Mannequins/Meshes/SKM_Manny` | 표시 메시(전투 몽타주 재생 대상) |
  | `/Game/Input/IMC_Sandbox`, `IA_Move/Look/Look_Gamepad/Jump/Walk/Sprint/Crouch` | 로코모션 입력 |
- 샘플의 DataDriven CVar(`DDCvar.*`)와 렌더 설정(Lumen, VSM, Nanite)은 그대로 둔다.

## A-2. 캐릭터 BP 결선 (`BP_Character`, 커맨드릿으로 재현)
- 부모 `ADsCharacter`. GASP 폰 인터페이스를 구현하고, 인터페이스의 애니 속성 함수 그래프 안에서 `FillAnimationProperties(구조체)`를 호출해 반환한다.
- `FillAnimationProperties`는 CustomThunk로 **구조체 필드 이름**을 보고 채운다: `Velocity`, `InputAcceleration`, `GroundNormal`, `GroundLocation`, `LandVelocity`, `ActorTransform`, `AimingRotation`, `OrientationIntent`, `WantsToWalk`, `WantsToSprint`, `WantsToCrouch`, `WantsToStrafe`(락온 중 true), `JustLanded`, `InputState`. 모르는 필드는 건드리지 않는다.
- 기본 메시 = UEFN + 소스 ABP(숨김, 틱은 유지). `MannyMesh` = Manny + `ABP_GenericRetarget`. 의상 메시는 `MannyMesh`의 리더 포즈를 따른다.
- 입력 프로퍼티는 A-1 입력 + 전투 `IMC_Combat`, `IA_AttackLight/AttackHeavy/Guard/Dodge/LockOn`. 피격·사망 몽타주, `DefaultAbilities`(약·강공격, 회피, 막기).
- 외형 BP(메타휴먼 등)는 `BP_Character`의 자식으로 만든다.

## A-3. 플러그인 (uproject)
DrFramework, DrTableSystem, GameplayAbilities, EnhancedInput, CommonUI, ModelViewViewModel, Metasound, AudioModulation, Niagara, PoseSearch, Chooser, BlendStack, MotionWarping, AnimationWarping, AnimationLocomotionLibrary, CurveExpression, EngineCameras, Mover, NetworkPrediction, DrawDebugLibrary. 메타휴먼·MCP·툴셋·콘텐츠 변환기는 이 샘플에 포함하지 않는다.

## A-4. 모듈 의존성
- 플러그인 런타임 `DrFramework`: Core, CoreUObject, Engine, AudioMixer, Niagara, AudioModulation, AnimGraphRuntime, EngineCameras, InputCore, EnhancedInput, GameplayAbilities, GameplayTags, GameplayTasks, UMG, CommonUI, CommonInput, CinematicCamera, Json, ModelViewViewModel, FieldNotification, DrTableRuntime / private: Slate, SlateCore, NavigationSystem, (에디터 빌드) UnrealEd, AssetRegistry. `PublicIncludePaths.Add(ModuleDirectory)`.
- 소비 게임의 선택 에디터 `<Game>Editor`(이번 저장소에는 없음): Core, CoreUObject, Engine, UnrealEd, AssetRegistry, AssetTools, AudioModulation, BlueprintGraph, Kismet, EnhancedInput, Json, JsonUtilities, NavigationSystem, `<Game>`. 파이썬이 못 하는 편집만 둔다(캐릭터 BP 결선 커맨드릿, OFPA·내비 볼륨, 메뉴·사운드 에셋 보조).

## A-5. 설정 (ini)
| 파일 · 섹션 | 항목 |
|---|---|
| DefaultEngine `[/Script/EngineSettings.GameMapsSettings]` | `GameInstanceClass=/Script/DrFramework.DsGameInstance`; GameDefaultMap은 소비 게임이 설정(샘플은 없음) |
| DefaultEngine `[/Script/Engine.Engine]` | `GameUserSettingsClassName=/Script/DrFramework.DsGameUserSettings` |
| DefaultEngine `[CoreRedirects]` | 클래스 이름을 바꿀 때마다 `+ClassRedirects` |
| DefaultGame `[/Script/UnrealEd.ProjectPackagingSettings]` | `MapsToCook` 타이틀·아레나, `DirectoriesToAlwaysCook` UI·Input·Audio(코드가 경로로만 부르는 에셋). 테이블 폴더는 플러그인이 쿡하므로 넣지 않는다 |
| DefaultGame `[/Script/CommonInput.CommonInputSettings]` | `bEnableEnhancedInputSupport=True`, `InputData=BP_UIInputData_C` |
| DefaultGame `[CommonInputPlatformSettings_Windows …]` | `ControllerData` 키보드·게임패드 BP |
| DrTable 설정 | 샘플은 `AssetRoot=`; 실제 게임은 구운 테이블 폴더로 설정 |

## A-6. 게임 모드·맵
- `ADsGameMode`: `PlayerControllerClass=ADsPlayerController`. `DefaultPawnClass` 기본값은 ADsCharacter이며 InitGame에서 GameAssets.Player의 게임 BP를 로드한다. 빈 경로는 오류를 알리고 기본 클래스를 유지한다. 테스트는 Engine.GameModeBase를 사용해 이 게임 콘텐츠 로드를 격리한다.
- 타이틀 맵 월드 설정의 GameMode Override = `ADsTitleGameMode`(플레이어 없이 시네 카메라 + 타이틀 메뉴). 게임 맵은 기본 게임 모드.
- HUD 위젯 클래스는 `/Game/Dr/UI/WBP_CombatHUD`(없으면 C++ 클래스).
- 아레나 맵: 월드 파티션 + OFPA. 액터 태그 `Dr.Boss`(보스 배치), `Dr.Checkpoint.Gate`, `Dr.Arena.Door`(정적 메시 문, 런타임에 Movable로 바꾸고 바닥까지 내려 닫음), `Dr.Player.Start`. NavMesh 볼륨(테스트가 경로를 확인). 보스는 내비 없이 `AddMovementInput`으로 직진한다.

## A-7. 코드가 경로로 찾는 에셋
| 경로 | 쓰는 곳 |
|---|---|
| `/Game/Dr/Combat/DA_PlayerCombat` | `UDsPlayerCombatData::Get()`(없으면 CDO 기본값) |
| `/Game/Dr/Combat/Boss/DA_Boss<이름>` | 보스 캐릭터 `BossData`(없으면 CDO 기본값) |
| `/Game/Dr/Combat/DA_CombatFeel` | `UDsCombatFeelData::Get()`(없으면 CDO 기본값) |
| `/Game/Dr/FX/DA_CombatFX` | `UDsCombatFXData::Get()` |
| `/Game/Dr/Audio/DA_Audio` | 오디오 서브시스템 |
| `/Game/Dr/UI/ST_UI` | 메뉴 문구 키: Title, Start, Settings, Quit, Paused, Continue, Restart, ToTitle, SFX, Voice, Music, Back |
| `/Game/Dr/UI/M_UI_Glass` | 유리 패널·버튼(파라미터 `PanelSize`, `CornerRadius`, `FillOpacity`, `RimStrength`, `Tint`) |
| `/Game/Dr/Input/IMC_UI`·`IMC_Menu`, `IA_Pause/UIClick/UIBack` | 메뉴 입력(우선순위 200), 일시정지 |
| `/Game/Dr/Maps/L_Title`, `L_BossArena` | 시작·재시작 이동 |
| `/Game/Data/DA_<Table>` | DrTable 구운 테이블 |

# 부록 B. 동작 상세 (구현 기준)

- **일정 속도 이동**(`ApplyConstantMove`, 보스 `StartMove`): `FRootMotionSource_ConstantForce`, Override 누적, 우선순위 5, `Force = 방향 × 거리 / 시간`, 끝 속도 Clamp(회피 대시는 지정값, 보스는 0). 같은 이름 소스를 먼저 지운다.
- **모션 따라가기**: 매 틱 `MotionTime += dt × PlayRate`; 기준 뼈(pelvis)의 메시 공간 위치를 부모 사슬을 곱해 구하고 Z=0; 직전 샘플과의 차이를 표시 메시 회전으로 월드 변환해 `SafeMoveUpdatedComponent`(막히면 `SlideAlongSurface`); 표시 메시는 `기준 위치 − 회전 × 누적 오프셋`으로 되돌려 캡슐 위에 둔다. 슬롯이 없는 애님 BP면 단일 애니 재생으로 덮고 `길이/속도` 뒤 복구.
- **락온 대상 고르기**: 획득 거리 안 + 시야 반각 안 + 화면 안 + 가시(트레이스) 후보 중 `|요(yaw) 차| + 거리×0.001`이 가장 작은 것. 전환(dir≠0)은 그 방향으로 1° 넘게 떨어진 후보만. 해제: 대상 무효·사망·해제 거리 초과·가림 유예 초과.
- **락온 카메라**: 컨트롤 회전을 대상(가슴) 방향으로 `RInterpTo(속도 7)`, 피치 = 대상 피치 + 바이어스(−8)를 [−35, 10]으로 고정; 몸 요도 같은 속도로 보간. 근접도 `Close = 1 − clamp(거리/350)`; 암 = 원래 + 60 + 150×Close, 측면 오프셋 = 70×(1−Close). 해제하면 오프셋이 원래 값에 닿을 때까지 보간 후 복구.
- **보스 회전·정면**: `FixedTurn(현재 요, 목표 요, TurnRate×dt)`; 정면 = 전방·대상 방향 내적 ≥ cos(60°).
- **타격감**: 공격자·피격자 모두 `CustomTimeDilation = 이전값 × 0.08`(Light .06/Heavy .12/Boss .1초, 겹치면 긴 쪽); 피격자 보이는 스켈레탈 메시에 오버레이 머티리얼 .1초; 셰이크 대상은 Boss면 피격자, 아니면 공격자(로컬 플레이어일 때만), Light는 약한 셰이크. 내려찍기는 1500 안 플레이어에게 `1 − 거리/1500` 세기. 복구는 월드 실시간.
- **오디오**: 원거리 원샷 16~24초마다 플레이어 주변 1800~3200 반경 임의 방향; 보스 발소리는 수평 속도 > 80일 때 .65초 간격. 교전 시작 → 페이즈 1, 음악 2초 페이드인(.65), 보스 음성; 플레이어 사망 → 음악 .3으로, 사망 음성; 승리 → 3초 페이드아웃, 그 밖 .5초. 피격 소리는 매번, 음성은 생존 시만(HP ≤ 30%이면서 피해 ≥ 최대 10%면 큰 신음). 채널 음량은 컨트롤 버스 글로벌 믹스 값(.1초).
- **HUD 배치(1920 기준)**: 플레이어 패널 (40,40) 420×76 — 체력 게이지 (24,22) 372×12, 기력 (24,46) 372×8; 보스 패널 하단 중앙 (0,−42) 800×96 — 이름 + 게이지 (28,60) 744×16; 결과 띠 전체 폭 180. 게이지 머티리얼 파라미터 `FillRate`, `DelayedRate`(잔상), `Opacity`, `ResultBand`.
- **엔카운터 문**: 열린 위치에서 문 바운드 아래로 바닥을 트레이스해 닫힌 Z를 구하고 1초 보간.

# 부록 C. 테스트 명세 (다시 만든 코드가 통과해야 할 기대 결과)

이름은 `<Game>.<영역>.<하위|PIE>.<항목>`. 이식된 테스트는 경량 월드(`FWorldFixture`)·nullrhi에서 돈다. PIE 표시가 없어도 BP·몽타주·구운 테이블 의존 항목은 소비 게임에서 검증한다. 구체적인 제외·부분 이식 목록은 EXTRACTION_REPORT.md를 따른다.

| 테스트 | 기대 결과 |
|---|---|
| Framework.Network.LatencyProfile | 지연 끔이면 즉시, 고정 지연이면 그 시간 뒤 도착, 페이로드 보존 |
| Framework.Network.PacketPayload | `FInstancedStruct` 페이로드가 왕복 후 같은 타입·값 |
| Framework.Network.FakeServerReceiveQueue | 가짜 서버 수신 큐가 도착 시각 순서를 지킨다 |
| Framework.Tables.GeneratedRowLookup | 서브시스템으로 생성 행을 기본키로 찾고 행 수·태그가 맞다 |
| Framework.Tables.BakedAsset | 구운 에셋 로드, 스키마·내용 해시 기록, 한글 FText 보존 |
| Framework.Tables.NoRegisteredTablesLoadsWithoutWarning | 등록이 없어도 경고 없이 로드, 등록 안 된 행 조회는 null |
| Framework.Tables.GameAssets | `AreAllTablesLoaded()` 참, GameAssets 필수 행(Player·PlayerCombat·CombatFX·Boss)의 `path`가 로드되고 형식이 맞다 |
| Stats.Trait.FormulaDerivesStats | 특성 공식(체력→생명력, 정신→저항 상한 80) |
| Stats.Trait.InitFromStatsAppliesTraits | 스탯 블록 적용 시 공식대로 최대치 파생, 현재치 = 최대치 |
| Effects.DataPath.BakedRowAppliesToAbilitySystem | 구운 효과 행(화상 누적·단계 보너스·배수 기본값)이 ASC에 그대로 적용 |
| Effects.DataPath.ApplyToRouting | Caster/Victim 대상 분기, 없는 Id·대상 누락은 실패하고 아무 영향 없음 |
| Effects.DataPath.AbilityAppliesEffect | ApplyEffect 어빌리티가 이벤트 대상에게만 효과 적용 |
| Effects.Modifier.AggregatesAsProduct | 이동·받는·주는 피해 배수가 활성 효과의 곱 |
| Effects.Modifier.PermanentSurvivesTimedExpiry | 같은 태그 한시 효과가 끝나도 상시 효과 유지, 한시가 우선 |
| Effects.DamageOverTime.Ticks | ASC 틱 누적으로 주기마다 체력 감소, 단계 바뀌면 새 피해량 |
| Effects.Stack.AccumulatesCapsAndExpires | 누적 단계 상한, 만료 후 단계 초기화 |
| Effects.Stack.ReapplyPreservesTickPhase | 주기보다 빠른 재적용도 틱 위상 유지(피해 누락 없음), Refresh는 1단계 |
| Effects.Stack.MultiplierAndRefresh | 빙결 3단계 이동 0.47배, 보호막 갱신 시 흡수량 재충전 |
| Effects.Shield.AbsorbsThenExpiresWhenDepleted | 보호막이 먼저 흡수, 초과분만 통과, 소진 시 태그 해제 |
| Characters.Spawn.InitializesAbilities | BP 스폰 시 ASC·속성 세트 등록, 액터 정보 초기화 |
| Characters.Spawn.GrantsCombatAbilities | 전투 어빌리티 부여, 전투 입력 매핑·액션 설정됨 |
| Characters.Spawn.UsesGaspAndRetarget | 숨김 UEFN 소스 메시 + 보이는 Manny 메시 |
| Characters.Animation.PawnInterface | BP가 GASP 인터페이스 구현, 애니 속성 함수가 구조체 반환 |
| Characters.Movement.WalkRunSprint | 걷기 200·달리기 500·질주 700, 입력 후 속도 증가 |
| Characters.Spawn.GameModeUsesGreystone / HeroUsesMetaHuman / GreystoneUsesMetaHumanArmor | 게임 모드 기본 폰이 우선순위 외형 BP, 메타휴먼 몸·갑옷 메시 |
| Combat.Attack.DamagesTargetOncePerSwing | 한 타에 대상당 피해 1회 |
| Combat.Attack.LightComboAdvances | 입력 버퍼로 1→2→3타 진행 |
| Combat.Attack.SpendsStaminaAndBlocksWhenEmpty | 타마다 기력 소모, 부족하면 발동·연계 불가 |
| Combat.Dodge.InvulnerableWindow | .05~.3초만 무적, 그 밖은 피해 |
| Combat.Dodge.RollDirections | 입력 방향별 8방향 구르기 번호(0=앞, 시계방향) |
| Combat.Guard.ParryNegatesAndReturnsPoise | 쳐내기 구간 정면 피해 0, 공격자 포이즈 −40 |
| Combat.Guard.BlockReducesAndSpendsStamina | 막기 중 피해 30%, 원피해 50% 기력 소모 |
| Combat.Guard.BackAttackIgnoresGuard | 뒤에서 맞으면 막기·쳐내기 무시 |
| Combat.Reaction.PoiseBreakStaggers | 포이즈 붕괴 시 경직 태그, 시간 뒤 해제 |
| Combat.Reaction.DeathBlocksAbilities | 사망 후 어빌리티 발동 불가 |
| Combat.Feel.HitStopOverlayAndRestore | 히트스톱·오버레이 적용 후 원래 값 복구 |
| Combat.Feel.ActualMeleeHitAndInvulnerability | 실제 명중에만 타격감, 무적 대상은 없음 |
| Combat.LockOn.AcquireReleaseAndHUD | 획득·해제, HUD 락온 대상 갱신 |
| Combat.LockOn.CameraSwitchAndOcclusion | 카메라 암·오프셋 변화, 대상 전환, 가림 유예 후 해제 |
| Combat.PIE.InputSequence | 실제 맵에서 입력 순서(공격·회피·막기·락온)가 동작, 예고 바닥 원 표시 |
| Boss.Tables.PhasesAndAttacksLoad | 페이즈·공격 테이블 로드와 패턴 묶음 연결 |
| Boss.Brain.AggroThenApproach | 인식 거리 안에서 Idle→Approach |
| Boss.Brain.StaysIdleUntilEngageAllowed | 교전 허락 전에는 Idle 유지 |
| Boss.Brain.WaitsUntilFacingTarget | 정면이 아니면 전진·일반 공격 보류 |
| Boss.Brain.PicksOnlyInRangeAndOffCooldown | 사거리 안·쿨다운 끝난 공격만 고름 |
| Boss.Brain.WeightedPickFollowsData | 같은 시드에서 가중치 비율대로 선택 |
| Boss.Brain.WarnsOnceWhenTablesMissing | 테이블이 없으면 경고 1회, 크래시 없음 |
| Boss.Brain.ResetEncounterRestoresInitialState | 리셋 시 상태·페이즈·쿨다운 초기화 |
| Boss.Attack.DamagesOnceInActiveWindow | 판정 구간에서만, 대상당 1회 피해 |
| Boss.Attack.MoveDistanceAdvances | 돌진 공격이 이동 거리만큼 전진 |
| Boss.Attack.SiphonHealsOnHit | 흡수 명중 시 보스 회복 |
| Boss.Phase.EntersAtDataThreshold | HP가 테이블 진입선 이하면 다음 페이즈 |
| Boss.Phase.UsesPhaseTwoPatternSet | 2페이즈 패턴 묶음 사용 |
| Boss.Phase.InvulnerableDuringTransition | 전환 중 무적 |
| Boss.Reaction.ParryBreaksPoiseIntoGroggy | 쳐내기 반격으로 포이즈 붕괴 → 그로기 |
| Boss.Reaction.PlayerDamagesBoss | 플레이어 공격이 보스 체력을 깎는다 |
| Boss.Reaction.DeathStopsEverything | 사망 후 이동·공격·FX 정지 |
| Boss.PIE.EngagesPlayer | 실제 맵에서 아레나 진입 후 보스가 플레이어를 공격 |
| Encounter.Setup.InactiveWithoutMarkers | 태그 액터가 없으면 비활성 |
| Encounter.Checkpoint.ActivatesInRadius | 반경 안에서 체크포인트 활성 |
| Encounter.Arena.EntryClosesDoorAndStartsFight | 진입 깊이를 넘으면 문 닫힘·Fighting |
| Encounter.Retry.PlayerRespawnsAfterDelay | 사망 3초 뒤 체크포인트 리스폰 |
| Encounter.Retry.ResetsBoss / DoorReopens / RevivedPlayerCanAttack | 보스 원위치·문 열림·부활 후 공격 가능 |
| Encounter.Retry.FallOutOfWorldRespawns | 맵 밖 추락도 사망 처리 후 리스폰 |
| Encounter.Victory.OpensDoorAndStays | 승리 후 문 열림, 상태 유지 |
| Encounter.PIE.DeathRetryLoop | 실제 맵에서 사망→리스폰→재도전, 전투 음악 실제 재생 |
| Audio.Assets.Coverage | 오디오 데이터의 음악·원거리 MetaSound·버스 3개·감쇠·보스 공격 소리 해석 |
| Audio.Mix.EncounterStateAndPhase | 엔카운터 상태·페이즈에 따라 음악 소스·파라미터 |
| FX.Data.AllBossAttacks | 모든 보스 공격 Id에 FX 매핑 |
| FX.Dash.ActivationAndCancellation | 대시 FX 시작·취소 시 정리 |
| FX.Boss.TelegraphAndInterrupt | 예고 진행도·중단 시 정리 |
| FX.Boss.PhaseAndRetry | 2페이즈 루프, 재도전 시 초기화 |
| FX.Boss.ImpactLightLifetime | 충격광이 수명 뒤 꺼짐 |
| FX.Conversion.ReuseSystem | 변환 스크립트가 시스템을 지우지 않고 다시 채움 |
| UI.CombatHUD.AttributesAndNotifications | 속성 이벤트로 비율 갱신, 최대치 0 안전, 진입 전 보스 숨김 |
| UI.CombatHUD.EncounterRetryAndVictory | 교전 시 보스 표시, 사망·승리 결과, 재도전 시 초기화 |
| UI.CombatHUD.RebindAndShutdown | 폰 교체 시 이벤트 재연결, 종료 시 해제 |
| UI.CombatHUD.DamageTrailTiming | 잔상 .4초 유지 후 .3초 감소, 회복은 즉시 |
| UI.Menu.PauseInputMode | 일시정지 시 월드 정지·메뉴 입력·커서, 중복 방지, 계속하기로 복원 |
| UI.Menu.BlockDuringDeath | 사망 중 메뉴 차단, 사망 타이머는 계속 |
| UI.Menu.VolumePersistence | 슬라이더 → 버스 값, 설정 파일 재로드 후 유지 |
| UI.PIE.TitleStartTravel | 타이틀에서 시작하면 아레나 맵으로 이동 |
| Levels.BossArena.LoadMarkersAndNavigation | 아레나 맵의 태그 액터·내비 경로 |
| Levels.BossArena.MoodActors | 포스트 프로세스·안개·화톳불 조명 구성 |
| Arena.PIE.PropShots / Visual.PIE.MoodShots | 소품·분위기 장면 캡처(렌더 모드에서 PNG) |
| Playthrough.PIE.TitleReframed / EntryToBossDeathRetry | 타이틀 구도, 진입로→보스→사망→재도전 전체 흐름 |

## 샘플 콘텐츠 연결과 재현

작업 #64 범위 축소로 아래 샘플은 재구성 전의 보존 상태다. 최종 반영 여부와 남은 확인은 [SAMPLE_NOTES.md](SAMPLE_NOTES.md)를 따른다.

이 절은 게임이 프레임워크에 제공해야 하는 최소 콘텐츠 계약이다. 특정 리소스 공급자에 종속된 코드를 플러그인에 넣지 않는다. `Source/DrSample`은 이 계약의 예시 소비 게임이다.

1. **표시 메시와 애님 BP**: `ADsCharacter` 파생 캐릭터 BP의 숨긴 `Mesh`와 표시 `MannyMesh`를 구분한다. 호환 스켈레톤을 가진 표시 메시와 자체 애님 BP를 MannyMesh에 설정한다. 애님 BP는 이동 블렌드 → `DefaultSlot` → 최종 포즈로 연결한다. 샘플 `ABP_DrSample`은 `UDrSampleAnimInstance.Speed`(소유 Pawn의 수평 속도)를 이동 블렌드 X에 공급한다. 에셋 그래프 생성은 에디터 전용 `UDrSampleContent.BuildAnimationBlueprint`가 담당하며 AnimGraph·BlueprintGraph·KismetCompiler 링크는 Editor 빌드에서만 추가한다.
2. **플레이어 데이터**: `UDsPlayerCombatData`의 LightStages/HeavyStages에 몽타주·실제 길이·판정 구간·피해·기력·전진 거리를 설정한다. GameAssets.PlayerCombat으로 연결한다. 샘플은 근접 시퀀스로 `AM_DrSample_AttackLight/AttackHeavy`를 만들고 판정은 .25~.45초, 추가 전진은 25cm로 지정한다. 회피 RollMontages를 비워 기존 대시 경로를 사용한다. 피격 앞·뒤와 사망 몽타주는 캐릭터 BP의 해당 속성에 연결한다. DefaultAbilities에 프레임워크 약공격·강공격·회피·막기를 등록한다.
3. **적 데이터**: `ADsBossCharacter` 파생 BP에 메시와 `UDsBossData`를 설정한다. BossName은 BossPhase.Boss와 맞추고 Phase.PatternSet으로 BossAttack 행을 찾는다. 샘플은 Boss 이름, 두 페이즈, Swing 두 행과 Attack 애니 이름을 사용한다. `GameAssets.BossAnim_Idle/Move/Hit/Death/Attack`이 각각 시퀀스 경로를 제공한다. 기존 보스 API는 몽타주가 아닌 시퀀스를 재생한다. 샘플의 `ADrSampleEnemy`는 일반 피해 델리게이트에서 자체 피격 몽타주를 재생하고 완료 뒤 기본 시퀀스로 돌아간다. 추적·공격·페이즈·사망 판단은 기존 Brain이 수행한다.
4. **입력**: Axis2D 이동·마우스 시점·패드 시점과 Boolean 공격·회피·락온·달리기·점프·막기 InputAction을 만든다. 이동은 WASD negate/swizzle 및 패드 왼쪽 스틱, 시점은 Mouse2D와 패드 오른쪽 스틱을 각각 매핑한다. 캐릭터 BP의 InputMapping과 각 Action 속성을 설정한다. 조작 목록은 README를 따른다.
5. **게임 모드와 맵**: `ADsGameMode` 파생 BP의 Pawn과 GameAssets.Player에 플레이어 BP 클래스를 지정한다(경로 끝 `_C`). 맵에는 충돌 바닥·조명·하늘·PlayerStart·NavMeshBoundsVolume·적을 배치한다. 샘플 적은 기본 Brain의 근거리 자동 인식을 사용한다. 별도의 엔카운터 표식은 필수가 아니다. DefaultEngine.ini의 GameDefaultMap/EditorStartupMap과 GlobalDefaultGameMode를 설정한다.
6. **테이블 굽기**: Design/Tables/GameAssets.xlsx·BossTables.xlsx를 게임 콘텐츠에 맞춰 편집한다. `Tools/gen_tables.ps1 -DrTable <drtable.exe>`를 실행한 뒤 `UnrealEditor-Cmd DrSample.uproject -run=DrTableBake -Input=<TEMP>/DrSample-DrTable/client -Out=/Game/Data -nullrhi -unattended`로 굽는다. AssetRoot=/Game/Data를 설정한다. 생성 C++를 손으로 수정하지 않는다. CombatFX와 사운드가 없는 샘플은 기본 연출과 무음 대체 동작을 사용한다.

### Epic 리소스 설치와 재생성

`Tools/epic_content_manifest.txt`는 Manny/Quinn 메시·스켈레톤·물리 에셋·이동 블렌드·선택 시퀀스의 에셋 레지스트리 하드/소프트 패키지 의존성 폐쇄를 고정한다. `/Engine`·`/Script`는 복사하지 않고 `/Game`은 원래 패키지 경로를 보존한다. 현재 115개다. 가져오기는 `Tools/import_epic_content.ps1 -LyraPath <설치된 Lyra 프로젝트>`로 실행한다. 외부 런타임 설치는 필요 없다. 빌드된 DrSampleEditor와 엔진의 PythonScriptPlugin을 사용한다.

정리 단계는 **복사본만** 수정한다. Lyra 전용/로드 불가 노티파이, 에디터 AnimationModifiers 사용자 데이터와 PA_Mannequin의 전용 물리 재질 참조를 제거한다. 런타임 판정은 노티파이를 사용하지 않는다. 의존성 목록을 갱신할 때는 `DRSAMPLE_DISCOVER=1`, `DRSAMPLE_LYRA_PATH=<Lyra 프로젝트>` 환경 변수를 설정한 뒤 DrFramework에서 `Tools/prepare_epic_content.py`를 PythonScript 커맨드릿으로 실행한다. 목록 밖 후보 파일을 제거하고 빈 상태에서 가져오기와 재실행을 검증한다. Lyra 자체를 에디터로 열거나 원본을 저장하지 않는다.

샘플 BP·입력·몽타주·DataAsset·맵은 `Tools/build_sample_content.py`로 다시 만들 수 있다. 실행 형식은 `UnrealEditor-Cmd DrSample.uproject -run=pythonscript -script=<절대경로>/Tools/build_sample_content.py -EnablePlugins=PythonScriptPlugin -nullrhi -unattended`이다. 스크립트는 이름으로 기존 에셋을 찾아 갱신하며 자체 애님 그래프도 재구성한다. 배포본에는 Content/DrSample과 Content/Data가 포함된다. Epic 복사 대상 폴더는 Git에서 제외한다.

### 실제 PIE 검증

`DrSample.PIE.BasicCombat`은 L_DrSample을 열고 PIE를 시작한다. 1/64초 고정 스텝과 보스 시드 64를 사용하며 종료 시 이전 시간 설정을 복구한다. EnhancedInput 액션 주입으로 이동을 확인한 뒤 프레임워크 공격 입력으로 적 피해·피격, 적 Brain의 추적·공격으로 플레이어 피해·피격, 마지막 공격으로 적 사망 상태·사망 시퀀스를 확인한다. 데스크톱 입력 주입은 하지 않는다. NullRHI에서는 캡처를 생략하며 `-RenderOffscreen` 실행은 `Saved/Screenshots/DrSample/01_idle.png`, `02_attack_hit.png`, `03_player_hit.png`, `04_death.png`를 저장한다.

프레임워크 경량 테스트의 FWorldFixture는 게임의 전투 에셋 수치에 영향받지 않도록 기본값을 임시 적용하고 종료 시 원래 에셋 값을 복구한다. 실제 콘텐츠를 검증하는 PIE 테스트는 이 픽스처를 사용하지 않는다.
