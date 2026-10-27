#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsGameplayAbility.h"
#include "DsSample.h"
#include "DrTableSubsystem.h"
#include "TableData/Generated/DsEffectDefsRow.h"
#include "TableData/Generated/EDsEffectApplyTo.h"
#include "Abilities/DsAttributeSet.h"
#include "Combat/DsPlayerCombatData.h"
#include "Abilities/Effects/DsGameplayEffect_Damage.h"
#include "Core/DsGameplayTags.h"
#include "TableData/Generated/DsEffectExtensionsRow.h"
namespace {
FDsResolvedEffect ScaleForStack(const FDsResolvedEffect &Base, int32 Level) {
  FDsResolvedEffect Scaled = Base;
  if (Base.StackRule != EDsEffectStackRule::Accumulate ||
      !Base.StackBonus.IsValidIndex(Level - 1))
    return Scaled;
  const float Strength = 1.f + Base.StackBonus[Level - 1] * .01f;
  Scaled.DamagePerTick = Base.DamagePerTick * Strength;
  Scaled.Magnitude = Base.Magnitude * Strength;
  Scaled.ProcInstantDamageCurHpPct =
      Base.ProcInstantDamageCurHpPct * Strength;
  Scaled.MoveSpeedMultiplier =
      1.f + (Base.MoveSpeedMultiplier - 1.f) * Strength;
  Scaled.DamageTakenMultiplier =
      1.f + (Base.DamageTakenMultiplier - 1.f) * Strength;
  Scaled.DamageDealtMultiplier =
      1.f + (Base.DamageDealtMultiplier - 1.f) * Strength;
  return Scaled;
}
} // namespace

UDsAbilitySystemComponent::UDsAbilitySystemComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}

float UDsAbilitySystemComponent::AggregateMultiplier(
    float FDsResolvedEffect::*Field) const {
  float Product = 1;
  // 한시 효과를 먼저 곱하고 같은 태그의 영구 효과는 중복 계산하지 않는다.
  for (const TPair<FGameplayTag, FDsResolvedEffect> &Pair : TimedEffects)
    Product *= Pair.Value.*Field;
  for (const TPair<FGameplayTag, FDsResolvedEffect> &Pair : PermanentEffects)
    if (!TimedEffects.Contains(Pair.Key))
      Product *= Pair.Value.*Field;
  return Product;
}

float UDsAbilitySystemComponent::GetMoveSpeedMultiplier() const {
  return AggregateMultiplier(&FDsResolvedEffect::MoveSpeedMultiplier);
}

float UDsAbilitySystemComponent::GetIncomingDamageMultiplier() const {
  return AggregateMultiplier(&FDsResolvedEffect::DamageTakenMultiplier);
}

float UDsAbilitySystemComponent::GetOutgoingDamageMultiplier() const {
  return AggregateMultiplier(&FDsResolvedEffect::DamageDealtMultiplier);
}

int32 UDsAbilitySystemComponent::GetStackCount(
    const FGameplayTag &StateTag) const {
  const int32 *Count = StackCounts.Find(StateTag);
  return Count ? *Count : 0;
}

void UDsAbilitySystemComponent::AddPermanentEffect(
    const FDsResolvedEffect &Effect) {
  if (Effect.StateTag.IsValid()) {
    AddLooseGameplayTag(Effect.StateTag);
    PermanentEffects.Add(Effect.StateTag, Effect);
  }
}

void UDsAbilitySystemComponent::RemovePermanentEffect(
    const FGameplayTag &StateTag) {
  if (StateTag.IsValid()) {
    RemoveLooseGameplayTag(StateTag);
    PermanentEffects.Remove(StateTag);
  }
}

void UDsAbilitySystemComponent::RemoveTimedEffect(const FGameplayTag &Tag) {
  RemoveLooseGameplayTag(Tag);
  TimedEffects.Remove(Tag);
  BaseTimedEffects.Remove(Tag);
  StackCounts.Remove(Tag);
  EffectClocks.Remove(Tag);
  StatusTickCausers.Remove(Tag);
}

