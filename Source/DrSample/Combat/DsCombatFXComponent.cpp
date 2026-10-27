#include "Combat/DsCombatFXComponent.h"
#include "Combat/DsCombatFXData.h"
#include "Boss/DsBossCharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TableData/Generated/DsBossAttackRow.h"

UDsCombatFXComponent::UDsCombatFXComponent() { PrimaryComponentTick.bCanEverTick = true; }
USkeletalMeshComponent* UDsCombatFXComponent::VisibleMesh() const {
  USkeletalMeshComponent* Best = nullptr;
  for (auto* Mesh : TInlineComponentArray<USkeletalMeshComponent*>(GetOwner())) {
    if (!Mesh->IsVisible()) continue;
    if (!Best || Mesh->GetFName() == TEXT("MannyMesh")) Best = Mesh;
  }
  return Best;
}
UNiagaraComponent* UDsCombatFXComponent::Attach(UNiagaraSystem* System, FName Socket, float Scale) {
  if (!System || !GetOwner()) return nullptr;
  auto* Mesh = VisibleMesh();
  auto* FX = NewObject<UNiagaraComponent>(GetOwner());
  FX->SetAutoActivate(false);
  FX->SetAsset(System);
  FX->SetupAttachment(Mesh ? static_cast<USceneComponent*>(Mesh) : GetOwner()->GetRootComponent(), Mesh && Mesh->DoesSocketExist(Socket) ? Socket : NAME_None);
  FX->SetRelativeScale3D(FVector(Scale));
  FX->RegisterComponent();
  FX->Activate(true);
  return FX;
}
void UDsCombatFXComponent::Burst(UNiagaraSystem* System, FVector Location, float Scale) {
  if (System) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, Location, FRotator::ZeroRotator, FVector(Scale));
}
void UDsCombatFXComponent::DestroyFX(TObjectPtr<UNiagaraComponent>& FX) {
  if (FX) { FX->DeactivateImmediate(); FX->DestroyComponent(); FX = nullptr; }
}
void UDsCombatFXComponent::PulseImpactLight(FVector Location) {
  const auto* D = UDsCombatFXData::Get();
  if (D->ImpactLightIntensity <= 0 || D->ImpactLightDuration <= 0) return;
  // 연속 피격에도 소유자당 광원 하나만 재사용하며 판정에는 관여하지 않는다.
  if (!ImpactLight) {
    ImpactLight = NewObject<UPointLightComponent>(GetOwner());
    ImpactLight->SetMobility(EComponentMobility::Movable);
    ImpactLight->SetCastShadows(false);
    ImpactLight->SetIntensityUnits(ELightUnits::Candelas);
    ImpactLight->RegisterComponent();
  }
  ImpactLight->SetWorldLocation(Location + FVector(0, 0, 100));
  ImpactLight->SetAttenuationRadius(FMath::Max(1.f, D->ImpactLightRadius));
  ImpactLight->SetLightColor(D->ImpactLightColor);
  ImpactLight->SetIntensity(D->ImpactLightIntensity);
  ImpactLightRemaining = D->ImpactLightDuration;
}
void UDsCombatFXComponent::SetDash(bool bEnabled) {
  if (bDashing == bEnabled) return;
  bDashing = bEnabled;
  const auto* D = UDsCombatFXData::Get();
  if (bEnabled) {
    DashWind = Attach(D->DashWind, D->BodySocket); DashBlur = Attach(D->DashBlur, D->BodySocket);
    FVector Ground = GetOwner()->GetActorLocation();
    if (auto* Capsule = GetOwner()->FindComponentByClass<UCapsuleComponent>()) Ground.Z -= Capsule->GetScaledCapsuleHalfHeight();
    Burst(D->DashDust, Ground); GhostElapsed = 0; Snapshot();
  } else { DestroyFX(DashWind); DestroyFX(DashBlur); }
}
void UDsCombatFXComponent::SetSword(bool bEnabled) {
  const auto* D = UDsCombatFXData::Get();
  if (bEnabled) {
    if (!SwordLeft) SwordLeft = Attach(D->SwordLeft, TEXT("hand_l"));
    if (!SwordRight) SwordRight = Attach(D->SwordRight, D->WeaponSocket);
  } else { DestroyFX(SwordLeft); DestroyFX(SwordRight); }
}
void UDsCombatFXComponent::Snapshot() {
  const auto* D = UDsCombatFXData::Get();
  if (!D->GhostMaterial) return;
  for (auto* Source : TInlineComponentArray<USkeletalMeshComponent*>(GetOwner())) {
    if (!Source->IsVisible() || !Source->GetSkeletalMeshAsset()) continue;
    auto* Mesh = NewObject<UPoseableMeshComponent>(GetOwner());
    Mesh->SetSkinnedAssetAndUpdate(Source->GetSkeletalMeshAsset());
    Mesh->SetWorldTransform(Source->GetComponentTransform());
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCastShadow(false); Mesh->RegisterComponent();
    Mesh->CopyPoseFromSkeletalComponent(Source);
    auto* Material = UMaterialInstanceDynamic::Create(D->GhostMaterial, Mesh);
    for (int32 I = 0; I < Mesh->GetNumMaterials(); ++I) Mesh->SetMaterial(I, Material);
    FDsGhostPose Pose; Pose.Mesh = Mesh; Pose.Material = Material; Pose.Remaining = D->GhostLifetime;
    Ghosts.Add(Pose);
  }
}
void UDsCombatFXComponent::BeginTelegraph(const FDsBossAttackRow& Row) {
  EndTelegraph(); Progress = 0;
  const auto* D = UDsCombatFXData::Get();
  const auto* Entry = D->BossAttacks.Find(Row.Id);
  if (!Entry) return;
  Telegraph = Attach(Entry->Telegraph, Entry->Socket, Entry->Scale);
  if (D->GroundCircleDecal) {
    // 판정 원을 지면에 투영한다. 데칼 X축이 투영 방향이므로 아래(-Z)로 눕히고, Y·Z 반크기가 반경이다.
    FVector Center = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * Row.HitForwardOffset;
    if (auto* Capsule = GetOwner()->FindComponentByClass<UCapsuleComponent>()) Center.Z -= Capsule->GetScaledCapsuleHalfHeight();
    CircleMID = UMaterialInstanceDynamic::Create(D->GroundCircleDecal, this);
    CircleMID->SetScalarParameterValue(TEXT("Progress"), 0.f);
    Circle = NewObject<UDecalComponent>(GetOwner());
    Circle->SetUsingAbsoluteLocation(true); Circle->SetUsingAbsoluteRotation(true); Circle->SetUsingAbsoluteScale(true);
    Circle->SetupAttachment(GetOwner()->GetRootComponent());
    Circle->SetDecalMaterial(CircleMID);
    Circle->DecalSize = FVector(150.f, Row.HitRadius, Row.HitRadius);
    Circle->RegisterComponent();
    Circle->SetWorldLocationAndRotation(Center, FRotator(-90.f, 0.f, 0.f));
  }
  if (auto* Mesh = VisibleMesh(); Mesh && D->TelegraphOverlay) {
    PreviousOverlay = Mesh->GetOverlayMaterial();
    TelegraphMID = UMaterialInstanceDynamic::Create(D->TelegraphOverlay, this);
    TelegraphMID->SetScalarParameterValue(TEXT("WeaponRadius"), D->WeaponGlowRadius);
    Mesh->SetOverlayMaterial(TelegraphMID); bOverlaySet = true;
    AdvanceTelegraph(0, 1);
  }
}
void UDsCombatFXComponent::AdvanceTelegraph(float Elapsed, float Duration) {
  if (bOverlaySet && TelegraphMID) {
    const auto* D = UDsCombatFXData::Get();
    if (auto* Mesh = VisibleMesh()) {
      const FVector Center = Mesh->GetSocketTransform(D->WeaponSocket).TransformPosition(D->WeaponCenterOffset);
      TelegraphMID->SetVectorParameterValue(TEXT("WeaponCenter"), FLinearColor(Center.X,Center.Y,Center.Z,1));
    }
  }
  Progress = Duration > 0 ? FMath::Clamp(Elapsed / Duration, 0.f, 1.f) : 1.f;
  if (CircleMID) CircleMID->SetScalarParameterValue(TEXT("Progress"), Progress);
  if (Progress >= 1.f) EndTelegraph();
}
void UDsCombatFXComponent::EndTelegraph() {
  DestroyFX(Telegraph);
  if (Circle) { Circle->DestroyComponent(); Circle = nullptr; }
  CircleMID = nullptr;
  if (bOverlaySet) {
    if (auto* Mesh = VisibleMesh(); Mesh && Mesh->GetOverlayMaterial() == TelegraphMID) Mesh->SetOverlayMaterial(PreviousOverlay);
    bOverlaySet = false;
  }
}
UMaterialInterface* UDsCombatFXComponent::ResolveOverlay(UMaterialInterface* Previous) const {
  // 히트 림이 끝나기 전에 예고가 종료되었으면 종료 전 MID를 되살리지 않는다.
  return Previous == TelegraphMID ? (bOverlaySet ? TelegraphMID.Get() : PreviousOverlay.Get()) : Previous;
}
void UDsCombatFXComponent::PlayAttackImpact(const FDsBossAttackRow& Row) {
  EndTelegraph();
  if (const auto* Entry = UDsCombatFXData::Get()->BossAttacks.Find(Row.Id)) {
    FVector Center = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * Row.HitForwardOffset;
    if (auto* Capsule = GetOwner()->FindComponentByClass<UCapsuleComponent>()) Center.Z -= Capsule->GetScaledCapsuleHalfHeight();
    Burst(Entry->Impact, Center, Entry->Scale);
    PulseImpactLight(Center);
  }
}
void UDsCombatFXComponent::EnterPhaseTwo() {
  if (bPhaseTwo) return;
  bPhaseTwo = true;
  const auto* D = UDsCombatFXData::Get();
  Burst(D->PhaseBurst, GetOwner()->GetActorLocation()); PhaseLoop = Attach(D->PhaseLoop, D->WeaponSocket);
}
void UDsCombatFXComponent::PlayHit(AActor* Victim) {
  if (!Victim) return;
  const auto* D = UDsCombatFXData::Get();
  const bool bBoss = GetOwner()->IsA<ADsBossCharacter>();
  const FVector Hit = (Victim->GetActorLocation() + GetOwner()->GetActorLocation()) * .5f;
  Burst(bBoss ? D->BossImpact : D->PlayerImpact, Hit);
  if (bBoss) PulseImpactLight(Hit);
  if (!bBoss) Burst(D->PlayerSparks, Hit);
}
void UDsCombatFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) {
  Super::TickComponent(DeltaTime, TickType, Function);
  const auto* D = UDsCombatFXData::Get();
  if (ImpactLight) {
    ImpactLightRemaining -= DeltaTime;
    if (ImpactLightRemaining <= 0) { ImpactLight->DestroyComponent(); ImpactLight = nullptr; }
    else ImpactLight->SetIntensity(D->ImpactLightIntensity * FMath::Square(
        FMath::Clamp(ImpactLightRemaining / FMath::Max(.001f, D->ImpactLightDuration), 0.f, 1.f)));
  }
  if (bDashing) {
    GhostElapsed += DeltaTime;
    if (GhostElapsed >= FMath::Max(.01f, D->GhostInterval)) { GhostElapsed = 0; Snapshot(); }
  }
  for (int32 I = Ghosts.Num()-1; I >= 0; --I) {
    auto& Ghost = Ghosts[I]; Ghost.Remaining -= DeltaTime;
    if (Ghost.Remaining <= 0) { Ghost.Mesh->DestroyComponent(); Ghosts.RemoveAtSwap(I); }
    else {
      Ghost.Material->SetScalarParameterValue(TEXT("Opacity"), Ghost.Remaining / FMath::Max(.01f, D->GhostLifetime));
      // 카메라가 잔상 안이나 바로 곁에 들어오면 화면을 덮지 않게 숨긴다.
      auto* Controller = GetWorld()->GetFirstPlayerController();
      auto* Camera = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
      Ghost.Mesh->SetHiddenInGame(Camera && Ghost.Mesh->Bounds.GetBox().ExpandBy(D->GhostCameraHideDistance).IsInsideOrOn(Camera->GetCameraLocation()));
    }
  }
}
void UDsCombatFXComponent::Reset() {
  if (ImpactLight) { ImpactLight->DestroyComponent(); ImpactLight = nullptr; }
  ImpactLightRemaining = 0;
  SetDash(false); SetSword(false); EndTelegraph(); DestroyFX(PhaseLoop); bPhaseTwo = false;
  for (auto& Ghost : Ghosts) if (Ghost.Mesh) Ghost.Mesh->DestroyComponent();
  Ghosts.Reset(); Progress = 0;
}
void UDsCombatFXComponent::EndPlay(const EEndPlayReason::Type Reason) { Reset(); Super::EndPlay(Reason); }

