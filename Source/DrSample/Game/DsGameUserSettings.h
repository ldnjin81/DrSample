#pragma once

#include "CoreMinimal.h"
#include "Game/DrGameUserSettings.h"
#include "Audio/DsAudioData.h"
#include "DsGameUserSettings.generated.h"

/** 월드 전환과 다음 실행에도 유지되는 음량 설정이다. */
UCLASS(Config=GameUserSettings)
class DRSAMPLE_API UDsGameUserSettings : public UDrGameUserSettings {
  GENERATED_BODY()
public:
  static UDsGameUserSettings* Get();
  float GetVolume(EDsAudioChannel Channel) const;
  void SetVolume(EDsAudioChannel Channel, float Value);
  virtual void SetToDefaults() override;
private:
  UPROPERTY(Config) float SFXVolume = 1.f;
  UPROPERTY(Config) float VoiceVolume = 1.f;
  UPROPERTY(Config) float MusicVolume = 1.f;
};
