#include "UI/DsCombatHUDWidget.h"
#include "TableData/DsGameAssets.h"

#include "UI/DsCombatHUDViewModel.h"
#include "Combat/DsLockOnTargetComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Input/CommonUIInputTypes.h"

TOptional<FUIInputConfig> UDsCombatHUDWidget::GetDesiredInputConfig() const {
  return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}

namespace {
void Place(UCanvasPanel *Parent, UWidget *Child, FVector2D Position, FVector2D Size,
           FVector2D Anchor = FVector2D::ZeroVector, FVector2D Alignment = FVector2D::ZeroVector) {
  auto *Slot = Parent->AddChildToCanvas(Child);
  Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
  Slot->SetAlignment(Alignment);
  Slot->SetPosition(Position);
  Slot->SetSize(Size);
}
void Fill(UOverlay *Parent, UWidget *Child) {
  auto *Slot = Parent->AddChildToOverlay(Child);
  Slot->SetHorizontalAlignment(HAlign_Fill);
  Slot->SetVerticalAlignment(VAlign_Fill);
}
}

UCanvasPanel *UDsCombatHUDWidget::MakePanel(UCanvasPanel *Root, FName Name, FVector2D Position,
    FVector2D Size, FVector2D Anchor, FVector2D Alignment, FLinearColor Tint, float Radius) {
  auto *Layers = WidgetTree->ConstructWidget<UOverlay>();
  Place(Root, Layers, Position, Size, Anchor, Alignment);
  auto *Shadow = WidgetTree->ConstructWidget<UImage>();
  if (auto *Material = LoadObject<UMaterialInterface>(nullptr, *DsGameAssets::Path(TEXT("M_UI_Glass")).ToString())) {
    auto *Instance = UMaterialInstanceDynamic::Create(Material, this);
    Instance->SetVectorParameterValue(TEXT("PanelSize"), FLinearColor(Size.X + 24, Size.Y + 24, 0));
    Instance->SetScalarParameterValue(TEXT("CornerRadius"), Radius);
    Instance->SetScalarParameterValue(TEXT("ShadowOnly"), 1.f);
    Shadow->SetBrushFromMaterial(Instance);
    PanelMaterials.Add(Instance);
  }
  Fill(Layers, Shadow);
  if (Name == TEXT("ResultPanel")) Shadow->SetVisibility(ESlateVisibility::Collapsed);
  CastChecked<UOverlaySlot>(Shadow->Slot)->SetPadding(FMargin(-12));
  Shadow->SetRenderTranslation(FVector2D(0, 6));
  auto *Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
  Blur->SetBlurStrength(28.6f);
  Blur->SetCornerRadius(FVector4(Radius, Radius, Radius, Radius));
  Fill(Layers, Blur);
  // 띠는 알파 페이드 재질만 사용해 블러의 직선 경계가 남지 않게 한다.
  if (Name == TEXT("ResultPanel")) Blur->SetVisibility(ESlateVisibility::Collapsed);
  auto *Glass = WidgetTree->ConstructWidget<UImage>();
  if (auto *Material = LoadObject<UMaterialInterface>(nullptr, *DsGameAssets::Path(TEXT("M_UI_Glass")).ToString())) {
    auto *Instance = UMaterialInstanceDynamic::Create(Material, this);
    Instance->SetVectorParameterValue(TEXT("PanelSize"), FLinearColor(Size.X, Size.Y, 0.f));
    Instance->SetVectorParameterValue(TEXT("Tint"), Tint);
    Instance->SetScalarParameterValue(TEXT("CornerRadius"), Radius);
    if (Name == TEXT("ResultPanel")) {
      Instance->SetScalarParameterValue(TEXT("FillOpacity"), .6f);
      Instance->SetScalarParameterValue(TEXT("ResultBand"), 1.f);
    }
    Glass->SetBrushFromMaterial(Instance);
    PanelMaterials.Add(Instance);
  } else Glass->SetColorAndOpacity(FLinearColor(Tint.R, Tint.G, Tint.B, .1f));
  Fill(Layers, Glass);
  auto *Contents = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), Name);
  Fill(Layers, Contents);
  return Contents;
}

UMaterialInstanceDynamic *UDsCombatHUDWidget::MakeGauge(UCanvasPanel *Panel, FName Name,
    FVector2D Position, FVector2D Size, const TCHAR *MaterialPath) {
  auto *Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
  Place(Panel, Image, Position, Size);
  auto *Parent = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
  if (!Parent) return nullptr;
  auto *Instance = UMaterialInstanceDynamic::Create(Parent, this);
  Instance->SetVectorParameterValue(TEXT("PanelSize"), FLinearColor(Size.X, Size.Y, 0.f));
  Instance->SetScalarParameterValue(TEXT("CornerRadius"), Size.Y * .5f);
  Image->SetBrushFromMaterial(Instance);
  return Instance;
}

