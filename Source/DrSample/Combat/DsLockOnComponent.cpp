#include "Combat/DsLockOnComponent.h"

#include "Combat/DsLockOnTargetComponent.h"
#include "Combat/DsCombatFeelData.h"
#include "Abilities/DsAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/DsGameplayTags.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

UDsLockOnComponent::UDsLockOnComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}
const UDsCombatFeelData* UDsLockOnComponent::Data() const { return Settings ? Settings.Get() : UDsCombatFeelData::Get(); }
AActor* UDsLockOnComponent::GetTarget() const { return Target.IsValid() ? Target->GetOwner() : nullptr; }
FVector UDsLockOnComponent::GetTargetLocation() const { return Target.IsValid() ? Target->GetTargetLocation() : FVector::ZeroVector; }
bool UDsLockOnComponent::IsVisible(UDsLockOnTargetComponent* Candidate) const {
  const auto* Pawn = Cast<APawn>(GetOwner());
  FVector Eye = Pawn->GetPawnViewLocation();
  FRotator Rotation = Pawn->GetViewRotation();
  if (Pawn->GetController()) Pawn->GetController()->GetPlayerViewPoint(Eye, Rotation);
  FCollisionQueryParams Params(SCENE_QUERY_STAT(DrLockSight), false, Pawn);
  Params.AddIgnoredActor(Candidate->GetOwner());
  return !GetWorld()->LineTraceTestByChannel(Eye, Candidate->GetTargetLocation(), ECC_Visibility, Params);
}
UDsLockOnTargetComponent* UDsLockOnComponent::FindTarget(float Direction) const {
  const auto* Pawn = Cast<APawn>(GetOwner());
  if (!Pawn || !GetWorld()) return nullptr;
  FVector Eye = Pawn->GetPawnViewLocation();
  FRotator View = Pawn->GetViewRotation();
  if (Pawn->GetController()) Pawn->GetController()->GetPlayerViewPoint(Eye, View);
  const float CurrentYaw = Target.IsValid() ? (GetTargetLocation() - Eye).Rotation().Yaw : View.Yaw;
  float BestScore = TNumericLimits<float>::Max();
  UDsLockOnTargetComponent* Best = nullptr;
  for (TActorIterator<AActor> It(GetWorld()); It; ++It) {
    auto* Candidate = It->FindComponentByClass<UDsLockOnTargetComponent>();
    if (*It == Pawn || !Candidate || Candidate == Target.Get() || !Candidate->IsAvailable()) continue;
    const float Distance = FVector::Dist(Pawn->GetActorLocation(), It->GetActorLocation());
    if (Distance > Data()->AcquireDistance) continue;
    const FVector ToTarget = (Candidate->GetTargetLocation() - Eye).GetSafeNormal();
    if (FVector::DotProduct(View.Vector(), ToTarget) < FMath::Cos(FMath::DegreesToRadians(Data()->AcquireHalfAngle))) continue;
    if (const auto* PC = Cast<APlayerController>(Pawn->GetController())) {
      int32 Width = 0, Height = 0;
      PC->GetViewportSize(Width, Height);
      if (Width > 0 && Height > 0) {
        FVector2D Screen = FVector2D::ZeroVector;
        if (!PC->ProjectWorldLocationToScreen(Candidate->GetTargetLocation(), Screen) ||
            Screen.X < 0 || Screen.Y < 0 || Screen.X > Width || Screen.Y > Height) continue;
      }
    }
    if (!IsVisible(Candidate)) continue;
    const float Angle = FMath::FindDeltaAngleDegrees(CurrentYaw, ToTarget.Rotation().Yaw);
    if (Direction != 0.f && Angle * Direction <= 1.f) continue;
    const float Score = FMath::Abs(Angle) + Distance * .001f;
    if (Score < BestScore) { Best = Candidate; BestScore = Score; }
  }
  return Best;
}
bool UDsLockOnComponent::Toggle() {
  if (bLocked) { Clear(); return false; }
  const auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
  if (ASC && (ASC->HasMatchingGameplayTag(DsTags::State_Dead) || ASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()) <= 0.f)) return false;
  if (auto* Candidate = FindTarget(0.f)) SetTarget(Candidate);
  return GetTarget() != nullptr;
}
void UDsLockOnComponent::SetTarget(UDsLockOnTargetComponent* Candidate) {
  auto* Character = Cast<ACharacter>(GetOwner());
  if (!Character || !Candidate) return;
  if (!bLocked) {
    auto* Movement = Character->GetCharacterMovement();
    bSavedOrient = Movement->bOrientRotationToMovement;
    bSavedDesired = Movement->bUseControllerDesiredRotation;
    bSavedYaw = Character->bUseControllerRotationYaw;
    Movement->bOrientRotationToMovement = false;
    Movement->bUseControllerDesiredRotation = false;
    Character->bUseControllerRotationYaw = true;
    if (auto* Boom = Character->FindComponentByClass<USpringArmComponent>()) {
      SavedArmLength = Boom->TargetArmLength;
      if (!bRestoreSocket) SavedSocketOffset = Boom->SocketOffset;
      bRestoreSocket = true;
      Boom->TargetArmLength += Data()->ArmLengthBonus;
    }
  }
  bLocked = true;
  Target = Candidate;
  OccludedFor = 0.f;
  OnTargetChanged.Broadcast(GetTarget());
}
void UDsLockOnComponent::Clear() {
  if (!bLocked) return;
  if (auto* Character = Cast<ACharacter>(GetOwner())) {
    Character->GetCharacterMovement()->bOrientRotationToMovement = bSavedOrient;
    Character->GetCharacterMovement()->bUseControllerDesiredRotation = bSavedDesired;
    Character->bUseControllerRotationYaw = bSavedYaw;
    if (auto* Boom = Character->FindComponentByClass<USpringArmComponent>()) Boom->TargetArmLength = SavedArmLength;
  }
  bLocked = false;
  Target.Reset();
  OccludedFor = 0.f;
  SwitchRemaining = 0.f;
  bSwitchLatched = false;
  OnTargetChanged.Broadcast(nullptr);
}
bool UDsLockOnComponent::SwitchTarget(float Direction) {
  if (!bLocked || Direction == 0.f || SwitchRemaining > 0.f) return false;
  if (auto* Candidate = FindTarget(FMath::Sign(Direction))) {
    SetTarget(Candidate);
    SwitchRemaining = Data()->SwitchCooldown;
    return true;
  }
  return false;
}
void UDsLockOnComponent::LookInput(float Axis) {
  if (FMath::Abs(Axis) < Data()->SwitchThreshold * .5f) bSwitchLatched = false;
  else if (!bSwitchLatched && FMath::Abs(Axis) >= Data()->SwitchThreshold) {
    SwitchTarget(Axis);
    bSwitchLatched = true;
  }
}
void UDsLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  SwitchRemaining = FMath::Max(0.f, SwitchRemaining - DeltaTime);
  if (bRestoreSocket) {
    if (auto* Boom = GetOwner()->FindComponentByClass<USpringArmComponent>()) {
      const float Distance = GetTarget() ? FVector::Dist2D(GetOwner()->GetActorLocation(), GetTarget()->GetActorLocation()) : Data()->CloseCameraDistance;
      const float Close = bLocked ? 1.f - FMath::Clamp(Distance / FMath::Max(1.f, Data()->CloseCameraDistance), 0.f, 1.f) : 0.f;
      const FVector Desired = SavedSocketOffset + (bLocked ? FVector(0, Data()->LockOnSideOffset * (1.f - Close), 0) : FVector::ZeroVector);
      if (bLocked) Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength,
          SavedArmLength + Data()->ArmLengthBonus + Data()->CloseArmLengthBonus * Close, DeltaTime, Data()->CameraInterpSpeed);
      Boom->SocketOffset = FMath::VInterpTo(Boom->SocketOffset, Desired, DeltaTime, Data()->CameraInterpSpeed);
      if (!bLocked && Boom->SocketOffset.Equals(SavedSocketOffset, .01f)) {
        Boom->SocketOffset = SavedSocketOffset;
        bRestoreSocket = false;
      }
    }
  }
  if (!bLocked) return;
  const auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
  if (!Target.IsValid() || !Target->IsAvailable() ||
      (ASC && (ASC->HasMatchingGameplayTag(DsTags::State_Dead) || ASC->GetNumericAttribute(UDsAttributeSet::GetHealthAttribute()) <= 0.f)) ||
      FVector::Dist(GetOwner()->GetActorLocation(), GetTarget()->GetActorLocation()) > Data()->ReleaseDistance) { Clear(); return; }
  OccludedFor = IsVisible(Target.Get()) ? 0.f : OccludedFor + DeltaTime;
  if (OccludedFor >= Data()->OcclusionGrace) { Clear(); return; }
  auto* Character = Cast<ACharacter>(GetOwner());
  if (Character && Character->GetController()) {
    FRotator Desired = (GetTargetLocation() - Character->GetPawnViewLocation()).Rotation();
    Desired.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Desired.Pitch) + Data()->PitchBias, Data()->MinPitch, Data()->MaxPitch);
    Desired.Roll = 0.f;
    Character->GetController()->SetControlRotation(FMath::RInterpTo(Character->GetControlRotation(), Desired, DeltaTime, Data()->CameraInterpSpeed));
    const float Yaw = (GetTargetLocation() - Character->GetActorLocation()).Rotation().Yaw;
    Character->SetActorRotation(FMath::RInterpTo(Character->GetActorRotation(), FRotator(0, Yaw, 0), DeltaTime, Data()->CameraInterpSpeed));
  }
}
void UDsLockOnComponent::EndPlay(const EEndPlayReason::Type Reason) {
  Clear();
  if (bRestoreSocket) if (auto* Boom = GetOwner()->FindComponentByClass<USpringArmComponent>()) Boom->SocketOffset = SavedSocketOffset;
  Super::EndPlay(Reason);
}
