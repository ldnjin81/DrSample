#include "UI/DsCombatHUDWidget.h"

#include "UI/DsCombatHUDViewModel.h"
#include "Combat/DsLockOnTargetComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Input/CommonUIInputTypes.h"
#include "Theme/DrUITheme.h"
#include "Theme/DrUIThemeSubsystem.h"
#include "TimerManager.h"
#include "Widgets/DrStatus.h"
#include "Widgets/DrSurface.h"
#include "Widgets/DrText.h"

#define LOCTEXT_NAMESPACE "DrHUD"

TOptional<FUIInputConfig> UDsCombatHUDWidget::GetDesiredInputConfig() const {
  return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}

namespace {
UCanvasPanelSlot* Place(UCanvasPanel* Parent, UWidget* Child, FVector2D Position, FVector2D Size,
    FVector2D Anchor = FVector2D::ZeroVector, FVector2D Alignment = FVector2D::ZeroVector) {
  auto* Slot = Parent->AddChildToCanvas(Child);
  Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
  Slot->SetAlignment(Alignment);
  Slot->SetPosition(Position);
  Slot->SetSize(Size);
  return Slot;
}
}

UDrSurface* UDsCombatHUDWidget::MakePanel(UCanvasPanel* Root, FVector2D Position, FVector2D Size, FVector2D Anchor, FVector2D Alignment) {
  auto* Panel = WidgetTree->ConstructWidget<UDrSurface>();
  Panel->RadiusToken = TEXT("radius-lg");
  Panel->PaddingToken = TEXT("space-4");
  Panel->SetElevation(DrUITokens::ShadowGlass);
  Place(Root, Panel, Position, Size, Anchor, Alignment);
  return Panel;
}

UDsCombatHUDWidget::FGauge UDsCombatHUDWidget::MakeGauge(UPanelWidget* Parent, FName FillToken, float Thickness) {
  // 아래 막대 = 잔상(피해 직후 잠시 남았다가 따라감), 위 막대 = 현재 값. 위 막대의 트랙(hairline)은 반투명이라 잔상이 비친다.
  auto* Layers = WidgetTree->ConstructWidget<UOverlay>();
  FGauge Gauge;
  Gauge.Trail = WidgetTree->ConstructWidget<UDrProgressBar>();
  Gauge.Trail->FillToken = TEXT("ember");
  Gauge.Fill = WidgetTree->ConstructWidget<UDrProgressBar>();
  Gauge.Fill->FillToken = FillToken;
  for (UDrProgressBar* Bar : {Gauge.Trail.Get(), Gauge.Fill.Get()}) {
    Bar->Thickness = Thickness;
    Bar->SetPercent(1.f);
    if (auto* S = Layers->AddChildToOverlay(Bar)) { S->SetHorizontalAlignment(HAlign_Fill); S->SetVerticalAlignment(VAlign_Center); }
  }
  if (auto* Box = Cast<UVerticalBox>(Parent)) {
    if (auto* S = Box->AddChildToVerticalBox(Layers)) S->SetPadding(FMargin(0, 4));
  } else Parent->AddChild(Layers);
  return Gauge;
}

