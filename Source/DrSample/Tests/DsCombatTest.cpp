// 플레이어 전투 어빌리티(공격·막기·회피)와 피격 반응을 검증하는 자동화 테스트이다.
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/Combat/DsGameplayAbility_Dodge.h"
#include "Abilities/Combat/DsGameplayAbility_Guard.h"
#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"
#include "Abilities/Combat/DsGameplayAbility_AttackLight.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Animation/AnimMontage.h"
#include "Characters/DsCharacter.h"
#include "Combat/DsPlayerCombatData.h"
#include "Core/DsGameplayTags.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTestFixture.h"

namespace DsCombatTests {
// 2의 거듭제곱 분수라 누적 오차 없이 구간 경계를 결정적으로 지난다.
constexpr float Step = 1.f / 64.f;

struct FWorldFixture {
  FWorldFixture() : Base(true) {
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
    Base.World->InitializeActorsForPlay(FURL());
    // 경량 테스트 월드에는 GameMode가 없으므로 시작 상태를 직접 설정한다.
    Base.World->SetBegunPlay(true);
  }
  ~FWorldFixture() {
    if (Base.World->HasBegunPlay())
      Base.World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(Base.World);
  }
  DsTestFixture::FWorldFixture Base;
  UWorld *GetWorld() const { return Base.World; }
};

/** 메시·애님 없는 네이티브 캐릭터에 전투 어빌리티를 부여해 로직만 검증한다. */
struct FFighter {
  FFighter(UWorld *World, const FVector &Location, float Yaw) {
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Character = World->SpawnActor<ADsCharacter>(
        ADsCharacter::StaticClass(), Location, FRotator(0.f, Yaw, 0.f), Params);
    if (!Character)
      return;
    ASC = Cast<UDsAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    Attributes = Character->GetAttributeSet();
    for (UClass *Class : {UDsGameplayAbility_AttackLight::StaticClass(),
                          UDsGameplayAbility_AttackHeavy::StaticClass(),
                          UDsGameplayAbility_Guard::StaticClass(),
                          UDsGameplayAbility_Dodge::StaticClass()})
      ASC->GiveAbility(FGameplayAbilitySpec(Class, 1));
  }
  bool IsValid() const { return Character && ASC && Attributes; }
  /** 월드 시간을 흘리지 않고 어빌리티와 경직 시간만 진행시킨다. */
  void Advance(float Seconds) {
    const int32 Steps = FMath::CeilToInt(Seconds / Step);
    for (int32 Index = 0; Index < Steps; ++Index) {
      ASC->TickComponent(Step, LEVELTICK_All, nullptr);
      Character->Tick(Step);
    }
  }
  bool Has(const FGameplayTag &Tag) const {
    return ASC->HasMatchingGameplayTag(Tag);
  }
  template <typename T> T *Ability() const {
    for (const FGameplayAbilitySpec &Spec : ASC->GetActivatableAbilities())
      if (T *Instance = Cast<T>(Spec.GetPrimaryInstance()))
        return Instance;
    return nullptr;
  }
  ADsCharacter *Character = nullptr;
  UDsAbilitySystemComponent *ASC = nullptr;
  UDsAttributeSet *Attributes = nullptr;
};
} // namespace DsCombatTests

#define DR_COMBAT_TEST(ClassName, Path)                                        \
  IMPLEMENT_SIMPLE_AUTOMATION_TEST(                                            \
      ClassName, Path,                                                         \
      EAutomationTestFlags_ApplicationContextMask |                            \
          EAutomationTestFlags::ProductFilter)

DR_COMBAT_TEST(FDrCombatAttackOnceTest,
               "DrSample.Combat.Attack.DamagesTargetOncePerSwing")
bool FDrCombatAttackOnceTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  DsCombatTests::FFighter Target(Fixture.GetWorld(), FVector(150.f, 0.f, 0.f), 0.f);
  DsCombatTests::FFighter Bystander(Fixture.GetWorld(), FVector(-300.f, 0.f, 0.f), 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Target.IsValid() &&
                                         Bystander.IsValid()))
    return false;
  TestTrue(TEXT("약공격 발동"), Attacker.Character->InputAttackLight());
  Attacker.Advance(.4f);
  TestEqual(TEXT("판정 구간 전에는 피해 없음"), Target.Attributes->GetHealth(), 100.f);
  // 판정 구간(0.50~0.84초)을 여러 틱에 걸쳐 지나도 한 번만 맞는다.
  Attacker.Advance(.6f);
  // 공격력 20 × 1.0 − 방어력 5 = 15
  TestEqual(TEXT("대상 피해는 1회"), Target.Attributes->GetHealth(), 85.f);
  TestEqual(TEXT("대상 포이즈 피해"), Target.Attributes->GetPoise(), 80.f);
  TestEqual(TEXT("뒤쪽 대상은 맞지 않음"), Bystander.Attributes->GetHealth(), 100.f);
  TestEqual(TEXT("공격자 자신은 맞지 않음"), Attacker.Attributes->GetHealth(), 100.f);
  return true;
}

