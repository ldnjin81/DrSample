// 샘플 테이블(EffectDefs)이 DrTable 플러그인으로 등록·로드·조회되는지 검증한다.
// 레지스트리 자체의 동작(정렬 계약, CSR, 해시 검사 등)은 플러그인 테스트(DrTable.*)가 맡는다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/DsTableTestFixture.h"
#include "TableData/Generated/DsEffectDefsTable.h"
#include "TableData/Generated/DsGeneratedTables.h"
#include "DrTableSubsystem.h"
#include "TableData/Generated/DsEffectDefsRow.h"
#include "Abilities/Effects/DsEffectTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrGeneratedRowLookupTest, "DrSample.Tables.GeneratedRowLookup",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrGeneratedRowLookupTest::RunTest(const FString &) {
  DsTableTests::FFixture TableFixture;
  const FDsEffectDefsRow *Burn = FDsEffectDefsRow::Find(2001);
  TestNotNull(TEXT("생성 기본키 조회"), Burn);
  if (Burn) {
    TestEqual(TEXT("화상 지속 시간"), Burn->Duration, 3.f);
    // 행을 런타임 효과로 해석하면 단계 보너스가 풀에서 펼쳐진다.
    const FDsResolvedEffect Resolved = DsEffects::Resolve(*Burn);
    TestEqual(TEXT("해석된 주기 피해"), Resolved.DamagePerTick, 5.f);
    TestEqual(TEXT("해석된 단계 보너스 수"), Resolved.StackBonus.Num(), 3);
    if (Resolved.StackBonus.Num() == 3)
      TestEqual(TEXT("해석된 3단계 보너스"), Resolved.StackBonus[2], 10);
  }

  const TArray<const FDsEffectDefsRow *> Debuffs =
      FDsEffectDefsRow::FindByCategory(EDsEffectCategory::Debuff);
  TestEqual(TEXT("디버프 행 수"), Debuffs.Num(), 2);
  TSet<int32> Ids;
  for (const FDsEffectDefsRow *Row : Debuffs)
    Ids.Add(Row->Id);
  TestTrue(TEXT("디버프 기본키 2001·2002"),
           Ids.Contains(2001) && Ids.Contains(2002));
  TestEqual(TEXT("전체 행 수"), FDsEffectDefsRow::GetAll().Num(), 3);
  TestNull(TEXT("없는 기본키"), FDsEffectDefsRow::Find(-1));

  UDrTableSubsystem::SetOverride(nullptr);
  TestNull(TEXT("레지스트리 없이 기본키 조회"), FDsEffectDefsRow::Find(2001));
  TestEqual(TEXT("레지스트리 없이 서브키 조회"),
            FDsEffectDefsRow::FindByCategory(EDsEffectCategory::Debuff).Num(),
            0);
  TestEqual(TEXT("레지스트리 없이 전체 조회"), FDsEffectDefsRow::GetAll().Num(),
            0);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDrEmptyTablesTest,
    "DrSample.Tables.NoRegisteredTablesLoadsWithoutWarning",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
bool FDrEmptyTablesTest::RunTest(const FString &) {
  // 등록된 테이블이 없으면 자동 로드 대상도 없고 안전하게 조회된다.
  UDrTableSubsystem *Data = NewObject<UDrTableSubsystem>();
  Data->LoadAllTables();
  TestNull(TEXT("등록 안 된 테이블 조회"),
           Data->FindTable<FDsEffectDefsRow>(TEXT("Missing")));
  return true;
}
#endif