TSharedRef<SWidget> UDsCombatHUDWidget::RebuildWidget() {
  if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
  if (!WidgetTree->RootWidget || WidgetTree->RootWidget->GetFName() != TEXT("HUDRoot")) {
    auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
    WidgetTree->RootWidget = Root;
    SetVisibility(ESlateVisibility::HitTestInvisible);
    LockOnMarker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LockOnMarker"));
    Place(Root, LockOnMarker, FVector2D::ZeroVector, FVector2D(18, 18), FVector2D::ZeroVector, FVector2D(.5f, .5f));
    LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);

    // 플레이어: 왼쪽 위 체력·기력(1920 기준, 엔진 기본 DPI 곡선으로 해상도에 맞춰 줄어든다).
    auto* PlayerPanel = MakePanel(Root, FVector2D(40, 40), FVector2D(420, 64), FVector2D::ZeroVector, FVector2D::ZeroVector);
    auto* PlayerColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    PlayerPanel->SetContent(PlayerColumn);
    const FGauge Health = MakeGauge(PlayerColumn, DrUITokens::Danger, 10.f);
    HealthTrailBar = Health.Trail; HealthBar = Health.Fill;
    const FGauge Stamina = MakeGauge(PlayerColumn, DrUITokens::Success, 6.f);
    StaminaTrailBar = Stamina.Trail; StaminaBar = Stamina.Fill;
    StaminaTrailBar->SetVisibility(ESlateVisibility::Collapsed);  // 기력은 잔상 없이 바로 따라간다.

    // 보스: 아래 가운데 이름 + 체력.
    BossPanel = MakePanel(Root, FVector2D(0, -42), FVector2D(800, 92), FVector2D(.5f, 1.f), FVector2D(.5f, 1.f));
    auto* BossColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    BossPanel->SetContent(BossColumn);
    BossNameText = WidgetTree->ConstructWidget<UDrText>();
    BossNameText->SetTokens(TEXT("title"), DrUITokens::Ink);
    BossNameText->SetJustification(ETextJustify::Center);
    if (auto* S = BossColumn->AddChildToVerticalBox(BossNameText)) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 0, 0, 6)); }
    const FGauge Boss = MakeGauge(BossColumn, DrUITokens::Danger, 12.f);
    BossTrailBar = Boss.Trail; BossBar = Boss.Fill;

    // 결과 띠: 화면 가운데 전체 폭, 테마의 막(scrim) 위에 큰 제목과 안내.
    ResultPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultPanel"));
    auto* ResultSlot = Root->AddChildToCanvas(ResultPanel);
    ResultSlot->SetAnchors(FAnchors(0, .5f, 1, .5f));
    ResultSlot->SetAlignment(FVector2D(0, .5f));
    ResultSlot->SetOffsets(FMargin(0, 0, 0, 180));
    auto* ResultColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    ResultPanel->SetContent(ResultColumn);
    ResultPanel->SetHorizontalAlignment(HAlign_Fill);
    ResultPanel->SetVerticalAlignment(VAlign_Center);
    ResultTitle = WidgetTree->ConstructWidget<UTextBlock>();
    ResultTitle->SetJustification(ETextJustify::Center);
    if (auto* S = ResultColumn->AddChildToVerticalBox(ResultTitle)) S->SetHorizontalAlignment(HAlign_Center);
    ResultSubtitle = WidgetTree->ConstructWidget<UDrText>();
    ResultSubtitle->SetTokens(DrUITokens::Body, DrUITokens::InkMuted);
    ResultSubtitle->SetJustification(ETextJustify::Center);
    if (auto* S = ResultColumn->AddChildToVerticalBox(ResultSubtitle)) { S->SetHorizontalAlignment(HAlign_Center); S->SetPadding(FMargin(0, 8, 0, 0)); }
    ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
    BossPanel->SetVisibility(ESlateVisibility::Collapsed);
    ApplyTheme(UDrUIThemeSubsystem::Current());
  }
  return Super::RebuildWidget();
}

void UDsCombatHUDWidget::ApplyTheme(const UDrUITheme& Theme) {
  // DrUIKit 부품은 스스로 테마를 따른다. 여기서는 엔진 위젯(결과 띠·큰 제목·락온 점)만 칠한다.
  if (LockOnMarker) LockOnMarker->SetBrush(FSlateRoundedBoxBrush(Theme.GetColor(DrUITokens::Danger), 9.f, Theme.GetColor(DrUITokens::Ink), 2.f, FVector2D(18)));
  if (ResultPanel) ResultPanel->SetBrushColor(Theme.GetColor(TEXT("scrim")));
  if (ResultTitle) {
    FSlateFontInfo Font = Theme.MakeFont(TEXT("display"));
    Font.Size = 56;
    Font.LetterSpacing = 180;
    ResultTitle->SetFont(Font);
  }
  Refresh();
}

void UDsCombatHUDWidget::NativeTick(const FGeometry& Geometry, float DeltaTime) {
  Super::NativeTick(Geometry, DeltaTime);
  if (!LockOnMarker) return;
  auto* Target = ViewModel ? ViewModel->LockOnTarget.Get() : nullptr;
  auto* Component = IsValid(Target) ? Target->FindComponentByClass<UDsLockOnTargetComponent>() : nullptr;
  FVector2D Position = FVector2D::ZeroVector;
  const bool bShow = Component && Component->IsAvailable() && GetOwningPlayer() &&
    UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), Component->GetTargetLocation(), Position, false);
  LockOnMarker->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  if (bShow) CastChecked<UCanvasPanelSlot>(LockOnMarker->Slot)->SetPosition(Position);
}

void UDsCombatHUDWidget::SetViewModel(UDsCombatHUDViewModel *InViewModel) {
  if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
  ViewModel = InViewModel;
  HealthTrail.Reset(ViewModel ? ViewModel->GetPlayerHealthRate() : 1.f);
  BossTrail.Reset(ViewModel ? ViewModel->BossHealthRate : 1.f);
  Subscribe();
  Refresh();
}

void UDsCombatHUDWidget::Subscribe() {
  if (!ViewModel) return;
  ViewModel->RemoveAllFieldValueChangedDelegates(this);
  // 네이티브 레이아웃도 MVVM FieldNotify를 사용하므로 프레임별 바인딩 평가가 없다.
  using FFields = UDsCombatHUDViewModel::FFieldNotificationClassDescriptor;
  for (auto Field : {FFields::GetPlayerHealthRate, FFields::GetPlayerStaminaRate, FFields::BossVisible,
                     FFields::BossDisplayName, FFields::BossHealthRate, FFields::EncounterState})
    ViewModel->AddFieldValueChangedDelegate(Field, INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateWeakLambda(this,
      [this](UObject *, UE::FieldNotification::FFieldId) { Refresh(); }));
}

