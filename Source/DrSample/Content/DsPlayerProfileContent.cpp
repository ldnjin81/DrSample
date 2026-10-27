#include "Content/DsPlayerProfileContent.h"
// 플레이어 프로필 요청, 응답, 갱신 처리를 구현한다.
#include "Network/DsNetTypes.h"
#include "Content/DrContentSubsystem.h"
#include "Network/DrNetworkSubsystem.h"
#include "DsSample.h"
void UDsPlayerProfileContent::HandleServerPacket(const FDrNetPacket &Packet) {
  if (Packet.Type == DsMessages::ServerPush_ProfileUpdated &&
      Packet.Result == EDrNetResult::Ok)
    if (const FDsPlayerProfileDataSet *ProfileDataSet =
            Packet.Payload.GetPtr<FDsPlayerProfileDataSet>())
      ApplyProfileDataSet(*ProfileDataSet);
}
TArray<FName>
UDsPlayerProfileContent::GetHandledMessageTypes() const {
  return {DsMessages::ServerPush_ProfileUpdated};
}
void UDsPlayerProfileContent::RequestProfile() {
  UDrNetworkSubsystem *NetworkSubsystem =
      Owner ? Owner->GetGameInstance()->GetSubsystem<UDrNetworkSubsystem>()
            : nullptr;
  if (!NetworkSubsystem) {
    UE_LOG(LogDs, Error,
           TEXT("[Content] 네트워크 없이 프로필을 요청할 수 없습니다."));
    return;
  }
  NetworkSubsystem->SendRequest(
      DsMessages::RequestPlayerProfile, FDrEmptyPayload(),
      FDrResponseDelegate::CreateUObject(
          this, &UDsPlayerProfileContent::HandleProfileResponse));
}
void UDsPlayerProfileContent::HandleProfileResponse(
    const FDrNetResponse &Response) {
  if (Response.Result == EDrNetResult::Ok)
    if (const FDsPlayerProfileDataSet *ProfileDataSet =
            Response.Payload.GetPtr<FDsPlayerProfileDataSet>())
      ApplyProfileDataSet(*ProfileDataSet);
}
void UDsPlayerProfileContent::ApplyProfileDataSet(
    const FDsPlayerProfileDataSet &InDataSet) {
  DataSet = InDataSet;
  OnProfileChanged.Broadcast(DataSet);
}
