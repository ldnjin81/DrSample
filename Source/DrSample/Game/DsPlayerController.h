#pragma once
#include "Game/DrPlayerController.h"
#include "DsPlayerController.generated.h"
class UDsCombatHUDViewModel;
class UDsCombatHUDWidget;
UCLASS()
class DRSAMPLE_API ADsPlayerController : public ADrPlayerController { GENERATED_BODY()
public:
 void RestartCheckpoint() { RequestRestart(); }
 UDsCombatHUDWidget* GetCombatHUD() const;
 virtual void RequestRestart_Implementation() override;
protected:
 virtual void OnHUDCreated(UDrHUDWidget* Widget) override;
 virtual void OnPawnChanged(APawn* PreviousPawn,APawn* NewPawn) override;
 virtual bool CanOpenPauseMenu() const override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 UPROPERTY(Transient) TObjectPtr<UDsCombatHUDViewModel> HUDViewModel;
};
