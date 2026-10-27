// 근접 공격(약공격 콤보·강공격) 어빌리티를 선언한다.
#pragma once

#include "Abilities/DsGameplayAbility.h"
#include "Combat/DsPlayerCombatData.h"
#include "DsGameplayAbility_MeleeAttack.generated.h"


/**
 * 타수 배열을 순서대로 진행하는 근접 공격이다.
 * 진행 중 입력을 버퍼해 두면 현재 타의 판정이 끝난 뒤 다음 타로 넘어간다.
 */
UCLASS(Abstract)
class DRSAMPLE_API UDsGameplayAbility_MeleeAttack : public UDsGameplayAbility {
  GENERATED_BODY()

public:
  UDsGameplayAbility_MeleeAttack();

  /** 타 배열. 약·강공격이 플레이어 전투 데이터에서 골라 준다. */
  virtual const TArray<FDsMeleeStage> &GetStages() const;

  /** 다음 타 입력을 예약한다. 마지막 타에서는 무시된다. */
  void BufferNextInput();
  /** 진행 중인 타의 번호(0부터)이다. */
  int32 GetStageIndex() const { return StageIndex; }

  virtual bool CanActivateAbility(
      const FGameplayAbilitySpecHandle Handle,
      const FGameplayAbilityActorInfo *ActorInfo,
      const FGameplayTagContainer *SourceTags = nullptr,
      const FGameplayTagContainer *TargetTags = nullptr,
      OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;
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
  /** 지정한 타를 시작한다. 스태미나가 모자라면 false다. */
  bool BeginStage(int32 Index);
  /** 전방 구 안의 대상에게 한 타당 한 번씩 피해를 준다. */
  void ApplyHits(const FDsMeleeStage &Stage);
  void Finish(bool bWasCancelled);

  int32 StageIndex = 0;
  float StageElapsed = 0.f;
  bool bInputBuffered = false;
  TSet<TWeakObjectPtr<AActor>> HitActors;
};
