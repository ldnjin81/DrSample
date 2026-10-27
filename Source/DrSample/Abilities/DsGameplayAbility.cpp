#include "Abilities/DsGameplayAbility.h"
// 공통 어빌리티 비용과 재사용 대기시간 처리를 구현한다.
#include "Abilities/DsAbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/DsCharacter.h"
#include "Core/DsGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Engine/World.h"
UDsGameplayAbility::UDsGameplayAbility() {
  InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
  NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}
bool UDsGameplayAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayTagContainer *SourceTags,
    const FGameplayTagContainer *TargetTags,
    FGameplayTagContainer *OptionalRelevantTags) const {
  // 쿨다운 태그가 ASC에 있으면 발동할 수 없다.
  if (CooldownTag.IsValid())
    if (const UDsAbilitySystemComponent *AbilitySystem = GetDsASC())
      if (AbilitySystem->HasMatchingGameplayTag(CooldownTag))
        return false;
  if (!HasEnoughStamina())
    return false;
  return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags,
                                   OptionalRelevantTags);
}
AActor *UDsGameplayAbility::GetDsActor() const {
  return GetAvatarActorFromActorInfo();
}
UDsAbilitySystemComponent *UDsGameplayAbility::GetDsASC() const {
  return Cast<UDsAbilitySystemComponent>(
      GetAbilitySystemComponentFromActorInfo());
}
bool UDsGameplayAbility::HasEnoughStamina() const {
  const float Cost = GetStaminaCost();
  return Cost <= 0 || (GetDsASC() && GetDsASC()->GetStamina() >= Cost);
}
bool UDsGameplayAbility::CommitStamina() {
  if (!HasEnoughStamina())
    return false;
  if (GetStaminaCost() > 0)
    GetDsASC()->SpendStamina(GetStaminaCost());
  return true;
}
void UDsGameplayAbility::CommitCooldown() {
  if (UDsAbilitySystemComponent *AbilitySystem = GetDsASC())
    AbilitySystem->StartCooldown(CooldownTag, CooldownDuration);
}
float UDsGameplayAbility::GetDsCooldownRemaining() const {
  const UDsAbilitySystemComponent *AbilitySystem = GetDsASC();
  return AbilitySystem ? AbilitySystem->GetCooldownRemaining(CooldownTag) : 0.f;
}
namespace {
const FName DsConstantMoveName(TEXT("DsConstantMove"));
}
void UDsGameplayAbility::ApplyConstantMove(const FVector &Direction,
                                           float Distance, float Duration,
                                           float EndSpeed) {
  ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
  UCharacterMovementComponent *Movement =
      Character ? Character->GetCharacterMovement() : nullptr;
  const FVector Dir = Direction.GetSafeNormal2D();
  if (!Movement || Distance <= 0 || Duration <= 0 || Dir.IsNearlyZero())
    return;
  ClearConstantMove();
  TSharedPtr<FRootMotionSource_ConstantForce> Move =
      MakeShared<FRootMotionSource_ConstantForce>();
  Move->InstanceName = DsConstantMoveName;
  Move->AccumulateMode = ERootMotionAccumulateMode::Override;
  Move->Priority = 5;
  Move->Force = Dir * (Distance / Duration);
  Move->Duration = Duration;
  Move->FinishVelocityParams.Mode =
      ERootMotionFinishVelocityMode::ClampVelocity;
  Move->FinishVelocityParams.ClampVelocity = EndSpeed;
  Movement->ApplyRootMotionSource(Move);
}
void UDsGameplayAbility::ClearConstantMove() {
  if (ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    if (UCharacterMovementComponent *Movement = Character->GetCharacterMovement())
      Movement->RemoveRootMotionSource(DsConstantMoveName);
}
UAnimMontage *UDsGameplayAbility::PlayAbilityMontage(
    const TSoftObjectPtr<UAnimMontage> &Montage, float PlayRate) {
  ADsCharacter *Character = Cast<ADsCharacter>(GetAvatarActorFromActorInfo());
  if (!Character || !Character->CanPlayCombatMontage() || Montage.IsNull())
    return nullptr;
  PlayingMontage = Montage.LoadSynchronous();
  if (!PlayingMontage)
    return nullptr;
  Character->PlayCombatMontage(PlayingMontage, false, PlayRate);
  return PlayingMontage;
}
void UDsGameplayAbility::StopAbilityMontage() {
  if (ADsCharacter *Character = Cast<ADsCharacter>(GetAvatarActorFromActorInfo()))
    if (PlayingMontage)
      Character->StopCombatMontage(PlayingMontage);
  PlayingMontage = nullptr;
}
void UDsGameplayAbility::BlockDuringCombatStates() {
  ActivationBlockedTags.AddTag(DsTags::State_Dead);
  ActivationBlockedTags.AddTag(DsTags::State_Staggered);
  ActivationBlockedTags.AddTag(DsTags::State_Attacking);
  ActivationBlockedTags.AddTag(DsTags::State_Guarding);
  ActivationBlockedTags.AddTag(DsTags::State_Parrying);
  ActivationBlockedTags.AddTag(DsTags::State_Dodging);
}