void UDsAbilitySystemComponent::TickTimedEffects(float DeltaTime) {
  if (DeltaTime <= 0.f || EffectClocks.IsEmpty())
    return;
  TArray<FGameplayTag> Tags;
  EffectClocks.GetKeys(Tags);
  for (const FGameplayTag &Tag : Tags) {
    FEffectClock *Clock = EffectClocks.Find(Tag);
    if (!Clock)
      continue;
    const FDsResolvedEffect *Effect = TimedEffects.Find(Tag);
    // 만료 시각까지의 주기 피해만 준다(만료와 같은 시각의 틱은 포함).
    if (Effect && Effect->DamagePerTick > 0 && Effect->Period > 0) {
      Clock->SinceTick += FMath::Min(DeltaTime, Clock->Remaining);
      while (Clock->SinceTick >= Effect->Period) {
        Clock->SinceTick -= Effect->Period;
        const TWeakObjectPtr<AActor> *Source = StatusTickCausers.Find(Tag);
        ApplyDirectDamage(Effect->DamagePerTick, Source ? Source->Get() : nullptr);
        Clock = EffectClocks.Find(Tag);
        Effect = TimedEffects.Find(Tag);
        if (!Clock || !Effect)
          break;
      }
    }
    if (Clock && (Clock->Remaining -= DeltaTime) <= 0.f)
      RemoveTimedEffect(Tag);
  }
}

void UDsAbilitySystemComponent::StartCooldown(const FGameplayTag &CooldownTag, float Duration) {
  if (!CooldownTag.IsValid() || Duration <= 0.f)
    return;
  SetLooseState(CooldownTag, true);
  CooldownRemaining.Add(CooldownTag, Duration);
}

float UDsAbilitySystemComponent::GetCooldownRemaining(const FGameplayTag &CooldownTag) const {
  const float *Remaining = CooldownRemaining.Find(CooldownTag);
  return Remaining ? FMath::Max(0.f, *Remaining) : 0.f;
}

void UDsAbilitySystemComponent::TickCooldowns(float DeltaTime) {
  for (auto It = CooldownRemaining.CreateIterator(); It; ++It) {
    It.Value() -= DeltaTime;
    if (It.Value() <= 0.f) {
      SetLooseState(It.Key(), false);
      It.RemoveCurrent();
    }
  }
}

void UDsAbilitySystemComponent::ApplyStatusEffect(
    const FDsResolvedEffect &Effect, AActor *Causer) {
  if (!Effect.StateTag.IsValid() || Effect.Duration <= 0)
    return;
  const FGameplayTag StateTag = Effect.StateTag;
  const int32 PreviousCount = GetStackCount(StateTag);
  const int32 MaxCount = FMath::Max(1, Effect.StackBonus.Num());
  const int32 Count = Effect.StackRule == EDsEffectStackRule::Accumulate
                          ? FMath::Min(PreviousCount + 1, MaxCount)
                          : 1;
  StackCounts.Add(StateTag, Count);
  BaseTimedEffects.Add(StateTag, Effect);
  const FDsResolvedEffect Scaled =
      ScaleForStack(BaseTimedEffects.FindChecked(StateTag), Count);
  // 보호막 재적용 시 흡수 풀을 효과 크기로 다시 채운다.
  if (IsShieldEffect(Scaled) && Scaled.Magnitude > 0)
    { ShieldPool = Scaled.Magnitude; ShieldTag = StateTag; }
  // 한시 효과와 영구 효과를 별도 맵에 보관해 만료 간섭을 막는다.
  TimedEffects.Add(StateTag, Scaled);
  if (Scaled.ProcInstantDamageCurHpPct > 0)
    ApplyDirectDamage(GetCurrentHealth() * Scaled.ProcInstantDamageCurHpPct * .01f, Causer);
  OnEffectApplied(Scaled, Causer);
  // 같은 한시 효과를 재적용하면 태그 수는 유지하고 남은 시간만 갱신한다. 주기 위상은 유지한다.
  if (!EffectClocks.Contains(StateTag))
    AddLooseGameplayTag(StateTag);
  EffectClocks.FindOrAdd(StateTag).Remaining = Scaled.Duration;
  StatusTickCausers.Add(StateTag, TWeakObjectPtr<AActor>(Causer));
}

