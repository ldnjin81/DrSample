#pragma once
#include "Debug/DrSmokeSubsystem.h"
#include "DsSmokeSubsystem.generated.h"
UCLASS()
class DRSAMPLE_API UDsSmokeSubsystem : public UDrSmokeSubsystem { GENERATED_BODY()
public: virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
