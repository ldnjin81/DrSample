#include "Characters/DsCharacter.h"
#include "Combat/DsHitDirection.h"
#include "Combat/DsPlayerCombatData.h"
#include "Combat/DsCombatFXComponent.h"

#include "Combat/DsLockOnComponent.h"
#include "Combat/DsHitFeedbackSubsystem.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"
#include "Abilities/Combat/DsGameplayAbility_AttackLight.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "Abilities/DsGameplayAbility.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Core/DsGameplayTags.h"
#include "Encounter/DsEncounterSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Components/SkeletalMeshComponent.h"
#include "StructUtils/UserDefinedStruct.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogDsCharacter, Log, All);

ADsCharacter::ADsCharacter(const FObjectInitializer& ObjectInitializer)
 : Super(ObjectInitializer) {
  AbilitySystem = CreateDefaultSubobject<UDsAbilitySystemComponent>(TEXT("AbilitySystem"));
  CombatFX = CreateDefaultSubobject<UDsCombatFXComponent>(TEXT("CombatFX"));
  // 경직 시간과 표시 메시 복구를 틱에서 센다.
  PrimaryActorTick.bCanEverTick = true;
  LockOn = CreateDefaultSubobject<UDsLockOnComponent>(TEXT("LockOn"));

  Attributes = CreateDefaultSubobject<UDsAttributeSet>(TEXT("Attributes"));
  AbilitySystem->SetIsReplicated(true);
  AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

  GetCapsuleComponent()->InitCapsuleSize(30.f, 86.f);
  // 이동 속도 기본값. 생성 뒤 RefreshMaxSpeed가 플레이어 전투 데이터로 맞춘다.
  GetCharacterMovement()->MaxWalkSpeed = 500.f;
  GetCharacterMovement()->MaxAcceleration = 500.f;
  GetCharacterMovement()->BrakingDecelerationWalking = 500.f;
  GetCharacterMovement()->GroundFriction = 8.f;
  GetCharacterMovement()->MaxWalkSpeedCrouched = 225.f;
  GetCharacterMovement()->JumpZVelocity = 500.f;
  GetCharacterMovement()->AirControl = .25f;
  GetCharacterMovement()->MaxStepHeight = 45.f;
  GetCharacterMovement()->SetWalkableFloorAngle(44.77f);
  GetCharacterMovement()->SetCrouchedHalfHeight(60.f);
  GetCharacterMovement()->RotationRate = FRotator(0.f, -1.f, 0.f);
  GetCharacterMovement()->bOrientRotationToMovement = true;
  GetCharacterMovement()->bUseControllerDesiredRotation = false;
  GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
  GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
  GetMesh()->SetVisibility(false);
  GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
  MannyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MannyMesh"));
  MannyMesh->SetupAttachment(GetMesh());
  MannyMesh->ComponentTags.Add(TEXT("RTG_UEFN_to_UE5_Mannequin"));
  MannyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(GetRootComponent());
  CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 12.f));
  CameraBoom->TargetArmLength = 300.f;
  CameraBoom->bUsePawnControlRotation = true;
  FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
  FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
  FollowCamera->bUsePawnControlRotation = false;
  FollowCamera->SetFieldOfView(90.f);

}

void ADsCharacter::FillAnimationProperties(int32 &) { checkNoEntry(); }

DEFINE_FUNCTION(ADsCharacter::execFillAnimationProperties) {
  Stack.StepCompiledIn<FStructProperty>(nullptr);
  FStructProperty *Property = CastField<FStructProperty>(Stack.MostRecentProperty);
  void *Data = Stack.MostRecentPropertyAddress;
  P_FINISH;
  P_NATIVE_BEGIN;
  if (Property && Data) {
    P_THIS->FillAnimationPropertiesRaw(Property->Struct, Data);
  }
  P_NATIVE_END;
}