bool UDsAbilitySystemComponent::ApplyEffectById(
    int32 EffectId, UDsAbilitySystemComponent *TargetASC, AActor *Causer,
    const UDrTableSubsystem *Tables) {
  const UDrTableSubsystem *TableData = Tables ? Tables : UDrTableSubsystem::Get();
  if (!TableData) {
    UE_LOG(LogDs, Warning,
           TEXT("[Effect] 테이블 저장소가 없어 효과를 적용할 수 없습니다: Id=%d"),
           EffectId);
    return false;
  }

  const FDsEffectDefsRow *Row =
      TableData->FindRowByKey<FDsEffectDefsRow>(EffectId);
  if (!Row) {
    UE_LOG(LogDs, Warning, TEXT("[Effect] 효과 Id를 찾지 못했습니다: %d"),
           EffectId);
    return false;
  }

  UDsAbilitySystemComponent *DestinationASC = nullptr;
  switch (Row->ApplyTo) {
  case EDsEffectApplyTo::Caster:
    DestinationASC = this;
    break;
  case EDsEffectApplyTo::Victim:
    if (!TargetASC) {
      UE_LOG(LogDs, Warning,
             TEXT("[Effect] 피격 대상 ASC가 없어 효과를 적용하지 않습니다: Id=%d"),
             EffectId);
      return false;
    }
    DestinationASC = TargetASC;
    break;
  default:
    UE_LOG(LogDs, Warning,
           TEXT("[Effect] 알 수 없는 ApplyTo 값입니다: Id=%d, ApplyTo=%d"),
           EffectId, static_cast<int32>(Row->ApplyTo));
    return false;
  }

  const FDsResolvedEffect Effect = DsEffects::Resolve(*Row);
  // ApplyStatusEffect가 조용히 거부하는 조건을 먼저 검사해 성공 여부를 보존한다.
  if (!DestinationASC->GetWorld() || !Effect.StateTag.IsValid() ||
      Effect.Duration <= 0) {
    UE_LOG(LogDs, Warning,
           TEXT("[Effect] 적용 조건을 충족하지 못했습니다: Id=%d, Tag=%s, Duration=%g"),
           EffectId, *Effect.StateTag.ToString(), Effect.Duration);
    return false;
  }

  DestinationASC->ApplyStatusEffect(Effect, Causer);
  return true;
}

float UDsAbilitySystemComponent::AbsorbWithShield(float Damage) {
  if (Damage <= 0 || ShieldPool <= 0)
    return Damage;
  const float AbsorbedDamage = FMath::Min(Damage, ShieldPool);
  ShieldPool -= AbsorbedDamage;
  if (ShieldPool <= 0) {
    // 풀이 소진되면 지속시간이 남아도 보호막을 즉시 해제한다.
    ShieldPool = 0;
    RemoveTimedEffect(ShieldTag);
  }
  return Damage - AbsorbedDamage;
}

bool UDsAbilitySystemComponent::TryActivateAbilityByTag(
    const FGameplayTag &AbilityTag) {
  FGameplayTagContainer AbilityTags(AbilityTag);
  return TryActivateAbilitiesByTag(AbilityTags);
}

void UDsAbilitySystemComponent::SetLooseState(const FGameplayTag &StateTag,
                                              bool bEnabled) {
  SetLooseGameplayTagCount(StateTag, bEnabled ? 1 : 0);
}

void UDsAbilitySystemComponent::TickActiveAbilities(float DeltaTime) {
  // 틱 도중 어빌리티가 끝나거나 취소될 수 있으므로 인스턴스를 먼저 모은다.
  TArray<TWeakObjectPtr<UDsGameplayAbility>, TInlineAllocator<4>> Active;
  for (const FGameplayAbilitySpec &Spec : GetActivatableAbilities())
    if (Spec.IsActive())
      if (UDsGameplayAbility *Ability =
              Cast<UDsGameplayAbility>(Spec.GetPrimaryInstance()))
        Active.Add(Ability);
  for (const TWeakObjectPtr<UDsGameplayAbility> &Ability : Active)
    if (Ability.IsValid() && Ability->IsActive())
      Ability->AbilityTick(DeltaTime);
}
void UDsAbilitySystemComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) {
 Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
 TickActiveAbilities(DeltaTime); TickTimedEffects(DeltaTime); TickCooldowns(DeltaTime);
 TickResources(DeltaTime);
}
void UDsAbilitySystemComponent::ApplyDamage(UAbilitySystemComponent *TargetASC,
                                            float Damage, float PoiseDamage,
                                            AActor *Causer) {
  if (!TargetASC || Damage <= 0)
    return;
  // 공격자 측 "주는 피해" 배수를 적용한다.
  Damage *= GetOutgoingDamageMultiplier();
  FGameplayEffectContextHandle Context = MakeEffectContext();
  Context.AddSourceObject(Causer);
  if (AActor *InstigatorActor = GetOwnerActor())
    Context.AddInstigator(InstigatorActor,
                          Causer ? Causer : InstigatorActor);
  FGameplayEffectSpecHandle Spec =
      MakeOutgoingSpec(UDsGameplayEffect_Damage::StaticClass(), 1, Context);
  if (Spec.IsValid()) {
    Spec.Data->SetSetByCallerMagnitude(DsTags::Data_Damage, Damage);
    Spec.Data->SetSetByCallerMagnitude(DsTags::Data_PoiseDamage, PoiseDamage);
    ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
  }
}

void UDsAbilitySystemComponent::ApplyDirectDamage(float Damage,
                                                  AActor *Causer) {
  ApplyDamage(this, Damage, 0, Causer);
}

