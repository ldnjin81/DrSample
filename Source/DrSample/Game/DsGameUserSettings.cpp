#include "Game/DsGameUserSettings.h"

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
}
