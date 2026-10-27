#include "UI/DsMenuWidget.h"
#include "TableData/DsGameAssets.h"

#include "UI/DsGlassButton.h"
#include "Game/DsPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/CoreStyle.h"
#include "InputMappingContext.h"
#include "Internationalization/StringTable.h"

FText UDsMenuWidget::MenuText(const TCHAR* Key) {
  // 첫 타이틀 프레임에도 문구가 보이도록 비동기 요청 전에 표를 확실히 로드한다.
  auto* Table = LoadObject<UStringTable>(nullptr, *DsGameAssets::Path(TEXT("ST_UI")).ToString());
  return FText::FromStringTable(Table ? Table->GetStringTableId() : NAME_None, Key);
}
void UDsMenuWidget::ApplyItemSpacing(UVerticalBoxSlot* LayoutSlot) {
  LayoutSlot->SetPadding(FMargin(32, 10));
  LayoutSlot->SetHorizontalAlignment(HAlign_Center);
}
UDsMenuWidget::UDsMenuWidget() { bIsBackHandler = true; }
void UDsMenuWidget::NativeOnActivated() {
  InputMapping = LoadObject<UInputMappingContext>(nullptr, *DsGameAssets::Path(TEXT("IMC_UI")).ToString());
  InputMappingPriority = 200;
  Super::NativeOnActivated();
}
TOptional<FUIInputConfig> UDsMenuWidget::GetDesiredInputConfig() const {
  return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
TSharedRef<SWidget> UDsMenuWidget::RebuildWidget() {
  FirstFocus = nullptr;
  auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
  WidgetTree->RootWidget = Root;
  auto* Dim = WidgetTree->ConstructWidget<UImage>();
  Dim->SetColorAndOpacity(FLinearColor(0, 0, 0, .25f));
  auto* DimSlot = Root->AddChildToCanvas(Dim);
  DimSlot->SetAnchors(FAnchors(0, 0, 1, 1)); DimSlot->SetOffsets(FMargin(0));
  auto* Layers = WidgetTree->ConstructWidget<UOverlay>();
  auto* PanelSlot = Root->AddChildToCanvas(Layers);
  PanelSlot->SetAnchors(FAnchors(.5f)); PanelSlot->SetAlignment(FVector2D(.5f));
  PanelSlot->SetAutoSize(true);
  auto Fill = [Layers](UWidget* Child) {
    auto* LayoutSlot = Layers->AddChildToOverlay(Child);
    LayoutSlot->SetHorizontalAlignment(HAlign_Fill); LayoutSlot->SetVerticalAlignment(VAlign_Fill);
  };
  auto* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
  Blur->SetBlurStrength(28.f); Blur->SetCornerRadius(FVector4(36)); Fill(Blur);
  auto* Glass = WidgetTree->ConstructWidget<UImage>();
  if (auto* Parent = LoadObject<UMaterialInterface>(nullptr, *DsGameAssets::Path(TEXT("M_UI_Glass")).ToString())) {
    auto* MID = UMaterialInstanceDynamic::Create(Parent, this);
    MID->SetVectorParameterValue(TEXT("PanelSize"), FLinearColor(464, 500, 0));
    MID->SetScalarParameterValue(TEXT("CornerRadius"), 36.f);
    MID->SetScalarParameterValue(TEXT("FillOpacity"), .24f);
    Glass->SetBrushFromMaterial(MID);
  }
  Fill(Glass);
  auto* Contents = WidgetTree->ConstructWidget<UVerticalBox>();
  Fill(Contents);
  BuildMenu(Contents);
  return UCommonActivatableWidget::RebuildWidget();
}
void UDsMenuWidget::AddHeading(UVerticalBox* Contents, const TCHAR* Key) {
  auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
  Text->SetText(MenuText(Key)); Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Light"), 38));
  auto* LayoutSlot = Contents->AddChildToVerticalBox(Text);
  LayoutSlot->SetPadding(FMargin(32, 30, 32, 20)); LayoutSlot->SetHorizontalAlignment(HAlign_Center);
}
UDsGlassButton* UDsMenuWidget::AddButton(UVerticalBox* Contents, const TCHAR* Key) {
  // 위젯 트리로 만들어야 플레이어가 없는 썸네일 렌더링에서도 버튼이 생긴다.
  auto* Button = WidgetTree->ConstructWidget<UDsGlassButton>(UDsGlassButton::StaticClass());
  Button->SetLabel(MenuText(Key)); ApplyItemSpacing(Contents->AddChildToVerticalBox(Button));
  if (!FirstFocus) FirstFocus = Button;
  return Button;
}
UWidget* UDsMenuWidget::NativeGetDesiredFocusTarget() const { return FirstFocus; }
bool UDsMenuWidget::NativeOnHandleBackAction() {
  if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->BackMenu();
  return true;
}
FReply UDsMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) {
  if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_Special_Right) {
    if (!Event.IsRepeat()) NativeOnHandleBackAction();
    return FReply::Handled();
  }
  return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
