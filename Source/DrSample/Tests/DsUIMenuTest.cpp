// 패키지 타깃에는 에디터 API를 포함하지 않는다.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Game/DsPlayerController.h"
#include "Flow/DrUIFlowSubsystem.h"
#include "CommonInputSettings.h"
#include "ICommonInputModule.h"
#include "UI/DsMenuWidget.h"
#include "UI/DsTitleMenuWidget.h"
#include "UI/DsPauseMenuWidget.h"
#include "UI/DsSettingsMenuWidget.h"
#include "Game/DsGameUserSettings.h"
#include "UI/DsCombatHUDWidget.h"
#include "Abilities/DsAttributeSet.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Audio/DsAudioSubsystem.h"
#include "Characters/DsCharacter.h"
#include "Boss/DsBossCharacter.h"
#include "Components/Slider.h"
#include "Theme/DrUIStyle.h"
#include "Theme/DrUITheme.h"
#include "Theme/DrUIThemeSubsystem.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTestFixture.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace DsMenuTests {
struct FFixture {
  DsTestFixture::FWorldFixture Base{true};
  ADsPlayerController* PC;
  ULocalPlayer* LocalPlayer;
  UDrUIFlowSubsystem* Flow;
  ADsCharacter* Hero;
  FFixture() {
    // 콘텐츠 없는 테스트는 CommonInput 선택 데이터의 초기화를 명시적으로 마친다.
    ICommonInputModule::GetSettings().LoadData();
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
    auto* Instance = NewObject<UGameInstance>(GEngine);
    Base.World->SetGameInstance(Instance);
    Base.World->SetGameMode(FURL(nullptr, TEXT("/Temp/DrMenuTest?game=/Script/Engine.GameModeBase"), TRAVEL_Absolute));
    Base.World->InitializeActorsForPlay(FURL());
    Base.World->SetBegunPlay(true);
    // 로컬 플레이어(UI 흐름 서브시스템 포함)를 먼저 붙인 뒤 BeginPlay가 돌아야 게임 화면 진입(OnEnterGame)이 흐름을 시작한다.
    PC = Base.World->SpawnActorDeferred<ADsPlayerController>(ADsPlayerController::StaticClass(), FTransform::Identity);
    LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    PC->Player = LocalPlayer;
    LocalPlayer->PlayerController = PC;
    LocalPlayer->PlayerAdded(nullptr, FPlatformUserId::CreateFromInternalId(0));
    PC->FinishSpawning(FTransform::Identity);
    Flow = UDrUIFlowSubsystem::Get(PC);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Hero = Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(), FVector(0,0,100), FRotator::ZeroRotator, Params);
    PC->Possess(Hero);
  }
  ~FFixture() {
    if (Flow) Flow->ClosePauseMenu();
    Base.World->EndPlay(EEndPlayReason::Quit);
    LocalPlayer->PlayerRemoved();
    LocalPlayer->PlayerController = nullptr;
    GEngine->DestroyWorldContext(Base.World);
  }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuPauseTest, "DrSample.UI.Menu.PauseInputMode",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuPauseTest::RunTest(const FString&) {
  DsMenuTests::FFixture F;
  if (!TestNotNull(TEXT("UI 흐름 서브시스템"), F.Flow)) return false;
  TestTrue(TEXT("일시정지 열기"), F.Flow->OpenPauseMenu());
  TestTrue(TEXT("월드 일시정지"), UGameplayStatics::IsGamePaused(F.Base.World));
  TestTrue(TEXT("메뉴 입력과 커서"), F.Flow->IsMenuInput() && F.PC->bShowMouseCursor);
  TestFalse(TEXT("중복 메뉴 방지"), F.Flow->OpenPauseMenu());
  F.Flow->ClosePauseMenu();
  TestFalse(TEXT("계속하기로 일시정지 해제"), UGameplayStatics::IsGamePaused(F.Base.World));
  TestFalse(TEXT("게임 입력 복원"), F.Flow->IsMenuInput() || F.PC->bShowMouseCursor);
  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuDeathTest, "DrSample.UI.Menu.BlockDuringDeath",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuDeathTest::RunTest(const FString&) {
  DsMenuTests::FFixture F;
  if (!TestNotNull(TEXT("UI 흐름 서브시스템"), F.Flow)) return false;
  auto* ASC = F.Hero->GetAbilitySystemComponent();
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), 0.f);
  TestFalse(TEXT("사망 연출 중 일시정지 차단"), F.Flow->OpenPauseMenu());
  TestFalse(TEXT("사망 타이머 진행 유지"), UGameplayStatics::IsGamePaused(F.Base.World));
  F.Hero->Revive(F.Hero->GetActorTransform());
  TestTrue(TEXT("부활 후 메뉴 허용"), F.Flow->OpenPauseMenu());
  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuSettingsTest, "DrSample.UI.Menu.VolumePersistence",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuSettingsTest::RunTest(const FString&) {
  DsMenuTests::FFixture F;
  // 샘플은 CommonUI 뒤로 액션을 지정하지 않는다(뒤로는 메뉴 기반의 Esc·패드 키 처리). 로컬 플레이어가 등록돼 액션 라우터가 있으면 그 사실을 오류로 남긴다.
  AddExpectedError(TEXT("no action provided"), EAutomationExpectedErrorFlags::Contains, 0);
  auto* Settings = UDsGameUserSettings::Get();
  if (!TestNotNull(TEXT("프로젝트 설정 클래스"), Settings)) return false;
  float Old[3];
  auto* Widget = NewObject<UDsSettingsMenuWidget>(F.Base.World);
  Widget->SetOwningPlayer(F.PC);
  Widget->Initialize();
  if (!TestNotNull(TEXT("설정 위젯 생성"), Widget)) return false;
  // nullrhi에서도 슬라이더 델리게이트를 실제 생성해 저장·버스 경로를 검증한다.
  Widget->TakeWidget();
  const float Values[] = {.2f, .45f, .7f};
  for (int32 I = 0; I < 3; ++I) {
    auto Channel = static_cast<EDsAudioChannel>(I);
    Old[I] = Settings->GetVolume(Channel);
    auto* Slider = Widget->GetSlider(Channel);
    if (TestNotNull(TEXT("음량 슬라이더"), Slider)) {
      Slider->SetValue(Values[I]); Slider->OnValueChanged.Broadcast(Values[I]);
    }
    TestEqual(TEXT("Control Bus API 적용값"), F.Base.World->GetSubsystem<UDsAudioSubsystem>()->GetChannelVolume(Channel), Values[I]);
  }
  Settings->LoadSettings(true);
  for (int32 I = 0; I < 3; ++I) {
    const auto Channel = static_cast<EDsAudioChannel>(I);
    TestEqual(TEXT("파일에서 재로드한 음량"), Settings->GetVolume(Channel), Values[I]);
    Settings->SetVolume(Channel, Old[I]);
  }
  Settings->SaveSettings();
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuThemeTest, "DrSample.UI.Menu.ThemeSelection",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuThemeTest::RunTest(const FString&) {
  // 밝기에 맞는 테마 id 고르기(없으면 그 스타일의 첫 테마).
  const TArray<FName> Pair = {TEXT("a-light"), TEXT("a-dark")};
  TestEqual(TEXT("어둡게"), UDsSettingsMenuWidget::PickThemeId(Pair, TEXT("a"), 1), FName(TEXT("a-dark")));
  TestEqual(TEXT("밝게"), UDsSettingsMenuWidget::PickThemeId(Pair, TEXT("a"), 0), FName(TEXT("a-light")));
  TestEqual(TEXT("밝기 테마가 없으면 첫 테마"), UDsSettingsMenuWidget::PickThemeId({TEXT("b-night")}, TEXT("b"), 0), FName(TEXT("b-night")));
  TestEqual(TEXT("테마 없음"), UDsSettingsMenuWidget::PickThemeId({}, TEXT("c"), 0), FName(NAME_None));

  // 실제 스타일 플러그인으로 전환·저장. 샘플은 DrUIGlass와 게임 스타일들을 켠다.
  const auto& Styles = DrUIStyle::GetLoadedStyles();
  if (!TestTrue(TEXT("스타일 플러그인 로드"), Styles.Num() > 1)) return false;
  TestTrue(TEXT("기본 스타일(다크 판타지) 로드"), Styles.ContainsByPredicate([](const FDrUIStyleInfo& Info) { return Info.Id == TEXT("darkfantasy"); }));
  auto* Themes = UDrUIThemeSubsystem::Get();
  auto* Settings = UDsGameUserSettings::Get();
  if (!TestNotNull(TEXT("테마 서브시스템"), Themes) || !TestNotNull(TEXT("사용자 설정"), Settings)) return false;
  const FName OldTheme = Themes->GetTheme().ThemeId;
  const FName OldSaved = Settings->GetUITheme();
  auto* Widget = NewObject<UDsSettingsMenuWidget>();
  const int32 Index = Styles.Num() - 1;
  TestTrue(TEXT("스타일 전환"), Widget->SelectTheme(Index, 0));
  const FName Expected = UDsSettingsMenuWidget::PickThemeId(Styles[Index].ThemeIds, Styles[Index].Id, 0);
  TestEqual(TEXT("현재 테마"), Themes->GetTheme().ThemeId, Expected);
  TestEqual(TEXT("사용자 설정에 저장"), Settings->GetUITheme(), Expected);
  TestFalse(TEXT("없는 스타일은 무시"), Widget->SelectTheme(Styles.Num(), 0));
  Themes->SetTheme(OldTheme);
  Settings->SetUITheme(OldSaved);
  Settings->SaveSettings();
  return true;
}

#endif
