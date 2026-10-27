// 상태 효과, 피해 계산, 특성 공식을 검증하는 자동화 테스트이다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Abilities/DsGameplayAbility_ApplyEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpec.h"
#include "Core/DsGameplayTags.h"
#include "Abilities/Effects/DsEffectTypes.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTableTestFixture.h"
#include "DrTableSubsystem.h"
#include "TableData/Generated/DsEffectDefsTable.h"
#include "Tests/DsTestFixture.h"

class FDsEffectSystemTestAccess {
public:
  static void RemoveTimedEffect(UDsAbilitySystemComponent *ASC,
                                const FGameplayTag &Tag) {
    ASC->RemoveTimedEffect(Tag);
  }
  static float GetTimedDamage(const UDsAbilitySystemComponent *ASC,
                              const FGameplayTag &Tag) {
    const FDsResolvedEffect *Effect = ASC->TimedEffects.Find(Tag);
    return Effect ? Effect->DamagePerTick : 0.f;
  }
};

namespace DsEffectTests {
struct FAbilityOwner {
  explicit FAbilityOwner(UWorld *World) {
    Actor = World->SpawnActor<AActor>();
    ASC = NewObject<UDsAbilitySystemComponent>(Actor);
    Actor->AddInstanceComponent(ASC);
    ASC->RegisterComponent();
    Attributes = NewObject<UDsAttributeSet>(Actor);
    ASC->AddAttributeSetSubobject(Attributes);
    ASC->InitAbilityActorInfo(Actor, Actor);
  }
  AActor *Actor = nullptr;
  UDsAbilitySystemComponent *ASC = nullptr;
  UDsAttributeSet *Attributes = nullptr;
};
} // namespace DsEffectTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrShieldTest, "DrSample.Effects.Shield.AbsorbsThenExpiresWhenDepleted",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrShieldTest::RunTest(const FString &) {
  // 보호막 흡수량과 소진 시 태그 해제를 함께 검증한다.
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect Effect;
  Effect.StateTag = DsTags::State_Buff_Shield;
  Effect.Duration = 6.f;
  Effect.Magnitude = 120.f;
  Owner.ASC->ApplyStatusEffect(Effect, Owner.Actor);
  TestEqual(TEXT("첫 흡수"), Owner.ASC->AbsorbWithShield(50.f), 0.f);
  TestEqual(TEXT("초과 잔여"), Owner.ASC->AbsorbWithShield(100.f), 30.f);
  TestFalse(TEXT("소진 태그 해제"),
            Owner.ASC->HasMatchingGameplayTag(DsTags::State_Buff_Shield));
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrModifierProductTest, "DrSample.Effects.Modifier.AggregatesAsProduct",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrModifierProductTest::RunTest(const FString &) {
  // 활성 효과의 이동 및 피해 배수가 곱으로 합산되는지 검증한다.
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect EnrageEffect;
  EnrageEffect.StateTag = DsTags::State_Buff_Enrage;
  EnrageEffect.MoveSpeedMultiplier = .8f;
  EnrageEffect.DamageTakenMultiplier = 1.2f;
  EnrageEffect.DamageDealtMultiplier = 1.25f;
  FDsResolvedEffect GuardEffect;
  GuardEffect.StateTag = DsTags::State_Buff_Guard;
  GuardEffect.MoveSpeedMultiplier = .5f;
  GuardEffect.DamageTakenMultiplier = .5f;
  GuardEffect.DamageDealtMultiplier = .8f;
  Owner.ASC->AddPermanentEffect(EnrageEffect);
  Owner.ASC->AddPermanentEffect(GuardEffect);
  TestEqual(TEXT("이동 배수 곱"), Owner.ASC->GetMoveSpeedMultiplier(), .4f,
            .001f);
  TestEqual(TEXT("받는 피해 배수 곱"), Owner.ASC->GetIncomingDamageMultiplier(),
            .6f, .001f);
  TestEqual(TEXT("주는 피해 배수 곱"), Owner.ASC->GetOutgoingDamageMultiplier(),
            1.f, .001f);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrPermanentTimedTest,
    "DrSample.Effects.Modifier.PermanentSurvivesTimedExpiry",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrPermanentTimedTest::RunTest(const FString &) {
  // 같은 태그의 한시 효과 만료가 영구 효과를 훼손하지 않는지 검증한다.
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  FDsResolvedEffect Permanent;
  Permanent.StateTag = DsTags::State_Buff_Guard;
  Permanent.DamageTakenMultiplier = .7f;
  Owner.ASC->AddPermanentEffect(Permanent);
  FDsResolvedEffect Timed = Permanent;
  Timed.Duration = 1.f;
  Timed.DamageTakenMultiplier = .5f;
  Owner.ASC->ApplyStatusEffect(Timed, Owner.Actor);
  TestEqual(TEXT("한시 우선"), Owner.ASC->GetIncomingDamageMultiplier(), .5f);
  FDsEffectSystemTestAccess::RemoveTimedEffect(Owner.ASC, Timed.StateTag);
  TestEqual(TEXT("영구 유지"), Owner.ASC->GetIncomingDamageMultiplier(), .7f);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrDotTest,
                                 "DrSample.Effects.DamageOverTime.Ticks",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::ProductFilter)
bool FDrDotTest::RunTest(const FString &) {
  // 헤드리스 월드에서 주기 피해가 ASC 틱 누적으로 들어가는지 검증한다.
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  const float HealthBefore = Owner.Attributes->GetHealth();
  FDsResolvedEffect Effect;
  Effect.StateTag = DsTags::State_Debuff_Burn;
  Effect.Duration = 2.f;
  Effect.DamagePerTick = 10.f;
  Effect.Period = .5f;
  Owner.ASC->ApplyStatusEffect(Effect, Owner.Actor);
  Owner.ASC->TickComponent(.6f, LEVELTICK_All, nullptr);
  TestTrue(TEXT("DoT 체력 감소"), Owner.Attributes->GetHealth() < HealthBefore);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrEffectStackTest, "DrSample.Effects.Stack.AccumulatesCapsAndExpires",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrEffectStackTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  Owner.ASC->SetNumericAttributeBase(UDsAttributeSet::GetDefenseAttribute(),
                                     0.f);
  FDsResolvedEffect Burn;
  Burn.StateTag = DsTags::State_Debuff_Burn;
  Burn.Duration = 2.f;
  Burn.DamagePerTick = 10.f;
  Burn.Period = 1.f;
  Burn.StackRule = EDsEffectStackRule::Accumulate;
  Burn.StackBonus = {0, 5, 10};
  const float ExpectedDamage[] = {10.f, 10.5f, 11.f, 11.f};
  const int32 ExpectedCount[] = {1, 2, 3, 3};
  for (int32 Index = 0; Index < 4; ++Index) {
    Owner.ASC->ApplyStatusEffect(Burn, Owner.Actor);
    TestEqual(TEXT("누적 단계와 상한"),
              Owner.ASC->GetStackCount(Burn.StateTag), ExpectedCount[Index]);
    TestEqual(TEXT("원본 기준 주기 피해"),
              FDsEffectSystemTestAccess::GetTimedDamage(Owner.ASC, Burn.StateTag),
              ExpectedDamage[Index], .001f);
  }
  const float HealthBeforeTick = Owner.Attributes->GetHealth();
  Owner.ASC->TickComponent(1.1f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("단계 변경 후 주기 피해가 새 피해량 사용"),
            Owner.Attributes->GetHealth(), HealthBeforeTick - 11.f, .001f);
  Owner.ASC->TickComponent(1.f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("만료 후 단계 초기화"),
            Owner.ASC->GetStackCount(Burn.StateTag), 0);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrEffectTickPhaseTest, "DrSample.Effects.Stack.ReapplyPreservesTickPhase",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrEffectTickPhaseTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  Owner.ASC->SetNumericAttributeBase(UDsAttributeSet::GetDefenseAttribute(),
                                     0.f);
  FDsResolvedEffect Burn;
  Burn.StateTag = DsTags::State_Debuff_Burn;
  Burn.Duration = 3.f;
  Burn.DamagePerTick = 10.f;
  Burn.Period = 1.f;
  const float HealthBefore = Owner.Attributes->GetHealth();
  Owner.ASC->ApplyStatusEffect(Burn, Owner.Actor);
  Owner.ASC->TickComponent(.4f, LEVELTICK_All, nullptr);
  Owner.ASC->ApplyStatusEffect(Burn, Owner.Actor);
  Owner.ASC->TickComponent(.4f, LEVELTICK_All, nullptr);
  Owner.ASC->ApplyStatusEffect(Burn, Owner.Actor);
  Owner.ASC->TickComponent(.3f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("주기보다 빠른 재적용에도 틱 피해 발생"),
            Owner.Attributes->GetHealth(), HealthBefore - 10.f, .001f);
  TestEqual(TEXT("갱신 규칙은 1단계 유지"),
            Owner.ASC->GetStackCount(Burn.StateTag), 1);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrEffectStackRulesTest, "DrSample.Effects.Stack.MultiplierAndRefresh",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrEffectStackRulesTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);
  Owner.ASC->SetNumericAttributeBase(UDsAttributeSet::GetDefenseAttribute(),
                                     0.f);
  FDsResolvedEffect Freeze;
  Freeze.StateTag = DsTags::State_Debuff_Chill;
  Freeze.Duration = 4.f;
  Freeze.MoveSpeedMultiplier = .5f;
  Freeze.StackRule = EDsEffectStackRule::Accumulate;
  Freeze.StackBonus = {0, 3, 6};
  for (int32 Index = 0; Index < 3; ++Index)
    Owner.ASC->ApplyStatusEffect(Freeze, Owner.Actor);
  TestEqual(TEXT("빙결 3단계는 이동 속도 0.47배"),
            Owner.ASC->GetMoveSpeedMultiplier(), .47f, .001f);

  FDsResolvedEffect Shield;
  Shield.StateTag = DsTags::State_Buff_Shield;
  Shield.Duration = 4.f;
  Shield.Magnitude = 120.f;
  Shield.StackRule = EDsEffectStackRule::Refresh;
  Shield.StackBonus = {0, 10, 20};
  Owner.ASC->ApplyStatusEffect(Shield, Owner.Actor);
  TestEqual(TEXT("첫 보호막 단계"), Owner.ASC->GetStackCount(Shield.StateTag), 1);
  TestEqual(TEXT("첫 보호막 흡수"), Owner.ASC->AbsorbWithShield(50.f), 0.f);
  Owner.ASC->ApplyStatusEffect(Shield, Owner.Actor);
  TestEqual(TEXT("갱신 후에도 1단계"), Owner.ASC->GetStackCount(Shield.StateTag),
            1);
  TestEqual(TEXT("갱신으로 120 재충전"), Owner.ASC->AbsorbWithShield(120.f),
            0.f);
  TestEqual(TEXT("보호막 소진 후 0단계"),
            Owner.ASC->GetStackCount(Shield.StateTag), 0);

  FDsResolvedEffect Proc;
  Proc.StateTag = DsTags::State_Debuff_Burn;
  Proc.Duration = 4.f;
  Proc.ProcInstantDamageCurHpPct = 10.f;
  Proc.StackRule = EDsEffectStackRule::Accumulate;
  Proc.StackBonus = {0, 5, 10};
  for (int32 Index = 0; Index < 3; ++Index)
    Owner.ASC->ApplyStatusEffect(Proc, Owner.Actor);
  const float HealthBeforeCapApply = Owner.Attributes->GetHealth();
  Owner.ASC->ApplyStatusEffect(Proc, Owner.Actor);
  TestEqual(TEXT("최대 단계 재적용 후에도 3단계"),
            Owner.ASC->GetStackCount(Proc.StateTag), 3);
  TestEqual(TEXT("최대 단계에서도 즉시 Proc 발동"),
            Owner.Attributes->GetHealth(), HealthBeforeCapApply * .89f,
            .001f);
  return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrEffectDataPathTest,
    "DrSample.Effects.DataPath.BakedRowAppliesToAbilitySystem",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrEffectDataPathTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Owner(Fixture.World);

  DsTableTests::FFixture TableFixture;
  UDrTableSubsystem *Data = TableFixture.Data.Get();
  Data->RegisterAutoTables();
  Data->LoadAllTables();

  const FDsEffectDefsRow *BurnRow =
      Data->FindRowByKey<FDsEffectDefsRow>(TEXT("DA_EffectDefs"), 2001);
  TestNotNull(TEXT("int32 Id로 화상 행 조회"), BurnRow);
  if (!BurnRow)
    return false;

  const FDsResolvedEffect Burn = DsEffects::Resolve(*BurnRow);
  TestEqual(TEXT("화상은 누적 규칙"), static_cast<int32>(Burn.StackRule),
            static_cast<int32>(EDsEffectStackRule::Accumulate));
  TestEqual(TEXT("화상 보너스 단계 수"), Burn.StackBonus.Num(), 3);
  if (Burn.StackBonus.Num() == 3)
    TestEqual(TEXT("화상 3단계 보너스"), Burn.StackBonus[2], 10);
  TestEqual(TEXT("이동 배수 열 기본값"), Burn.MoveSpeedMultiplier, 1.f);
  TestEqual(TEXT("받는 피해 배수 열 기본값"),
            Burn.DamageTakenMultiplier, 1.f);
  TestEqual(TEXT("주는 피해 배수 열 기본값"), Burn.DamageDealtMultiplier,
            1.f);
  const float HealthBefore = Owner.Attributes->GetHealth();
  Owner.ASC->ApplyStatusEffect(Burn, Owner.Actor);
  TestTrue(TEXT("데이터 Proc가 현재 체력을 감소"),
           Owner.Attributes->GetHealth() < HealthBefore);

  const FDsEffectDefsRow *FreezeRow =
      Data->FindRowByKey<FDsEffectDefsRow>(TEXT("DA_EffectDefs"), 2002);
  TestNotNull(TEXT("int32 Id로 빙결 행 조회"), FreezeRow);
  if (FreezeRow) {
    const FDsResolvedEffect Freeze = DsEffects::Resolve(*FreezeRow);
    TestEqual(TEXT("빙결은 누적 규칙"), static_cast<int32>(Freeze.StackRule),
              static_cast<int32>(EDsEffectStackRule::Accumulate));
    Owner.ASC->ApplyStatusEffect(Freeze, Owner.Actor);
    TestEqual(TEXT("데이터 이동 배수 적용"),
              Owner.ASC->GetMoveSpeedMultiplier(), .5f);
  }
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrEffectApplyRoutingTest, "DrSample.Effects.DataPath.ApplyToRouting",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrEffectApplyRoutingTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Caster(Fixture.World);
  DsEffectTests::FAbilityOwner Victim(Fixture.World);

  DsTableTests::FFixture TableFixture;
  UDrTableSubsystem *Data = TableFixture.Data.Get();
  TestNotNull(TEXT("격리된 테이블 저장소 생성"), Data);
  if (!Data)
    return false;
  Data->RegisterAutoTables();
  Data->LoadAllTables();

  const float CasterHealthBefore = Caster.Attributes->GetHealth();
  const float VictimHealthBefore = Victim.Attributes->GetHealth();
  AddExpectedError(TEXT("[Effect] 효과 Id를 찾지 못했습니다"),
                   EAutomationExpectedErrorFlags::Contains, 1, false);
  TestFalse(TEXT("없는 Id는 실패"),
            Caster.ASC->ApplyEffectById(999999, Victim.ASC, Caster.Actor,
                                        Data));
  TestEqual(TEXT("없는 Id가 시전자에게 영향 없음"),
            Caster.Attributes->GetHealth(), CasterHealthBefore);
  TestEqual(TEXT("없는 Id가 피격자에게 영향 없음"),
            Victim.Attributes->GetHealth(), VictimHealthBefore);

  AddExpectedError(TEXT("[Effect] 피격 대상 ASC가 없어"),
                   EAutomationExpectedErrorFlags::Contains, 1, false);
  TestFalse(TEXT("피격 대상 누락은 실패"),
            Caster.ASC->ApplyEffectById(2001, nullptr, Caster.Actor, Data));
  TestEqual(TEXT("대상 누락이 시전자에게 영향 없음"),
            Caster.Attributes->GetHealth(), CasterHealthBefore);
  TestFalse(TEXT("대상 누락이 시전자에게 디버프를 부여하지 않음"),
            Caster.ASC->HasMatchingGameplayTag(DsTags::State_Debuff_Burn));

  TestTrue(TEXT("Victim 행 적용 성공"),
           Caster.ASC->ApplyEffectById(2001, Victim.ASC, Caster.Actor, Data));
  TestTrue(TEXT("Victim 행이 피격자에게 적용"),
           Victim.ASC->HasMatchingGameplayTag(DsTags::State_Debuff_Burn));
  TestTrue(TEXT("Victim 행이 피격자 체력을 감소"),
           Victim.Attributes->GetHealth() < VictimHealthBefore);
  TestEqual(TEXT("Victim 행이 시전자 체력에 영향 없음"),
            Caster.Attributes->GetHealth(), CasterHealthBefore);
  TestFalse(TEXT("Victim 행이 시전자에게 적용되지 않음"),
            Caster.ASC->HasMatchingGameplayTag(DsTags::State_Debuff_Burn));

  TestTrue(TEXT("Caster 행 적용 성공"),
           Caster.ASC->ApplyEffectById(2003, Victim.ASC, Caster.Actor, Data));
  TestTrue(TEXT("Caster 행이 시전자에게 적용"),
           Caster.ASC->HasMatchingGameplayTag(DsTags::State_Buff_Shield));
  TestFalse(TEXT("Caster 행이 피격자에게 적용되지 않음"),
            Victim.ASC->HasMatchingGameplayTag(DsTags::State_Buff_Shield));
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrApplyEffectAbilityTest, "DrSample.Effects.DataPath.AbilityAppliesEffect",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrApplyEffectAbilityTest::RunTest(const FString &) {
  DsTestFixture::FWorldFixture Fixture;
  DsEffectTests::FAbilityOwner Caster(Fixture.World);
  DsEffectTests::FAbilityOwner Victim(Fixture.World);

  DsTableTests::FFixture TableFixture;
  UDrTableSubsystem *Data = TableFixture.Data.Get();
  TestNotNull(TEXT("게임 전역 테이블 저장소 조회"), Data);
  if (!Data)
    return false;
  // 실제 어빌리티 호출부는 주입 인자 없이 게임 전역 저장소를 사용한다.
  Data->RegisterAutoTables();
  Data->LoadAllTables();

  FGameplayAbilitySpec Spec(UDsGameplayAbility_ApplyEffect::StaticClass(), 1);
  const FGameplayAbilitySpecHandle Handle = Caster.ASC->GiveAbility(Spec);
  FGameplayAbilitySpec *GrantedSpec =
      Caster.ASC->FindAbilitySpecFromHandle(Handle);
  UDsGameplayAbility_ApplyEffect *AbilityInstance =
      GrantedSpec
          ? Cast<UDsGameplayAbility_ApplyEffect>(GrantedSpec->GetPrimaryInstance())
          : nullptr;
  TestNotNull(TEXT("어빌리티 인스턴스 생성"), AbilityInstance);
  if (AbilityInstance)
    AbilityInstance->EffectId = 2002;

  FGameplayEventData EventData;
  EventData.Instigator = Caster.Actor;
  EventData.Target = Victim.Actor;
  const bool bActivated =
      AbilityInstance && Caster.ASC->InternalTryActivateAbility(
                             Handle, FPredictionKey(), nullptr, nullptr,
                             &EventData);

  TestTrue(TEXT("어빌리티가 발동됨"), bActivated);
  TestTrue(TEXT("어빌리티가 이벤트 대상에게 효과 적용"),
           Victim.ASC->HasMatchingGameplayTag(DsTags::State_Debuff_Chill));
  TestFalse(TEXT("어빌리티가 시전자에게 피해 효과를 적용하지 않음"),
            Caster.ASC->HasMatchingGameplayTag(DsTags::State_Debuff_Chill));
  TestEqual(TEXT("어빌리티 효과의 이동 배수"),
            Victim.ASC->GetMoveSpeedMultiplier(), .5f);
  return true;
}
#endif