DR_COMBAT_TEST(FDrCombatComboTest, "DrSample.Combat.Attack.LightComboAdvances")
bool FDrCombatComboTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Fighter(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Fighter.IsValid())) return false;
  // 소모만 검사한다. 재생 지연(1초)이 틱으로 흐르므로 재생을 끈다.
  Fighter.ASC->SetNumericAttributeBase(UDsAttributeSet::GetStaminaRegenRateAttribute(), 0.f);
  UDsGameplayAbility_AttackLight *Attack =
      Fighter.Ability<UDsGameplayAbility_AttackLight>();
  if (!TestNotNull(TEXT("약공격 인스턴스"), Attack)) return false;

  // 입력 없이 두면 1타만 하고 끝난다.
  Fighter.Character->InputAttackLight();
  Fighter.Advance(1.f);
  TestEqual(TEXT("입력 없으면 1타 유지"), Attack->GetStageIndex(), 0);
  TestTrue(TEXT("1타 진행 중"), Attack->IsActive());
  Fighter.Advance(.75f);
  TestFalse(TEXT("1타 길이 뒤 종료"), Attack->IsActive());
  TestFalse(TEXT("공격 상태 해제"), Fighter.Has(DsTags::State_Attacking));
  TestEqual(TEXT("1타 스태미나"), Fighter.ASC->GetStamina(), 88.f);

  // 진행 중 입력을 버퍼하면 판정이 끝난 뒤 다음 타로 넘어간다.
  Fighter.Character->InputAttackLight();
  Fighter.Advance(.3f);
  Fighter.Character->InputAttackLight();
  TestEqual(TEXT("판정이 끝나기 전에는 1타"), Attack->GetStageIndex(), 0);
  Fighter.Advance(.6f);
  TestEqual(TEXT("버퍼 입력으로 2타"), Attack->GetStageIndex(), 1);
  Fighter.Character->InputAttackLight();
  Fighter.Advance(.75f);
  TestEqual(TEXT("버퍼 입력으로 3타"), Attack->GetStageIndex(), 2);
  // 마지막 타에서는 입력을 더 받아도 넘어가지 않는다.
  Fighter.Character->InputAttackLight();
  Fighter.Advance(1.9f);
  TestFalse(TEXT("3타 뒤 종료"), Attack->IsActive());
  TestEqual(TEXT("3타 스태미나"), Fighter.ASC->GetStamina(), 52.f);
  return true;
}

DR_COMBAT_TEST(FDrCombatStaminaTest,
               "DrSample.Combat.Attack.SpendsStaminaAndBlocksWhenEmpty")
