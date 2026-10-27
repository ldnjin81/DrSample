#include "UI/DsCombatHUDViewModel.h"

#include "Abilities/DsAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Boss/DsBossCharacter.h"
#include "Boss/DsBossData.h"
#include "Characters/DsCharacter.h"
#include "Combat/DsLockOnComponent.h"

namespace {
float Rate(float Value, float Maximum) {
  return Maximum > 0.f ? FMath::Clamp(Value / Maximum, 0.f, 1.f) : 0.f;
}
}

float UDsCombatHUDViewModel::GetPlayerHealthRate() const { return Rate(PlayerHealth, PlayerMaxHealth); }
float UDsCombatHUDViewModel::GetPlayerStaminaRate() const { return Rate(PlayerStamina, PlayerMaxStamina); }
bool UDsCombatHUDViewModel::IsPlayerDead() const { return EncounterState == EDsEncounterState::PlayerDead; }
bool UDsCombatHUDViewModel::IsVictory() const { return EncounterState == EDsEncounterState::Victory; }

void UDsCombatHUDViewModel::Initialize(UDsEncounterSubsystem *InEncounter) {
  if (Encounter.IsValid()) {
    Encounter->OnActorsChanged.RemoveAll(this);
    Encounter->OnStateChanged.RemoveAll(this);
  }
  Encounter = InEncounter;
  if (InEncounter) {
    InEncounter->OnActorsChanged.AddUObject(this, &ThisClass::RefreshEncounter);
    InEncounter->OnStateChanged.AddUObject(this, &ThisClass::HandleStateChanged);
  }
  RefreshEncounter();
}

void UDsCombatHUDViewModel::UnbindAttributes(UAbilitySystemComponent *ASC) {
  if (!ASC) return;
  for (const auto Attribute : {UDsAttributeSet::GetHealthAttribute(), UDsAttributeSet::GetMaxHealthAttribute(),
                               UDsAttributeSet::GetStaminaAttribute(), UDsAttributeSet::GetMaxStaminaAttribute()})
    ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
}

void UDsCombatHUDViewModel::BindPlayer(ADsCharacter *InPlayer) {
  if (Player.IsValid()) Player->GetLockOn()->OnTargetChanged.RemoveAll(this);
  Player = InPlayer;
  if (InPlayer) InPlayer->GetLockOn()->OnTargetChanged.AddUObject(this, &ThisClass::HandleLockOnChanged);
  HandleLockOnChanged(InPlayer ? InPlayer->GetLockOn()->GetTarget() : nullptr);
  UnbindAttributes(PlayerASC.Get());
  PlayerASC = InPlayer ? InPlayer->GetAbilitySystemComponent() : nullptr;
  if (auto *ASC = PlayerASC.Get()) {
    for (const auto Attribute : {UDsAttributeSet::GetHealthAttribute(), UDsAttributeSet::GetMaxHealthAttribute(),
                                 UDsAttributeSet::GetStaminaAttribute(), UDsAttributeSet::GetMaxStaminaAttribute()})
      ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
        [this](const FOnAttributeChangeData &) { RefreshPlayer(); });
  }
  RefreshPlayer();
}

void UDsCombatHUDViewModel::RefreshPlayer() {
  auto *ASC = PlayerASC.Get();
  const auto Read = [ASC](FGameplayAttribute Attribute) { return ASC ? ASC->GetNumericAttribute(Attribute) : 0.f; };
  // 먼저 네 값을 확정한 뒤 파생 비율을 알린다. 최대치와 현재치가 함께 바뀌어도 일관된다.
  const bool HealthChanged = UE_MVVM_SET_PROPERTY_VALUE(PlayerHealth, Read(UDsAttributeSet::GetHealthAttribute()));
  const bool MaxHealthChanged = UE_MVVM_SET_PROPERTY_VALUE(PlayerMaxHealth, Read(UDsAttributeSet::GetMaxHealthAttribute()));
  const bool StaminaChanged = UE_MVVM_SET_PROPERTY_VALUE(PlayerStamina, Read(UDsAttributeSet::GetStaminaAttribute()));
  const bool MaxStaminaChanged = UE_MVVM_SET_PROPERTY_VALUE(PlayerMaxStamina, Read(UDsAttributeSet::GetMaxStaminaAttribute()));
  if (HealthChanged || MaxHealthChanged) UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPlayerHealthRate);
  if (StaminaChanged || MaxStaminaChanged) UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPlayerStaminaRate);
}

void UDsCombatHUDViewModel::RefreshEncounter() {
  UnbindAttributes(BossASC.Get());
  ADsBossCharacter *Boss = Encounter.IsValid() ? Encounter->GetBoss() : nullptr;
  BossASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
  UE_MVVM_SET_PROPERTY_VALUE(BossDisplayName, Boss ? Boss->GetData()->DisplayName : FText::GetEmpty());
  if (auto *ASC = BossASC.Get()) {
    for (const auto Attribute : {UDsAttributeSet::GetHealthAttribute(), UDsAttributeSet::GetMaxHealthAttribute()})
      ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
        [this](const FOnAttributeChangeData &) { RefreshBoss(); });
  }
  RefreshBoss();
  HandleStateChanged(EncounterState, Encounter.IsValid() ? Encounter->GetState() : EDsEncounterState::Idle);
}

void UDsCombatHUDViewModel::RefreshBoss() {
  auto *ASC = BossASC.Get();
  UE_MVVM_SET_PROPERTY_VALUE(BossHealthRate, ASC ? Rate(ASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()),
    ASC->GetNumericAttribute(UDsAttributeSet::GetMaxHealthAttribute())) : 0.f);
}

void UDsCombatHUDViewModel::HandleStateChanged(EDsEncounterState, EDsEncounterState NewState) {
  if (UE_MVVM_SET_PROPERTY_VALUE(EncounterState, NewState)) {
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsPlayerDead);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsVictory);
  }
  UE_MVVM_SET_PROPERTY_VALUE(BossVisible, NewState == EDsEncounterState::Fighting && BossASC.IsValid());
}

void UDsCombatHUDViewModel::Shutdown() {
  if (Player.IsValid()) Player->GetLockOn()->OnTargetChanged.RemoveAll(this);
  Player.Reset();
  HandleLockOnChanged(nullptr);
  if (Encounter.IsValid()) {
    Encounter->OnActorsChanged.RemoveAll(this);
    Encounter->OnStateChanged.RemoveAll(this);
  }
  UnbindAttributes(PlayerASC.Get());
  UnbindAttributes(BossASC.Get());
  PlayerASC.Reset();
  BossASC.Reset();
  Encounter.Reset();
}

void UDsCombatHUDViewModel::HandleLockOnChanged(AActor* Target) {
  UE_MVVM_SET_PROPERTY_VALUE(LockOnTarget, Target);
}

void UDsCombatHUDViewModel::BeginDestroy() {
  Shutdown();
  Super::BeginDestroy();
}
