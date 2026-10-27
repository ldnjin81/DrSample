#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DsLockOnTargetComponent.generated.h"

/** 소켓이 없는 경량 대상도 가슴 오프셋으로 락온할 수 있다. */
UCLASS(ClassGroup=(Dr), meta=(BlueprintSpawnableComponent))
class DRSAMPLE_API UDsLockOnTargetComponent : public UActorComponent {
  GENERATED_BODY()
public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bLockable = true;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SocketName = TEXT("spine_03");
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector ChestOffset = FVector(0, 0, 50);
  UFUNCTION(BlueprintPure) FVector GetTargetLocation() const;
  bool IsAvailable() const;
};
