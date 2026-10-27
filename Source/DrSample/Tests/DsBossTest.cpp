// 주입한 행과 구운 테이블로 보스의 전투·상태 전이를 검증한다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Boss/DsBossCharacter.h"
#include "Boss/DsBossBrainComponent.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"
#include "Abilities/Combat/DsGameplayAbility_AttackLight.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "Characters/DsCharacter.h"
#include "Core/DsGameplayTags.h"
#include "DrTableSubsystem.h"
#include "Tests/DsTableTestFixture.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTestFixture.h"

namespace DsBossTests {
FDsBossPhaseRow Phase(int32 Number, int32 Threshold=50) {
  FDsBossPhaseRow R; R.Id=Number; R.Boss=TEXT("Boss"); R.Phase=Number;
  R.EnterHpRate=Number==1?100:Threshold; R.MoveSpeedRate=Number==1?100:120;
  R.PatternSet=Number==1?TEXT("P1"):TEXT("P2"); return R;
}
FDsBossAttackRow Attack(int32 Id=1, FName Set=TEXT("P1")) {
  FDsBossAttackRow R; R.Id=Id; R.PatternSet=Set; R.Weight=1; R.Damage=20;
  R.Cooldown=FTimespan::FromSeconds(5); R.Duration=FTimespan::FromSeconds(1);
  R.TelegraphTime=FTimespan::FromSeconds(.25); R.ActiveStart=FTimespan::FromSeconds(.5);
  R.ActiveEnd=FTimespan::FromSeconds(.75); R.HitRadius=180; R.HitForwardOffset=120;
  R.MaxRange=350; R.StaminaDamage=10; return R;
}
struct FFixture {
  DsTestFixture::FWorldFixture Base{true};
  ADsBossCharacter* Boss;
  ADsCharacter* Player;
  UDsBossBrainComponent* Brain;
  UDsAbilitySystemComponent* PlayerASC;
  FFixture() {
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
    Base.World->InitializeActorsForPlay(FURL()); Base.World->SetBegunPlay(true);
    FActorSpawnParameters P; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Boss=Base.World->SpawnActor<ADsBossCharacter>(ADsBossCharacter::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,P);
    Player=Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(),FVector(200,0,0),FRotator(0,180,0),P);
    Brain=Boss->GetBrain(); PlayerASC=Cast<UDsAbilitySystemComponent>(Player->GetAbilitySystemComponent());
    Brain->SetTarget(Player); Brain->InjectTables({Phase(1),Phase(2)},{Attack(),Attack(2,TEXT("P2"))});
    Player->GetAttributeSet()->SetDefense(0);
  }
  ~FFixture() { Base.World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(Base.World); }
  void Tick(float Seconds) { Brain->TickComponent(Seconds,LEVELTICK_All,nullptr); }
  void Engage() { Tick(.01f); Tick(.01f); }
  void Hurt(float Damage) { PlayerASC->ApplyDamage(Boss->GetDrASC(),Damage,0,Player); }
  void Advance(float Seconds) { for (int32 I=0;I<FMath::CeilToInt(Seconds*64);++I) Tick(1.f/64); }
};
}
#define DR_BOSS_TEST(C,P) IMPLEMENT_SIMPLE_AUTOMATION_TEST(C,P,EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

