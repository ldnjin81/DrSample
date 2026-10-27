#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DsLockOnComponent.generated.h"

class UDsLockOnTargetComponent;
class UDsCombatFeelData;
DECLARE_MULTICAST_DELEGATE_OneParam(FDsLockOnChangedDelegate, AActor*);

/** 획득·전환·해제와 카메라/스트레이프 상태 복구를 담당한다. */
UCLASS(ClassGroup=(Dr), meta=(BlueprintSpawnableComponent))
class DRSAMPLE_API UDsLockOnComponent : public UActorComponent {
  GENERATED_BODY()
public:
  UDsLockOnComponent();
  bool Toggle();
  void Clear();
  bool SwitchTarget(float Direction);
  void LookInput(float Axis);
  AActor* GetTarget() const;
  FVector GetTargetLocation() const;
  virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
  virtual void EndPlay(const EEndPlayReason::Type Reason) override;
  FDsLockOnChangedDelegate OnTargetChanged;
  UPROPERTY(EditAnywhere) TObjectPtr<UDsCombatFeelData> Settings;
private:
  const UDsCombatFeelData* Data() const;
  bool IsVisible(UDsLockOnTargetComponent* Candidate) const;
  UDsLockOnTargetComponent* FindTarget(float Direction) const;
  void SetTarget(UDsLockOnTargetComponent* Candidate);
  UPROPERTY(Transient) TWeakObjectPtr<UDsLockOnTargetComponent> Target;
  bool bLocked = false;
  bool bSavedOrient = true;
  bool bSavedDesired = false;
  bool bSavedYaw = false;
  bool bSwitchLatched = false;
  float SavedArmLength = 300.f;
  FVector SavedSocketOffset = FVector::ZeroVector;
  bool bRestoreSocket = false;
  float OccludedFor = 0.f;
  float SwitchRemaining = 0.f;
};
