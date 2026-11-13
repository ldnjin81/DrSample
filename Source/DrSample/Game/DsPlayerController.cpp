#include "Game/DsPlayerController.h"
#include "UI/DsCombatHUDViewModel.h"
#include "UI/DsCombatHUDWidget.h"
#include "Characters/DsCharacter.h"
#include "Abilities/DsAttributeSet.h"
#include "Encounter/DsEncounterSubsystem.h"
#include "Flow/DrUIFlowSubsystem.h"
#include "Engine/World.h"
void ADsPlayerController::OnEnterTitle() { Super::OnEnterTitle(); if(auto* Flow=UDrUIFlowSubsystem::Get(this)) Flow->EnterTitle(); }
void ADsPlayerController::OnEnterGame() {
 Super::OnEnterGame();
 HUDViewModel=NewObject<UDsCombatHUDViewModel>(this); HUDViewModel->Initialize(GetWorld()->GetSubsystem<UDsEncounterSubsystem>());
 if(auto* Flow=UDrUIFlowSubsystem::Get(this)) { Flow->CanOpenPauseMenu.BindUObject(this,&ThisClass::CanOpenPauseMenu); Flow->EnterGame(); }
 if(auto* CombatWidget=GetCombatHUD()) CombatWidget->SetViewModel(HUDViewModel);
}
void ADsPlayerController::OnPawnChanged(APawn*,APawn* NewPawn) { if(HUDViewModel) HUDViewModel->BindPlayer(Cast<ADsCharacter>(NewPawn)); }
bool ADsPlayerController::CanOpenPauseMenu() const { const auto* HeroPawn=Cast<ADsCharacter>(GetPawn()); const auto* Encounter=GetWorld()->GetSubsystem<UDsEncounterSubsystem>(); return HeroPawn && HeroPawn->GetAttributeSet() && HeroPawn->GetAttributeSet()->GetHealth()>0 && (!Encounter || Encounter->GetState()!=EDsEncounterState::PlayerDead); }
void ADsPlayerController::RequestRestart_Implementation() { if(auto* Flow=UDrUIFlowSubsystem::Get(this)) Flow->ClosePauseMenu(); Super::RequestRestart_Implementation(); if(auto* Encounter=GetWorld()->GetSubsystem<UDsEncounterSubsystem>()) Encounter->RestartCheckpoint(); }
UDsCombatHUDWidget* ADsPlayerController::GetCombatHUD() const { const auto* Flow=UDrUIFlowSubsystem::Get(this); return Flow ? Cast<UDsCombatHUDWidget>(Flow->GetHUD()) : nullptr; }
void ADsPlayerController::EndPlay(const EEndPlayReason::Type Reason) {
 if(auto* Flow=UDrUIFlowSubsystem::Get(this)) if(Flow->CanOpenPauseMenu.IsBoundToObject(this)) Flow->CanOpenPauseMenu.Unbind();
 if(HUDViewModel) HUDViewModel->Shutdown(); Super::EndPlay(Reason);
}
