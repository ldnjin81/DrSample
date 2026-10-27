// 주입 액터와 틱 누적으로 체크포인트·재도전·승리의 경계를 검증한다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/DsEncounterTestFixture.h"

#define DR_ENCOUNTER_TEST(C, P) IMPLEMENT_SIMPLE_AUTOMATION_TEST(C, P, EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)


DR_ENCOUNTER_TEST(FDrCheckpointTest, "DrSample.Encounter.Checkpoint.ActivatesInRadius")
bool FDrCheckpointTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Encounter->Advance(.1f);
  TestFalse(TEXT("반경 밖 미활성"), F.Encounter->IsCheckpointActive());
  TestTrue(TEXT("미활성 부활은 시작점"), F.Encounter->GetRespawnTransform().GetLocation().Equals(F.Start->GetActorLocation()));
  F.Activate();
  TestTrue(TEXT("반경 안 활성"), F.Encounter->IsCheckpointActive());
  TestTrue(TEXT("활성 부활은 체크포인트"), F.Encounter->GetRespawnTransform().GetLocation().Equals(F.Checkpoint->GetActorLocation()));
  TestTrue(TEXT("부활 방향은 아레나"), F.Encounter->GetRespawnTransform().GetRotation().GetForwardVector().Equals(FVector::ForwardVector));
  return true;
}

DR_ENCOUNTER_TEST(FDrEntryTest, "DrSample.Encounter.Arena.EntryClosesDoorAndStartsFight")
bool FDrEntryTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  int32 Changes = 0;
  F.Encounter->OnStateChanged.AddLambda([&Changes](EDsEncounterState Old, EDsEncounterState New) {
    if (Old == EDsEncounterState::Idle && New == EDsEncounterState::Fighting) ++Changes;
  });
  F.Player->SetActorLocation(FVector(299, 0, 100));
  F.Encounter->Advance(.1f);
  F.Boss->GetBrain()->TickComponent(.1f, LEVELTICK_All, nullptr);
  TestTrue(TEXT("깊이 전 대기"), F.Encounter->GetState() == EDsEncounterState::Idle);
  TestTrue(TEXT("감지 범위 안이어도 보스 대기"), F.Boss->GetBrain()->GetState() == EDsBossState::Idle);
  F.Player->SetActorLocation(FVector(301, 0, 100));
  F.Encounter->Advance(.1f);
  TestTrue(TEXT("진입 전투 시작"), F.Encounter->GetState() == EDsEncounterState::Fighting);
  F.Encounter->Advance(.5f);
  TestEqual(TEXT("선형 이동 중간"), F.Door->GetActorLocation().Z, 525.0);
  F.Encounter->Advance(.5f);
  TestEqual(TEXT("문 닫힘"), F.Door->GetActorLocation().Z, 250.0);
  TestEqual(TEXT("전이 델리게이트 1회"), Changes, 1);
  TestTrue(TEXT("보스 전투 허용"), F.Boss->GetBrain()->IsEngageAllowed());
  return true;
}

DR_ENCOUNTER_TEST(FDrRespawnTest, "DrSample.Encounter.Retry.PlayerRespawnsAfterDelay")
bool FDrRespawnTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Activate();
  F.Enter();
  F.ASC->SpendStamina(90);
  F.KillPlayer();
  TestTrue(TEXT("사망 상태"), F.Encounter->GetState() == EDsEncounterState::PlayerDead);
  F.Encounter->Advance(F.Encounter->RespawnDelay - .1f);
  TestEqual(TEXT("지연 전 사망 유지"), F.Player->GetAttributeSet()->GetHealth(), 0.f);
  F.Encounter->Advance(.11f);
  TestTrue(TEXT("체크포인트에서 부활"), F.Player->GetActorLocation().Equals(F.Checkpoint->GetActorLocation()));
  TestEqual(TEXT("체력 최대"), F.Player->GetAttributeSet()->GetHealth(), F.Player->GetAttributeSet()->GetMaxHealth());
  TestEqual(TEXT("기력 최대"), F.ASC->GetStamina(), F.Player->GetAttributeSet()->GetMaxStamina());
  TestEqual(TEXT("강인도 최대"), F.Player->GetAttributeSet()->GetPoise(), F.Player->GetAttributeSet()->GetMaxPoise());
  TestFalse(TEXT("사망 태그 제거"), F.ASC->HasMatchingGameplayTag(DsTags::State_Dead));
  TestTrue(TEXT("이동 복구"), F.Player->GetCharacterMovement()->MovementMode == MOVE_Walking);
  TestTrue(TEXT("30초 이내 조작 가능"), F.Encounter->RespawnDelay + .01f <= 30.f && F.Player->InputAttackLight());
  TestTrue(TEXT("체크포인트 유지"), F.Encounter->IsCheckpointActive());
  return true;
}

