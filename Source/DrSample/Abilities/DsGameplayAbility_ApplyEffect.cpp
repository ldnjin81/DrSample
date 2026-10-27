// 이벤트 대상에게 데이터 기반 효과를 적용한다.
#include "Abilities/DsGameplayAbility_ApplyEffect.h"

#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Actor.h"

void UDsGameplayAbility_ApplyEffect::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData *TriggerEventData) {
  UDsAbilitySystemComponent *SourceASC = GetDsASC();
  if (!SourceASC) {
    EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
    return;
  }

  UDsAbilitySystemComponent *TargetASC = nullptr;
  if (TriggerEventData && TriggerEventData->Target)
    TargetASC =
        TriggerEventData->Target->FindComponentByClass<UDsAbilitySystemComponent>();

  const bool bApplied =
      SourceASC->ApplyEffectById(EffectId, TargetASC, GetDsActor());
  if (bApplied) {
    // 적용이 보장된 뒤 비용과 재사용 대기시간을 커밋한다.
    CommitCooldown();
  }
  EndAbility(Handle, ActorInfo, ActivationInfo, false, !bApplied);
}
