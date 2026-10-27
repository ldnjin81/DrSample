#pragma once

#include "CoreMinimal.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "DsGameplayAbility_AttackLight.generated.h"

/** 약공격 3타 콤보(Greystone Primary A·B·C). */
UCLASS()
class DRSAMPLE_API UDsGameplayAbility_AttackLight
    : public UDsGameplayAbility_MeleeAttack {
  GENERATED_BODY()
public:
  UDsGameplayAbility_AttackLight();
  virtual const TArray<FDsMeleeStage> &GetStages() const override;
};