DR_BOSS_TEST(FDrBossTablesTest,"DrSample.Boss.Tables.PhasesAndAttacksLoad")
bool FDrBossTablesTest::RunTest(const FString&) {
  DsTableTests::FFixture TableFixture;
  auto* Tables=UDrTableSubsystem::Get(); if (!TestNotNull(TEXT("테이블 서브시스템"),Tables)) return false;
  Tables->RegisterAutoTables(); Tables->LoadAllTables();
  int32 Count=0;
  for (const auto& R:FDsBossPhaseRow::GetAll()) if (R.Boss==TEXT("Boss")) {
    ++Count; TestTrue(TEXT("페이즈 공격 존재"),R.GetPatternSet().Num()>0);
  }
  TestEqual(TEXT("Boss 페이즈 2개"),Count,2);
  TestEqual(TEXT("공격 행 14개"),FDsBossAttackRow::GetAll().Num(),14);
  for (const auto& R:FDsBossAttackRow::GetAll()) {
    TestTrue(TEXT("판정 시간 순서"),R.TelegraphTime<=R.ActiveStart && R.ActiveStart<R.ActiveEnd && R.ActiveEnd<=R.Duration);
    TestTrue(TEXT("거리 순서"),R.MinRange<=R.MaxRange);
  }
  return true;
}
DR_BOSS_TEST(FDrBossAggroTest,"DrSample.Boss.Brain.AggroThenApproach")
bool FDrBossAggroTest::RunTest(const FString&) {
  DsBossTests::FFixture F; F.Player->SetActorLocation(FVector(3000,0,0)); F.Tick(.1f);
  TestTrue(TEXT("범위 밖 대기"),F.Brain->GetState()==EDsBossState::Idle);
  F.Player->SetActorLocation(FVector(1800,0,0)); F.Tick(.1f);
  TestTrue(TEXT("접근 상태"),F.Brain->GetState()==EDsBossState::Approach); F.Tick(.1f);
  TestTrue(TEXT("공격 거리 밖에서는 접근 유지"),F.Brain->GetState()==EDsBossState::Approach);
  TestFalse(TEXT("이동 입력"),F.Boss->GetPendingMovementInputVector().IsNearlyZero()); return true;
}
DR_BOSS_TEST(FDrBossPickTest,"DrSample.Boss.Brain.PicksOnlyInRangeAndOffCooldown")
bool FDrBossPickTest::RunTest(const FString&) {
  DsBossTests::FFixture F; auto R=DsBossTests::Attack(); R.MinRange=100; R.MaxRange=300;
  F.Brain->InjectTables({DsBossTests::Phase(1)},{R});
  TestEqual(TEXT("최소 밖"),F.Brain->PickAttack(99),INDEX_NONE); TestEqual(TEXT("최대 밖"),F.Brain->PickAttack(301),INDEX_NONE);
  TestEqual(TEXT("최소 포함"),F.Brain->PickAttack(100),0); TestEqual(TEXT("최대 포함"),F.Brain->PickAttack(300),0);
  F.Engage(); TestEqual(TEXT("쿨다운 중 제외"),F.Brain->PickAttack(200),INDEX_NONE);
  F.Tick(5.1f); TestEqual(TEXT("쿨다운 끝"),F.Brain->PickAttack(200),0); return true;
}
DR_BOSS_TEST(FDrBossWeightTest,"DrSample.Boss.Brain.WeightedPickFollowsData")
bool FDrBossWeightTest::RunTest(const FString&) {
  DsBossTests::FFixture F; auto A=DsBossTests::Attack(); auto B=DsBossTests::Attack(2);
  auto Count=[&](int32 WA,int32 WB) { A.Weight=WA; B.Weight=WB; F.Brain->InjectTables({DsBossTests::Phase(1)},{A,B});
    F.Brain->SetRandomSeed(38); int32 N=0; for(int32 I=0;I<1000;++I) N+=F.Brain->PickAttack(200)==0; return N; };
  TestTrue(TEXT("높은 가중치로 선택 증가"),Count(100,1)>950);
  TestTrue(TEXT("엑셀 행 가중치 교환으로 선택 감소"),Count(1,100)<50); return true;
}
DR_BOSS_TEST(FDrBossHitTest,"DrSample.Boss.Attack.DamagesOnceInActiveWindow")
bool FDrBossHitTest::RunTest(const FString&) {
  DsBossTests::FFixture F; F.Engage(); F.Tick(.4f); TestEqual(TEXT("판정 전 무피해"),F.Player->GetAttributeSet()->GetHealth(),100.f);
  F.Tick(.15f); TestEqual(TEXT("첫 피해"),F.Player->GetAttributeSet()->GetHealth(),80.f);
  F.Tick(.1f); TestEqual(TEXT("여러 틱에도 1회"),F.Player->GetAttributeSet()->GetHealth(),80.f);
  TestEqual(TEXT("기력 1회 차감"),F.PlayerASC->GetStamina(),90.f);
  F.Brain->InjectTables({DsBossTests::Phase(1)},{DsBossTests::Attack()}); F.Engage(); F.Tick(1.2f);
  TestEqual(TEXT("긴 틱으로 구간을 건너도 적중"),F.Player->GetAttributeSet()->GetHealth(),60.f); return true;
}
DR_BOSS_TEST(FDrBossMoveTest,"DrSample.Boss.Attack.MoveDistanceAdvances")
bool FDrBossMoveTest::RunTest(const FString&) {
  DsBossTests::FFixture F; auto R=DsBossTests::Attack(); R.MoveDistance=160; R.TelegraphTime=FTimespan::FromSeconds(.125);
  F.Brain->InjectTables({DsBossTests::Phase(1)},{R}); F.Player->SetActorLocation(FVector(340,0,0)); F.Engage();
  auto* Move=F.Boss->GetCharacterMovement(); Move->SetMovementMode(MOVE_Flying);
  for(int32 I=0;I<30;++I) { F.Tick(1.f/64); Move->TickComponent(1.f/64,LEVELTICK_All,nullptr); }
  TestTrue(TEXT("루트 모션으로 전진"),F.Boss->GetActorLocation().X>100);
  TestTrue(TEXT("순간이동 없이 거리 제한"),F.Boss->GetActorLocation().X<180); return true;
}
DR_BOSS_TEST(FDrBossSiphonTest,"DrSample.Boss.Attack.SiphonHealsOnHit")
bool FDrBossSiphonTest::RunTest(const FString&) {
  DsBossTests::FFixture F; auto R=DsBossTests::Attack(); R.Kind=EDsBossAttackKind::Siphon; R.Heal=100;
  F.Brain->InjectTables({DsBossTests::Phase(1)},{R}); F.Hurt(205); F.Engage(); const float H=F.Boss->GetAttributes()->GetHealth();
  F.Tick(.6f); TestEqual(TEXT("적중 회복"),F.Boss->GetAttributes()->GetHealth(),H+100);
  F.Tick(.1f); TestEqual(TEXT("중복 회복 없음"),F.Boss->GetAttributes()->GetHealth(),H+100); return true;
}
DR_BOSS_TEST(FDrBossThresholdTest,"DrSample.Boss.Phase.EntersAtDataThreshold")
bool FDrBossThresholdTest::RunTest(const FString&) {
  for(int32 Threshold:{50,70}) {
    DsBossTests::FFixture F; F.Brain->InjectTables({DsBossTests::Phase(1),DsBossTests::Phase(2,Threshold)},{DsBossTests::Attack()});
    F.Hurt(805); F.Tick(.01f);
    TestTrue(TEXT("행의 임계값에 따라 전환 시점 변경"),(F.Brain->GetState()==EDsBossState::PhaseTransition)==(Threshold==70));
  }
  DsBossTests::FFixture F; F.Engage(); F.Hurt(1105); F.Tick(.1f);
  TestTrue(TEXT("공격 중 전환 유예"),F.Brain->GetState()==EDsBossState::Attack);
  F.Tick(1.f); F.Tick(.01f); TestTrue(TEXT("공격 종료 뒤 전환"),F.Brain->GetState()==EDsBossState::PhaseTransition);
  F.Boss->GetDrASC()->ApplyHeal(500); F.Tick(2.1f);
  TestEqual(TEXT("회복해도 전환 되돌리지 않음"),F.Brain->GetPhase(),2); return true;
}
DR_BOSS_TEST(FDrBossInvulnerableTest,"DrSample.Boss.Phase.InvulnerableDuringTransition")
bool FDrBossInvulnerableTest::RunTest(const FString&) {
  DsBossTests::FFixture F; F.Hurt(1105); F.Tick(.01f); const float H=F.Boss->GetAttributes()->GetHealth();
  TestTrue(TEXT("전환 무적 태그"),F.Boss->GetDrASC()->HasMatchingGameplayTag(DsTags::State_Invulnerable));
  F.Hurt(500); TestEqual(TEXT("전환 중 피해 차단"),F.Boss->GetAttributes()->GetHealth(),H);
  F.Tick(2.1f); TestFalse(TEXT("전환 종료 무적 해제"),F.Boss->GetDrASC()->HasMatchingGameplayTag(DsTags::State_Invulnerable));
  F.Hurt(100); TestTrue(TEXT("전환 뒤 피해"),F.Boss->GetAttributes()->GetHealth()<H); return true;
}
DR_BOSS_TEST(FDrBossPatternTest,"DrSample.Boss.Phase.UsesPhaseTwoPatternSet")
bool FDrBossPatternTest::RunTest(const FString&) {
  DsBossTests::FFixture F; TestEqual(TEXT("1페이즈 공격"),F.Brain->PickAttack(200),0);
  F.Hurt(1105); F.Tick(.01f); F.Tick(2.1f);
  TestEqual(TEXT("2페이즈 적용"),F.Brain->GetPhase(),2); TestEqual(TEXT("2페이즈 묶음만 선택"),F.Brain->PickAttack(200),1);
  F.Boss->GetDrASC()->ApplyHeal(1000); F.Tick(.01f); TestEqual(TEXT("흡수 회복 후 페이즈 유지"),F.Brain->GetPhase(),2); return true;
}
DR_BOSS_TEST(FDrBossParryTest,"DrSample.Boss.Reaction.ParryBreaksPoiseIntoGroggy")
bool FDrBossParryTest::RunTest(const FString&) {
  DsBossTests::FFixture F; F.Boss->GetDrASC()->SetNumericAttributeBase(UDsAttributeSet::GetPoiseAttribute(),30);
  F.PlayerASC->SetLooseState(DsTags::State_Parrying,true); F.Engage(); F.Tick(.6f);
  TestTrue(TEXT("쳐내기 반환 피해로 그로기"),F.Brain->GetState()==EDsBossState::Groggy);
  TestFalse(TEXT("공격 취소"),F.Boss->GetDrASC()->HasMatchingGameplayTag(DsTags::State_Attacking));
  TestEqual(TEXT("쳐내기로 플레이어 피해 없음"),F.Player->GetAttributeSet()->GetHealth(),100.f);
  F.PlayerASC->SetLooseState(DsTags::State_Parrying,false); F.Advance(2.7f);
  TestEqual(TEXT("그로기 종료 강인도 복구"),F.Boss->GetAttributes()->GetPoise(),100.f);
  TestTrue(TEXT("그로기 종료 접근"),F.Brain->GetState()==EDsBossState::Approach); return true;
}
DR_BOSS_TEST(FDrBossDeathTest,"DrSample.Boss.Reaction.DeathStopsEverything")
bool FDrBossDeathTest::RunTest(const FString&) {
  DsBossTests::FFixture F; F.Engage(); F.Hurt(10000); F.Tick(10);
  TestTrue(TEXT("사망 상태"),F.Brain->GetState()==EDsBossState::Dead);
  TestTrue(TEXT("사망 태그"),F.Boss->GetDrASC()->HasMatchingGameplayTag(DsTags::State_Dead));
  TestEqual(TEXT("사망 뒤 공격 없음"),F.Player->GetAttributeSet()->GetHealth(),100.f);
  TestTrue(TEXT("이동 정지"),F.Boss->GetCharacterMovement()->MovementMode==MOVE_None);
  F.Brain->EnterGroggy(); TestTrue(TEXT("사망 상태 유지"),F.Brain->GetState()==EDsBossState::Dead); return true;
}
DR_BOSS_TEST(FDrPlayerBossTest,"DrSample.Boss.Reaction.PlayerDamagesBoss")
bool FDrPlayerBossTest::RunTest(const FString&) {
  DsBossTests::FFixture F; F.PlayerASC->GiveAbility(FGameplayAbilitySpec(UDsGameplayAbility_AttackLight::StaticClass(),1));
  TestTrue(TEXT("플레이어 약공격 입력"),F.Player->InputAttackLight());
  for(int32 I=0;I<64;++I) F.PlayerASC->TickComponent(1.f/64,LEVELTICK_All,nullptr);
  TestTrue(TEXT("기존 공격이 보스 ASC를 찾아 피해"),F.Boss->GetAttributes()->GetHealth()<2000); return true;
}
DR_BOSS_TEST(FDrBossFacingTest, "DrSample.Boss.Brain.WaitsUntilFacingTarget")
bool FDrBossFacingTest::RunTest(const FString &) {
  DsBossTests::FFixture F;
  F.Player->SetActorLocation(FVector(-200, 0, 0));
  F.Engage();
  TestTrue(TEXT("뒤쪽 대상이면 접근 상태 유지"), F.Brain->GetState() == EDsBossState::Approach);
  TestTrue(TEXT("정면 밖에서는 이동 없이 회전만"), F.Boss->GetPendingMovementInputVector().IsNearlyZero());
  F.Tick(.5f);
  TestTrue(TEXT("정면 밖 공격 보류"), F.Brain->GetState() == EDsBossState::Approach);
  F.Tick(.3f);
  TestTrue(TEXT("정면 원뿔 안에 들어오면 공격"), F.Brain->GetState() == EDsBossState::Attack);
  for (EDsBossAttackKind Kind : {EDsBossAttackKind::Area, EDsBossAttackKind::Siphon}) {
    auto Row = DsBossTests::Attack();
    Row.Kind = Kind;
    Row.HitForwardOffset = 0;
    F.Brain->InjectTables({DsBossTests::Phase(1)}, {Row});
    F.Boss->SetActorRotation(FRotator::ZeroRotator);
    F.Engage();
    TestTrue(TEXT("광역·흡수는 방향 무관"), F.Brain->GetState() == EDsBossState::Attack);
  }
  return true;
}

