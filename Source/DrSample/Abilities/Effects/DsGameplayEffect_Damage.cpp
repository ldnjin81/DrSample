#include "Abilities/Effects/DsGameplayEffect_Damage.h"
// 즉시형 피해 GameplayEffect의 메타 속성 변경자를 구성한다.
#include "Abilities/DsAttributeSet.h"
#include "Core/DsGameplayTags.h"
UDsGameplayEffect_Damage::UDsGameplayEffect_Damage() {
  DurationPolicy = EGameplayEffectDurationType::Instant;
  FGameplayModifierInfo DamageModifier;
  // IncomingDamage += SetByCaller(Data.Damage)
  DamageModifier.Attribute = UDsAttributeSet::GetIncomingDamageAttribute();
  DamageModifier.ModifierOp = EGameplayModOp::Additive;
  FSetByCallerFloat DamageSetByCaller;
  DamageSetByCaller.DataTag = DsTags::Data_Damage;
  DamageModifier.ModifierMagnitude =
      FGameplayEffectModifierMagnitude(DamageSetByCaller);
  Modifiers.Add(DamageModifier);

  FGameplayModifierInfo PoiseDamageModifier;
  // IncomingPoiseDamage += SetByCaller(Data.PoiseDamage)
  PoiseDamageModifier.Attribute =
      UDsAttributeSet::GetIncomingPoiseDamageAttribute();
  PoiseDamageModifier.ModifierOp = EGameplayModOp::Additive;
  FSetByCallerFloat PoiseDamageSetByCaller;
  PoiseDamageSetByCaller.DataTag = DsTags::Data_PoiseDamage;
  PoiseDamageModifier.ModifierMagnitude =
      FGameplayEffectModifierMagnitude(PoiseDamageSetByCaller);
  Modifiers.Add(PoiseDamageModifier);
}
