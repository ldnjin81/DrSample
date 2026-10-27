// 패키지 타깃에는 에디터 API를 포함하지 않는다.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Game/DsPlayerController.h"
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
    PC = Base.World->SpawnActor<ADsPlayerController>();
    auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    PC->Player = LocalPlayer;
    LocalPlayer->PlayerController = PC;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Hero = Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(), FVector(0,0,100), FRotator::ZeroRotator, Params);
    PC->Possess(Hero);
  }
  ~FFixture() {
    PC->ClosePauseMenu();
    Base.World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(Base.World);
  }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuPauseTest, "DrSample.UI.Menu.PauseInputMode",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuPauseTest::RunTest(const FString&) {
  DsMenuTests::FFixture F;
  TestTrue(TEXT("일시정지 열기"), F.PC->OpenPauseMenu());
  TestTrue(TEXT("월드 일시정지"), UGameplayStatics::IsGamePaused(F.Base.World));
  TestTrue(TEXT("메뉴 입력과 커서"), F.PC->IsMenuInput() && F.PC->bShowMouseCursor);
  TestFalse(TEXT("중복 메뉴 방지"), F.PC->OpenPauseMenu());
  F.PC->ClosePauseMenu();
  TestFalse(TEXT("계속하기로 일시정지 해제"), UGameplayStatics::IsGamePaused(F.Base.World));
  TestFalse(TEXT("게임 입력 복원"), F.PC->IsMenuInput() || F.PC->bShowMouseCursor);
  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuDeathTest, "DrSample.UI.Menu.BlockDuringDeath",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuDeathTest::RunTest(const FString&) {
  DsMenuTests::FFixture F;
  auto* ASC = F.Hero->GetAbilitySystemComponent();
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), 0.f);
  TestFalse(TEXT("사망 연출 중 일시정지 차단"), F.PC->OpenPauseMenu());
  TestFalse(TEXT("사망 타이머 진행 유지"), UGameplayStatics::IsGamePaused(F.Base.World));
  F.Hero->Revive(F.Hero->GetActorTransform());
  TestTrue(TEXT("부활 후 메뉴 허용"), F.PC->OpenPauseMenu());
  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrMenuSettingsTest, "DrSample.UI.Menu.VolumePersistence",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrMenuSettingsTest::RunTest(const FString&) {
  DsMenuTests::FFixture F;
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

#endif