bool FDrCombatStaminaTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Fighter(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Fighter.IsValid())) return false;
  // 소모만 검사한다. 재생 지연(1초)이 틱으로 흐르므로 재생을 끈다.
  Fighter.ASC->SetNumericAttributeBase(UDsAttributeSet::GetStaminaRegenRateAttribute(), 0.f);
  TestTrue(TEXT("강공격 발동"), Fighter.Character->InputAttackHeavy());
  TestEqual(TEXT("강공격 스태미나"), Fighter.ASC->GetStamina(), 70.f);
  TestFalse(TEXT("공격 중 회피 불가"), Fighter.Character->InputDodge());
  TestFalse(TEXT("공격 중 막기 불가"), Fighter.Character->InputGuard());
  Fighter.Advance(3.1f);
  TestFalse(TEXT("강공격 종료"), Fighter.Has(DsTags::State_Attacking));

  Fighter.ASC->SetNumericAttributeBase(UDsAttributeSet::GetStaminaAttribute(), 5.f);
  TestFalse(TEXT("스태미나 부족 시 약공격 불가"), Fighter.Character->InputAttackLight());
  TestFalse(TEXT("스태미나 부족 시 강공격 불가"), Fighter.Character->InputAttackHeavy());
  TestFalse(TEXT("스태미나 부족 시 회피 불가"), Fighter.Character->InputDodge());
  TestFalse(TEXT("스태미나 부족 시 막기 불가"), Fighter.Character->InputGuard());
  TestEqual(TEXT("실패한 발동은 스태미나를 쓰지 않음"), Fighter.ASC->GetStamina(), 5.f);

  // 콤보 도중 스태미나가 모자라면 다음 타로 넘어가지 않는다.
  Fighter.ASC->SetNumericAttributeBase(UDsAttributeSet::GetStaminaAttribute(), 20.f);
  UDsGameplayAbility_AttackLight *Attack =
      Fighter.Ability<UDsGameplayAbility_AttackLight>();
  Fighter.Character->InputAttackLight();
  Fighter.Character->InputAttackLight();
  Fighter.Advance(1.f);
  TestEqual(TEXT("스태미나 부족 시 1타 유지"), Attack ? Attack->GetStageIndex() : -1, 0);
  TestEqual(TEXT("1타만 소모"), Fighter.ASC->GetStamina(), 8.f);
  return true;
}

DR_COMBAT_TEST(FDrCombatDodgeTest, "DrSample.Combat.Dodge.InvulnerableWindow")
bool FDrCombatDodgeTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector(200.f, 0.f, 0.f), 180.f);
  DsCombatTests::FFighter Dodger(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Dodger.IsValid()))
    return false;
  TestTrue(TEXT("회피 발동"), Dodger.Character->InputDodge());
  TestEqual(TEXT("회피 스태미나"), Dodger.ASC->GetStamina(), 80.f);
  TestFalse(TEXT("시작 직후에는 무적 아님"), Dodger.Has(DsTags::State_Invulnerable));
  Dodger.Advance(.1f);
  TestTrue(TEXT("구간 안 무적"), Dodger.Has(DsTags::State_Invulnerable));
  Attacker.ASC->ApplyDamage(Dodger.ASC, 40.f, 30.f, Attacker.Character);
  TestEqual(TEXT("무적 중 피해 없음"), Dodger.Attributes->GetHealth(), 100.f);
  TestEqual(TEXT("무적 중 포이즈 피해 없음"), Dodger.Attributes->GetPoise(), 100.f);
  Dodger.Advance(.2f);
  TestFalse(TEXT("구간 뒤 무적 해제"), Dodger.Has(DsTags::State_Invulnerable));
  TestTrue(TEXT("아직 회피 중"), Dodger.Has(DsTags::State_Dodging));
  Attacker.ASC->ApplyDamage(Dodger.ASC, 40.f, 30.f, Attacker.Character);
  TestEqual(TEXT("구간 밖 피해 적용"), Dodger.Attributes->GetHealth(), 65.f);
  Dodger.Advance(.1f);
  TestFalse(TEXT("회피 종료"), Dodger.Has(DsTags::State_Dodging));
  return true;
}

DR_COMBAT_TEST(FDrCombatDodgeRollTest, "DrSample.Combat.Dodge.RollDirections")
bool FDrCombatDodgeRollTest::RunTest(const FString &) {
  using UDodge = UDsGameplayAbility_Dodge;
  TestEqual(TEXT("앞"), UDodge::SelectRollIndex(FVector(1, 0, 0)), 0);
  TestEqual(TEXT("앞오른쪽"), UDodge::SelectRollIndex(FVector(1, 1, 0)), 1);
  TestEqual(TEXT("오른쪽"), UDodge::SelectRollIndex(FVector(0, 1, 0)), 2);
  TestEqual(TEXT("뒤"), UDodge::SelectRollIndex(FVector(-1, 0, 0)), 4);
  TestEqual(TEXT("뒤왼쪽"), UDodge::SelectRollIndex(FVector(-1, -1, 0)), 5);
  TestEqual(TEXT("왼쪽"), UDodge::SelectRollIndex(FVector(0, -1, 0)), 6);
  TestEqual(TEXT("앞왼쪽(경계 근처)"), UDodge::SelectRollIndex(FVector(1, -.9f, 0)), 7);
  // 방향 판정만 검증한다. 몽타주 실물·길이 검증은 소비 게임 테스트에 둔다.
  return true;
}

