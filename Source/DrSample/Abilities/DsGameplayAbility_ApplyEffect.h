// 데이터 테이블의 효과 Id를 게임플레이 이벤트 대상으로 적용하는 어빌리티이다.
#pragma once

#include "Abilities/DsGameplayAbility.h"
#include "DsGameplayAbility_ApplyEffect.generated.h"

/** 이벤트 페이로드의 Target 액터를 피격 대상으로 삼아 데이터 효과를 적용한다. */
UCLASS()
class DRSAMPLE_API UDsGameplayAbility_ApplyEffect : public UDsGameplayAbility {
  GENERATED_BODY()

public:
  /** EffectDefs 테이블에서 적용할 효과의 기본키이다. */
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Effect")
  int32 EffectId = 0;

protected:
  virtual void ActivateAbility(
      const FGameplayAbilitySpecHandle Handle,
      const FGameplayAbilityActorInfo *ActorInfo,
      const FGameplayAbilityActivationInfo ActivationInfo,
      const FGameplayEventData *TriggerEventData) override;
};