DR_ENCOUNTER_TEST(FDrRetryBossTest, "DrSample.Encounter.Retry.ResetsBoss")
bool FDrRetryBossTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Activate();
  F.Enter();
  auto *Brain = F.Boss->GetBrain();
  F.ASC->ApplyDamage(F.Boss->GetDrASC(), 1105, 0, F.Player);
  Brain->TickComponent(.01f, LEVELTICK_All, nullptr);
  Brain->TickComponent(2.1f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("재시도 전 2페이즈"), Brain->GetPhase(), 2);
  Brain->TickComponent(.01f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("재시도 전 쿨다운"), Brain->PickAttack(200), INDEX_NONE);
  F.Boss->SetActorLocation(FVector(1800, 200, 100));
  F.KillPlayer();
  F.Retry();
  TestEqual(TEXT("보스 체력 최대"), F.Boss->GetAttributes()->GetHealth(), F.Boss->GetAttributes()->GetMaxHealth());
  TestEqual(TEXT("페이즈 초기화"), Brain->GetPhase(), 1);
  TestTrue(TEXT("대기 초기화"), Brain->GetState() == EDsBossState::Idle);
  TestTrue(TEXT("스폰 위치 복구"), F.Boss->GetActorLocation().Equals(FVector(2000, 0, 100)));
  TestEqual(TEXT("쿨다운 제거"), Brain->PickAttack(200), 0);
  TestFalse(TEXT("다음 진입까지 대기"), Brain->IsEngageAllowed());
  return true;
}

DR_ENCOUNTER_TEST(FDrRetryDoorTest, "DrSample.Encounter.Retry.DoorReopens")
bool FDrRetryDoorTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Enter();
  F.KillPlayer();
  F.Retry();
  F.Encounter->Advance(1.f);
  TestEqual(TEXT("문 다시 열림"), F.Door->GetActorLocation().Z, 800.0);
  TestTrue(TEXT("미활성 시작점 부활"), F.Player->GetActorLocation().Equals(F.Start->GetActorLocation()));
  TestTrue(TEXT("재진입 가능 상태"), F.Encounter->GetState() == EDsEncounterState::Idle);
  return true;
}

DR_ENCOUNTER_TEST(FDrRetryAttackTest, "DrSample.Encounter.Retry.RevivedPlayerCanAttack")
bool FDrRetryAttackTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  TestTrue(TEXT("사망 전 공격"), F.Player->InputAttackLight());
  F.KillPlayer();
  F.Retry();
  TestTrue(TEXT("부활 뒤 같은 어빌리티 발동"), F.Player->InputAttackLight());
  return true;
}

DR_ENCOUNTER_TEST(FDrVictoryTest, "DrSample.Encounter.Victory.OpensDoorAndStays")
bool FDrVictoryTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Enter();
  F.KillBoss();
  F.Encounter->Advance(1.f);
  TestTrue(TEXT("승리 상태"), F.Encounter->GetState() == EDsEncounterState::Victory);
  TestEqual(TEXT("승리 문 열림"), F.Door->GetActorLocation().Z, 800.0);
  F.KillPlayer();
  TestTrue(TEXT("승리 후 사망에도 승리 유지"), F.Encounter->GetState() == EDsEncounterState::Victory);
  F.Retry();
  TestTrue(TEXT("플레이어 부활"), F.Player->GetAttributeSet()->GetHealth() > 0.f);
  TestEqual(TEXT("보스 사망 유지"), F.Boss->GetAttributes()->GetHealth(), 0.f);
  TestTrue(TEXT("승리 후 부활에도 승리 유지"), F.Encounter->GetState() == EDsEncounterState::Victory);
  return true;
}

DR_ENCOUNTER_TEST(FDrFallTest, "DrSample.Encounter.Retry.FallOutOfWorldRespawns")
bool FDrFallTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Activate();
  F.ASC->SetLooseState(DsTags::State_Invulnerable, true);
  F.Player->FellOutOfWorld(*GetDefault<UDamageType>());
  TestFalse(TEXT("낙사 후 액터 유지"), F.Player->IsActorBeingDestroyed());
  TestEqual(TEXT("무적이어도 낙사"), F.Player->GetAttributeSet()->GetHealth(), 0.f);
  F.Retry();
  TestTrue(TEXT("낙사 후 부활"), F.Player->GetAttributeSet()->GetHealth() > 0.f);
  TestTrue(TEXT("체크포인트 복구"), F.Player->GetActorLocation().Equals(F.Checkpoint->GetActorLocation()));
  return true;
}

DR_ENCOUNTER_TEST(FDrInactiveEncounterTest, "DrSample.Encounter.Setup.InactiveWithoutMarkers")
bool FDrInactiveEncounterTest::RunTest(const FString &) {
  DsEncounterTests::FFixture F;
  F.Encounter->BindActors(F.Player, nullptr, nullptr, F.Door, F.Start);
  F.KillPlayer();
  F.Encounter->Advance(30.f);
  TestTrue(TEXT("표식 없으면 상태 전이 없음"), F.Encounter->GetState() == EDsEncounterState::Idle);
  TestFalse(TEXT("표식 없으면 체크포인트 없음"), F.Encounter->IsCheckpointActive());
  TestEqual(TEXT("임의 부활 없음"), F.Player->GetAttributeSet()->GetHealth(), 0.f);
  TestFalse(TEXT("낙사 경로 비활성"), F.Encounter->IsBoundPlayer(F.Player));
  return true;
}

#undef DR_ENCOUNTER_TEST
#endif
