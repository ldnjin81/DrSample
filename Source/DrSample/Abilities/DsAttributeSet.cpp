#include "Abilities/DsAttributeSet.h"
// 전투 속성의 초기값, 제한, 피해 후처리를 구현한다.
#include "Combat/DsPlayerCombatData.h"
#include "Engine/World.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Core/DsGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
UDsAttributeSet::UDsAttributeSet() {
  // 합리적인 기본값. 액터별 초기화용 GameplayEffect가 이 값을 덮어쓸 수 있다.
  InitHealth(100);
  InitMaxHealth(100);
  InitStamina(100);
  InitMaxStamina(100);
  InitStaminaRegenRate(25);
  InitPoise(100);
  InitMaxPoise(100);
  InitPoiseRegenRate(20);
  InitAttackPower(20);
  InitDefense(5);
  InitElementalResist(0);
  // 특성 기본값 0 — 액터가 초기화할 때 설정하고 스탯을 파생한다.
  InitStrength(0);
  InitDexterity(0);
  InitVitality(0);
  InitSpirit(0);
  InitWill(0);
}
void UDsAttributeSet::PreAttributeChange(const FGameplayAttribute &Attribute,
                                         float &NewValue) {
  Super::PreAttributeChange(Attribute, NewValue);
  if (Attribute == GetHealthAttribute())
    NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
  else if (Attribute == GetStaminaAttribute())
    NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
  else if (Attribute == GetPoiseAttribute())
    NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPoise());
}
void UDsAttributeSet::PostGameplayEffectExecute(
    const FGameplayEffectModCallbackData &Data) {
  Super::PostGameplayEffectExecute(Data);
  const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
  AActor *Instigator = Context.GetOriginalInstigator();
  AActor *Causer = Context.GetEffectCauser();
  const FGameplayAttribute &Attribute = Data.EvaluatedData.Attribute;
  if (Attribute == GetIncomingDamageAttribute()) {
    float RawDamage = GetIncomingDamage();
    SetIncomingDamage(0);
    float HealthBefore = GetHealth();
    // 이미 사망한 대상에는 데미지와 사망 이벤트를 다시 적용하지 않는다.
    if (RawDamage > 0 && HealthBefore > 0) {
      // 무적 상태면 데미지를 완전히 차단한다.
      if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent())
        if (ASC->HasMatchingGameplayTag(DsTags::State_Invulnerable))
          return;
      UDsAbilitySystemComponent *GuardASC = Cast<UDsAbilitySystemComponent>(
          GetOwningAbilitySystemComponent());
      if (GuardASC && IsAttackFromFront(Instigator, Causer)) {
        if (GuardASC->HasMatchingGameplayTag(DsTags::State_Parrying)) {
          // 쳐내기: 피해를 전부 막고 공격자의 포이즈를 깎는다.
          OnParried.Broadcast(Instigator, Causer);
          AActor *Attacker = Causer ? Causer : Instigator;
          if (UDsAbilitySystemComponent *AttackerASC =
                  Attacker ? Attacker->FindComponentByClass<UDsAbilitySystemComponent>()
                           : nullptr)
            AttackerASC->TakePoiseDamage(UDsPlayerCombatData::Get()->Guard.ParryPoiseReturn,
                                         GuardASC->GetAvatarActor());
          return;
        }
        if (GuardASC->HasMatchingGameplayTag(DsTags::State_Guarding)) {
          const float GuardCost = RawDamage * UDsPlayerCombatData::Get()->Guard.GuardStaminaRate;
          if (GuardASC->GetStamina() >= GuardCost) {
            GuardASC->SpendStamina(GuardCost);
            RawDamage *= UDsPlayerCombatData::Get()->Guard.GuardDamageRate;
          } else {
            // 스태미나가 모자라면 막기가 깨져 전액 피해와 경직을 받는다.
            OnPoiseBroken.Broadcast(Instigator, Causer);
          }
        }
      }
      // Defense는 고정값만큼 경감하지만 최소 5%는 항상 관통한다.
      float Damage = FMath::Max(RawDamage - GetDefense(), RawDamage * .05f);
      if (UDsAbilitySystemComponent *ASC = Cast<UDsAbilitySystemComponent>(
              GetOwningAbilitySystemComponent())) {
        // 활성 효과들의 "받는 피해" 배수를 적용한다.
        Damage *= ASC->GetIncomingDamageMultiplier();
        // 보호막 흡수 풀에서 먼저 차감하고 잔여 피해만 통과시킨다.
        Damage = ASC->AbsorbWithShield(Damage);
        // 피격 시 스태미나 재생을 1.5초 중단한다.
        ASC->PushStaminaRegenDelay(UDsPlayerCombatData::Get()->StaminaRegenDelayAfterHit);
      }
      float NewHealth =
          FMath::Clamp(HealthBefore - Damage, 0.f, GetMaxHealth());
      SetHealth(NewHealth);
      OnDamaged.Broadcast(Damage, Instigator, Causer);
      // 살아있다가 0으로 떨어지는 전이에서만 사망을 알린다.
      if (NewHealth <= 0)
        OnOutOfHealth.Broadcast(Instigator, Causer);
    }
  } else if (Attribute == GetIncomingPoiseDamageAttribute()) {
    float RawPoiseDamage = GetIncomingPoiseDamage();
    SetIncomingPoiseDamage(0);
    // 쳐내기·막기가 정면 공격을 받아내는 동안에는 포이즈가 깎이지 않는다.
    // 무적 구간에는 방향과 무관하게 깎이지 않는다.
    if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent())
      if (ASC->HasMatchingGameplayTag(DsTags::State_Invulnerable) ||
          (IsAttackFromFront(Instigator, Causer) &&
           (ASC->HasMatchingGameplayTag(DsTags::State_Parrying) ||
            ASC->HasMatchingGameplayTag(DsTags::State_Guarding))))
        return;
    if (RawPoiseDamage > 0) {
      float NewPoise = GetPoise() - RawPoiseDamage;
      if (NewPoise <= 0) {
        // 포이즈 붕괴: 풀을 다시 채우고 경직을 알린다.
        SetPoise(GetMaxPoise());
        OnPoiseBroken.Broadcast(Instigator, Causer);
      } else
        SetPoise(NewPoise);
    }
  }
}
bool UDsAttributeSet::IsAttackFromFront(const AActor *Instigator,
                                        const AActor *Causer) const {
  const AActor *Attacker = Causer ? Causer : Instigator;
  const UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
  const AActor *Victim = ASC ? ASC->GetAvatarActor() : nullptr;
  if (!Attacker || !Victim || Attacker == Victim)
    return false;
  const FVector ToAttacker =
      (Attacker->GetActorLocation() - Victim->GetActorLocation())
          .GetSafeNormal2D();
  return FVector::DotProduct(Victim->GetActorForwardVector(), ToAttacker) > 0.f;
}
void UDsAttributeSet::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);
  DOREPLIFETIME_CONDITION_NOTIFY(UDsAttributeSet, Health, COND_None,
                                 REPNOTIFY_Always);
  DOREPLIFETIME_CONDITION_NOTIFY(UDsAttributeSet, MaxHealth, COND_None,
                                 REPNOTIFY_Always);
  DOREPLIFETIME_CONDITION_NOTIFY(UDsAttributeSet, Stamina, COND_None,
                                 REPNOTIFY_Always);
  DOREPLIFETIME_CONDITION_NOTIFY(UDsAttributeSet, MaxStamina, COND_None,
                                 REPNOTIFY_Always);
  DOREPLIFETIME_CONDITION_NOTIFY(UDsAttributeSet, Poise, COND_None,
                                 REPNOTIFY_Always);
}
void UDsAttributeSet::OnRep_Health(const FGameplayAttributeData &OldValue) {
  GAMEPLAYATTRIBUTE_REPNOTIFY(UDsAttributeSet, Health, OldValue);
}
void UDsAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData &OldValue) {
  GAMEPLAYATTRIBUTE_REPNOTIFY(UDsAttributeSet, MaxHealth, OldValue);
}
void UDsAttributeSet::OnRep_Stamina(const FGameplayAttributeData &OldValue) {
  GAMEPLAYATTRIBUTE_REPNOTIFY(UDsAttributeSet, Stamina, OldValue);
}
void UDsAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData &OldValue) {
  GAMEPLAYATTRIBUTE_REPNOTIFY(UDsAttributeSet, MaxStamina, OldValue);
}
void UDsAttributeSet::OnRep_Poise(const FGameplayAttributeData &OldValue) {
  GAMEPLAYATTRIBUTE_REPNOTIFY(UDsAttributeSet, Poise, OldValue);
}
void UDsAttributeSet::InitFromStats(const FDsStatBlock &Stats) {
  InitStrength(Stats.Strength);
  InitDexterity(Stats.Dexterity);
  InitVitality(Stats.Vitality);
  InitSpirit(Stats.Spirit);
  InitWill(Stats.Will);
  const float DerivedHealth = FDsStatFormula::MaxHealth(Stats.BaseMaxHealth, Stats.Vitality);
  const float DerivedStamina = FDsStatFormula::MaxStamina(Stats.BaseMaxStamina, Stats.Spirit);
  const float DerivedPoise = FDsStatFormula::MaxPoise(Stats.BaseMaxPoise, Stats.Strength);
  InitMaxHealth(DerivedHealth);
  InitHealth(DerivedHealth);
  InitMaxStamina(DerivedStamina);
  InitStamina(DerivedStamina);
  InitStaminaRegenRate(FDsStatFormula::StaminaRegen(Stats.BaseStaminaRegen, Stats.Dexterity));
  InitMaxPoise(DerivedPoise);
  InitPoise(DerivedPoise);
  InitPoiseRegenRate(Stats.PoiseRegenRate);
  InitAttackPower(FDsStatFormula::AttackPower(Stats.BaseAttackPower, Stats.Strength));
  InitDefense(FDsStatFormula::Defense(Stats.BaseDefense, Stats.Vitality));
  InitElementalResist(FDsStatFormula::ElementalResist(Stats.Will));
}
