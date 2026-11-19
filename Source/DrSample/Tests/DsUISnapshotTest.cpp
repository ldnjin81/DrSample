// UI 스타일 확인용 그림: 메뉴·HUD를 여러 테마로 그려 Saved/Screenshots/DrSample/UI/에 PNG로 남긴다.
// 렌더러가 있을 때만(-RenderOffscreen 등) 그린다. -nullrhi 실행에서는 건너뛴다.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Game/DsPlayerController.h"
#include "Flow/DrUIFlowSubsystem.h"
#include "CommonInputSettings.h"
#include "ICommonInputModule.h"
#include "UI/DsCombatHUDViewModel.h"
#include "UI/DsCombatHUDWidget.h"
#include "UI/DsPauseMenuWidget.h"
#include "UI/DsSettingsMenuWidget.h"
#include "UI/DsTitleMenuWidget.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Boss/DsBossCharacter.h"
#include "Characters/DsCharacter.h"
#include "Encounter/DsEncounterSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TargetPoint.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMeshActor.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Tests/DsTestFixture.h"
#include "Theme/DrUITheme.h"
#include "Theme/DrUIThemeSubsystem.h"

namespace DsSnapshot {
bool Save(UUserWidget* Widget, const FString& Name, FVector2D Size) {
  FWidgetRenderer Renderer(true);
  // Slate 위젯을 붙잡아 둔다. 놓으면 다음 TakeWidget이 위젯을 새로 만들어 매번 첫 프레임만 그린다
  // (틱에서 채우는 행동 바 키 안내가 크기를 못 받아 빈칸으로 찍혔다).
  const TSharedRef<SWidget> Slate = Widget->TakeWidget();
  // 스타일 그림(9분할 PNG)은 처음 그릴 때 텍스처로 만들어지고, 틱에서 바뀐 레이아웃은 다음 프레임에 잡히므로 몇 번 그린다.
  UTextureRenderTarget2D* Target = nullptr;
  for (int32 Pass = 0; Pass < 3; ++Pass) {
    Target = Renderer.DrawWidget(Slate, Size);
    FlushRenderingCommands();
  }
  if (!Target) return false;
  FlushRenderingCommands();
  const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots/DrSample/UI") / (Name + TEXT(".png"));
  TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*Path));
  return File && FImageUtils::ExportRenderTarget2DAsPNG(Target, *File);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsUISnapshotTest, "DrSample.UI.Snapshot.MenusAndHUD",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDsUISnapshotTest::RunTest(const FString&) {
  if (!FApp::CanEverRender() || !GDynamicRHI || FCString::Stricmp(GDynamicRHI->GetName(), TEXT("Null")) == 0) {
    AddInfo(TEXT("렌더러가 없어 UI 그림을 건너뜁니다(-RenderOffscreen으로 실행)."));
    return true;
  }
  // 메뉴 테스트와 같다: 샘플은 CommonUI 뒤로 액션을 지정하지 않는다(뒤로는 메뉴 기반이 Esc·패드 키로 처리).
  AddExpectedError(TEXT("no action provided"), EAutomationExpectedErrorFlags::Contains, 0);
  ICommonInputModule::GetSettings().LoadData();
  DsTestFixture::FWorldFixture Base{true};
  GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
  Base.World->SetGameInstance(NewObject<UGameInstance>(GEngine));
  Base.World->InitializeActorsForPlay(FURL());
  Base.World->SetBegunPlay(true);
  auto* PC = Base.World->SpawnActorDeferred<ADsPlayerController>(ADsPlayerController::StaticClass(), FTransform::Identity);
  auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
  PC->Player = LocalPlayer;
  LocalPlayer->PlayerController = PC;
  LocalPlayer->PlayerAdded(nullptr, FPlatformUserId::CreateFromInternalId(0));
  PC->FinishSpawning(FTransform::Identity);
  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  auto* Player = Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(), FVector(400, 0, 100), FRotator::ZeroRotator, Params);
  auto* Boss = Base.World->SpawnActor<ADsBossCharacter>(ADsBossCharacter::StaticClass(), FVector(2000, 0, 100), FRotator::ZeroRotator, Params);
  auto* Point = Base.World->SpawnActor<ATargetPoint>(FVector(-500, 0, 100), FRotator::ZeroRotator);
  auto* Door = Base.World->SpawnActor<AStaticMeshActor>(FVector(0, 0, 800), FRotator::ZeroRotator);
  auto* Encounter = Base.World->GetSubsystem<UDsEncounterSubsystem>();
  Encounter->BindActors(Player, Boss, Point, Door, Point);
  Encounter->Advance(.01f);
  auto* VM = NewObject<UDsCombatHUDViewModel>();
  VM->Initialize(Encounter);
  VM->BindPlayer(Player);
  Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), Player->GetAttributeSet()->GetMaxHealth() * .62f);
  Boss->GetAttributes()->SetHealth(Boss->GetAttributes()->GetMaxHealth() * .4f);

  auto* Themes = UDrUIThemeSubsystem::Get();
  const FName OldTheme = Themes->GetTheme().ThemeId;
  int32 Saved = 0;
  for (const TCHAR* Theme : {TEXT("darkfantasy-dark"), TEXT("fantasy-dark"), TEXT("parchment-light"), TEXT("scifi-dark"), TEXT("glass-light")}) {
    if (!Themes->SetTheme(Theme)) { AddWarning(FString::Printf(TEXT("테마 없음: %s"), Theme)); continue; }
    const FString Suffix = FString(TEXT("-")) + Theme;
    auto Draw = [&](UUserWidget* Widget, const TCHAR* Name) {
      Widget->SetOwningPlayer(PC);
      Saved += DsSnapshot::Save(Widget, Name + Suffix, FVector2D(1280, 720)) ? 1 : 0;
    };
    Draw(CreateWidget<UDsTitleMenuWidget>(PC), TEXT("title"));
    Draw(CreateWidget<UDsPauseMenuWidget>(PC), TEXT("pause"));
    Draw(CreateWidget<UDsSettingsMenuWidget>(PC), TEXT("settings"));
    auto* HUD = CreateWidget<UDsCombatHUDWidget>(PC);
    HUD->TakeWidget();
    HUD->SetViewModel(VM);
    Draw(HUD, TEXT("hud"));
    HUD->SetViewModel(nullptr);
  }
  Themes->SetTheme(OldTheme);
  TestTrue(TEXT("UI 그림 저장"), Saved > 0);
  AddInfo(FString::Printf(TEXT("UI 그림 %d장: %s"), Saved, *(FPaths::ProjectSavedDir() / TEXT("Screenshots/DrSample/UI"))));
  VM->Shutdown();
  Base.World->EndPlay(EEndPlayReason::Quit);
  LocalPlayer->PlayerRemoved();
  LocalPlayer->PlayerController = nullptr;
  GEngine->DestroyWorldContext(Base.World);
  return true;
}
#endif
