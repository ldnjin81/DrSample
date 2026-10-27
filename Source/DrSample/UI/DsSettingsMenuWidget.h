#pragma once

#include "CoreMinimal.h"
#include "UI/DsMenuWidget.h"
#include "Audio/DsAudioData.h"
#include "DsSettingsMenuWidget.generated.h"

class USlider;

/** 설정 메뉴. 채널별 음량 슬라이더를 저장하고 즉시 반영한다. */
UCLASS()
class DRSAMPLE_API UDsSettingsMenuWidget : public UDsMenuWidget {
  GENERATED_BODY()
public:
  void SetVolume(EDsAudioChannel Channel, float Value);
  USlider* GetSlider(EDsAudioChannel Channel) const;
protected:
  virtual void BuildMenu(UVerticalBox* Contents) override;
private:
  UFUNCTION() void SFXChanged(float Value);
  UFUNCTION() void VoiceChanged(float Value);
  UFUNCTION() void MusicChanged(float Value);
  UPROPERTY(Transient) TArray<TObjectPtr<USlider>> Sliders;
};
