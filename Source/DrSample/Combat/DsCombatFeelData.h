#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Camera/CameraShakeBase.h"
#include "DsCombatFeelData.generated.h"

/** 카메라와 타격 연출을 에셋 한 곳에서 조율한다. */
UCLASS(BlueprintType)
class DRSAMPLE_API UDsCombatFeelData : public UDataAsset {
  GENERATED_BODY()
public:
  static const UDsCombatFeelData* Get();
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float AcquireDistance = 2500.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReleaseDistance = 3000.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float OcclusionGrace = 2.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float CameraInterpSpeed = 7.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float ArmLengthBonus = 60.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float LockOnSideOffset = 70.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float CloseCameraDistance = 350.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float CloseArmLengthBonus = 150.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float PitchBias = -8.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinPitch = -35.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxPitch = 10.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float AcquireHalfAngle = 60.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float SwitchThreshold = .7f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float SwitchCooldown = .3f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float LightStop = .06f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeavyStop = .12f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float BossStop = .1f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float StopDilation = .08f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float FlashDuration = .1f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float LightShakeScale = .5f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float HeavyShakeScale = 1.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<UCameraShakeBase> LightShake;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<UCameraShakeBase> HeavyShake;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UMaterialInterface> HitOverlay;
};
