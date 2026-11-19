#include "UI/DsMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Theme/DrUITheme.h"
#include "Theme/DrUIThemeSubsystem.h"
#include "Widgets/DrActionBar.h"
#include "Widgets/DrButton.h"
#include "Widgets/DrSurface.h"
#include "Widgets/DrText.h"

#define LOCTEXT_NAMESPACE "DsMenu"

TSharedRef<SWidget> UDsMenuWidget::RebuildWidget() {
  FirstFocus = nullptr;
  auto* Root = WidgetTree->ConstructWidget<UOverlay>();
  WidgetTree->RootWidget = Root;
  Scrim = WidgetTree->ConstructWidget<UBorder>();
  ApplyScrim();
  if (auto* S = Root->AddChildToOverlay(Scrim)) { S->SetHorizontalAlignment(HAlign_Fill); S->SetVerticalAlignment(VAlign_Fill); }

  auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
  if (auto* S = Root->AddChildToOverlay(Column)) { S->SetHorizontalAlignment(HAlign_Center); S->SetVerticalAlignment(VAlign_Center); }
  auto* Size = WidgetTree->ConstructWidget<USizeBox>();
  Size->SetWidthOverride(GetPanelWidth());
  auto* Panel = WidgetTree->ConstructWidget<UDrSurface>();
  Panel->RadiusToken = TEXT("radius-xl");
  Panel->PaddingToken = TEXT("space-6");
  Panel->SetElevation(DrUITokens::ShadowGlass);
  Size->AddChild(Panel);
  Column->AddChildToVerticalBox(Size);
  auto* Contents = WidgetTree->ConstructWidget<UVerticalBox>();
  Panel->SetContent(Contents);
  BuildMenu(Contents);

  // 아래 안내: 포커스한 부품의 행동(확인 등)과 메뉴 공통 행동(뒤로)을 키보드·패드 모양으로 보인다.
  // 뒤로가 없는 메뉴(타이틀)는 안내할 공통 행동이 없어 바를 만들지 않는다.
  ActionBar = nullptr;
  if (HasBackAction()) {
    ActionBar = WidgetTree->ConstructWidget<UDrActionBar>();
    ActionBar->bSurface = false;
    ActionBar->SetBaseHints({{TEXT("Back"), EKeys::Gamepad_FaceButton_Right, EKeys::Escape, LOCTEXT("HintBack", "뒤로")}});
    if (auto* S = Column->AddChildToVerticalBox(ActionBar)) { S->SetPadding(FMargin(0, 16, 0, 0)); S->SetHorizontalAlignment(HAlign_Center); }
  }
  return Super::RebuildWidget();
}

void UDsMenuWidget::NativeConstruct() {
  Super::NativeConstruct();
  if (auto* Themes = UDrUIThemeSubsystem::Get(); Themes && !ThemeHandle.IsValid())
    ThemeHandle = Themes->OnThemeChanged.AddWeakLambda(this, [this](const UDrUITheme&) { ApplyScrim(); });
}

void UDsMenuWidget::NativeDestruct() {
  if (auto* Themes = UDrUIThemeSubsystem::Get()) Themes->OnThemeChanged.Remove(ThemeHandle);
  ThemeHandle.Reset();
  Super::NativeDestruct();
}

void UDsMenuWidget::ApplyScrim() {
  if (Scrim) Scrim->SetBrushColor(UDrUIThemeSubsystem::Current().GetColor(TEXT("scrim")));
}

void UDsMenuWidget::AddHeading(UVerticalBox* Contents, const FText& Text) {
  auto* Title = WidgetTree->ConstructWidget<UDrText>();
  Title->SetTokens(TEXT("display"), DrUITokens::Ink);
  Title->SetText(Text);
  if (auto* S = Contents->AddChildToVerticalBox(Title)) { S->SetPadding(FMargin(0, 4, 0, 20)); S->SetHorizontalAlignment(HAlign_Center); }
}

UDrButton* UDsMenuWidget::AddButton(UVerticalBox* Contents, const FText& Label) {
  // 위젯 트리로 만들어야 플레이어가 없는 썸네일 렌더링에서도 버튼이 생긴다.
  auto* Button = WidgetTree->ConstructWidget<UDrButton>();
  Button->Label = Label;
  Button->Variant = FirstFocus ? EDrButtonVariant::Secondary : EDrButtonVariant::Primary;
  if (auto* S = Contents->AddChildToVerticalBox(Button)) { S->SetPadding(FMargin(0, 6)); S->SetHorizontalAlignment(HAlign_Fill); }
  if (!FirstFocus) FirstFocus = Button;
  return Button;
}

#undef LOCTEXT_NAMESPACE