DR_COMBAT_TEST(FDrCombatParryTest,
               "DrSample.Combat.Guard.ParryNegatesAndReturnsPoise")
bool FDrCombatParryTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector(200.f, 0.f, 0.f), 180.f);
  DsCombatTests::FFighter Guard(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Guard.IsValid()))
    return false;
  int32 ParryCount = 0;
  Guard.Attributes->OnParried.AddLambda([&ParryCount](AActor *, AActor *) { ++ParryCount; });
  TestTrue(TEXT("막기 발동"), Guard.Character->InputGuard());
  TestEqual(TEXT("막기 스태미나"), Guard.ASC->GetStamina(), 92.f);
  Guard.Advance(.1f);
  Attacker.ASC->ApplyDamage(Guard.ASC, 40.f, 30.f, Attacker.Character);
  TestEqual(TEXT("쳐내기는 피해 없음"), Guard.Attributes->GetHealth(), 100.f);
  TestEqual(TEXT("쳐내기는 포이즈 피해 없음"), Guard.Attributes->GetPoise(), 100.f);
  TestEqual(TEXT("쳐내기는 추가 스태미나를 쓰지 않음"), Guard.ASC->GetStamina(), 92.f);
  TestEqual(TEXT("쳐내기 알림"), ParryCount, 1);
  TestEqual(TEXT("공격자 포이즈 반격"), Attacker.Attributes->GetPoise(), 60.f);
  TestEqual(TEXT("공격자 생명력은 그대로"), Attacker.Attributes->GetHealth(), 100.f);
  Guard.Attributes->OnParried.Clear();
  return true;
}

DR_COMBAT_TEST(FDrCombatBlockTest,
               "DrSample.Combat.Guard.BlockReducesAndSpendsStamina")
bool FDrCombatBlockTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector(200.f, 0.f, 0.f), 180.f);
  DsCombatTests::FFighter Guard(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Guard.IsValid()))
    return false;
  Guard.Character->InputGuard();
  Guard.Advance(.4f);
  TestTrue(TEXT("쳐내기 구간 뒤 막기"), Guard.Has(DsTags::State_Guarding));
  TestFalse(TEXT("쳐내기 해제"), Guard.Has(DsTags::State_Parrying));
  Attacker.ASC->ApplyDamage(Guard.ASC, 40.f, 30.f, Attacker.Character);
  // 40 × 0.3 = 12, 방어력 5 차감 → 7
  TestEqual(TEXT("막기 경감 피해"), Guard.Attributes->GetHealth(), 93.f);
  // 막기 8 + 원 피해 40의 절반 20
  TestEqual(TEXT("막기 스태미나 소모"), Guard.ASC->GetStamina(), 72.f);
  TestEqual(TEXT("막기 중 포이즈 유지"), Guard.Attributes->GetPoise(), 100.f);
  TestEqual(TEXT("공격자 포이즈는 그대로"), Attacker.Attributes->GetPoise(), 100.f);

  // 스태미나가 모자라면 막기가 깨져 전액 피해와 경직을 받는다.
  Guard.ASC->SetNumericAttributeBase(UDsAttributeSet::GetStaminaAttribute(), 5.f);
  Attacker.ASC->ApplyDamage(Guard.ASC, 40.f, 30.f, Attacker.Character);
  TestEqual(TEXT("막기 실패 시 전액 피해"), Guard.Attributes->GetHealth(), 58.f);
  TestTrue(TEXT("막기 실패 시 경직"), Guard.Has(DsTags::State_Staggered));
  TestFalse(TEXT("막기 해제"), Guard.Has(DsTags::State_Guarding));
  return true;
}

DR_COMBAT_TEST(FDrCombatBackAttackTest,
               "DrSample.Combat.Guard.BackAttackIgnoresGuard")
