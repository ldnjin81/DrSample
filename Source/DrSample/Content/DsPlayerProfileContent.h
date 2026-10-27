// 서버에서 받은 플레이어 프로필 데이터와 콘텐츠 객체를 선언한다.
#pragma once
#include "Content/DrContentBase.h"
#include "DsPlayerProfileContent.generated.h"

/** 서버 응답에서 전달되는 플레이어 프로필 데이터 집합이다. */
USTRUCT()
struct FDsPlayerProfileDataSet {
  GENERATED_BODY()

  UPROPERTY()
  FString DisplayName;

  UPROPERTY()
  int32 Level = 1;

  UPROPERTY()
  int64 Exp = 0;
};
DECLARE_MULTICAST_DELEGATE_OneParam(FDsProfileChangedDelegate,
                                    const FDsPlayerProfileDataSet &);
UCLASS()
class DRSAMPLE_API UDsPlayerProfileContent : public UDrContentBase {
  GENERATED_BODY()
public:
  virtual void HandleServerPacket(const FDrNetPacket &Packet) override;
  virtual TArray<FName> GetHandledMessageTypes() const override;
  void RequestProfile();
  void ApplyProfileDataSet(const FDsPlayerProfileDataSet &NewDataSet);
  const FDsPlayerProfileDataSet &GetDataSet() const { return DataSet; }
  FDsProfileChangedDelegate OnProfileChanged;

private:
  void HandleProfileResponse(const FDrNetResponse &Response);
  UPROPERTY() FDsPlayerProfileDataSet DataSet;
};
