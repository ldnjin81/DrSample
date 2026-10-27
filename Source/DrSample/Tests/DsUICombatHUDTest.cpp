#include "UI/DsCombatHUDViewModel.h"
#include "UI/DsHUDDamageTrail.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Boss/DsBossCharacter.h"
#include "Boss/DsBossData.h"
#include "Characters/DsCharacter.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTestFixture.h"

namespace DrHUDTests {
struct FFixture {
  DsTestFixture::FWorldFixture Base{true};
  ADsCharacter *Player;
  ADsBossCharacter *Boss;
  UDsEncounterSubsystem *Encounter;
  UDsCombatHUDViewModel *VM;
  AActor *Checkpoint;
  AActor *Door;
  FFixture() {
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
    Base.World->InitializeActorsForPlay(FURL());
    Base.World->SetBegunPlay(true);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player = Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(), FVector(-1000, 0, 100), FRotator::ZeroRotator, Params);
    Boss = Base.World->SpawnActor<ADsBossCharacter>(ADsBossCharacter::StaticClass(), FVector(2000, 0, 100), FRotator::ZeroRotator, Params);
    Checkpoint = Base.World->SpawnActor<ATargetPoint>(FVector(-500, 0, 100), FRotator::ZeroRotator);
    Door = Base.World->SpawnActor<AStaticMeshActor>(FVector(0, 0, 800), FRotator::ZeroRotator);
    Encounter = Base.World->GetSubsystem<UDsEncounterSubsystem>();
    Encounter->BindActors(Player, Boss, Checkpoint, Door, Checkpoint);
    VM = NewObject<UDsCombatHUDViewModel>();
    VM->Initialize(Encounter);
    VM->BindPlayer(Player);
  }
  ~FFixture() {
    VM->Shutdown();
    Base.World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(Base.World);
  }
  void Enter() {
    Player->SetActorLocation(FVector(400, 0, 100));
    Encounter->Advance(.01f);
  }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrHUDAttributesTest, "DrSample.UI.CombatHUD.AttributesAndNotifications",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrHUDAttributesTest::RunTest(const FString &) {
  DrHUDTests::FFixture F;
  auto *ASC = F.Player->GetAbilitySystemComponent();
  int32 HealthNotifications = 0;
  const auto Handle = F.VM->AddFieldValueChangedDelegate(UDsCombatHUDViewModel::FFieldNotificationClassDescriptor::GetPlayerHealthRate,
    INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda(
      [&HealthNotifications](UObject *, UE::FieldNotification::FFieldId) { ++HealthNotifications; }));
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), 50.f);
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetMaxHealthAttribute(), 200.f);
  TestEqual(TEXT("속성 이벤트로 체력 비율 갱신"), F.VM->GetPlayerHealthRate(), .25f);
  TestEqual(TEXT("현재치·최대치 변경 모두 파생 비율 알림"), HealthNotifications, 2);
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetStaminaAttribute(), 30.f);
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetMaxStaminaAttribute(), 60.f);
  TestEqual(TEXT("스태미나 비율 갱신"), F.VM->GetPlayerStaminaRate(), .5f);
  ASC->SetNumericAttributeBase(UDsAttributeSet::GetMaxStaminaAttribute(), 0.f);
  TestEqual(TEXT("최대치 0 안전 처리"), F.VM->GetPlayerStaminaRate(), 0.f);
  F.VM->RemoveFieldValueChangedDelegate(UDsCombatHUDViewModel::FFieldNotificationClassDescriptor::GetPlayerHealthRate, Handle);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrHUDEncounterTest, "DrSample.UI.CombatHUD.EncounterRetryAndVictory",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrHUDEncounterTest::RunTest(const FString &) {
  DrHUDTests::FFixture F;
  TestFalse(TEXT("진입 전 보스 숨김"), F.VM->BossVisible);
  TestEqual(TEXT("보스 표시 이름"), F.VM->BossDisplayName.ToString(), F.Boss->GetData()->DisplayName.ToString());
  F.Enter();
  TestTrue(TEXT("전투 진입 즉시 보스 표시"), F.VM->BossVisible);
  auto *ASC = Cast<UDsAbilitySystemComponent>(F.Player->GetAbilitySystemComponent());
  F.Boss->GetAttributes()->SetHealth(1000.f);
  TestEqual(TEXT("보스 체력 비율 갱신"), F.VM->BossHealthRate, .5f);
  ASC->ApplyDamage(ASC, 100000.f, 0.f, F.Boss);
  TestTrue(TEXT("사망 표시"), F.VM->IsPlayerDead());
  TestFalse(TEXT("사망 때 보스 숨김"), F.VM->BossVisible);
  F.Encounter->Advance(3.1f);
  TestFalse(TEXT("재도전 시 사망 표시 해제"), F.VM->IsPlayerDead());
  TestEqual(TEXT("부활 체력 이벤트"), F.VM->GetPlayerHealthRate(), 1.f);
  TestEqual(TEXT("리셋 보스 체력 이벤트"), F.VM->BossHealthRate, 1.f);
  F.Enter();
  ASC->ApplyDamage(F.Boss->GetDrASC(), 100000.f, 0.f, F.Player);
  TestTrue(TEXT("승리 표시"), F.VM->IsVictory());
  TestFalse(TEXT("승리 후 보스 숨김"), F.VM->BossVisible);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrHUDRebindTest, "DrSample.UI.CombatHUD.RebindAndShutdown",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDrHUDRebindTest::RunTest(const FString &) {
  DrHUDTests::FFixture F;
  F.Player->GetAttributeSet()->SetHealth(60.f);
  auto *Other = F.Base.World->SpawnActor<ADsCharacter>();
  F.VM->BindPlayer(Other);
  F.Player->GetAttributeSet()->SetHealth(20.f);
  TestEqual(TEXT("이전 플레이어 이벤트 해제"), F.VM->PlayerHealth, Other->GetAttributeSet()->GetHealth());
  Other->GetAttributeSet()->SetHealth(40.f);
  TestEqual(TEXT("새 플레이어 이벤트 연결"), F.VM->PlayerHealth, 40.f);
  F.Enter();
  F.Encounter->BindActors(F.Player, nullptr, nullptr, nullptr, nullptr);
  TestFalse(TEXT("보스 연결 해제 즉시 숨김"), F.VM->BossVisible);
  F.VM->Shutdown();
  Other->GetAttributeSet()->SetHealth(10.f);
  TestEqual(TEXT("종료 후 이벤트 해제"), F.VM->PlayerHealth, 40.f);
  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsHUDDamageTrailTest, "DrSample.UI.CombatHUD.DamageTrailTiming",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDsHUDDamageTrailTest::RunTest(const FString &) {
  FDsHUDDamageTrail Trail;
  Trail.SetTarget(.5f, 10.0);
  Trail.Advance(10.39);
  TestEqual(TEXT("피해 후 0.4초 동안 잔상 유지"), Trail.Value, 1.f);
  Trail.Advance(10.55);
  TestTrue(TEXT("대기 후 부드럽게 감소"), FMath::IsNearlyEqual(Trail.Value, .75f));
  Trail.SetTarget(.25f, 10.56);
  Trail.Advance(10.95);
  TestTrue(TEXT("연속 피해 시 남은 잔상 유지"), FMath::IsNearlyEqual(Trail.Value, .75f));
  Trail.Advance(11.27);
  TestEqual(TEXT("목표 도달"), Trail.Value, .25f);
  TestFalse(TEXT("완료 후 갱신 정지"), Trail.bActive);
  Trail.SetTarget(.1f, 12.0);
  Trail.SetTarget(1.f, 12.1);
  TestEqual(TEXT("회복·재도전 즉시 잔상 리셋"), Trail.Value, 1.f);
  TestFalse(TEXT("회복 후 갱신 정지"), Trail.bActive);
  return true;
}
#endif