bool FDrCombatBackAttackTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  // 공격자가 막는 쪽의 등 뒤에 있다.
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector(-200.f, 0.f, 0.f), 0.f);
  DsCombatTests::FFighter Guard(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Guard.IsValid()))
    return false;
  Guard.Character->InputGuard();
  Guard.Advance(.1f);
  TestTrue(TEXT("쳐내기 구간"), Guard.Has(DsTags::State_Parrying));
  Attacker.ASC->ApplyDamage(Guard.ASC, 40.f, 30.f, Attacker.Character);
  TestEqual(TEXT("등 뒤 공격은 쳐내지 못함"), Guard.Attributes->GetHealth(), 65.f);
  TestEqual(TEXT("등 뒤 공격은 포이즈 피해"), Guard.Attributes->GetPoise(), 70.f);
  TestEqual(TEXT("반격 없음"), Attacker.Attributes->GetPoise(), 100.f);
  Guard.Advance(.3f);
  TestTrue(TEXT("막기 구간"), Guard.Has(DsTags::State_Guarding));
  Attacker.ASC->ApplyDamage(Guard.ASC, 40.f, 0.f, Attacker.Character);
  TestEqual(TEXT("등 뒤 공격은 막지 못함"), Guard.Attributes->GetHealth(), 30.f);
  TestEqual(TEXT("막기 스태미나만 소모"), Guard.ASC->GetStamina(), 92.f);
  return true;
}

DR_COMBAT_TEST(FDrCombatStaggerTest,
               "DrSample.Combat.Reaction.PoiseBreakStaggers")
bool FDrCombatStaggerTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector(200.f, 0.f, 0.f), 180.f);
  DsCombatTests::FFighter Victim(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Victim.IsValid()))
    return false;
  TestTrue(TEXT("공격 시작"), Victim.Character->InputAttackLight());
  Victim.Advance(.2f);
  Attacker.ASC->ApplyDamage(Victim.ASC, 10.f, 100.f, Attacker.Character);
  TestTrue(TEXT("포이즈 붕괴 시 경직"), Victim.Has(DsTags::State_Staggered));
  TestFalse(TEXT("진행 중 공격 취소"), Victim.Has(DsTags::State_Attacking));
  TestEqual(TEXT("포이즈 재충전"), Victim.Attributes->GetPoise(), 100.f);
  TestFalse(TEXT("경직 중 공격 불가"), Victim.Character->InputAttackLight());
  TestFalse(TEXT("경직 중 회피 불가"), Victim.Character->InputDodge());
  TestFalse(TEXT("경직 중 막기 불가"), Victim.Character->InputGuard());
  Victim.Advance(.5f);
  TestTrue(TEXT("0.5초 뒤에도 경직"), Victim.Has(DsTags::State_Staggered));
  Victim.Advance(.4f);
  TestFalse(TEXT("0.8초 뒤 경직 해제"), Victim.Has(DsTags::State_Staggered));
  TestTrue(TEXT("경직 뒤 공격 가능"), Victim.Character->InputAttackLight());
  return true;
}

DR_COMBAT_TEST(FDrCombatDeathTest,
               "DrSample.Combat.Reaction.DeathBlocksAbilities")
bool FDrCombatDeathTest::RunTest(const FString &) {
  DsCombatTests::FWorldFixture Fixture;
  DsCombatTests::FFighter Attacker(Fixture.GetWorld(), FVector(200.f, 0.f, 0.f), 180.f);
  DsCombatTests::FFighter Victim(Fixture.GetWorld(), FVector::ZeroVector, 0.f);
  if (!TestTrue(TEXT("캐릭터 스폰"), Attacker.IsValid() && Victim.IsValid()))
    return false;
  Victim.Character->InputGuard();
  Victim.Advance(.5f);
  // 등 뒤가 아니어도 막기를 뚫을 만큼 큰 피해로 사망시킨다.
  Attacker.ASC->ApplyDamage(Victim.ASC, 1000.f, 0.f, Attacker.Character);
  TestEqual(TEXT("생명력 0"), Victim.Attributes->GetHealth(), 0.f);
  TestTrue(TEXT("사망 태그"), Victim.Has(DsTags::State_Dead));
  TestFalse(TEXT("사망 시 막기 취소"), Victim.Has(DsTags::State_Guarding));
  TestFalse(TEXT("사망 시 경직 없음"), Victim.Has(DsTags::State_Staggered));
  TestTrue(TEXT("이동 차단"),
           Victim.Character->GetCharacterMovement()->MovementMode == MOVE_None);
  TestFalse(TEXT("사망 후 약공격 불가"), Victim.Character->InputAttackLight());
  TestFalse(TEXT("사망 후 강공격 불가"), Victim.Character->InputAttackHeavy());
  TestFalse(TEXT("사망 후 막기 불가"), Victim.Character->InputGuard());
  TestFalse(TEXT("사망 후 회피 불가"), Victim.Character->InputDodge());
  return true;
}

#undef DR_COMBAT_TEST
#endif
