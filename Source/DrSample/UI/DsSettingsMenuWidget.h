#pragma once

#include "CoreMinimal.h"
#include "UI/DsMenuWidget.h"
#include "Audio/DsAudioData.h"
#include "DsSettingsMenuWidget.generated.h"

class UDrHorizontalSelector;
class USlider;

/**
 * 설정 메뉴. UI 스타일(DrUIKit 스타일 플러그인)·밝기와 채널별 음량을 옵션 줄(UDrOptionRow)로 고르고,
 * 바로 반영하며 사용자 설정(UDsGameUserSettings)에 저장한다.
 */
UCLASS()
class DRSAMPLE_API UDsSettingsMenuWidget : public UDsMenuWidget {
  GENERATED_BODY()
public:
  void SetVolume(EDsAudioChannel Channel, float Value);
  USlider* GetSlider(EDsAudioChannel Channel) const;
  /** 불러온 스타일 목록의 StyleIndex번째 스타일을 밝기(0 밝게, 1 어둡게)로 적용·저장한다. 테마가 바뀌었으면 true. */
  bool SelectTheme(int32 StyleIndex, int32 ModeIndex);
  /** 스타일의 테마 id 목록에서 밝기에 맞는 것(<스타일>-light/-dark). 없으면 첫 테마. */
  static FName PickThemeId(const TArray<FName>& ThemeIds, FName StyleId, int32 ModeIndex);
protected:
  virtual void BuildMenu(UVerticalBox* Contents) override;
  virtual float GetPanelWidth() const override { return 640.f; }
private:
  UFUNCTION() void SFXChanged(float Value);
  UFUNCTION() void VoiceChanged(float Value);
  UFUNCTION() void MusicChanged(float Value);
  UFUNCTION() void StyleChanged(int32 Index);
  UFUNCTION() void ModeChanged(int32 Index);
  UPROPERTY(Transient) TArray<TObjectPtr<USlider>> Sliders;
  UPROPERTY(Transient) TObjectPtr<UDrHorizontalSelector> StyleSelector;
  UPROPERTY(Transient) TObjectPtr<UDrHorizontalSelector> ModeSelector;
};
