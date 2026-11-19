#include "Game/DsGameUserSettings.h"
#include "Theme/DrUIThemeSubsystem.h"

UDsGameUserSettings* UDsGameUserSettings::Get() {
  return Cast<UDsGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}
float UDsGameUserSettings::GetVolume(EDsAudioChannel Channel) const {
  const float Value = Channel == EDsAudioChannel::SFX ? SFXVolume :
      Channel == EDsAudioChannel::Voice ? VoiceVolume : MusicVolume;
  return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 1.f;
}
void UDsGameUserSettings::SetVolume(EDsAudioChannel Channel, float Value) {
  Value = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 1.f;
  switch (Channel) {
  case EDsAudioChannel::SFX: SFXVolume = Value; break;
  case EDsAudioChannel::Voice: VoiceVolume = Value; break;
  case EDsAudioChannel::Music: MusicVolume = Value; break;
  default: return;
  }
}
void UDsGameUserSettings::SetToDefaults() {
  Super::SetToDefaults();
  SFXVolume = VoiceVolume = MusicVolume = 1.f;
  UITheme = NAME_None;
}
void UDsGameUserSettings::ApplyUITheme() const {
  // 없어진 스타일(플러그인을 끈 경우)이면 SetTheme이 false를 돌려주고 시작 테마가 그대로 남는다.
  if (auto* Themes = UDrUIThemeSubsystem::Get(); Themes && !UITheme.IsNone()) Themes->SetTheme(UITheme);
}