UTextBlock *UDsCombatHUDWidget::MakeText(UCanvasPanel *Panel, FName Name, const FText &Text,
    FVector2D Position, FVector2D Size, int32 FontSize, ETextJustify::Type Justification) {
  auto *Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
  Label->SetText(Text);
  auto Font = FCoreStyle::GetDefaultFontStyle(TEXT("Light"), FontSize);
  Label->SetFont(Font);
  Label->SetColorAndOpacity(FSlateColor(FLinearColor(.96f, .97f, 1.f)));
  Label->SetShadowOffset(FVector2D(0.f, 1.f));
  Label->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .5f));
  Label->SetJustification(Justification);
  Place(Panel, Label, Position, Size);
  return Label;
}

TSharedRef<SWidget> UDsCombatHUDWidget::RebuildWidget() {
  if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
  if (!WidgetTree->RootWidget || WidgetTree->RootWidget->GetFName() != TEXT("HUDRoot")) {
    PanelMaterials.Reset();
    auto *Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
    WidgetTree->RootWidget = Root;
    LockOnMarker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LockOnMarker"));
    Place(Root, LockOnMarker, FVector2D::ZeroVector, FVector2D(24, 24), FVector2D::ZeroVector, FVector2D(.5f, .5f));
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, *DsGameAssets::Path(TEXT("M_UI_Glass")).ToString())) {
      auto* Instance = UMaterialInstanceDynamic::Create(Material, this);
      Instance->SetVectorParameterValue(TEXT("PanelSize"), FLinearColor(24, 24, 0));
      Instance->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.65f, .12f, .15f));
      Instance->SetScalarParameterValue(TEXT("CornerRadius"), 12.f);
      Instance->SetScalarParameterValue(TEXT("RimStrength"), 4.f);
      Instance->SetScalarParameterValue(TEXT("FillOpacity"), .24f);
      LockOnMarker->SetBrushFromMaterial(Instance);
      PanelMaterials.Add(Instance);
    }
    LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    // 엔진 기본 ShortestSide DPI 곡선: 1080p=1, 720p≈2/3.
    auto *PlayerPanel = MakePanel(Root, TEXT("PlayerPanel"), FVector2D(40, 40), FVector2D(420, 76),
        FVector2D(0, 0), FVector2D(0, 0), FLinearColor(.85f, .9f, 1.f), 38.f);
    HealthMaterial = MakeGauge(PlayerPanel, TEXT("HealthGauge"), FVector2D(24, 22), FVector2D(372, 12),
        *DsGameAssets::Path(TEXT("MI_UI_Health")).ToString());
    StaminaMaterial = MakeGauge(PlayerPanel, TEXT("StaminaGauge"), FVector2D(24, 46), FVector2D(372, 8),
        *DsGameAssets::Path(TEXT("MI_UI_Stamina")).ToString());
    BossPanel = MakePanel(Root, TEXT("BossPanel"), FVector2D(0, -42), FVector2D(800, 96),
        FVector2D(.5f, 1.f), FVector2D(.5f, 1.f), FLinearColor(.7f, .3f, .35f), 48.f);
    BossNameText = MakeText(BossPanel, TEXT("BossName"), FText::GetEmpty(), FVector2D(24, 14), FVector2D(752, 32), 21, ETextJustify::Center);
    BossMaterial = MakeGauge(BossPanel, TEXT("BossGauge"), FVector2D(28, 60), FVector2D(744, 16),
        *DsGameAssets::Path(TEXT("MI_UI_Boss")).ToString());
    ResultPanel = MakePanel(Root, TEXT("ResultPanel"), FVector2D(0, 0), FVector2D(1920, 180),
        FVector2D(.5f, .5f), FVector2D(.5f, .5f), FLinearColor::Black, 0.f);
    auto *ResultSlot = CastChecked<UCanvasPanelSlot>(ResultPanel->GetParent()->Slot);
    ResultSlot->SetAnchors(FAnchors(0, .5f, 1, .5f));
    ResultSlot->SetOffsets(FMargin(0, 0, 0, 180));
    ResultTitle = MakeText(ResultPanel, TEXT("ResultTitle"), FText::GetEmpty(), FVector2D(24, 12), FVector2D(1872, 112), 72, ETextJustify::Center);
    auto Font = ResultTitle->GetFont();
    Font.LetterSpacing = 180;
    Font.OutlineSettings = FFontOutlineSettings(2, FLinearColor(.25f, .005f, .005f, .3f));
    ResultTitle->SetFont(Font);
    ResultTitle->SetShadowOffset(FVector2D(0, 0));
    ResultSubtitle = MakeText(ResultPanel, TEXT("ResultSubtitle"), FText::GetEmpty(), FVector2D(24, 138), FVector2D(1872, 32), 16, ETextJustify::Center);
    ResultSubtitle->SetColorAndOpacity(FSlateColor(FLinearColor(1, 1, 1, .8f)));
    for (auto *Text : {ResultTitle.Get(), ResultSubtitle.Get()}) {
      auto *TextSlot = CastChecked<UCanvasPanelSlot>(Text->Slot);
      TextSlot->SetAnchors(FAnchors(0, 0, 1, 0));
      const auto Offsets = TextSlot->GetOffsets();
      TextSlot->SetOffsets(FMargin(24, Offsets.Top, 24, Offsets.Bottom));
    }
  }
  return Super::RebuildWidget();
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
  Subscribe();
  Refresh();
}

