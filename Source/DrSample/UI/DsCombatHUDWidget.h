#pragma once

#include "CoreMinimal.h"
#include "Flow/DrUIScreens.h"
#include "UI/DsHUDDamageTrail.h"
#include "DsCombatHUDWidget.generated.h"

class UDsCombatHUDViewModel;
class UBorder;
class UCanvasPanel;
class UDrProgressBar;
class UDrSurface;
class UDrText;
class UImage;
class UTextBlock;
class UDrUITheme;
class UPanelWidget;

/**
 * 전투 HUD: 플레이어 체력·기력, 보스 이름·체력, 결과 띠, 락온 표시.
 * 표면·게이지·글자는 DrUIKit 부품과 테마 토큰으로 칠하므로 UI 스타일을 바꾸면 HUD도 같이 바뀐다.
 * 게이지는 진행 막대 두 개를 겹쳐, 아래 막대가 피해 잔상(FDsHUDDamageTrail)을 보인다.
 */
UCLASS()
class DRSAMPLE_API UDsCombatHUDWidget : public UDrUIHUDWidget {
  GENERATED_BODY()
public:
  virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
  void SetViewModel(UDsCombatHUDViewModel *InViewModel);
  /** 검증용: 게이지가 보이는 값(채움, 잔상). */
  float GetHealthFill() const;
  float GetHealthTrail() const;
  float GetBossFill() const;
  bool IsBossPanelVisible() const;
  bool IsResultVisible() const;
  FText GetResultTitle() const;
protected:
  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;
  virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
  struct FGauge { TObjectPtr<UDrProgressBar> Trail; TObjectPtr<UDrProgressBar> Fill; };
  UDrSurface* MakePanel(UCanvasPanel* Root, FVector2D Position, FVector2D Size, FVector2D Anchor, FVector2D Alignment);
  FGauge MakeGauge(UPanelWidget* Parent, FName FillToken, float Thickness);
  void Subscribe();
  void Refresh();
  void AdvanceFade();
  void AdvanceDamageTrails();
  void ApplyTheme(const UDrUITheme& Theme);
  UPROPERTY(Transient) TObjectPtr<UDsCombatHUDViewModel> ViewModel;
  UPROPERTY(Transient) TObjectPtr<UImage> LockOnMarker;
  UPROPERTY(Transient) TObjectPtr<UDrSurface> BossPanel;
  UPROPERTY(Transient) TObjectPtr<UBorder> ResultPanel;
  UPROPERTY(Transient) TObjectPtr<UDrText> BossNameText;
  UPROPERTY(Transient) TObjectPtr<UTextBlock> ResultTitle;
  UPROPERTY(Transient) TObjectPtr<UDrText> ResultSubtitle;
  UPROPERTY(Transient) TObjectPtr<UDrProgressBar> HealthTrailBar;
  UPROPERTY(Transient) TObjectPtr<UDrProgressBar> HealthBar;
  UPROPERTY(Transient) TObjectPtr<UDrProgressBar> StaminaTrailBar;
  UPROPERTY(Transient) TObjectPtr<UDrProgressBar> StaminaBar;
  UPROPERTY(Transient) TObjectPtr<UDrProgressBar> BossTrailBar;
  UPROPERTY(Transient) TObjectPtr<UDrProgressBar> BossBar;
  FTimerHandle FadeTimer;
  FTimerHandle DamageTrailTimer;
  FDelegateHandle ThemeHandle;
  FDsHUDDamageTrail HealthTrail;
  FDsHUDDamageTrail BossTrail;
  double FadeStart = 0.0;
  bool bResultShown = false;
};
