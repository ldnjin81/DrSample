#pragma once
#include "Game/DrPlayerController.h"
#include "DsPlayerController.generated.h"
class UDsCombatHUDViewModel;
class UDsCombatHUDWidget;
/** 게임 흐름(DrFramework)에서 UI 흐름(DrUIKit UDrUIFlowSubsystem)을 시작하고, 전투 HUD에 표시 모델을 붙인다. */
UCLASS()
class DRSAMPLE_API ADsPlayerController : public ADrPlayerController { GENERATED_BODY()
public:
 void RestartCheckpoint() { RequestRestart(); }
 UDsCombatHUDWidget* GetCombatHUD() const;
 virtual void RequestRestart_Implementation() override;
 /** 사망 연출 중에는 일시정지 메뉴를 막는다(UI 흐름의 CanOpenPauseMenu에 묶임). */
 bool CanOpenPauseMenu() const;
protected:
 virtual void OnEnterTitle() override;
 virtual void OnEnterGame() override;
 virtual void OnPawnChanged(APawn* PreviousPawn,APawn* NewPawn) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 UPROPERTY(Transient) TObjectPtr<UDsCombatHUDViewModel> HUDViewModel;
};
