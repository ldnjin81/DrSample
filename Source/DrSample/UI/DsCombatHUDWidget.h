#pragma once

#include "CoreMinimal.h"
#include "UI/DrHUDWidget.h"
#include "UI/DsHUDDamageTrail.h"
#include "DsCombatHUDWidget.generated.h"

class UDsCombatHUDViewModel;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

/** 파이썬에 노출되지 않은 WidgetTree 편집은 네이티브 레이아웃으로 재현한다. */
UCLASS()
class DRSAMPLE_API UDsCombatHUDWidget : public UDrHUDWidget {
  GENERATED_BODY()
public:
  virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
  void SetViewModel(UDsCombatHUDViewModel *InViewModel);
protected:
  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;
  virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
  UPROPERTY(Transient) TObjectPtr<UImage> LockOnMarker;
  void Subscribe();
  void Refresh();
  void AdvanceFade();
  void AdvanceDamageTrails();
  UCanvasPanel *MakePanel(UCanvasPanel *Root, FName Name, FVector2D Position, FVector2D Size,
                        FVector2D Anchor, FVector2D Alignment, FLinearColor Tint, float Radius);
  UMaterialInstanceDynamic *MakeGauge(UCanvasPanel *Panel, FName Name, FVector2D Position,
                                    FVector2D Size, const TCHAR *MaterialPath);
  UTextBlock *MakeText(UCanvasPanel *Panel, FName Name, const FText &Text, FVector2D Position,
                      FVector2D Size, int32 FontSize, ETextJustify::Type Justification);
  UPROPERTY(Transient) TObjectPtr<UDsCombatHUDViewModel> ViewModel;
  UPROPERTY(Transient) TObjectPtr<UCanvasPanel> BossPanel;
  UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ResultPanel;
  UPROPERTY(Transient) TObjectPtr<UTextBlock> BossNameText;
  UPROPERTY(Transient) TObjectPtr<UTextBlock> ResultTitle;
  UPROPERTY(Transient) TObjectPtr<UTextBlock> ResultSubtitle;
  UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HealthMaterial;
  UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> StaminaMaterial;
  UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BossMaterial;
  UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> PanelMaterials;
  FTimerHandle FadeTimer;
  FTimerHandle DamageTrailTimer;
  FDsHUDDamageTrail HealthTrail;
  FDsHUDDamageTrail BossTrail;
  double FadeStart = 0.0;
  bool bResultShown = false;
};
