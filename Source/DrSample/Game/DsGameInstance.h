#pragma once
#include "Game/DrGameInstance.h"
#include "Network/DrNetTypes.h"
#include "DsGameInstance.generated.h"
UCLASS()
class DRSAMPLE_API UDsGameInstance : public UDrGameInstance { GENERATED_BODY()
protected:
 virtual TArray<FName> GetBootSteps() const override;
 virtual void ExecuteBootStep(FName Step) override;
 virtual void OnBootFinished(bool bSucceeded) override;
 virtual void Shutdown() override;
private:
 void OnConnected(EDrConnectionState State);
 bool bAutoSmoke=false;
};
