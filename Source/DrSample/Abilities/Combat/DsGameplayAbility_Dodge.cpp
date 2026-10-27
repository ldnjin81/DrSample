#include "Abilities/Combat/DsGameplayAbility_Dodge.h"
// 구르기 회피의 방향 선택, 이동, 무적 구간을 구현한다.
#include "Combat/DsCombatFXComponent.h"

#include "Abilities/DsAbilitySystemComponent.h"
#include "Combat/DsPlayerCombatData.h"
#include "Animation/AnimMontage.h"
#include "Core/DsGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDsGameplayAbility_Dodge::UDsGameplayAbility_Dodge() {
  SetAssetTags(FGameplayTagContainer(DsTags::Ability_Dodge));
  BlockDuringCombatStates();
}

float UDsGameplayAbility_Dodge::GetStaminaCost() const {
  return UDsPlayerCombatData::Get()->Dodge.StaminaCost;
}

int32 UDsGameplayAbility_Dodge::SelectRollIndex(const FVector &LocalDirection) {
  // 액터 공간은 X가 앞, Y가 오른쪽이라 각도가 시계 방향으로 커진다.
  const float Degrees = FMath::RadiansToDegrees(
      FMath::Atan2(LocalDirection.Y, LocalDirection.X));
  return (FMath::RoundToInt(Degrees / 45.f) + 8) % 8;
}

void UDsGameplayAbility_Dodge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData *TriggerEventData) {
  Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
  if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !CommitStamina()) {
    EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    return;
  }
  const FDsDodgeTuning &Tuning = UDsPlayerCombatData::Get()->Dodge;
  Elapsed = 0.f;
  ActiveDuration = Tuning.Duration;
  if (auto* FX = GetAvatarActorFromActorInfo()->FindComponentByClass<UDsCombatFXComponent>()) FX->SetDash(true);
  GetDsASC()->SetLooseState(DsTags::State_Dodging, true);
  if (const ACharacter *Character =
          Cast<ACharacter>(GetAvatarActorFromActorInfo())) {
    // 이동 입력이 있으면 그 방향, 없으면 뒤로 물러난다.
    FVector Direction =
        Character->GetCharacterMovement()->GetCurrentAcceleration();
    if (Direction.IsNearlyZero())
      Direction = Character->GetLastMovementInputVector();
    if (Direction.IsNearlyZero())
      Direction = -Character->GetActorForwardVector();
    const FVector Local = Character->GetActorTransform().InverseTransformVectorNoScale(
        Direction.GetSafeNormal2D());
    const int32 Index = SelectRollIndex(Local);
    // 구르기 모션은 이동이 뼈에 들어 있어 캐릭터가 모션을 따라 캡슐을 옮긴다.
    if (Tuning.RollMontages.IsValidIndex(Index) && Tuning.RollPlayRate > 0.f)
      if (const UAnimMontage *Played = PlayAbilityMontage(Tuning.RollMontages[Index], Tuning.RollPlayRate)) {
        ActiveDuration = Played->GetPlayLength() / Tuning.RollPlayRate;
        return;
      }
    ApplyConstantMove(Direction, Tuning.Distance, Tuning.Duration,
                      Character->GetCharacterMovement()->MaxWalkSpeed);
  }
}

void UDsGameplayAbility_Dodge::AbilityTick(float DeltaTime) {
  Elapsed += DeltaTime;
  const FDsDodgeTuning &Tuning = UDsPlayerCombatData::Get()->Dodge;
  SetInvulnerable(Elapsed >= Tuning.InvulnerableStart && Elapsed < Tuning.InvulnerableEnd);
  if (Elapsed >= ActiveDuration)
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UDsGameplayAbility_Dodge::SetInvulnerable(bool bEnabled) {
  UDsAbilitySystemComponent *ASC = GetDsASC();
  // 다른 출처가 건 무적을 지우지 않도록 자신이 건 것만 해제한다.
  if (!ASC || bGrantedInvulnerable == bEnabled)
    return;
  bGrantedInvulnerable = bEnabled;
  if (bEnabled)
    ASC->AddLooseGameplayTag(DsTags::State_Invulnerable);
  else
    ASC->RemoveLooseGameplayTag(DsTags::State_Invulnerable);
}

void UDsGameplayAbility_Dodge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled) {
  if (auto* Avatar = GetAvatarActorFromActorInfo())
    if (auto* FX = Avatar->FindComponentByClass<UDsCombatFXComponent>()) FX->SetDash(false);
  SetInvulnerable(false);
  if (UDsAbilitySystemComponent *ASC = GetDsASC())
    ASC->SetLooseState(DsTags::State_Dodging, false);
  if (bWasCancelled) {
    ClearConstantMove();
    StopAbilityMontage();
  }
  Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
                    bWasCancelled);
}
