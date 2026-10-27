#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "DsHeavyHitShake.generated.h"

/** 강공격 명중에 쓰는 길고 강한 카메라 셰이크. */
UCLASS(Blueprintable)
class DRSAMPLE_API UDsHeavyHitShake : public UCameraShakeBase {
  GENERATED_BODY()
public:
  UDsHeavyHitShake(const FObjectInitializer& Initializer);
};
