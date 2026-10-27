#include "Boss/DsBossBrainComponent.h"
#include "Combat/DsCombatFXComponent.h"
#include "Audio/DsAudioSubsystem.h"

#include "Boss/DsBossCharacter.h"
#include "Boss/DsBossData.h"
#include "Combat/DsHitFeedbackSubsystem.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Characters/DsCharacter.h"
#include "Core/DsGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

namespace { const FName BossMoveName(TEXT("DrBossMove")); }
DEFINE_LOG_CATEGORY_STATIC(LogDsBossBrain, Log, All);
UDsBossBrainComponent::UDsBossBrainComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}
ADsBossCharacter* UDsBossBrainComponent::Boss() const { return Cast<ADsBossCharacter>(GetOwner()); }
void UDsBossBrainComponent::BeginPlay() {
  Super::BeginPlay();
  if (!bInjected) {
    TArray<FDsBossPhaseRow> LoadedPhases;
    TArray<FDsBossAttackRow> LoadedAttacks;
    for (const auto& Row : FDsBossPhaseRow::GetAll())
      if (Boss() && Row.Boss == Boss()->GetData()->BossName) LoadedPhases.Add(Row);
    for (const auto& Row : FDsBossAttackRow::GetAll()) LoadedAttacks.Add(Row);
    InjectTables(LoadedPhases, LoadedAttacks);
  }
  if (Boss()) Boss()->PlaySequence(Boss()->GetData()->IdleAnim, true);
}
void UDsBossBrainComponent::InjectTables(const TArray<FDsBossPhaseRow>& InPhases, const TArray<FDsBossAttackRow>& InAttacks) {
  StopMove();
  Phases = InPhases;
  Phases.Sort([](const auto& A, const auto& B) { return A.Phase < B.Phase; });
  Attacks = InAttacks;
  PhaseIndex = 0; PendingPhase = INDEX_NONE; AttackIndex = INDEX_NONE;
  CooldownUntil.Reset(); HitActors.Reset(); Clock = 0;
  bInjected = true;
  SetState(EDsBossState::Idle);
}
bool UDsBossBrainComponent::HasLivingTarget() const {
  if (!Target.IsValid()) return false;
  const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target.Get());
  return !ASC || (!ASC->HasMatchingGameplayTag(DsTags::State_Dead) &&
      ASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()) > 0);
}
void UDsBossBrainComponent::ResetEncounter() {
  StopMove();
  if (auto* FX = Boss()->FindComponentByClass<UDsCombatFXComponent>()) FX->Reset();
  PhaseIndex = 0;
  PendingPhase = INDEX_NONE;
  AttackIndex = INDEX_NONE;
  CooldownUntil.Reset();
  HitActors.Reset();
  Clock = 0;
  StateElapsed = 0;
  LockedDirection = FVector::ForwardVector;
  bDirectionLocked = false;
  SetState(EDsBossState::Idle);
}

bool UDsBossBrainComponent::IsFacingTarget() const {
  if (!Target.IsValid() || !Boss())
    return false;
  const FVector Direction = (Target->GetActorLocation() - Boss()->GetActorLocation()).GetSafeNormal2D();
  return Direction.IsNearlyZero() || FVector::DotProduct(Boss()->GetActorForwardVector(), Direction) >=
      FMath::Cos(FMath::DegreesToRadians(Boss()->GetData()->AttackFacingAngle));
}

