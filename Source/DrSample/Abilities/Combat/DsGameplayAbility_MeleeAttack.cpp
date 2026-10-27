#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
// 근접 공격의 타수 진행, 타격 판정, 콤보 입력 버퍼를 구현한다.

#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"
#include "Combat/DsCombatFXComponent.h"
#include "Combat/DsHitFeedbackSubsystem.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Core/DsGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"


const TArray<FDsMeleeStage> &UDsGameplayAbility_MeleeAttack::GetStages() const {
  static const TArray<FDsMeleeStage> Empty;
  return Empty;
}

UDsGameplayAbility_MeleeAttack::UDsGameplayAbility_MeleeAttack() {
  BlockDuringCombatStates();
}

bool UDsGameplayAbility_MeleeAttack::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayTagContainer *SourceTags,
    const FGameplayTagContainer *TargetTags,
    FGameplayTagContainer *OptionalRelevantTags) const {
  const UDsAbilitySystemComponent *ASC =
      ActorInfo ? Cast<UDsAbilitySystemComponent>(
                      ActorInfo->AbilitySystemComponent.Get())
                : nullptr;
  // 첫 타의 스태미나가 없으면 시작하지 않는다.
  if (!ASC || GetStages().IsEmpty() || ASC->GetStamina() < GetStages()[0].StaminaCost)
    return false;
  return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags,
                                   OptionalRelevantTags);
}

void UDsGameplayAbility_MeleeAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData *TriggerEventData) {
  Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
  if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !BeginStage(0)) {
    EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    return;
  }
  GetDsASC()->SetLooseState(DsTags::State_Attacking, true);
}

bool UDsGameplayAbility_MeleeAttack::BeginStage(int32 Index) {
  UDsAbilitySystemComponent *ASC = GetDsASC();
  AActor *Avatar = GetAvatarActorFromActorInfo();
  if (!ASC || !Avatar || !GetStages().IsValidIndex(Index) ||
      ASC->GetStamina() < GetStages()[Index].StaminaCost)
    return false;
  const FDsMeleeStage &Stage = GetStages()[Index];
  ASC->SpendStamina(Stage.StaminaCost);
  StageIndex = Index;
  StageElapsed = 0.f;
  bInputBuffered = false;
  HitActors.Reset();
  StopAbilityMontage();
  PlayAbilityMontage(Stage.Montage);
  // 모션의 이동이 작은 타는 판정 직전까지 짧게 전진시킨다.
  ApplyConstantMove(Avatar->GetActorForwardVector(), Stage.LungeDistance,
                    Stage.ActiveStart, 0.f);
  return true;
}

void UDsGameplayAbility_MeleeAttack::BufferNextInput() {
  if (IsActive() && GetStages().IsValidIndex(StageIndex + 1))
    bInputBuffered = true;
}

void UDsGameplayAbility_MeleeAttack::AbilityTick(float DeltaTime) {
  if (!GetStages().IsValidIndex(StageIndex))
    return;
  const FDsMeleeStage &Stage = GetStages()[StageIndex];
  const float Previous = StageElapsed;
  StageElapsed += DeltaTime;
  if (auto* FX = GetAvatarActorFromActorInfo()->FindComponentByClass<UDsCombatFXComponent>())
    FX->SetSword(StageElapsed >= Stage.ActiveStart && StageElapsed < Stage.ActiveEnd);
  // 틱 간격이 길어도 판정 구간을 건너뛰지 않게 구간 겹침으로 판단한다.
  if (Previous < Stage.ActiveEnd && StageElapsed >= Stage.ActiveStart)
    ApplyHits(Stage);
  if (StageElapsed < Stage.ActiveEnd)
    return;
  if (bInputBuffered) {
    // 스태미나가 모자라면 넘어가지 않고 현재 타를 끝까지 재생한다.
    if (BeginStage(StageIndex + 1))
      return;
    bInputBuffered = false;
  }
  if (StageElapsed >= Stage.Duration)
    Finish(false);
}

void UDsGameplayAbility_MeleeAttack::ApplyHits(const FDsMeleeStage &Stage) {
  UDsAbilitySystemComponent *ASC = GetDsASC();
  AActor *Avatar = GetAvatarActorFromActorInfo();
  UWorld *World = GetWorld();
  const UDsAttributeSet *Attributes =
      ASC ? ASC->GetSet<UDsAttributeSet>() : nullptr;
  if (!Avatar || !World || !Attributes)
    return;
  const FVector Center = Avatar->GetActorLocation() +
                         Avatar->GetActorForwardVector() * UDsPlayerCombatData::Get()->MeleeHitForwardOffset;
  FCollisionQueryParams Params(SCENE_QUERY_STAT(DrMeleeHit), false, Avatar);
  TArray<FOverlapResult> Overlaps;
  World->OverlapMultiByObjectType(
      Overlaps, Center, FQuat::Identity,
      FCollisionObjectQueryParams(ECC_Pawn),
      FCollisionShape::MakeSphere(UDsPlayerCombatData::Get()->MeleeHitRadius), Params);
  const float Damage = Attributes->GetAttackPower() * Stage.DamageRate;
  for (const FOverlapResult &Overlap : Overlaps) {
    AActor *Target = Overlap.GetActor();
    if (!Target || Target == Avatar || HitActors.Contains(Target))
      continue;
    HitActors.Add(Target);
    if (UAbilitySystemComponent *TargetASC =
            UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
    {
      const float Before = TargetASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute());
      ASC->ApplyDamage(TargetASC, Damage, Stage.PoiseDamage, Avatar);
      if (TargetASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()) < Before)
        World->GetSubsystem<UDsHitFeedbackSubsystem>()->PlayHit(Avatar, Target,
          IsA<UDsGameplayAbility_AttackHeavy>() ? EDsHitWeight::Heavy : EDsHitWeight::Light);
    }
  }
}

void UDsGameplayAbility_MeleeAttack::Finish(bool bWasCancelled) {
  EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
             GetCurrentActivationInfo(), true, bWasCancelled);
}

void UDsGameplayAbility_MeleeAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled) {
  if (UDsAbilitySystemComponent *ASC = GetDsASC())
    ASC->SetLooseState(DsTags::State_Attacking, false);
  if (auto* Avatar = GetAvatarActorFromActorInfo())
    if (auto* FX = Avatar->FindComponentByClass<UDsCombatFXComponent>()) FX->SetSword(false);
  ClearConstantMove();
  if (bWasCancelled)
    StopAbilityMontage();
  bInputBuffered = false;
  HitActors.Reset();
  Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
                    bWasCancelled);
}
