#include "Game/DsPlayerController.h"
#include "UI/DsCombatHUDViewModel.h"
#include "UI/DsCombatHUDWidget.h"
#include "Characters/DsCharacter.h"
#include "Abilities/DsAttributeSet.h"
#include "Encounter/DsEncounterSubsystem.h"
#include "Engine/World.h"
void ADsPlayerController::OnHUDCreated(UDrHUDWidget* Widget) { HUDViewModel=NewObject<UDsCombatHUDViewModel>(this); HUDViewModel->Initialize(GetWorld()->GetSubsystem<UDsEncounterSubsystem>()); if(auto* CombatWidget=Cast<UDsCombatHUDWidget>(Widget)) CombatWidget->SetViewModel(HUDViewModel); }
void ADsPlayerController::OnPawnChanged(APawn*,APawn* NewPawn) { if(HUDViewModel) HUDViewModel->BindPlayer(Cast<ADsCharacter>(NewPawn)); }
bool ADsPlayerController::CanOpenPauseMenu() const { const auto* HeroPawn=Cast<ADsCharacter>(GetPawn()); const auto* Encounter=GetWorld()->GetSubsystem<UDsEncounterSubsystem>(); return HeroPawn && HeroPawn->GetAttributeSet() && HeroPawn->GetAttributeSet()->GetHealth()>0 && (!Encounter || Encounter->GetState()!=EDsEncounterState::PlayerDead); }
void ADsPlayerController::RequestRestart_Implementation() { Super::RequestRestart_Implementation(); if(auto* Encounter=GetWorld()->GetSubsystem<UDsEncounterSubsystem>()) Encounter->RestartCheckpoint(); }
UDsCombatHUDWidget* ADsPlayerController::GetCombatHUD() const { return Cast<UDsCombatHUDWidget>(GetHUDWidget()); }
void ADsPlayerController::EndPlay(const EEndPlayReason::Type Reason) { if(HUDViewModel) HUDViewModel->Shutdown(); Super::EndPlay(Reason); }
