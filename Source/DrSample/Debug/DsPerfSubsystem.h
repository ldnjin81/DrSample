#pragma once

#include "CoreMinimal.h"
#include "Debug/DrPerfSubsystem.h"
#include "Containers/Ticker.h"
#include "DsPerfSubsystem.generated.h"

class ACameraActor;

/** 패키지의 고정 60초 카메라 경로를 같은 조건으로 측정한다. */
UCLASS()
class DRSAMPLE_API UDsPerfSubsystem : public UDrPerfSubsystem {
  GENERATED_BODY()
public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;
private:
  bool Tick(float DeltaSeconds);
  bool Finish(const FString& Error = FString());

  FTSTicker::FDelegateHandle Ticker;
  TWeakObjectPtr<ACameraActor> Camera;
  double Started = 0, CaptureStarted = 0, LastFrame = 0, WarmupStarted = 0;
  FVector ArenaCenter = FVector::ZeroVector;
  bool bTravelStarted = false;
};