DR_BOSS_TEST(FDrBossResetTest, "DrSample.Boss.Brain.ResetEncounterRestoresInitialState")
bool FDrBossResetTest::RunTest(const FString &) {
  DsBossTests::FFixture F;
  F.Engage();
  F.Hurt(1105);
  F.Tick(1.1f);
  F.Tick(.01f);
  F.Tick(2.1f);
  TestEqual(TEXT("전환 완료"), F.Brain->GetPhase(), 2);
  F.Brain->ResetEncounter();
  TestEqual(TEXT("1페이즈 복구"), F.Brain->GetPhase(), 1);
  TestTrue(TEXT("대기 복구"), F.Brain->GetState() == EDsBossState::Idle);
  TestNull(TEXT("진행 중 공격 제거"), F.Brain->GetCurrentAttack());
  TestEqual(TEXT("쿨다운 제거"), F.Brain->PickAttack(200), 0);
  F.Engage();
  TestTrue(TEXT("대상 유지"), F.Brain->GetState() == EDsBossState::PhaseTransition);
  F.Boss->ResetBoss(F.Boss->GetHomeTransform());
  F.Engage();
  TestTrue(TEXT("재공격 시작"), F.Brain->GetState() == EDsBossState::Attack);
  F.Tick(.6f);
  TestTrue(TEXT("적중 목록 초기화 후 피해"), F.Player->GetAttributeSet()->GetHealth() < 100.f);
  F.Hurt(10000);
  F.Boss->ResetBoss(F.Boss->GetHomeTransform());
  TestFalse(TEXT("사망 태그 초기화"), F.Boss->GetDrASC()->HasMatchingGameplayTag(DsTags::State_Dead));
  TestTrue(TEXT("이동 복구"), F.Boss->GetCharacterMovement()->MovementMode == MOVE_Walking);
  return true;
}

