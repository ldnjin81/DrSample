#pragma once

#include "CoreMinimal.h"
#include "Debug/DrPerfSubsystem.h"
#include "DsPerfSubsystem.generated.h"

class ACameraActor;

/** 패키지의 고정 60초 카메라 경로(구간 4개 × 15초)를 같은 조건으로 측정한다. 측정 틀은 UDrPerfSubsystem(-DrPerfCapture). */
UCLASS()
class DRSAMPLE_API UDsPerfSubsystem : public UDrPerfSubsystem {
  GENERATED_BODY()
public:
  UDsPerfSubsystem();
protected:
  virtual bool PrepareCapture(UWorld& World, FString& OutError) override;
  virtual bool UpdateCapture(UWorld& World, int32 Section, float Alpha) override;
private:
  TWeakObjectPtr<ACameraActor> Camera;
  /** 구간 경계 지점(구간 수 + 1개). 구간 i는 Waypoints[i] → Waypoints[i + 1]. */
  TArray<FVector> Waypoints;
  FVector ArenaCenter = FVector::ZeroVector;
  bool bTravelStarted = false;
};
