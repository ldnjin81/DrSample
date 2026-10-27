#pragma once

#include "CoreMinimal.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "DsGameplayAbility_AttackHeavy.generated.h"

/** 긴 선딜의 강공격 1타(Greystone RMB). */
UCLASS()
class DRSAMPLE_API UDsGameplayAbility_AttackHeavy
    : public UDsGameplayAbility_MeleeAttack {
  GENERATED_BODY()
public:
  UDsGameplayAbility_AttackHeavy();
  virtual const TArray<FDsMeleeStage> &GetStages() const override;
};