void ADsCharacter::FillAnimationPropertiesRaw(const UScriptStruct *Struct, void *Data) const {
  const UUserDefinedStruct *UserStruct = Cast<UUserDefinedStruct>(Struct);
  if (!UserStruct || !Data) return;
  const UCharacterMovementComponent *Movement = GetCharacterMovement();
  for (TFieldIterator<FProperty> It(Struct); It; ++It) {
    FProperty *Property = *It;
    const FString Name = UserStruct->GetAuthoredNameForField(Property);
    void *Value = Property->ContainerPtrToValuePtr<void>(Data);
    bool bHandled = true;
    if (FStructProperty *StructField = CastField<FStructProperty>(Property)) {
      if (Name == TEXT("InputState")) {
        FillAnimationPropertiesRaw(StructField->Struct, Value);
      } else if (StructField->Struct == TBaseStructure<FTransform>::Get() && Name == TEXT("ActorTransform")) {
        const FTransform Result = GetActorTransform();
        StructField->CopyCompleteValue(Value, &Result);
      } else if (StructField->Struct == TBaseStructure<FVector>::Get()) {
        FVector Result = FVector::ZeroVector;
        if (Name == TEXT("Velocity")) Result = Movement->Velocity;
        else if (Name == TEXT("InputAcceleration")) Result = Movement->GetCurrentAcceleration();
        else if (Name == TEXT("GroundNormal")) Result = Movement->CurrentFloor.HitResult.ImpactNormal;
        else if (Name == TEXT("GroundLocation")) Result = Movement->CurrentFloor.HitResult.ImpactPoint;
        else if (Name == TEXT("LandVelocity")) Result = LastLandVelocity;
        else bHandled = false;
        StructField->CopyCompleteValue(Value, &Result);
      } else if (StructField->Struct == TBaseStructure<FRotator>::Get()) {
        const FRotator Result = Name == TEXT("AimingRotation") ? GetBaseAimRotation() : GetActorRotation();
        if (Name != TEXT("AimingRotation") && Name != TEXT("OrientationIntent")) bHandled = false;
        StructField->CopyCompleteValue(Value, &Result);
      } else bHandled = false;
    } else if (FBoolProperty *BoolField = CastField<FBoolProperty>(Property)) {
      bool Result = false;
      if (Name == TEXT("WantsToWalk")) Result = bWantsToWalk;
      else if (Name == TEXT("WantsToSprint")) Result = bWantsToSprint;
      else if (Name == TEXT("WantsToCrouch")) Result = bIsCrouched;
      else if (Name == TEXT("JustLanded")) Result = GetWorld() && LastLandingTime >= 0.f &&
          GetWorld()->GetTimeSeconds() - LastLandingTime < .2f;
      else if (Name == TEXT("WantsToStrafe")) Result = LockOn && LockOn->GetTarget();
      else if (Name != TEXT("WantsToAim")) bHandled = false;
      BoolField->SetPropertyValue(Value, Result);
    } else if (FNumericProperty *NumberField = CastField<FNumericProperty>(Property)) {
      if (NumberField->IsFloatingPoint()) {
        double Result = 0.0;
        if (Name == TEXT("CurrentMaxAcceleration")) Result = Movement->MaxAcceleration;
        else if (Name == TEXT("CurrentMaxDeceleration")) Result = Movement->BrakingDecelerationWalking;
        else if (Name != TEXT("SteeringTime")) bHandled = false;
        NumberField->SetFloatingPointPropertyValue(Value, Result);
      } else if (FByteProperty *ByteField = CastField<FByteProperty>(Property)) {
        const UEnum *Enum = ByteField->Enum;
        FString Desired;
        if (Name == TEXT("Gait")) Desired = bWantsToSprint ? TEXT("Sprint") : bWantsToWalk ? TEXT("Walk") : TEXT("Run");
        else if (Name == TEXT("Stance")) Desired = bIsCrouched ? TEXT("Crouch") : TEXT("Stand");
        else if (Name == TEXT("MovementMode")) Desired = Movement->IsFalling() ? TEXT("InAir") : TEXT("OnGround");
        else if (Name == TEXT("RotationMode")) Desired = LockOn && LockOn->GetTarget() ? TEXT("Strafe") : TEXT("OrientToMovement");
        else if (Name == TEXT("MovementDirection")) Desired = TEXT("F");
        else bHandled = false;
        if (Enum && !Desired.IsEmpty()) {
          for (int32 Index = 0; Index < Enum->NumEnums(); ++Index) {
            if (Enum->GetDisplayNameTextByIndex(Index).ToString() == Desired) {
              ByteField->SetPropertyValue(Value, static_cast<uint8>(Enum->GetValueByIndex(Index)));
              break;
            }
          }
        }
      } else bHandled = false;
    } else bHandled = false;
    if (!bHandled) {
      static TSet<FString> WarnedFields;
      const FString Key = Struct->GetName() + TEXT(".") + Name;
      if (!WarnedFields.Contains(Key)) {
        WarnedFields.Add(Key);
        UE_LOG(LogDsCharacter, Warning, TEXT("알 수 없는 GASP 애니 속성: %s"), *Key);
      }
    }
  }
}