void UDsBossBrainComponent::WarnMissingTables() {
  if (bWarnedMissingTables)
    return;
  bool bHasPatterns = Phases.IsValidIndex(PhaseIndex);
  if (bHasPatterns)
    bHasPatterns = Attacks.ContainsByPredicate([this](const FDsBossAttackRow &Row) {
      return Row.PatternSet == Phases[PhaseIndex].PatternSet;
    });
  if (!bHasPatterns) {
    bWarnedMissingTables = true;
    UE_LOG(LogDsBossBrain, Warning, TEXT("보스 페이즈 또는 공격 행이 비어 있습니다. 테이블 로드를 확인하세요."));
  }
}
int32 UDsBossBrainComponent::PickAttack(float Distance) {
  WarnMissingTables();
  if (!Phases.IsValidIndex(PhaseIndex)) return INDEX_NONE;
  TArray<int32> Eligible;
  int32 Total = 0;
  for (int32 I=0; I<Attacks.Num(); ++I) {
    const auto& Row = Attacks[I];
    if (State == EDsBossState::Approach && !IsFacingTarget() &&
        Row.Kind != EDsBossAttackKind::Area && Row.Kind != EDsBossAttackKind::Siphon)
      continue;
    const double* Until = CooldownUntil.Find(Row.Id);
    if (Row.PatternSet != Phases[PhaseIndex].PatternSet || Row.Weight <= 0 ||
        Distance < Row.MinRange || Distance > Row.MaxRange || (Until && *Until > Clock)) continue;
    Eligible.Add(I); Total += Row.Weight;
  }
  if (Total <= 0) return INDEX_NONE;
  int32 Draw = Random.RandRange(1, Total);
  for (int32 I : Eligible) { Draw -= Attacks[I].Weight; if (Draw <= 0) return I; }
  return INDEX_NONE;
}
void UDsBossBrainComponent::CheckPhaseThreshold() {
  ADsBossCharacter* Owner = Boss();
  if (!Owner || State == EDsBossState::Dead || Owner->GetAttributes()->GetHealth() <= 0) return;
  const float Rate = Owner->GetAttributes()->GetHealth() / FMath::Max(1.f, Owner->GetAttributes()->GetMaxHealth());
  if (Phases.IsValidIndex(PhaseIndex+1) && Rate <=
      static_cast<float>(Phases[PhaseIndex+1].EnterHpRate) / FDsBossPhaseRow::EnterHpRateScale)
    PendingPhase = PhaseIndex+1;
}
void UDsBossBrainComponent::SetState(EDsBossState InState) {
  ADsBossCharacter* Owner = Boss();
  if (!Owner) return;
  StopMove();
  if (auto* FX = Owner->FindComponentByClass<UDsCombatFXComponent>()) {
    FX->EndTelegraph();
    if (InState == EDsBossState::PhaseTransition) FX->EnterPhaseTwo();
    if (InState == EDsBossState::Dead) FX->Reset();
  }
  State = InState; StateElapsed = 0;
  UDsAbilitySystemComponent* ASC = Owner->GetDrASC();
  ASC->SetLooseState(DsTags::State_Attacking, State == EDsBossState::Attack);
  ASC->SetLooseState(DsTags::State_Groggy, State == EDsBossState::Groggy);
  ASC->SetLooseState(DsTags::State_Invulnerable, State == EDsBossState::PhaseTransition);
  ASC->SetLooseState(DsTags::State_Dead, State == EDsBossState::Dead);
  FName Anim = Owner->GetData()->IdleAnim;
  bool bLoop = true;
  if (State == EDsBossState::Approach) Anim = Owner->GetData()->MoveAnim;
  if (State == EDsBossState::Groggy) { Anim = Owner->GetData()->GroggyAnim; bLoop = false; }
  if (State == EDsBossState::Dead) { Anim = Owner->GetData()->DeathAnim; bLoop = false; Owner->GetCharacterMovement()->DisableMovement(); }
  if (State != EDsBossState::Attack) { AttackIndex = INDEX_NONE; Owner->PlaySequence(Anim,bLoop); }
}
void UDsBossBrainComponent::EnterGroggy() {
  if (State == EDsBossState::Dead || State == EDsBossState::PhaseTransition || State == EDsBossState::Groggy) return;
  SetState(EDsBossState::Groggy);
  Boss()->GetDrASC()->SetNumericAttributeBase(UDsAttributeSet::GetPoiseAttribute(),0.f);
}
void UDsBossBrainComponent::EnterDead() {
  if (State == EDsBossState::Dead) return;
  PendingPhase = INDEX_NONE;
  SetState(EDsBossState::Dead);
  Boss()->GetDrASC()->CancelAllAbilities();
}
void UDsBossBrainComponent::TurnToTarget(float Seconds) {
  if (!Target.IsValid()) return;
  const FVector Direction = (Target->GetActorLocation() - Boss()->GetActorLocation()).GetSafeNormal2D();
  if (Direction.IsNearlyZero()) return;
  const float Yaw = FMath::FixedTurn(Boss()->GetActorRotation().Yaw,Direction.Rotation().Yaw,Boss()->GetData()->TurnRate*Seconds);
  Boss()->SetActorRotation(FRotator(0,Yaw,0));
}
void UDsBossBrainComponent::BeginAttack(int32 Index) {
  SetState(EDsBossState::Attack);
  AttackIndex = Index; HitActors.Reset(); bDirectionLocked = false;
  const auto& Row = Attacks[Index];
  CooldownUntil.Add(Row.Id, Clock + Row.Cooldown.GetTotalSeconds());
  Boss()->PlaySequence(Row.Anim,false);
  if (auto* FX = Boss()->FindComponentByClass<UDsCombatFXComponent>()) FX->BeginTelegraph(Row);
}
void UDsBossBrainComponent::StartMove(float Duration) {
  const auto* Row = GetCurrentAttack();
  if (!Row || Row->MoveDistance <= 0 || Duration <= 0) return;
  auto Move = MakeShared<FRootMotionSource_ConstantForce>();
  Move->InstanceName = BossMoveName;
  Move->AccumulateMode = ERootMotionAccumulateMode::Override;
  Move->Priority = 5; Move->Duration = Duration;
  Move->Force = LockedDirection * (Row->MoveDistance / Duration);
  Move->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
  Move->FinishVelocityParams.ClampVelocity = 0;
  Boss()->GetCharacterMovement()->ApplyRootMotionSource(Move);
}
void UDsBossBrainComponent::StopMove() {
  if (Boss()) {
    auto* Movement = Boss()->GetCharacterMovement();
    Movement->RemoveRootMotionSource(BossMoveName);
    Movement->StopMovementImmediately();
    Boss()->ConsumeMovementInputVector();
  }
}
void UDsBossBrainComponent::ApplyHits() {
  const auto* Current = GetCurrentAttack();
  if (!Current || !GetWorld()) return;
  // 피해 델리게이트가 그로기·사망으로 재진입해도 이번 판정 데이터는 유지한다.
  const FDsBossAttackRow Row = *Current;
  TArray<FOverlapResult> Overlaps;
  const FVector Center = Boss()->GetActorLocation()+LockedDirection*Row.HitForwardOffset;
  GetWorld()->OverlapMultiByObjectType(Overlaps,Center,FQuat::Identity,FCollisionObjectQueryParams(ECC_Pawn),
      FCollisionShape::MakeSphere(Row.HitRadius),FCollisionQueryParams(SCENE_QUERY_STAT(DrBossHit),false,Boss()));
  for (const auto& Overlap : Overlaps) {
    if (State != EDsBossState::Attack) break;
    AActor* Actor = Overlap.GetActor();
    if (!Actor || Actor == Boss() || HitActors.Contains(Actor)) continue;
    auto* TargetASC = Cast<UDsAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor));
    if (!TargetASC || TargetASC->HasMatchingGameplayTag(DsTags::State_Dead)) continue;
    HitActors.Add(Actor);
    const float Before = TargetASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute());
    Boss()->GetDrASC()->ApplyDamage(TargetASC,Row.Damage,Row.PoiseDamage,Boss());
    // 쳐내기·무적으로 피해를 막은 대상에서는 흡수 회복과 부가 효과도 발생하지 않는다.
    if (TargetASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()) < Before) {
      GetWorld()->GetSubsystem<UDsHitFeedbackSubsystem>()->PlayHit(Boss(), Actor, EDsHitWeight::Boss);
      TargetASC->SpendStamina(Row.StaminaDamage);
      if (Row.Effect != 0) Boss()->GetDrASC()->ApplyEffectById(Row.Effect,TargetASC,Boss());
      if (State != EDsBossState::Dead) Boss()->GetDrASC()->ApplyHeal(Row.Heal);
    }
  }
}
void UDsBossBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
  ADsBossCharacter* Owner = Boss();
  if (!Owner || !Owner->HasAuthority() || State == EDsBossState::Dead || DeltaTime <= 0) return;
  Clock += DeltaTime;
  const double Previous = StateElapsed;
  StateElapsed += DeltaTime;
  if (!Target.IsValid()) if (TActorIterator<ADsCharacter> It(GetWorld()); It) Target = *It;
  CheckPhaseThreshold();
  if (PendingPhase != INDEX_NONE && State != EDsBossState::Attack && State != EDsBossState::Groggy && State != EDsBossState::PhaseTransition) {
    SetState(EDsBossState::PhaseTransition); return;
  }
  switch (State) {
  case EDsBossState::Idle:
    if (bEngageAllowed && HasLivingTarget() && FVector::Dist2D(Target->GetActorLocation(),Owner->GetActorLocation()) <= Owner->GetData()->AggroRadius)
      SetState(EDsBossState::Approach);
    break;
  case EDsBossState::Approach: {
    if (!HasLivingTarget()) { SetState(EDsBossState::Idle); break; }
    TurnToTarget(DeltaTime);
    const float Distance = FVector::Dist2D(Target->GetActorLocation(),Owner->GetActorLocation());
    const int32 Picked = PickAttack(Distance);
    if (Picked != INDEX_NONE) { BeginAttack(Picked); break; }
    if (!IsFacingTarget()) break;
    const float Rate = Phases.IsValidIndex(PhaseIndex) ? static_cast<float>(Phases[PhaseIndex].MoveSpeedRate)/FDsBossPhaseRow::MoveSpeedRateScale : 1.f;
    Owner->GetCharacterMovement()->MaxWalkSpeed = Owner->GetData()->BaseMoveSpeed*Rate*Owner->GetDrASC()->GetMoveSpeedMultiplier();
    Owner->AddMovementInput((Target->GetActorLocation()-Owner->GetActorLocation()).GetSafeNormal2D());
    break;
  }
  case EDsBossState::Attack: {
    if (!HasLivingTarget()) { SetState(EDsBossState::Recover); break; }
    const auto* Row = GetCurrentAttack();
    if (!Row) { SetState(EDsBossState::Recover); break; }
    const double Telegraph = Row->TelegraphTime.GetTotalSeconds();
    const double Start = Row->ActiveStart.GetTotalSeconds();
    if (auto* FX = Owner->FindComponentByClass<UDsCombatFXComponent>()) {
      FX->AdvanceTelegraph(StateElapsed, Telegraph);
      if (Previous <= Start && StateElapsed >= Start && (Previous < Start || Previous == 0.0)) FX->PlayAttackImpact(*Row);
    }
    if (!bDirectionLocked) {
      TurnToTarget(static_cast<float>(FMath::Max(0.0,FMath::Min(StateElapsed,Telegraph)-Previous)));
      if (StateElapsed >= Telegraph) {
        bDirectionLocked = true; LockedDirection = Owner->GetActorForwardVector();
        StartMove(static_cast<float>(FMath::Max(0.0,Start-Telegraph)));
      }
    }
    if (Previous <= Start && StateElapsed >= Start && (Previous < Start || Previous == 0.0))
      GetWorld()->GetSubsystem<UDsAudioSubsystem>()->PlayBossAttack(Owner, Row->Kind);
    if (Row->Kind == EDsBossAttackKind::Slam && Previous < Start && StateElapsed >= Start)
      GetWorld()->GetSubsystem<UDsHitFeedbackSubsystem>()->PlaySlam(Owner);
    const double Duration = Row->Duration.GetTotalSeconds();
    if (Previous < Row->ActiveEnd.GetTotalSeconds() && StateElapsed >= Start) ApplyHits();
    if (State == EDsBossState::Attack && StateElapsed >= Duration) SetState(EDsBossState::Recover);
    break;
  }
  case EDsBossState::Recover:
    if (StateElapsed >= Owner->GetData()->RecoverDuration) SetState(EDsBossState::Approach);
    break;
  case EDsBossState::Groggy:
    if (StateElapsed >= Owner->GetData()->GroggyDuration) {
      Owner->GetDrASC()->SetNumericAttributeBase(UDsAttributeSet::GetPoiseAttribute(),Owner->GetAttributes()->GetMaxPoise());
      SetState(EDsBossState::Approach);
    }
    break;
  case EDsBossState::PhaseTransition:
    if (StateElapsed >= Owner->GetData()->PhaseTransitionDuration) {
      PhaseIndex = PendingPhase; PendingPhase = INDEX_NONE;
      GetWorld()->GetSubsystem<UDsAudioSubsystem>()->SetPhase(2);
      SetState(EDsBossState::Approach);
    }
    break;
  case EDsBossState::Dead: break;
  }
}
void UDsBossBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  StopMove(); Target.Reset(); HitActors.Reset();
  Super::EndPlay(EndPlayReason);
}
