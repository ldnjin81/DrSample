#include "UI/DsGlassButton.h"
#include "TableData/DsGameAssets.h"

#include "UI/DsGlassButtonStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"

UDsGlassButton::UDsGlassButton() { Style = UDsGlassButtonStyle::StaticClass(); }

void UDsGlassButton::SetLabel(const FText& InLabel) {
  Label = InLabel;
  if (Text) Text->SetText(Label);
}
void UDsGlassButton::NativeOnInitialized() {
  Super::NativeOnInitialized();
  auto* Box = WidgetTree->ConstructWidget<USizeBox>();
  Box->SetWidthOverride(400.f);
  Box->SetHeightOverride(64.f);
  auto* Layers = WidgetTree->ConstructWidget<UOverlay>();
  Box->AddChild(Layers);
  auto Fill = [Layers](UWidget* Child) {
    auto* LayoutSlot = Layers->AddChildToOverlay(Child);
    LayoutSlot->SetHorizontalAlignment(HAlign_Fill);
    LayoutSlot->SetVerticalAlignment(VAlign_Fill);
  };
  auto* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
  Blur->SetBlurStrength(24.f);
  Blur->SetCornerRadius(FVector4(32));
  Fill(Blur);
  auto* Image = WidgetTree->ConstructWidget<UImage>();
  if (auto* Material = LoadObject<UMaterialInterface>(nullptr, *DsGameAssets::Path(TEXT("M_UI_Glass")).ToString())) {
    Glass = UMaterialInstanceDynamic::Create(Material, this);
    Glass->SetVectorParameterValue(TEXT("PanelSize"), FLinearColor(400, 64, 0));
    Glass->SetScalarParameterValue(TEXT("CornerRadius"), 32.f);
    Image->SetBrushFromMaterial(Glass);
  }
  Fill(Image);
  Text = WidgetTree->ConstructWidget<UTextBlock>();
  Text->SetText(Label);
  Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 23));
  auto* TextSlot = Layers->AddChildToOverlay(Text);
  TextSlot->SetHorizontalAlignment(HAlign_Center);
  TextSlot->SetVerticalAlignment(VAlign_Center);
  WidgetTree->RootWidget = Box;
  SetRenderTransformPivot(FVector2D(.5f));
  RefreshGlass();
}
void UDsGlassButton::RefreshGlass() {
  const bool bLit = bFocused || IsHovered();
  if (Glass) {
    Glass->SetScalarParameterValue(TEXT("RimStrength"), bLit ? 3.f : 1.f);
    Glass->SetScalarParameterValue(TEXT("FillOpacity"), bLit ? .28f : .15f);
  }
  SetRenderScale(FVector2D(bLit ? 1.025f : 1.f));
}
void UDsGlassButton::NativeOnHovered() { Super::NativeOnHovered(); RefreshGlass(); }
void UDsGlassButton::NativeOnUnhovered() { Super::NativeOnUnhovered(); RefreshGlass(); }
void UDsGlassButton::NativeOnAddedToFocusPath(const FFocusEvent& Event) {
  Super::NativeOnAddedToFocusPath(Event); bFocused = true; RefreshGlass();
}
void UDsGlassButton::NativeOnRemovedFromFocusPath(const FFocusEvent& Event) {
  Super::NativeOnRemovedFromFocusPath(Event); bFocused = false; RefreshGlass();
}
void UDsGlassButton::NativeOnClicked() {
  if (auto* Sound = LoadObject<USoundBase>(nullptr, *DsGameAssets::Path(TEXT("S_UI_Click")).ToString()))
    UGameplayStatics::PlaySound2D(this, Sound);
  Super::NativeOnClicked();
}
