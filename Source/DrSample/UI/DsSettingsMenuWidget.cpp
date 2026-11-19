#include "UI/DsSettingsMenuWidget.h"

#include "Audio/DsAudioSubsystem.h"
#include "Game/DsGameUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Theme/DrUIStyle.h"
#include "Theme/DrUITheme.h"
#include "Theme/DrUIThemeSubsystem.h"
#include "Widgets/DrButton.h"
#include "Widgets/DrInput.h"
#include "Widgets/DrOptionRow.h"
#include "Widgets/DrSlider.h"

#define LOCTEXT_NAMESPACE "DsMenu"

namespace {
// 밝기 선택기 순서와 tokens.json 테마 id 끝부분.
const TCHAR* const ModeSuffixes[] = {TEXT("light"), TEXT("dark")};

int32 FindStyleIndex(FName StyleId) {
  const auto& Styles = DrUIStyle::GetLoadedStyles();
  return Styles.IndexOfByPredicate([StyleId](const FDrUIStyleInfo& Info) { return Info.Id == StyleId; });
}
}

FName UDsSettingsMenuWidget::PickThemeId(const TArray<FName>& ThemeIds, FName StyleId, int32 ModeIndex) {
  if (ThemeIds.IsEmpty()) return NAME_None;
  const FName Wanted(FString::Printf(TEXT("%s-%s"), *StyleId.ToString(), ModeSuffixes[FMath::Clamp(ModeIndex, 0, 1)]));
  return ThemeIds.Contains(Wanted) ? Wanted : ThemeIds[0];
}

void UDsSettingsMenuWidget::BuildMenu(UVerticalBox* Contents) {
  Sliders.Reset();
  AddHeading(Contents, LOCTEXT("SettingsTitle", "설정"));
  UWidget* FirstRow = nullptr;
  auto AddRow = [&](const FText& Label, UWidget* Control) {
    auto* Row = WidgetTree->ConstructWidget<UDrOptionRow>();
    Row->Label = Label;
    Row->SetControl(Control);
    if (auto* S = Contents->AddChildToVerticalBox(Row)) S->SetPadding(FMargin(0, 2));
    if (!FirstRow) FirstRow = Row;
  };

  // 스타일·밝기: DrUIKit이 불러온 스타일 플러그인 목록에서 고른다. 스타일이 하나도 없으면(스타일 플러그인을 모두 끈 경우) 줄을 만들지 않는다.
  const auto& Styles = DrUIStyle::GetLoadedStyles();
  if (!Styles.IsEmpty()) {
    const UDrUITheme& Current = UDrUIThemeSubsystem::Current();
    TArray<FText> Names;
    for (const FDrUIStyleInfo& Info : Styles) Names.Add(Info.Name);
    StyleSelector = WidgetTree->ConstructWidget<UDrHorizontalSelector>();
    StyleSelector->SetOptions(Names);
    StyleSelector->SetSelectedIndex(FMath::Max(0, FindStyleIndex(Current.StyleId)));
    StyleSelector->OnSelectionChanged.AddDynamic(this, &ThisClass::StyleChanged);
    AddRow(LOCTEXT("Style", "UI 스타일"), StyleSelector);
    ModeSelector = WidgetTree->ConstructWidget<UDrHorizontalSelector>();
    ModeSelector->SetOptions({LOCTEXT("Light", "밝게"), LOCTEXT("Dark", "어둡게")});
    ModeSelector->SetSelectedIndex(Current.ThemeId.ToString().EndsWith(TEXT("-light")) ? 0 : 1);
    ModeSelector->OnSelectionChanged.AddDynamic(this, &ThisClass::ModeChanged);
    AddRow(LOCTEXT("Mode", "밝기"), ModeSelector);
  }

  const FText Labels[] = {LOCTEXT("SFX", "효과음"), LOCTEXT("Voice", "음성"), LOCTEXT("Music", "음악")};
  auto* Settings = UDsGameUserSettings::Get();
  for (int32 I = 0; I < UE_ARRAY_COUNT(Labels); ++I) {
    auto* Slider = WidgetTree->ConstructWidget<UDrSlider>();
    Slider->SetStepSize(.05f);
    Slider->SetValue(Settings ? Settings->GetVolume(static_cast<EDsAudioChannel>(I)) : 1.f);
    AddRow(Labels[I], Slider);
    Sliders.Add(Slider);
  }
  Sliders[0]->OnValueChanged.AddDynamic(this, &ThisClass::SFXChanged);
  Sliders[1]->OnValueChanged.AddDynamic(this, &ThisClass::VoiceChanged);
  Sliders[2]->OnValueChanged.AddDynamic(this, &ThisClass::MusicChanged);
  auto* Back = AddButton(Contents, LOCTEXT("Back", "돌아가기"));
  Back->Variant = EDrButtonVariant::Secondary;
  Back->OnClicked().AddWeakLambda(this, [this] { NativeOnHandleBackAction(); });
  FirstFocus = FirstRow;
}

bool UDsSettingsMenuWidget::SelectTheme(int32 StyleIndex, int32 ModeIndex) {
  const auto& Styles = DrUIStyle::GetLoadedStyles();
  auto* Themes = UDrUIThemeSubsystem::Get();
  if (!Themes || !Styles.IsValidIndex(StyleIndex)) return false;
  const FName ThemeId = PickThemeId(Styles[StyleIndex].ThemeIds, Styles[StyleIndex].Id, ModeIndex);
  if (ThemeId.IsNone() || !Themes->SetTheme(ThemeId)) return false;
  if (auto* Settings = UDsGameUserSettings::Get()) { Settings->SetUITheme(ThemeId); Settings->SaveSettings(); }
  return true;
}

void UDsSettingsMenuWidget::StyleChanged(int32 Index) { SelectTheme(Index, ModeSelector ? ModeSelector->GetSelectedIndex() : 1); }
void UDsSettingsMenuWidget::ModeChanged(int32 Index) { SelectTheme(StyleSelector ? StyleSelector->GetSelectedIndex() : 0, Index); }

void UDsSettingsMenuWidget::SetVolume(EDsAudioChannel Channel, float Value) {
  if (auto* Settings = UDsGameUserSettings::Get()) {
    Settings->SetVolume(Channel, Value); Settings->SaveSettings();
    Value = Settings->GetVolume(Channel);
  }
  if (auto* Audio = GetWorld()->GetSubsystem<UDsAudioSubsystem>()) Audio->SetChannelVolume(Channel, Value);
}
USlider* UDsSettingsMenuWidget::GetSlider(EDsAudioChannel Channel) const {
  const int32 Index = static_cast<int32>(Channel);
  return Sliders.IsValidIndex(Index) ? Sliders[Index].Get() : nullptr;
}
void UDsSettingsMenuWidget::SFXChanged(float Value) { SetVolume(EDsAudioChannel::SFX, Value); }
void UDsSettingsMenuWidget::VoiceChanged(float Value) { SetVolume(EDsAudioChannel::Voice, Value); }
void UDsSettingsMenuWidget::MusicChanged(float Value) { SetVolume(EDsAudioChannel::Music, Value); }

#undef LOCTEXT_NAMESPACE