UAbilitySystemComponent *ADsCharacter::GetAbilitySystemComponent() const {
  return AbilitySystem;
}

void ADsCharacter::InitializeAbilities() {
  if (Attributes && !bStatsInitialized) {
    // 기본 스탯과 특성은 데이터에 있고, 특성 공식으로 전투 스탯을 파생한다.
    Attributes->InitFromStats(UDsPlayerCombatData::Get()->Stats);
    bStatsInitialized = true;
    RefreshMaxSpeed();
  }
  if (AbilitySystem) {
    if (!AbilitySystem->GetSet<UDsAttributeSet>()) {
      AbilitySystem->AddAttributeSetSubobject(Attributes.Get());
    }
    AbilitySystem->InitAbilityActorInfo(this, this);
  }
}
float ADsCharacter::GetWalkSpeed() const { return UDsPlayerCombatData::Get()->WalkSpeed; }
float ADsCharacter::GetRunSpeed() const { return UDsPlayerCombatData::Get()->RunSpeed; }
float ADsCharacter::GetSprintSpeed() const { return UDsPlayerCombatData::Get()->SprintSpeed; }

void ADsCharacter::PostInitializeComponents() {
  Super::PostInitializeComponents();
  FollowVisibleMeshPose();
  InitializeAbilities();
  GrantDefaultAbilities();
  BindCombatEvents();
}

void ADsCharacter::FollowVisibleMeshPose() {
  // 표시 메시 아래의 애님 BP 없는 메시(메타휴먼 의상 등)는 표시 메시 포즈를 그대로 따른다.
  // 얼굴처럼 자체 애님 BP가 있는 메시는 그 BP가 부모 포즈를 복사한다.
  TArray<USceneComponent *> Attached;
  MannyMesh->GetChildrenComponents(true, Attached);
  for (USceneComponent *Child : Attached)
    if (USkeletalMeshComponent *Part = Cast<USkeletalMeshComponent>(Child))
      if (!Part->GetAnimClass() && !Part->LeaderPoseComponent.IsValid())
        Part->SetLeaderPoseComponent(MannyMesh);
}

void ADsCharacter::GrantDefaultAbilities() {
  // 초기화 경로가 여러 번 불려도 한 번만 부여한다.
  if (bAbilitiesGranted || !AbilitySystem || !HasAuthority() ||
      IsTemplate() || !GetWorld() || !GetWorld()->IsGameWorld())
    return;
  bAbilitiesGranted = true;
  for (const TSubclassOf<UDsGameplayAbility> &AbilityClass : DefaultAbilities)
    if (AbilityClass)
      AbilitySystem->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
}

void ADsCharacter::BindCombatEvents() {
  if (bCombatEventsBound || !Attributes)
    return;
  bCombatEventsBound = true;
  Attributes->OnDamaged.AddUObject(this, &ADsCharacter::HandleDamaged);
  Attributes->OnPoiseBroken.AddUObject(this, &ADsCharacter::HandlePoiseBroken);
  Attributes->OnOutOfHealth.AddUObject(this, &ADsCharacter::HandleOutOfHealth);
}

void ADsCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (Attributes) {
    Attributes->OnDamaged.RemoveAll(this);
    Attributes->OnPoiseBroken.RemoveAll(this);
    Attributes->OnOutOfHealth.RemoveAll(this);
  }
  bCombatEventsBound = false;
  Super::EndPlay(EndPlayReason);
}

void ADsCharacter::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  if (StaggerRemaining > 0.f) {
    StaggerRemaining -= DeltaSeconds;
    if (StaggerRemaining <= 0.f && AbilitySystem)
      AbilitySystem->SetLooseState(DsTags::State_Staggered, false);
  }
  FollowCombatMotion(DeltaSeconds);
  if (bPoseOverridden && PoseOverrideRemaining >= 0.f) {
    PoseOverrideRemaining -= DeltaSeconds;
    if (PoseOverrideRemaining < 0.f)
      RestoreAnimBlueprint();
  }
}

