#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"

#include "Core/DsGameplayTags.h"

UDsGameplayAbility_AttackHeavy::UDsGameplayAbility_AttackHeavy() {
  SetAssetTags(FGameplayTagContainer(DsTags::Ability_Attack_Heavy));
}

const TArray<FDsMeleeStage> &UDsGameplayAbility_AttackHeavy::GetStages() const {
  return UDsPlayerCombatData::Get()->HeavyStages;
}
