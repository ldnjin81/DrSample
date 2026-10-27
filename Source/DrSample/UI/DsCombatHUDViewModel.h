#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Encounter/DsEncounterSubsystem.h"
#include "DsCombatHUDViewModel.generated.h"

class ADsCharacter;
class UAbilitySystemComponent;

/** GAS와 엔카운터 이벤트만 구독하는 전투 표시 모델이다. */
UCLASS(BlueprintType)
class DRSAMPLE_API UDsCombatHUDViewModel : public UMVVMViewModelBase {
  GENERATED_BODY()
public:
  void Initialize(UDsEncounterSubsystem *InEncounter);
  void BindPlayer(ADsCharacter *InPlayer);
  void Shutdown();
  virtual void BeginDestroy() override;

  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") float PlayerHealth = 0.f;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") float PlayerMaxHealth = 0.f;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") float PlayerStamina = 0.f;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") float PlayerMaxStamina = 0.f;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") bool BossVisible = false;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") FText BossDisplayName;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") float BossHealthRate = 0.f;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") EDsEncounterState EncounterState = EDsEncounterState::Idle;
  UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Ds|HUD") TObjectPtr<AActor> LockOnTarget;

  UFUNCTION(BlueprintPure, FieldNotify, Category="Ds|HUD") float GetPlayerHealthRate() const;
  UFUNCTION(BlueprintPure, FieldNotify, Category="Ds|HUD") float GetPlayerStaminaRate() const;
  UFUNCTION(BlueprintPure, FieldNotify, Category="Ds|HUD") bool IsPlayerDead() const;
  UFUNCTION(BlueprintPure, FieldNotify, Category="Ds|HUD") bool IsVictory() const;
private:
  void HandleLockOnChanged(AActor* Target);
  TWeakObjectPtr<ADsCharacter> Player;
  void RefreshPlayer();
  void RefreshBoss();
  void RefreshEncounter();
  void HandleStateChanged(EDsEncounterState OldState, EDsEncounterState NewState);
  void UnbindAttributes(UAbilitySystemComponent *ASC);
  TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
  TWeakObjectPtr<UAbilitySystemComponent> BossASC;
  TWeakObjectPtr<UDsEncounterSubsystem> Encounter;
};