bool ADsCharacter::InputAttackLight() {
  if (!AbilitySystem)
    return false;
  // 약공격이 진행 중이면 새로 발동하지 않고 다음 타를 예약한다.
  for (const FGameplayAbilitySpec &Spec : AbilitySystem->GetActivatableAbilities())
    if (Spec.IsActive())
      if (UDsGameplayAbility_AttackLight *Attack =
              Cast<UDsGameplayAbility_AttackLight>(Spec.GetPrimaryInstance())) {
        Attack->BufferNextInput();
        return true;
      }
  AbilitySystem->TryActivateAbilityByTag(DsTags::Ability_Attack_Light);
  return AbilitySystem->HasMatchingGameplayTag(DsTags::State_Attacking);
}

bool ADsCharacter::InputAttackHeavy() {
  if (!AbilitySystem ||
      AbilitySystem->HasMatchingGameplayTag(DsTags::State_Attacking))
    return false;
  AbilitySystem->TryActivateAbilityByTag(DsTags::Ability_Attack_Heavy);
  return AbilitySystem->HasMatchingGameplayTag(DsTags::State_Attacking);
}

bool ADsCharacter::InputGuard() {
  if (!AbilitySystem)
    return false;
  AbilitySystem->TryActivateAbilityByTag(DsTags::Ability_Guard);
  return AbilitySystem->HasMatchingGameplayTag(DsTags::State_Parrying);
}

bool ADsCharacter::InputLockOn() { return LockOn->Toggle(); }

bool ADsCharacter::InputDodge() {
  if (!AbilitySystem)
    return false;
  AbilitySystem->TryActivateAbilityByTag(DsTags::Ability_Dodge);
  return AbilitySystem->HasMatchingGameplayTag(DsTags::State_Dodging);
}

bool ADsCharacter::IsMoveInputBlocked() const {
  if (!AbilitySystem)
    return false;
  static const FGameplayTagContainer Blocking =
      FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>{
          DsTags::State_Dead, DsTags::State_Staggered, DsTags::State_Attacking,
          DsTags::State_Guarding, DsTags::State_Parrying,
          DsTags::State_Dodging});
  return AbilitySystem->HasAnyMatchingGameplayTags(Blocking);
}

bool ADsCharacter::CanJumpInternal_Implementation() const {
  return Super::CanJumpInternal_Implementation() && !IsMoveInputBlocked();
}

void ADsCharacter::HandleDamaged(float Damage, AActor *DamageInstigator,
                                 AActor *Causer) {
  if (Damage <= 0.f || !AbilitySystem || IsMoveInputBlocked())
    return;
  // 전투 동작 중에는 그 모션을 끊지 않도록 피격 모션을 생략한다.
  PlayCombatMontage(GetHitReactMontage(Causer ? Causer : DamageInstigator));
}

UAnimMontage* ADsCharacter::GetHitReactMontage(const AActor* Attacker) const {
  const auto Direction = DsHitDirection(GetActorRotation(), Attacker ? Attacker->GetActorLocation() - GetActorLocation() : FVector::ZeroVector);
  return DsSelectHitMontage(Direction, HitReactFrontMontage.Get(), HitReactBackMontage.Get(), HitReactLeftMontage.Get(), HitReactRightMontage.Get());
}

void ADsCharacter::HandlePoiseBroken(AActor *DamageInstigator, AActor *Causer) {
  if (!AbilitySystem || AbilitySystem->HasMatchingGameplayTag(DsTags::State_Dead))
    return;
  AbilitySystem->CancelAllAbilities();
  AbilitySystem->SetLooseState(DsTags::State_Staggered, true);
  StaggerRemaining = UDsPlayerCombatData::Get()->StaggerDuration;
  PlayCombatMontage(GetHitReactMontage(Causer ? Causer : DamageInstigator));
}

void ADsCharacter::HandleOutOfHealth(AActor *, AActor *) {
  LockOn->Clear();
  if (!AbilitySystem)
    return;
  AbilitySystem->SetLooseState(DsTags::State_Dead, true);
  AbilitySystem->CancelAllAbilities();
  AbilitySystem->SetLooseState(DsTags::State_Staggered, false);
  StaggerRemaining = 0.f;
  GetCharacterMovement()->DisableMovement();
  PlayCombatMontage(DeathMontage, true);
}