void UDsCombatHUDWidget::NativeConstruct() {
  Super::NativeConstruct();
  if (auto* Themes = UDrUIThemeSubsystem::Get(); Themes && !ThemeHandle.IsValid())
    ThemeHandle = Themes->OnThemeChanged.AddUObject(this, &ThisClass::ApplyTheme);
  Subscribe();
  Refresh();
}

void UDsCombatHUDWidget::NativeDestruct() {
  if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
  if (auto* Themes = UDrUIThemeSubsystem::Get()) Themes->OnThemeChanged.Remove(ThemeHandle);
  ThemeHandle.Reset();
  if (GetWorld()) {
    GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
    GetWorld()->GetTimerManager().ClearTimer(DamageTrailTimer);
  }
  bResultShown = false;
  Super::NativeDestruct();
}

void UDsCombatHUDWidget::Refresh() {
  if (!ViewModel || !ResultPanel) return;
  HealthBar->SetPercent(ViewModel->GetPlayerHealthRate());
  StaminaBar->SetPercent(ViewModel->GetPlayerStaminaRate());
  BossBar->SetPercent(ViewModel->BossHealthRate);
  if (GetWorld()) {
    const double Now = GetWorld()->GetTimeSeconds();
    HealthTrail.SetTarget(ViewModel->GetPlayerHealthRate(), Now);
    BossTrail.SetTarget(ViewModel->BossHealthRate, Now);
    AdvanceDamageTrails();
    if ((HealthTrail.bActive || BossTrail.bActive) && !GetWorld()->GetTimerManager().IsTimerActive(DamageTrailTimer))
      GetWorld()->GetTimerManager().SetTimer(DamageTrailTimer, this, &ThisClass::AdvanceDamageTrails, .02f, true);
  }
  BossPanel->SetVisibility(ViewModel->BossVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  BossNameText->SetText(ViewModel->BossDisplayName);
  const bool bShow = ViewModel->IsPlayerDead() || ViewModel->IsVictory();
  ResultPanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  if (bShow) {
    const bool bDead = ViewModel->IsPlayerDead();
    const UDrUITheme& Theme = UDrUIThemeSubsystem::Current();
    ResultTitle->SetText(bDead ? LOCTEXT("Died", "YOU DIED") : LOCTEXT("Victory", "적을 쓰러뜨렸다"));
    ResultTitle->SetColorAndOpacity(FSlateColor(Theme.GetColor(bDead ? DrUITokens::Danger : DrUITokens::Warning)));
    ResultSubtitle->SetText(bDead ? LOCTEXT("Retry", "곧 체크포인트에서 다시 시작합니다") : FText::GetEmpty());
  }
  if (bShow && !bResultShown && GetWorld()) {
    FadeStart = GetWorld()->GetTimeSeconds();
    ResultPanel->SetRenderOpacity(0.f);
    GetWorld()->GetTimerManager().SetTimer(FadeTimer, this, &ThisClass::AdvanceFade, .02f, true);
  } else if (!bShow && GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
  bResultShown = bShow;
}

void UDsCombatHUDWidget::AdvanceFade() {
  if (!GetWorld() || !ResultPanel) return;
  const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - FadeStart) / .65), 0.f, 1.f);
  ResultPanel->SetRenderOpacity(Alpha);
  if (Alpha >= 1.f) GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
}

void UDsCombatHUDWidget::AdvanceDamageTrails() {
  if (!GetWorld()) return;
  const double Now = GetWorld()->GetTimeSeconds();
  HealthTrail.Advance(Now);
  BossTrail.Advance(Now);
  if (HealthTrailBar) HealthTrailBar->SetPercent(HealthTrail.Value);
  if (BossTrailBar) BossTrailBar->SetPercent(BossTrail.Value);
  if (!HealthTrail.bActive && !BossTrail.bActive) GetWorld()->GetTimerManager().ClearTimer(DamageTrailTimer);
}

float UDsCombatHUDWidget::GetHealthFill() const { return HealthBar ? HealthBar->GetPercent() : 0.f; }
float UDsCombatHUDWidget::GetHealthTrail() const { return HealthTrailBar ? HealthTrailBar->GetPercent() : 0.f; }
float UDsCombatHUDWidget::GetBossFill() const { return BossBar ? BossBar->GetPercent() : 0.f; }
bool UDsCombatHUDWidget::IsBossPanelVisible() const { return BossPanel && BossPanel->GetVisibility() != ESlateVisibility::Collapsed; }
bool UDsCombatHUDWidget::IsResultVisible() const { return ResultPanel && ResultPanel->GetVisibility() != ESlateVisibility::Collapsed; }
FText UDsCombatHUDWidget::GetResultTitle() const { return ResultTitle ? ResultTitle->GetText() : FText::GetEmpty(); }

#undef LOCTEXT_NAMESPACE
