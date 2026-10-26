#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/DsHitDirection.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDsHitDirectionTest, "DrSample.Combat.HitReaction.DirectionAndBoundaries",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDsHitDirectionTest::RunTest(const FString&) {
 struct FCase { double Angle; EDsHitDirection Expected; };
 const FCase Cases[] = {
  {0, EDsHitDirection::Front}, {180, EDsHitDirection::Back},
  {-90, EDsHitDirection::Left}, {90, EDsHitDirection::Right},
  {-45, EDsHitDirection::Front}, {45, EDsHitDirection::Front},
  {-135, EDsHitDirection::Back}, {135, EDsHitDirection::Back},
  {-45.01, EDsHitDirection::Left}, {45.01, EDsHitDirection::Right},
  {-134.99, EDsHitDirection::Left}, {134.99, EDsHitDirection::Right},
  {-44.99, EDsHitDirection::Front}, {44.99, EDsHitDirection::Front},
  {-135.01, EDsHitDirection::Back}, {135.01, EDsHitDirection::Back}
 };
 for (double Yaw : {0., 71., -150.}) {
  for (const auto& Entry : Cases) {
   FVector Delta = FRotator(0, Yaw + Entry.Angle, 0).Vector() * 200.;
   Delta.Z = 900.;
   TestTrue(FString::Printf(TEXT("방위 %.2f, 공격 각도 %.2f"), Yaw, Entry.Angle),
    DsHitDirection(FRotator(35, Yaw, 20), Delta) == Entry.Expected);
  }
 }
 TestTrue(TEXT("겹친 위치는 정면"), DsHitDirection(FRotator::ZeroRotator, FVector::ZeroVector) == EDsHitDirection::Front);
 TestTrue(TEXT("높이만 다른 위치는 정면"), DsHitDirection(FRotator::ZeroRotator, FVector(0,0,100)) == EDsHitDirection::Front);
 int Front = 1, Back = 2, Left = 3, Right = 4;
 TestEqual(TEXT("후면 선택"), DsSelectHitMontage(EDsHitDirection::Back, &Front, &Back, &Left, &Right), &Back);
 TestEqual(TEXT("좌측 선택"), DsSelectHitMontage(EDsHitDirection::Left, &Front, &Back, &Left, &Right), &Left);
 TestEqual(TEXT("우측 선택"), DsSelectHitMontage(EDsHitDirection::Right, &Front, &Back, &Left, &Right), &Right);
 TestEqual(TEXT("미설정 방향은 정면 대체"), DsSelectHitMontage<int>(EDsHitDirection::Left, &Front, &Back, nullptr, &Right), &Front);
 return true;
}
#endif
