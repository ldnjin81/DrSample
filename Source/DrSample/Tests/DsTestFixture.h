// 자동화 테스트용 경량 월드와 타이머 보조 함수를 제공한다.
#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Combat/DsPlayerCombatData.h"
#include "Boss/DsBossCharacter.h"
#include "Boss/DsBossData.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
namespace DsTestFixture {
/** 게임 에셋이 설치되어도 단위 테스트는 고정 기본값으로 검증한다. */
class FScopedCombatDefaults {
public:
 FScopedCombatDefaults()
  : Combat(const_cast<UDsPlayerCombatData*>(UDsPlayerCombatData::Get())),
    Saved(DuplicateObject<UDsPlayerCombatData>(Combat.Get(), GetTransientPackage())),
    BossData(const_cast<UDsBossData*>(GetMutableDefault<ADsBossCharacter>()->GetData())),
    SavedBoss(DuplicateObject<UDsBossData>(BossData.Get(), GetTransientPackage())) {
  CopyEditable(GetDefault<UDsPlayerCombatData>(), Combat.Get());
  CopyEditable(GetDefault<UDsBossData>(), BossData.Get());
 }
 ~FScopedCombatDefaults() {
  CopyEditable(Saved.Get(), Combat.Get());
  CopyEditable(SavedBoss.Get(), BossData.Get());
 }
private:
 static void CopyEditable(const UObject* From, UObject* To) {
  for (TFieldIterator<FProperty> It(From->GetClass()); It; ++It)
   if (It->HasAnyPropertyFlags(CPF_Edit))
    It->CopyCompleteValue_InContainer(To, From);
 }
 TStrongObjectPtr<UDsPlayerCombatData> Combat;
 TStrongObjectPtr<UDsPlayerCombatData> Saved;
 TStrongObjectPtr<UDsBossData> BossData;
 TStrongObjectPtr<UDsBossData> SavedBoss;
};
class FWorldFixture {
public:
  explicit FWorldFixture(bool bCreatePhysicsScene = false) {
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false)
        .CreatePhysicsScene(bCreatePhysicsScene)
        .CreateNavigation(false)
        .CreateAISystem(false)
        .RequiresHitProxies(false)
        .ShouldSimulatePhysics(false)
        .SetTransactional(false);
    static int32 Serial = 0;
    // 각 픽스처가 고유한 월드 이름을 사용하게 한다.
    World = UWorld::CreateWorld(
        EWorldType::Game, false,
        FName(*FString::Printf(TEXT("DrAutomationWorld_%d"), Serial++)),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
  }
  ~FWorldFixture() {
    if (World)
      World->DestroyWorld(false);
  }
  UWorld *World = nullptr;
private:
  FScopedCombatDefaults CombatDefaults;
};
inline void WarmUpTimers(UWorld &World) {
  // 헤드리스 환경에서는 프레임과 타이머 매니저를 직접 한 번 진행시킨다.
  ++GFrameCounter;
  World.GetTimerManager().Tick(0.f);
}
} // namespace DsTestFixture
