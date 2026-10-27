#include "Abilities/Combat/DsGameplayAbility_Guard.h"
// 막기 어빌리티의 쳐내기·막기 구간 전환을 구현한다.

#include "Abilities/DsAbilitySystemComponent.h"
#include "Combat/DsPlayerCombatData.h"
#include "Animation/AnimMontage.h"
#include "Core/DsGameplayTags.h"

UDsGameplayAbility_Guard::UDsGameplayAbility_Guard() {
  SetAssetTags(FGameplayTagContainer(DsTags::Ability_Guard));
  BlockDuringCombatStates();
}

float UDsGameplayAbility_Guard::GetStaminaCost() const {
  return UDsPlayerCombatData::Get()->Guard.StaminaCost;
}

void UDsGameplayAbility_Guard::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData *TriggerEventData) {
  Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
  if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !CommitStamina()) {
    EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    return;
  }
  Elapsed = 0.f;
  GetDsASC()->SetLooseState(DsTags::State_Parrying, true);
  PlayAbilityMontage(UDsPlayerCombatData::Get()->Guard.Montage);
}

void UDsGameplayAbility_Guard::AbilityTick(float DeltaTime) {
  Elapsed += DeltaTime;
  UDsAbilitySystemComponent *ASC = GetDsASC();
  if (ASC && Elapsed >= UDsPlayerCombatData::Get()->Guard.ParryWindow &&
      ASC->HasMatchingGameplayTag(DsTags::State_Parrying)) {
    ASC->SetLooseState(DsTags::State_Parrying, false);
    ASC->SetLooseState(DsTags::State_Guarding, true);
  }
  if (Elapsed >= UDsPlayerCombatData::Get()->Guard.Duration)
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UDsGameplayAbility_Guard::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled) {
  if (UDsAbilitySystemComponent *ASC = GetDsASC()) {
    ASC->SetLooseState(DsTags::State_Parrying, false);
    ASC->SetLooseState(DsTags::State_Guarding, false);
  }
  if (bWasCancelled)
    StopAbilityMontage();
  Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
                    bWasCancelled);
}
