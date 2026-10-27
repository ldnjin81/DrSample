#include "Combat/DsHitFeedbackSubsystem.h"
#include "Combat/DsCombatFXComponent.h"

#include "Combat/DsCombatFeelData.h"
#include "Combat/DsHeavyHitShake.h"
#include "Combat/DsLightHitShake.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

const UDsCombatFeelData* UDsHitFeedbackSubsystem::Data() {
  if (!Settings) Settings = const_cast<UDsCombatFeelData*>(UDsCombatFeelData::Get());
  return Settings;
}
void UDsHitFeedbackSubsystem::Stop(AActor* Actor, float Duration) {
  if (!IsValid(Actor) || Duration <= 0.f) return;
  auto& State = Active.FindOrAdd(Actor);
  if (!State.bStopped) State.PreviousDilation = Actor->CustomTimeDilation;
  State.bStopped = true;
  State.StopRemaining = FMath::Max(State.StopRemaining, Duration);
  Actor->CustomTimeDilation = State.PreviousDilation * FMath::Clamp(Data()->StopDilation, .01f, 1.f);
}
void UDsHitFeedbackSubsystem::Flash(AActor* Actor) {
  if (!IsValid(Actor) || !Data()->HitOverlay) return;
  auto& State = Active.FindOrAdd(Actor);
  if (State.Meshes.IsEmpty()) {
    for (auto* Mesh : TInlineComponentArray<USkeletalMeshComponent*>(Actor)) {
      if (!Mesh->IsVisible()) continue;
      FDsHitFlashMesh Entry;
      Entry.Mesh = Mesh;
      Entry.Previous = Mesh->GetOverlayMaterial();
      State.Meshes.Add(Entry);
      Mesh->SetOverlayMaterial(Data()->HitOverlay);
    }
  }
  State.FlashRemaining = Data()->FlashDuration;
}
void UDsHitFeedbackSubsystem::Shake(AActor* Actor, bool bHeavy, float Attenuation) {
  const auto* Pawn = Cast<APawn>(Actor);
  auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
  if (!PC || !PC->IsLocalController() || !PC->PlayerCameraManager) return;
  TSubclassOf<UCameraShakeBase> Class = bHeavy ? Data()->HeavyShake : Data()->LightShake;
  if (!Class) Class = bHeavy ? UDsHeavyHitShake::StaticClass() : UDsLightHitShake::StaticClass();
  PC->PlayerCameraManager->StartCameraShake(Class, Attenuation * (bHeavy ? Data()->HeavyShakeScale : Data()->LightShakeScale));
}
void UDsHitFeedbackSubsystem::PlayHit(AActor* Attacker, AActor* Victim, EDsHitWeight Weight) {
  if (!IsValid(Attacker) || !IsValid(Victim)) return;
  const float Duration = Weight == EDsHitWeight::Light ? Data()->LightStop :
                         Weight == EDsHitWeight::Heavy ? Data()->HeavyStop : Data()->BossStop;
  Stop(Attacker, Duration);
  Stop(Victim, Duration);
  Flash(Victim);
  if (auto* FX = Attacker->FindComponentByClass<UDsCombatFXComponent>()) FX->PlayHit(Victim);
  Shake(Weight == EDsHitWeight::Boss ? Victim : Attacker, Weight != EDsHitWeight::Light);
}
void UDsHitFeedbackSubsystem::PlaySlam(AActor* Source) {
  if (!Source) return;
  for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It) {
    APawn* Pawn = It->Get()->GetPawn();
    if (!Pawn) continue;
    const float Distance = FVector::Dist(Pawn->GetActorLocation(), Source->GetActorLocation());
    if (Distance < 1500.f) Shake(Pawn, true, 1.f - Distance / 1500.f);
  }
}
void UDsHitFeedbackSubsystem::RestoreFlash(FDsHitFeedbackState& State) {
  for (auto& Entry : State.Meshes)
    if (Entry.Mesh.IsValid() && Entry.Mesh->GetOverlayMaterial() == Data()->HitOverlay) {
      auto* FX = Entry.Mesh->GetOwner()->FindComponentByClass<UDsCombatFXComponent>();
      Entry.Mesh->SetOverlayMaterial(FX ? FX->ResolveOverlay(Entry.Previous) : Entry.Previous.Get());
    }
  State.Meshes.Reset();
}
void UDsHitFeedbackSubsystem::ResetActor(AActor* Actor) {
  if (auto* State = Active.Find(Actor)) {
    if (IsValid(Actor) && State->bStopped) Actor->CustomTimeDilation = State->PreviousDilation;
    RestoreFlash(*State);
    Active.Remove(Actor);
  }
}
void UDsHitFeedbackSubsystem::Tick(float DeltaTime) {
  for (auto It = Active.CreateIterator(); It; ++It) {
    auto& State = It.Value();
    if (!It.Key().IsValid()) { RestoreFlash(State); It.RemoveCurrent(); continue; }
    State.StopRemaining -= DeltaTime;
    State.FlashRemaining -= DeltaTime;
    if (State.bStopped && State.StopRemaining <= 0.f) {
      It.Key()->CustomTimeDilation = State.PreviousDilation;
      State.bStopped = false;
    }
    if (State.FlashRemaining <= 0.f && !State.Meshes.IsEmpty()) RestoreFlash(State);
    if (!State.bStopped && State.FlashRemaining <= 0.f) It.RemoveCurrent();
  }
}
void UDsHitFeedbackSubsystem::Deinitialize() {
  TArray<TWeakObjectPtr<AActor>> Actors;
  Active.GetKeys(Actors);
  for (auto Actor : Actors) ResetActor(Actor.Get());
  Active.Reset();
  Super::Deinitialize();
}
bool UDsHitFeedbackSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const {
  return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
