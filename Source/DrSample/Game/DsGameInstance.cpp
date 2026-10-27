#include "Game/DsGameInstance.h"
#include "Network/DsNetTypes.h"
#include "Network/DrNetworkSubsystem.h"
#include "Backend/DrFakeServer.h"
#include "Content/DrContentSubsystem.h"
#include "Content/DsPlayerProfileContent.h"
#include "DsSample.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
TArray<FName> UDsGameInstance::GetBootSteps() const { return {TEXT("Connect"),TEXT("Ping"),TEXT("Login"),TEXT("Profile")}; }
void UDsGameInstance::ExecuteBootStep(FName Step) {
 auto* Net=GetSubsystem<UDrNetworkSubsystem>();
 if(!Net) { CompleteBootStep(false); return; }
 bAutoSmoke=FParse::Param(FCommandLine::Get(),TEXT("DsAutoNetSmoke"));
 if(Step==TEXT("Connect")) {
  if(auto* Server=Cast<UDrFakeServer>(Net->GetServerEndpoint())) {
   Server->RegisterHandler(DsMessages::Login, [](const FDrNetPacket&) { FDsLoginResponse Payload; Payload.SessionToken=TEXT("local-session"); FDrNetResponse R; R.Result=EDrNetResult::Ok; R.Payload=FInstancedStruct::Make(Payload); return R; });
   Server->RegisterHandler(DsMessages::RequestPlayerProfile, [](const FDrNetPacket&) { FDsPlayerProfileDataSet Payload; Payload.DisplayName=TEXT("Player"); FDrNetResponse R; R.Result=EDrNetResult::Ok; R.Payload=FInstancedStruct::Make(Payload); return R; });
  }
  Net->OnConnectionStateChanged.AddUObject(this,&ThisClass::OnConnected);
  Net->Connect(); return;
 }
 FName Message=Step==TEXT("Profile") ? DsMessages::RequestPlayerProfile : Step;
 FInstancedStruct Payload=FInstancedStruct::Make(FDrEmptyPayload());
 if(Step==TEXT("Login")) { FDsLoginRequest Login; Login.UserId=TEXT("LocalPlayer"); Payload=FInstancedStruct::Make(Login); }
 Net->SendRequest(Message,Payload,FDrResponseDelegate::CreateWeakLambda(this,[this,Net,Step](const FDrNetResponse& R) {
  bool Ok=R.Result==EDrNetResult::Ok;
  if(Ok && Step==TEXT("Login")) { const auto* Login=R.Payload.GetPtr<FDsLoginResponse>(); Ok=Login!=nullptr; if(Login) Net->SetSessionToken(Login->SessionToken); }
  if(Ok && Step==TEXT("Profile")) { const auto* Profile=R.Payload.GetPtr<FDsPlayerProfileDataSet>(); auto* Content=GetSubsystem<UDrContentSubsystem>()->FindContent<UDsPlayerProfileContent>(); Ok=Profile && Content; if(Ok) Content->ApplyProfileDataSet(*Profile); }
  UE_LOG(LogDs,Log,TEXT("[부팅] %s: %s"),*Step.ToString(),Ok ? TEXT("성공") : TEXT("실패"));
  CompleteBootStep(Ok);
 }));
}
void UDsGameInstance::OnConnected(EDrConnectionState State) { if(State==EDrConnectionState::Connected) { GetSubsystem<UDrNetworkSubsystem>()->OnConnectionStateChanged.RemoveAll(this); CompleteBootStep(true); } }
void UDsGameInstance::OnBootFinished(bool bSucceeded) { if(bAutoSmoke) FPlatformMisc::RequestExitWithStatus(false,bSucceeded ? 0 : 1); }
void UDsGameInstance::Shutdown() { if(auto* Net=GetSubsystem<UDrNetworkSubsystem>()) Net->OnConnectionStateChanged.RemoveAll(this); Super::Shutdown(); }
