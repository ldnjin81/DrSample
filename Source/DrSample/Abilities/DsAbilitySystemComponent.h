#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Effects/DsEffectTypes.h"
#include "DsAbilitySystemComponent.generated.h"
class UDrTableSubsystem;
UCLASS(ClassGroup=(Ds), meta=(BlueprintSpawnableComponent))
class DRSAMPLE_API UDsAbilitySystemComponent : public UAbilitySystemComponent {
 GENERATED_BODY()
 friend class FDsEffectSystemTestAccess;
public:
 UDsAbilitySystemComponent();
 virtual bool GetShouldTick() const override { return true; }
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
 virtual void ApplyStatusEffect(const FDsResolvedEffect& Effect,AActor* Causer);
 virtual bool ApplyEffectById(int32 EffectId,UDsAbilitySystemComponent* TargetASC,AActor* Causer,const UDrTableSubsystem* Tables=nullptr);
 void AddPermanentEffect(const FDsResolvedEffect& Effect);
 void RemovePermanentEffect(const FGameplayTag& StateTag);
 float GetMoveSpeedMultiplier() const;
 float GetIncomingDamageMultiplier() const;
 float GetOutgoingDamageMultiplier() const;
 int32 GetStackCount(const FGameplayTag& StateTag) const;
 float AbsorbWithShield(float Damage);
 bool TryActivateAbilityByTag(const FGameplayTag& AbilityTag);
 void SetLooseState(const FGameplayTag& StateTag,bool bEnabled);
 void StartCooldown(const FGameplayTag& CooldownTag,float Duration);
 float GetCooldownRemaining(const FGameplayTag& CooldownTag) const;
 // 샘플 전투 속성(UDsAttributeSet)에 대한 피해·기력·포이즈 처리
 void ApplyDamage(UAbilitySystemComponent* TargetASC,float Damage,float PoiseDamage,AActor* Causer);
 void SpendStamina(float Amount);
 float GetStamina() const;
 void PushStaminaRegenDelay(float Seconds);
 void ApplyHeal(float Amount);
 void TakePoiseDamage(float Amount,AActor* Causer);
protected:
 // 상태 효과가 샘플 속성에 피해를 주고 체력을 읽는 연결부
 void ApplyDirectDamage(float Damage,AActor* Causer);
 void ApplyDirectPoiseDamage(float Damage,AActor* Causer);
 float GetCurrentHealth() const;
 void OnEffectApplied(const FDsResolvedEffect& Effect,AActor* Causer);
 bool IsShieldEffect(const FDsResolvedEffect& Effect) const;
 void RemoveTimedEffect(const FGameplayTag& Tag);
 TMap<FGameplayTag,FDsResolvedEffect> TimedEffects;
private:
 void TickActiveAbilities(float DeltaTime);
 void TickTimedEffects(float DeltaTime);
 void TickCooldowns(float DeltaTime);
 void TickResources(float DeltaTime);
 float AggregateMultiplier(float FDsResolvedEffect::*Field) const;
 TMap<FGameplayTag,FDsResolvedEffect> BaseTimedEffects;
 TMap<FGameplayTag,int32> StackCounts;
 TMap<FGameplayTag,FDsResolvedEffect> PermanentEffects;
 struct FEffectClock { float Remaining=0; float SinceTick=0; };
 TMap<FGameplayTag,FEffectClock> EffectClocks;
 TMap<FGameplayTag,float> CooldownRemaining;
 TMap<FGameplayTag,TWeakObjectPtr<AActor>> StatusTickCausers;
 float ShieldPool=0;
 float StaminaRegenDelayRemaining=0;
 FGameplayTag ShieldTag;
};