void UDsCombatHUDWidget::NativeDestruct() {
  if (ViewModel) ViewModel->RemoveAllFieldValueChangedDelegates(this);
  if (GetWorld()) {
    GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
    GetWorld()->GetTimerManager().ClearTimer(DamageTrailTimer);
  }
  bResultShown = false;
  Super::NativeDestruct();
}

void UDsCombatHUDWidget::Refresh() {
  if (!ViewModel || !ResultPanel) return;
  if (HealthMaterial) HealthMaterial->SetScalarParameterValue(TEXT("FillRate"), ViewModel->GetPlayerHealthRate());
  if (StaminaMaterial) StaminaMaterial->SetScalarParameterValue(TEXT("FillRate"), ViewModel->GetPlayerStaminaRate());
  if (BossMaterial) BossMaterial->SetScalarParameterValue(TEXT("FillRate"), ViewModel->BossHealthRate);
  if (GetWorld()) {
    const double Now = GetWorld()->GetTimeSeconds();
    HealthTrail.SetTarget(ViewModel->GetPlayerHealthRate(), Now);
    BossTrail.SetTarget(ViewModel->BossHealthRate, Now);
    AdvanceDamageTrails();
    if ((HealthTrail.bActive || BossTrail.bActive) && !GetWorld()->GetTimerManager().IsTimerActive(DamageTrailTimer))
      GetWorld()->GetTimerManager().SetTimer(DamageTrailTimer, this, &ThisClass::AdvanceDamageTrails, .02f, true);
  }
  // 컨텐츠 패널의 부모가 블러와 유리까지 포함하는 실제 패널이다.
  BossPanel->GetParent()->SetVisibility(ViewModel->BossVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  BossNameText->SetText(ViewModel->BossDisplayName);
  const bool bShow = ViewModel->IsPlayerDead() || ViewModel->IsVictory();
  ResultPanel->GetParent()->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
  if (bShow) {
    const bool bDead = ViewModel->IsPlayerDead();
    ResultTitle->SetText(bDead ? FText::FromString(TEXT("YOU DIED")) : NSLOCTEXT("DrHUD", "Victory", "적을 쓰러뜨렸다"));
    ResultTitle->SetColorAndOpacity(FSlateColor(bDead ? FLinearColor::FromSRGBColor(FColor(176, 30, 30)) : FLinearColor(1.f, .78f, .32f)));
    auto Font = ResultTitle->GetFont();
    Font.OutlineSettings.OutlineColor = bDead ? FLinearColor(.25f, .005f, .005f, .3f) : FLinearColor(.3f, .18f, .02f, .3f);
    ResultTitle->SetFont(Font);
    ResultSubtitle->SetText(bDead ? NSLOCTEXT("DrHUD", "Retry", "곧 체크포인트에서 다시 시작합니다") : FText::GetEmpty());
  }
  if (bShow && !bResultShown && GetWorld()) {
    FadeStart = GetWorld()->GetTimeSeconds();
    ResultPanel->GetParent()->SetRenderOpacity(0.f);
    GetWorld()->GetTimerManager().SetTimer(FadeTimer, this, &ThisClass::AdvanceFade, .02f, true);
  } else if (!bShow && GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
  bResultShown = bShow;
}

void UDsCombatHUDWidget::AdvanceFade() {
  if (!GetWorld() || !ResultPanel) return;
  const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - FadeStart) / .65), 0.f, 1.f);
  ResultPanel->GetParent()->SetRenderOpacity(Alpha);
  if (Alpha >= 1.f) GetWorld()->GetTimerManager().ClearTimer(FadeTimer);
}

void UDsCombatHUDWidget::AdvanceDamageTrails() {
  if (!GetWorld()) return;
  const double Now = GetWorld()->GetTimeSeconds();
  HealthTrail.Advance(Now);
  BossTrail.Advance(Now);
  if (HealthMaterial) HealthMaterial->SetScalarParameterValue(TEXT("DelayedRate"), HealthTrail.Value);
  if (BossMaterial) BossMaterial->SetScalarParameterValue(TEXT("DelayedRate"), BossTrail.Value);
  if (!HealthTrail.bActive && !BossTrail.bActive) GetWorld()->GetTimerManager().ClearTimer(DamageTrailTimer);
}
