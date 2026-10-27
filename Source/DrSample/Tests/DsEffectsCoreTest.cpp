// 상태 효과 체계의 공통 규칙(배수 곱, 누적 상한, 만료, 영구·한시 우선순위, 쿨다운)을 속성 세트 없이 검증한다.
// 체력·보호막·주기 피해처럼 속성이 필요한 동작은 DsEffectsTest가 검증한다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include "Tests/DsTestFixture.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "GameFramework/Actor.h"

namespace DsEffectCoreTests {
/** 속성 세트 없이 ASC만 붙인 액터. */
struct FAbilityOwner {
  explicit FAbilityOwner(UWorld *World) {
    Actor = World->SpawnActor<AActor>();
    ASC = NewObject<UDsAbilitySystemComponent>(Actor);
    Actor->AddInstanceComponent(ASC);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Actor, Actor);
  }
  AActor *Actor = nullptr;
  UDsAbilitySystemComponent *ASC = nullptr;
};
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_Slow, "DrSample.Test.Slow");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_Guard, "DrSample.Test.Guard");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_Rage, "DrSample.Test.Rage");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_Cooldown, "DrSample.Test.Cooldown");
constexpr EAutomationTestFlags Flags =
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
} // namespace DsEffectCoreTests
using namespace DsEffectCoreTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsCoreModifierProductTest,
                                 "DrSample.Effects.Core.ModifiersAggregateAsProduct", Flags)
bool FDsCoreModifierProductTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect Rage;
  Rage.StateTag = TAG_Test_Rage;
  Rage.MoveSpeedMultiplier = .8f;
  Rage.DamageTakenMultiplier = 1.2f;
  Rage.DamageDealtMultiplier = 1.25f;
  FDsResolvedEffect Guard;
  Guard.StateTag = TAG_Test_Guard;
  Guard.MoveSpeedMultiplier = .5f;
  Guard.DamageTakenMultiplier = .5f;
  Guard.DamageDealtMultiplier = .8f;
  Owner.ASC->AddPermanentEffect(Rage);
  Owner.ASC->AddPermanentEffect(Guard);
  TestEqual(TEXT("이동 배수 곱"), Owner.ASC->GetMoveSpeedMultiplier(), .4f, .001f);
  TestEqual(TEXT("받는 피해 배수 곱"), Owner.ASC->GetIncomingDamageMultiplier(), .6f, .001f);
  TestEqual(TEXT("주는 피해 배수 곱"), Owner.ASC->GetOutgoingDamageMultiplier(), 1.f, .001f);
  TestTrue(TEXT("영구 효과 상태 태그"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Guard));
  Owner.ASC->RemovePermanentEffect(TAG_Test_Guard);
  TestFalse(TEXT("영구 효과 제거 시 태그 해제"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Guard));
  TestEqual(TEXT("제거 후 배수"), Owner.ASC->GetMoveSpeedMultiplier(), .8f, .001f);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsCoreTimedOverPermanentTest,
                                 "DrSample.Effects.Core.TimedOverridesPermanentUntilExpiry", Flags)
bool FDsCoreTimedOverPermanentTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect Permanent;
  Permanent.StateTag = TAG_Test_Guard;
  Permanent.DamageTakenMultiplier = .7f;
  Owner.ASC->AddPermanentEffect(Permanent);
  FDsResolvedEffect Timed = Permanent;
  Timed.Duration = 1.f;
  Timed.DamageTakenMultiplier = .5f;
  Owner.ASC->ApplyStatusEffect(Timed, Owner.Actor);
  TestEqual(TEXT("같은 태그는 한시 효과 우선"), Owner.ASC->GetIncomingDamageMultiplier(), .5f, .001f);
  Owner.ASC->TickComponent(1.1f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("한시 만료 후 영구 효과 유지"), Owner.ASC->GetIncomingDamageMultiplier(), .7f, .001f);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsCoreStackTest,
                                 "DrSample.Effects.Core.AccumulateCapsScalesAndExpires", Flags)