void ADsCharacter::Revive(const FTransform &Where) {
  LockOn->Clear();
  CombatFX->Reset();
  GetWorld()->GetSubsystem<UDsHitFeedbackSubsystem>()->ResetActor(this);
  AbilitySystem->CancelAllAbilities();
  const TArray<FGameplayTag> ResetTags = {DsTags::State_Dead, DsTags::State_Staggered,
       DsTags::State_Attacking, DsTags::State_Guarding, DsTags::State_Parrying,
       DsTags::State_Dodging, DsTags::State_Invulnerable};
  for (const FGameplayTag &Tag : ResetTags)
    AbilitySystem->SetLooseState(Tag, false);
  AbilitySystem->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
  AbilitySystem->SetNumericAttributeBase(UDsAttributeSet::GetStaminaAttribute(), Attributes->GetMaxStamina());
  AbilitySystem->SetNumericAttributeBase(UDsAttributeSet::GetPoiseAttribute(), Attributes->GetMaxPoise());
  StaggerRemaining = 0.f;
  RestoreAnimBlueprint();
  if (MannyMesh && MannyMesh->GetAnimInstance())
    MannyMesh->GetAnimInstance()->Montage_Stop(0.f);
  MotionTime = 0.f;
  MotionLastOffset = FVector::ZeroVector;
  SetActorLocationAndRotation(Where.GetLocation(), Where.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
  if (Controller)
    Controller->SetControlRotation(Where.Rotator());
  GetCharacterMovement()->StopMovementImmediately();
  ConsumeMovementInputVector();
  GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ADsCharacter::FellOutOfWorld(const UDamageType &DamageType) {
  UDsEncounterSubsystem *Encounter = GetWorld() ? GetWorld()->GetSubsystem<UDsEncounterSubsystem>() : nullptr;
  if (!Encounter || !Encounter->IsBoundPlayer(this)) {
    Super::FellOutOfWorld(DamageType);
    return;
  }
  // 낙사는 무적·막기에 관계없이 사망시키되 재도전을 위해 액터는 유지한다.
  if (Attributes->GetHealth() > 0.f) {
    AbilitySystem->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), 0.f);
    Attributes->OnOutOfHealth.Broadcast(this, this);
  }
}

bool ADsCharacter::CanPlayCombatMontage() const {
  return MannyMesh && MannyMesh->GetSkeletalMeshAsset() &&
         MannyMesh->IsRegistered();
}

bool ADsCharacter::HasMontageSlot() const {
  const IAnimClassInterface *AnimClass =
      MannyMesh ? IAnimClassInterface::GetFromClass(MannyMesh->GetAnimClass())
                : nullptr;
  if (!AnimClass)
    return false;
  for (const FStructProperty *Property : AnimClass->GetAnimNodeProperties())
    if (Property && Property->Struct &&
        Property->Struct->IsChildOf(FAnimNode_Slot::StaticStruct()))
      return true;
  return false;
}

void ADsCharacter::PlayCombatMontage(UAnimMontage *Montage, bool bHoldLastPose,
                                     float PlayRate) {
  if (!Montage || !CanPlayCombatMontage())
    return;
  const bool bHasSlot = HasMontageSlot();
  UE_LOG(LogDsCharacter, Verbose, TEXT("전투 모션 %s 재생 경로: %s"),
         *Montage->GetName(), bHasSlot ? TEXT("몽타주 슬롯") : TEXT("표시 메시 직접 재생"));
  if (bHasSlot) {
    if (UAnimInstance *AnimInstance = MannyMesh->GetAnimInstance())
      AnimInstance->Montage_Play(Montage, PlayRate);
    return;
  }
  // 리타깃 애님 BP에 슬롯이 없으면 동작 길이만큼 표시 메시를 직접 재생한다.
  // 추가형(피격) 모션은 단독 재생하면 자세가 깨지므로 생략한다.
  if (Montage->IsValidAdditive() || Montage->SlotAnimTracks.IsEmpty() ||
      Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.IsEmpty())
    return;
  UAnimSequenceBase *Sequence =
      Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference();
  if (!Sequence)
    return;
  // 콤보로 이어 재생할 때 앞 모션이 남긴 메시 보정이 누적되지 않게 먼저 되돌린다.
  if (MotionSequence)
    MannyMesh->SetRelativeLocation(MotionMeshBaseLocation);
  MannyMesh->PlayAnimation(Sequence, false);
  MannyMesh->SetPlayRate(PlayRate);
  bPoseOverridden = true;
  PoseOverrideRemaining = bHoldLastPose ? -1.f : Sequence->GetPlayLength() / PlayRate;
  // 리타깃 모션은 이동이 root가 아니라 골반에 들어 있어, 골반을 따라 캡슐을 옮긴다.
  MotionSequence = Cast<UAnimSequence>(Sequence);
  MotionTime = 0.f;
  MotionPlayRate = PlayRate;
  if (MotionSequence) {
    MotionMeshBaseLocation = MannyMesh->GetRelativeLocation();
    MotionLastOffset = SampleCombatMotion(0.f);
    MannyMesh->SetRelativeLocation(
        MotionMeshBaseLocation -
        MannyMesh->GetRelativeRotation().RotateVector(MotionLastOffset));
  }
}

