// 특성에서 전투 스탯을 파생하는 공식을 검증한다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/DsAttributeSet.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrStatFormulaTest,
                                 "DrSample.Stats.Trait.FormulaDerivesStats",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::ProductFilter)
bool FDrStatFormulaTest::RunTest(const FString &) {
  // 특성에서 전투 스탯을 파생하는 순수 공식을 검증한다.
  TestEqual(TEXT("활력 생명력"), FDsStatFormula::MaxHealth(100.f, 10.f), 220.f);
  TestEqual(TEXT("의지 저항 상한"), FDsStatFormula::ElementalResist(100.f),
            80.f);
  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrStatInitTest,
                                 "DrSample.Stats.Trait.InitFromStatsAppliesTraits",
                                 EAutomationTestFlags_ApplicationContextMask |
                                     EAutomationTestFlags::ProductFilter)
bool FDrStatInitTest::RunTest(const FString &) {
  // 데이터의 기본값과 특성이 속성 세트에 공식대로 반영되고 현재치가 최대치로 찬다.
  UDsAttributeSet *Attributes = NewObject<UDsAttributeSet>();
  FDsStatBlock Stats;
  Stats.Vitality = 10.f;
  Stats.Strength = 5.f;
  Stats.Spirit = 5.f;
  Stats.Dexterity = 10.f;
  Stats.Will = 50.f;
  Attributes->InitFromStats(Stats);
  TestEqual(TEXT("최대 생명력 = 100 + 체력×12"), Attributes->GetMaxHealth(), 220.f);
  TestEqual(TEXT("생명력은 최대치"), Attributes->GetHealth(), 220.f);
  TestEqual(TEXT("방어력 = 5 + 체력"), Attributes->GetDefense(), 15.f);
  TestEqual(TEXT("공격력 = 20 + 힘×2"), Attributes->GetAttackPower(), 30.f);
  TestEqual(TEXT("최대 포이즈 = 100 + 힘×3"), Attributes->GetMaxPoise(), 115.f);
  TestEqual(TEXT("최대 기력 = 100 + 기×4"), Attributes->GetMaxStamina(), 120.f);
  TestEqual(TEXT("기력 회복 = 25 + 기예×0.8"), Attributes->GetStaminaRegenRate(), 33.f);
  TestEqual(TEXT("속성 저항 상한 80"), Attributes->GetElementalResist(), 80.f);
  TestEqual(TEXT("특성 저장"), Attributes->GetVitality(), 10.f);
  return true;
}
#endif