bool FDsCoreStackTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect Slow;
  Slow.StateTag = TAG_Test_Slow;
  Slow.Duration = 2.f;
  Slow.MoveSpeedMultiplier = .5f;
  Slow.StackRule = EDsEffectStackRule::Accumulate;
  Slow.StackBonus = {0, 3, 6};
  const int32 ExpectedCount[] = {1, 2, 3, 3};
  for (int32 Index = 0; Index < 4; ++Index) {
    Owner.ASC->ApplyStatusEffect(Slow, Owner.Actor);
    TestEqual(TEXT("누적 단계와 상한"), Owner.ASC->GetStackCount(TAG_Test_Slow), ExpectedCount[Index]);
  }
  // 3단계 보너스 6%: 1 + (0.5 - 1) * 1.06 = 0.47
  TestEqual(TEXT("단계 보너스가 배수에 반영"), Owner.ASC->GetMoveSpeedMultiplier(), .47f, .001f);
  TestTrue(TEXT("한시 효과 상태 태그"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Slow));
  Owner.ASC->TickComponent(2.1f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("만료 후 단계 초기화"), Owner.ASC->GetStackCount(TAG_Test_Slow), 0);
  TestFalse(TEXT("만료 후 태그 해제"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Slow));
  TestEqual(TEXT("만료 후 배수 복귀"), Owner.ASC->GetMoveSpeedMultiplier(), 1.f, .001f);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsCoreRefreshTest,
                                 "DrSample.Effects.Core.RefreshKeepsSingleStack", Flags)
bool FDsCoreRefreshTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect Rage;
  Rage.StateTag = TAG_Test_Rage;
  Rage.Duration = 1.f;
  Rage.DamageDealtMultiplier = 1.5f;
  Rage.StackRule = EDsEffectStackRule::Refresh;
  Rage.StackBonus = {0, 10, 20};
  Owner.ASC->ApplyStatusEffect(Rage, Owner.Actor);
  Owner.ASC->TickComponent(.8f, LEVELTICK_All, nullptr);
  Owner.ASC->ApplyStatusEffect(Rage, Owner.Actor);
  TestEqual(TEXT("갱신 규칙은 1단계 유지"), Owner.ASC->GetStackCount(TAG_Test_Rage), 1);
  TestEqual(TEXT("갱신 규칙은 단계 보너스 없음"), Owner.ASC->GetOutgoingDamageMultiplier(), 1.5f, .001f);
  Owner.ASC->TickComponent(.8f, LEVELTICK_All, nullptr);
  TestTrue(TEXT("재적용으로 지속 시간 갱신"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Rage));
  Owner.ASC->TickComponent(.3f, LEVELTICK_All, nullptr);
  TestFalse(TEXT("갱신된 지속 시간 뒤 만료"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Rage));
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsCoreCooldownTest,
                                 "DrSample.Effects.Core.CooldownTagExpires", Flags)
bool FDsCoreCooldownTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  FAbilityOwner Owner(Fixture.World);
  Owner.ASC->StartCooldown(TAG_Test_Cooldown, 1.f);
  TestTrue(TEXT("쿨다운 태그"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Cooldown));
  Owner.ASC->TickComponent(.4f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("남은 쿨다운"), Owner.ASC->GetCooldownRemaining(TAG_Test_Cooldown), .6f, .001f);
  Owner.ASC->TickComponent(.7f, LEVELTICK_All, nullptr);
  TestFalse(TEXT("쿨다운 종료 시 태그 해제"), Owner.ASC->HasMatchingGameplayTag(TAG_Test_Cooldown));
  TestEqual(TEXT("종료 후 남은 시간 0"), Owner.ASC->GetCooldownRemaining(TAG_Test_Cooldown), 0.f);
  return true;
}
#endif
