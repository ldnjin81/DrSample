#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DsCombatFXComponent.generated.h"
class UNiagaraComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;
class UPoseableMeshComponent;
class UMaterialInstanceDynamic;
class UDecalComponent;
class UPointLightComponent;
struct FDsBossAttackRow;

USTRUCT()
struct FDsGhostPose {
  GENERATED_BODY()
  UPROPERTY() TObjectPtr<UPoseableMeshComponent> Mesh;
  UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
  float Remaining = 0;
};

/** 어빌리티·보스 상태 전이에서 호출하는 전투 FX 수명 관리. */
UCLASS(ClassGroup=(Dr), meta=(BlueprintSpawnableComponent))
class DRSAMPLE_API UDsCombatFXComponent : public UActorComponent {
  GENERATED_BODY()
public:
  UDsCombatFXComponent();
  void SetDash(bool bEnabled);
  void SetSword(bool bEnabled);
  void BeginTelegraph(const FDsBossAttackRow& Row);
  void AdvanceTelegraph(float Elapsed, float Duration);
  void EndTelegraph();
  void PlayAttackImpact(const FDsBossAttackRow& Row);
  void EnterPhaseTwo();
  void PlayHit(AActor* Victim);
  void Reset();
  UMaterialInterface* ResolveOverlay(UMaterialInterface* Previous) const;
  UNiagaraComponent* GetDashFX() const { return DashWind; }
  UDecalComponent* GetCircleFX() const { return Circle; }
  UMaterialInstanceDynamic* GetCircleMaterial() const { return CircleMID; }
  UNiagaraComponent* GetPhaseFX() const { return PhaseLoop; }
  float GetTelegraphProgress() const { return Progress; }
  int32 GetGhostCount() const { return Ghosts.Num(); }
  UPointLightComponent* GetImpactLight() const { return ImpactLight; }
  virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
  virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
  USkeletalMeshComponent* VisibleMesh() const;
  UNiagaraComponent* Attach(UNiagaraSystem* System, FName Socket, float Scale = 1.f);
  void Burst(UNiagaraSystem* System, FVector Location, float Scale = 1.f);
  void Snapshot();
  void PulseImpactLight(FVector Location);
  static void DestroyFX(TObjectPtr<UNiagaraComponent>& FX);
  UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> DashWind;
  UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> DashBlur;
  UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> SwordLeft;
  UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> SwordRight;
  UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Telegraph;
  UPROPERTY(Transient) TObjectPtr<UDecalComponent> Circle;
  UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CircleMID;
  UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> PhaseLoop;
  UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PreviousOverlay;
  UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TelegraphMID;
  UPROPERTY(Transient) TArray<FDsGhostPose> Ghosts;
  UPROPERTY(Transient) TObjectPtr<UPointLightComponent> ImpactLight;
  float ImpactLightRemaining = 0;
  float GhostElapsed = 0;
  float Progress = 0;
  bool bDashing = false;
  bool bPhaseTwo = false;
  bool bOverlaySet = false;
};
