#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DsHitFeedbackSubsystem.generated.h"

class UDsCombatFeelData;
class USkeletalMeshComponent;
class UMaterialInterface;
enum class EDsHitWeight : uint8 { Light, Heavy, Boss };

USTRUCT()
struct FDsHitFlashMesh {
  GENERATED_BODY()
  TWeakObjectPtr<USkeletalMeshComponent> Mesh;
  UPROPERTY() TObjectPtr<UMaterialInterface> Previous;
};
USTRUCT()
struct FDsHitFeedbackState {
  GENERATED_BODY()
  float StopRemaining = 0.f;
  float FlashRemaining = 0.f;
  float PreviousDilation = 1.f;
  bool bStopped = false;
  UPROPERTY() TArray<FDsHitFlashMesh> Meshes;
};

/** 액터의 CustomTimeDilation과 무관한 월드 시간으로 연출을 복구한다. */
UCLASS()
class DRSAMPLE_API UDsHitFeedbackSubsystem : public UTickableWorldSubsystem {
  GENERATED_BODY()
public:
  void PlayHit(AActor* Attacker, AActor* Victim, EDsHitWeight Weight);
  void PlaySlam(AActor* Source);
  void ResetActor(AActor* Actor);
  virtual void Tick(float DeltaTime) override;
  virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDsHitFeedbackSubsystem, STATGROUP_Tickables); }
  virtual void Deinitialize() override;
protected:
  virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
public:
  UPROPERTY() TObjectPtr<UDsCombatFeelData> Settings;
private:
  const UDsCombatFeelData* Data();
  void Stop(AActor* Actor, float Duration);
  void Flash(AActor* Actor);
  void Shake(AActor* Actor, bool bHeavy, float Attenuation = 1.f);
  void RestoreFlash(FDsHitFeedbackState& State);
  UPROPERTY() TMap<TWeakObjectPtr<AActor>, FDsHitFeedbackState> Active;
};
