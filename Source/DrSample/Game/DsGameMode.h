#pragma once

#include "CoreMinimal.h"
#include "Game/DrGameMode.h"
#include "DsGameMode.generated.h"

/** 프로젝트 캐릭터를 기본 폰으로 사용하는 게임 모드이다. */
UCLASS()
class DRSAMPLE_API ADsGameMode : public ADrGameMode {
  GENERATED_BODY()
public:
  ADsGameMode();
  virtual void InitGame(const FString &MapName, const FString &Options,
                        FString &ErrorMessage) override;
};
