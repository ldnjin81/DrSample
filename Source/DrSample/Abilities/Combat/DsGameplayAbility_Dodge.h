// 구르기 회피 어빌리티를 선언한다.
#pragma once

#include "Abilities/DsGameplayAbility.h"
#include "DsGameplayAbility_Dodge.generated.h"


/**
 * 이동 입력 방향(없으면 뒤쪽)으로 구르고 중간 구간에 무적을 준다.
 * 표시 메시가 있으면 8방향 구르기 모션이 이동을 이끌고, 없으면 일정 속도로 대시한다.
 */
UCLASS()
class DRSAMPLE_API UDsGameplayAbility_Dodge : public UDsGameplayAbility {
  GENERATED_BODY()

public:
  UDsGameplayAbility_Dodge();

  /** 수치는 UDsPlayerCombatData::Dodge에서 읽는다. */
  virtual float GetStaminaCost() const override;
  /** 캐릭터 기준 방향에 맞는 구르기 몽타주 번호(0=앞, 시계 방향)를 고른다. */
  static int32 SelectRollIndex(const FVector &LocalDirection);

  virtual void AbilityTick(float DeltaTime) override;

protected:
  virtual void ActivateAbility(
      const FGameplayAbilitySpecHandle Handle,
      const FGameplayAbilityActorInfo *ActorInfo,
      const FGameplayAbilityActivationInfo ActivationInfo,
      const FGameplayEventData *TriggerEventData) override;
  virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                          const FGameplayAbilityActorInfo *ActorInfo,
                          const FGameplayAbilityActivationInfo ActivationInfo,
                          bool bReplicateEndAbility,
                          bool bWasCancelled) override;

private:
  void SetInvulnerable(bool bEnabled);
  float Elapsed = 0.f;
  /** 이번 회피의 실제 길이. 모션을 재생하면 모션 길이를 따른다. */
  float ActiveDuration = 0.f;
  bool bGrantedInvulnerable = false;
};