FVector ADsCharacter::SampleCombatMotion(float Time) const {
  const USkeleton *Skeleton = MotionSequence ? MotionSequence->GetSkeleton() : nullptr;
  const int32 BoneIndex = Skeleton
      ? Skeleton->GetReferenceSkeleton().FindBoneIndex(CombatMotionBone)
      : INDEX_NONE;
  if (BoneIndex == INDEX_NONE)
    return FVector::ZeroVector;
  // 부모를 거슬러 올라가며 메시 공간 위치로 합친다.
  const FAnimExtractContext Context(static_cast<double>(Time));
  FTransform Result = FTransform::Identity;
  for (int32 Index = BoneIndex; Index != INDEX_NONE;
       Index = Skeleton->GetReferenceSkeleton().GetParentIndex(Index)) {
    FTransform Local;
    MotionSequence->GetBoneTransform(Local, FSkeletonPoseBoneIndex(Index), Context, false);
    Result *= Local;
  }
  FVector Offset = Result.GetLocation();
  Offset.Z = 0.f;
  return Offset;
}

void ADsCharacter::FollowCombatMotion(float DeltaSeconds) {
  if (!MotionSequence || !MannyMesh)
    return;
  const float Length = MotionSequence->GetPlayLength();
  if (MotionTime >= Length)
    return;
  MotionTime = FMath::Min(MotionTime + DeltaSeconds * MotionPlayRate, Length);
  const FVector Offset = SampleCombatMotion(MotionTime);
  const FVector WorldDelta =
      MannyMesh->GetComponentQuat().RotateVector(Offset - MotionLastOffset);
  MotionLastOffset = Offset;
  // 벽·난간에 막히면 면을 따라 미끄러진다. 바닥 처리는 이동 컴포넌트가 다음 틱에 한다.
  if (UMovementComponent *Movement = GetCharacterMovement()) {
    FHitResult Hit;
    Movement->SafeMoveUpdatedComponent(WorldDelta, GetActorQuat(), true, Hit);
    if (Hit.IsValidBlockingHit())
      Movement->SlideAlongSurface(WorldDelta, 1.f - Hit.Time, Hit.Normal, Hit, false);
  }
  // 캡슐이 옮겨 간 만큼 메시를 반대로 밀어 몸이 캡슐 위에 머물게 한다.
  MannyMesh->SetRelativeLocation(
      MotionMeshBaseLocation -
      MannyMesh->GetRelativeRotation().RotateVector(Offset));
}

void ADsCharacter::StopCombatMontage(UAnimMontage *Montage) {
  if (!Montage || !MannyMesh)
    return;
  if (bPoseOverridden) {
    if (PoseOverrideRemaining >= 0.f)
      RestoreAnimBlueprint();
    return;
  }
  if (UAnimInstance *AnimInstance = MannyMesh->GetAnimInstance())
    AnimInstance->Montage_Stop(.15f, Montage);
}

void ADsCharacter::RestoreAnimBlueprint() {
  bPoseOverridden = false;
  PoseOverrideRemaining = 0.f;
  if (MannyMesh) {
    if (MotionSequence)
      MannyMesh->SetRelativeLocation(MotionMeshBaseLocation);
    MannyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
  }
  MotionSequence = nullptr;
}

void ADsCharacter::PostActorCreated() {
  Super::PostActorCreated();
  InitializeAbilities();
}

