#include "UI/DsSettingsMenuWidget.h"

#include "UI/DsGlassButton.h"
#include "Audio/DsAudioSubsystem.h"
#include "Game/DsGameUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void UDsSettingsMenuWidget::BuildMenu(UVerticalBox* Contents) {
  Sliders.Reset(); AddHeading(Contents, TEXT("Settings"));
  const TCHAR* Keys[] = {TEXT("SFX"), TEXT("Voice"), TEXT("Music")};
  auto* Settings = UDsGameUserSettings::Get();
  for (int32 I = 0; I < 3; ++I) {
    auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(MenuText(Keys[I])); Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 22));
    ApplyItemSpacing(Contents->AddChildToVerticalBox(Text));
    auto* Slider = WidgetTree->ConstructWidget<USlider>();
    Slider->SetStepSize(.05f);
    Slider->SetValue(Settings ? Settings->GetVolume(static_cast<EDsAudioChannel>(I)) : 1.f);
    auto* LayoutSlot = Contents->AddChildToVerticalBox(Slider);
    LayoutSlot->SetPadding(FMargin(48, 6, 48, 18));
    Sliders.Add(Slider);
  }
  Sliders[0]->OnValueChanged.AddDynamic(this, &ThisClass::SFXChanged);
  Sliders[1]->OnValueChanged.AddDynamic(this, &ThisClass::VoiceChanged);
  Sliders[2]->OnValueChanged.AddDynamic(this, &ThisClass::MusicChanged);
  AddButton(Contents, TEXT("Back"))->OnClicked().AddWeakLambda(this, [this] { NativeOnHandleBackAction(); });
  FirstFocus = Sliders[0];
}
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