void UDsAbilitySystemComponent::ApplyDirectPoiseDamage(float Damage,
                                                       AActor *Causer) {
  if (Damage <= 0)
    return;
  FGameplayEffectContextHandle Context = MakeEffectContext();
  Context.AddSourceObject(Causer);
  FGameplayEffectSpecHandle Spec =
      MakeOutgoingSpec(UDsGameplayEffect_Damage::StaticClass(), 1, Context);
  if (Spec.IsValid()) {
    Spec.Data->SetSetByCallerMagnitude(DsTags::Data_Damage, 0);
    Spec.Data->SetSetByCallerMagnitude(DsTags::Data_PoiseDamage, Damage);
    ApplyGameplayEffectSpecToSelf(*Spec.Data);
  }
}

void UDsAbilitySystemComponent::SpendStamina(float Amount) {
  const UDsAttributeSet *Attributes = GetSet<UDsAttributeSet>();
  if (Attributes && Amount > 0) {
    SetNumericAttributeBase(UDsAttributeSet::GetStaminaAttribute(),
                            FMath::Max(0.f, Attributes->GetStamina() - Amount));
    StaminaRegenDelayRemaining = UDsPlayerCombatData::Get()->StaminaRegenDelayAfterSpend;
  }
}

float UDsAbilitySystemComponent::GetStamina() const {
  const UDsAttributeSet *Attributes = GetSet<UDsAttributeSet>();
  return Attributes ? Attributes->GetStamina() : 0;
}

void UDsAbilitySystemComponent::PushStaminaRegenDelay(float Seconds) {
  StaminaRegenDelayRemaining = FMath::Max(StaminaRegenDelayRemaining, Seconds);
}

void UDsAbilitySystemComponent::ApplyHeal(float Amount) {
  const UDsAttributeSet *Attributes = GetSet<UDsAttributeSet>();
  if (Attributes && Amount > 0)
    SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(),
                            FMath::Min(Attributes->GetMaxHealth(),
                                       Attributes->GetHealth() + Amount));
}

void UDsAbilitySystemComponent::TakePoiseDamage(float Amount, AActor *Causer) {
  if (Amount > 0)
    ApplyDirectPoiseDamage(Amount, Causer);
}
float UDsAbilitySystemComponent::GetCurrentHealth() const { const auto* A=GetSet<UDsAttributeSet>(); return A ? A->GetHealth() : 0; }
bool UDsAbilitySystemComponent::IsShieldEffect(const FDsResolvedEffect& Effect) const { return Effect.StateTag == DsTags::State_Buff_Shield; }

void UDsAbilitySystemComponent::OnEffectApplied(const FDsResolvedEffect& Effect,AActor* Causer) {
 if(const auto* Row=FDsEffectExtensionsRow::Find(Effect.DefinitionId)) {
  float Strength=1;
  const int32 Index=GetStackCount(Effect.StateTag)-1;
  if(Effect.StackRule==EDsEffectStackRule::Accumulate && Effect.StackBonus.IsValidIndex(Index)) Strength+=Effect.StackBonus[Index]*.01f;
  ApplyDirectPoiseDamage(Row->ProcPoiseDamage*Strength,Causer);
 }
}

void UDsAbilitySystemComponent::TickResources(float DeltaTime) {
  StaminaRegenDelayRemaining = FMath::Max(0.f, StaminaRegenDelayRemaining - DeltaTime);
  const UDsAttributeSet *Attributes = GetSet<UDsAttributeSet>();
  // 죽은 액터는 아무것도 재생하지 않는다.
  if (!Attributes || HasMatchingGameplayTag(DsTags::State_Dead))
    return;
  // 스태미나는 소모 후 지연 시간이 지나면 최대치를 향해 재생한다.
  if (StaminaRegenDelayRemaining <= 0.f &&
      Attributes->GetStamina() < Attributes->GetMaxStamina())
    SetNumericAttributeBase(
        UDsAttributeSet::GetStaminaAttribute(),
        FMath::Min(Attributes->GetMaxStamina(),
                   Attributes->GetStamina() +
                       Attributes->GetStaminaRegenRate() * DeltaTime));
  // 포이즈는 최대치를 향해 계속 재생한다.
  if (Attributes->GetPoise() < Attributes->GetMaxPoise())
    SetNumericAttributeBase(
        UDsAttributeSet::GetPoiseAttribute(),
        FMath::Min(Attributes->GetMaxPoise(),
                   Attributes->GetPoise() +
                       Attributes->GetPoiseRegenRate() * DeltaTime));
}
