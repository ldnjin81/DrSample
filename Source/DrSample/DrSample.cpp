#include "Modules/ModuleManager.h"
#include "Game/DsGameUserSettings.h"
#include "Containers/Ticker.h"
#include "Misc/CoreDelegates.h"

/** 엔진이 뜬 뒤(DrUIKit이 스타일 플러그인을 불러온 뒤) 사용자가 고른 UI 테마를 다시 적용한다. */
class FDrSampleModule : public FDefaultGameModuleImpl {
public:
  virtual void StartupModule() override {
    // 멀티캐스트 델리게이트는 나중에 더한 것부터 부르므로, DrUIKit의 스타일 로드보다 먼저 불릴 수 있다. 다음 틱으로 미룬다.
    PostEngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddLambda([] {
      FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) {
        if (const auto* Settings = UDsGameUserSettings::Get()) Settings->ApplyUITheme();
        return false;
      }));
    });
  }
  virtual void ShutdownModule() override { FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle); }
private:
  FDelegateHandle PostEngineInitHandle;
};
IMPLEMENT_PRIMARY_GAME_MODULE(FDrSampleModule, DrSample, "DrSample");

#include "DsSample.h"
DEFINE_LOG_CATEGORY(LogDs);