void ADsCharacter::PossessedBy(AController *NewController) {
  Super::PossessedBy(NewController);
  InitializeAbilities();
  if (APlayerController *PC = Cast<APlayerController>(NewController)) {
    if (ULocalPlayer *LocalPlayer = PC->GetLocalPlayer()) {
      if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
              ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)) {
        if (InputMapping) Subsystem->AddMappingContext(InputMapping, 0);
        if (CombatInputMapping) Subsystem->AddMappingContext(CombatInputMapping, 1);
      }
    }
  }
}

void ADsCharacter::OnRep_Controller() {
  Super::OnRep_Controller();
  InitializeAbilities();
}

void ADsCharacter::Landed(const FHitResult &Hit) {
  LastLandVelocity = GetCharacterMovement()->Velocity;
  LastLandingTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.f;
  Super::Landed(Hit);
}

void ADsCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);
  UEnhancedInputComponent *Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
  if (!Input) return;
  if (MoveAction) Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADsCharacter::Move);
  if (LookAction) Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADsCharacter::Look);
  if (LookAction) Input->BindAction(LookAction, ETriggerEvent::Completed, this, &ADsCharacter::LookCompleted);
  if (GamepadLookAction) {
    Input->BindAction(GamepadLookAction, ETriggerEvent::Triggered, this, &ADsCharacter::Look);
    Input->BindAction(GamepadLookAction, ETriggerEvent::Completed, this, &ADsCharacter::LookCompleted);
  }
  if (JumpAction) {
    Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
    Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
  }
  if (WalkAction) {
    Input->BindAction(WalkAction, ETriggerEvent::Started, this, &ADsCharacter::StartWalk);
    Input->BindAction(WalkAction, ETriggerEvent::Completed, this, &ADsCharacter::StopWalk);
  }
  if (SprintAction) {
    Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ADsCharacter::StartSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ADsCharacter::StopSprint);
  }
  if (CrouchAction) Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ADsCharacter::ToggleCrouch);
  if (AttackLightAction) Input->BindAction(AttackLightAction, ETriggerEvent::Started, this, &ADsCharacter::OnInputAttackLight);
  if (AttackHeavyAction) Input->BindAction(AttackHeavyAction, ETriggerEvent::Started, this, &ADsCharacter::OnInputAttackHeavy);
  if (GuardAction) Input->BindAction(GuardAction, ETriggerEvent::Started, this, &ADsCharacter::OnInputGuard);
  if (LockOnAction) Input->BindAction(LockOnAction, ETriggerEvent::Started, this, &ADsCharacter::OnInputLockOn);
  if (DodgeAction) Input->BindAction(DodgeAction, ETriggerEvent::Started, this, &ADsCharacter::OnInputDodge);
}

void ADsCharacter::Move(const FInputActionValue &Value) {
  if (IsMoveInputBlocked()) return;
  const FVector2D Axis = Value.Get<FVector2D>();
  const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ADsCharacter::Look(const FInputActionValue &Value) {
  const FVector2D Axis = Value.Get<FVector2D>();
  if (LockOn->GetTarget()) { LockOn->LookInput(Axis.X); return; }
  AddControllerYawInput(Axis.X);
  AddControllerPitchInput(Axis.Y);
}

void ADsCharacter::ResetLockOnLook() { LockOn->LookInput(0.f); }

void ADsCharacter::RefreshMaxSpeed() {
  GetCharacterMovement()->MaxWalkSpeed = bWantsToSprint ? GetSprintSpeed() : bWantsToWalk ? GetWalkSpeed() : GetRunSpeed();
}

void ADsCharacter::SetWalking(bool bInWalking) {
  bWantsToWalk = bInWalking;
  if (bInWalking) bWantsToSprint = false;
  RefreshMaxSpeed();
}

void ADsCharacter::SetSprinting(bool bInSprinting) {
  bWantsToSprint = bInSprinting;
  if (bInSprinting) bWantsToWalk = false;
  RefreshMaxSpeed();
}

void ADsCharacter::StartWalk() { SetWalking(true); }
void ADsCharacter::StopWalk() { SetWalking(false); }
void ADsCharacter::StartSprint() { SetSprinting(true); }
void ADsCharacter::StopSprint() { SetSprinting(false); }
void ADsCharacter::ToggleCrouch() {
  if (bIsCrouched) UnCrouch();
  else Crouch();
}
