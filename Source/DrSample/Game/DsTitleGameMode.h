#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DsTitleGameMode.generated.h"

class ACineCameraActor;

/** 타이틀에서는 플레이어를 만들지 않고 진입로 고정 카메라를 사용한다. */
UCLASS()
class DRSAMPLE_API ADsTitleGameMode : public AGameModeBase {
  GENERATED_BODY()
public:
  ADsTitleGameMode();
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaSeconds) override;
private:
  UPROPERTY(Transient) TObjectPtr<ACineCameraActor> Camera;
  FRotator BaseRotation;
};