DR_BOSS_TEST(FDrBossEngageAllowedTest, "DrSample.Boss.Brain.StaysIdleUntilEngageAllowed")
bool FDrBossEngageAllowedTest::RunTest(const FString &) {
  DsBossTests::FFixture F;
  TestTrue(TEXT("기본값 허용"), F.Brain->IsEngageAllowed());
  F.Brain->SetEngageAllowed(false);
  F.Tick(10.f);
  TestTrue(TEXT("허용 전 대기"), F.Brain->GetState() == EDsBossState::Idle);
  F.Brain->SetEngageAllowed(true);
  F.Engage();
  TestTrue(TEXT("허용 후 공격"), F.Brain->GetState() == EDsBossState::Attack);
  F.Brain->SetEngageAllowed(false);
  F.Tick(.1f);
  TestTrue(TEXT("진행 중 공격은 유지"), F.Brain->GetState() == EDsBossState::Attack);
  return true;
}

DR_BOSS_TEST(FDrBossEmptyTablesTest, "DrSample.Boss.Brain.WarnsOnceWhenTablesMissing")
bool FDrBossEmptyTablesTest::RunTest(const FString &) {
  AddExpectedError(TEXT("보스 페이즈 또는 공격 행이 비어 있습니다"), EAutomationExpectedErrorFlags::Contains, 2);
  for (bool bHavePhase : {false, true}) {
    DsBossTests::FFixture F;
    TArray<FDsBossPhaseRow> Phases;
    if (bHavePhase) Phases.Add(DsBossTests::Phase(1));
    F.Brain->InjectTables(Phases, {});
    for (int32 Index = 0; Index < 3; ++Index)
      TestEqual(TEXT("행이 없으면 공격 없음"), F.Brain->PickAttack(200), INDEX_NONE);
  }
  return true;
}

#undef DR_BOSS_TEST
#endif
