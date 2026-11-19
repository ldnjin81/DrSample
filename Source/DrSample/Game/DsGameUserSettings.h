#pragma once

#include "CoreMinimal.h"
#include "Game/DrGameUserSettings.h"
#include "Audio/DsAudioData.h"
#include "DsGameUserSettings.generated.h"

/** 월드 전환과 다음 실행에도 유지되는 음량·UI 테마 설정이다. */
UCLASS(Config=GameUserSettings)
class DRSAMPLE_API UDsGameUserSettings : public UDrGameUserSettings {
  GENERATED_BODY()
public:
  static UDsGameUserSettings* Get();
  float GetVolume(EDsAudioChannel Channel) const;
  void SetVolume(EDsAudioChannel Channel, float Value);
  virtual void SetToDefaults() override;
  /** 사용자가 고른 DrUIKit 테마 id(<스타일>-<밝기>). 비어 있으면 프로젝트 설정의 시작 테마를 쓴다. */
  FName GetUITheme() const { return UITheme; }
  void SetUITheme(FName ThemeId) { UITheme = ThemeId; }
  /** 저장된 테마를 적용한다. 스타일 플러그인은 엔진이 뜬 뒤에 들어오므로 그 뒤에 부른다(DrSample 모듈 시작). */
  void ApplyUITheme() const;
private:
  UPROPERTY(Config) float SFXVolume = 1.f;
  UPROPERTY(Config) float VoiceVolume = 1.f;
  UPROPERTY(Config) float MusicVolume = 1.f;
  UPROPERTY(Config) FName UITheme;
};
