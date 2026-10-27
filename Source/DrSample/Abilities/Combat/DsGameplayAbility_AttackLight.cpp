#include "Abilities/Combat/DsGameplayAbility_AttackLight.h"

#include "Core/DsGameplayTags.h"

UDsGameplayAbility_AttackLight::UDsGameplayAbility_AttackLight() {
  SetAssetTags(FGameplayTagContainer(DsTags::Ability_Attack_Light));
}

const TArray<FDsMeleeStage> &UDsGameplayAbility_AttackLight::GetStages() const {
  return UDsPlayerCombatData::Get()->LightStages;
}
