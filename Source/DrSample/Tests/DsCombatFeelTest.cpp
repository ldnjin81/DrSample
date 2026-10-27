#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/DsLockOnComponent.h"
#include "Combat/DsLockOnTargetComponent.h"
#include "Combat/DsCombatFeelData.h"
#include "Combat/DsHitFeedbackSubsystem.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Abilities/Combat/DsGameplayAbility_AttackHeavy.h"
#include "Abilities/Combat/DsGameplayAbility_AttackLight.h"
#include "Abilities/Combat/DsGameplayAbility_MeleeAttack.h"
#include "Boss/DsBossCharacter.h"
#include "Characters/DsCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/DsGameplayTags.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Tests/DsTestFixture.h"
#include "UI/DsCombatHUDViewModel.h"

namespace DrLockOnTests {
struct FFixture {
  DsTestFixture::FWorldFixture Base{true};
  ADsCharacter* Player;
  ADsBossCharacter* Boss;
  APlayerController* PC;
  FFixture() {
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Base.World);
    Base.World->InitializeActorsForPlay(FURL());
    Base.World->SetBegunPlay(true);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player = Base.World->SpawnActor<ADsCharacter>(ADsCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
    Boss = Base.World->SpawnActor<ADsBossCharacter>(ADsBossCharacter::StaticClass(), FVector(1000, 0, 0), FRotator::ZeroRotator, Params);
    PC = Base.World->SpawnActor<APlayerController>();
    PC->Possess(Player);
    PC->SetControlRotation(FRotator::ZeroRotator);
  }
  ~FFixture() {
    Base.World->EndPlay(EEndPlayReason::Quit);
    GEngine->DestroyWorldContext(Base.World);
  }
  void AdvanceLock(float Seconds) {
    for (float T = 0; T < Seconds; T += .01f) Player->GetLockOn()->TickComponent(.01f, LEVELTICK_All, nullptr);
  }
  UDsAbilitySystemComponent* ASC() const { return Cast<UDsAbilitySystemComponent>(Player->GetAbilitySystemComponent()); }
};
}

#define DR_FEEL_TEST(Name, Path) IMPLEMENT_SIMPLE_AUTOMATION_TEST(Name, Path, EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

DR_FEEL_TEST(FDrLockAcquireTest, "DrSample.Combat.LockOn.AcquireReleaseAndHUD")
bool FDrLockAcquireTest::RunTest(const FString&) {
  DrLockOnTests::FFixture F;
  auto* Lock = F.Player->GetLockOn();
  auto* Boom = F.Player->FindComponentByClass<USpringArmComponent>();
  const float Arm = Boom->TargetArmLength;
  const FVector Offset = Boom->SocketOffset;
  auto* VM = NewObject<UDsCombatHUDViewModel>();
  VM->BindPlayer(F.Player);
  TestTrue(TEXT("시야 내 보스 획득"), F.Player->InputLockOn());
  TestEqual(TEXT("HUD 대상 전달"), VM->LockOnTarget.Get(), static_cast<AActor*>(F.Boss));
  TestTrue(TEXT("스프링암 증가"), Boom->TargetArmLength > Arm);
  F.AdvanceLock(.01f);
  TestTrue(TEXT("측면 오프셋 보간 시작"), Boom->SocketOffset.Y > Offset.Y && Boom->SocketOffset.Y < Offset.Y + 70.f);
  F.AdvanceLock(1.5f);
  TestTrue(TEXT("측면 70cm 수렴"), FMath::IsNearlyEqual(Boom->SocketOffset.Y, Offset.Y + 70.f, .05f));
  TestFalse(TEXT("이동 방향 회전 해제"), F.Player->GetCharacterMovement()->bOrientRotationToMovement);
  TestFalse(TEXT("재입력 해제"), F.Player->InputLockOn());
  TestNull(TEXT("HUD 대상 해제"), VM->LockOnTarget.Get());
  TestEqual(TEXT("스프링암 원복"), Boom->TargetArmLength, Arm);
  F.AdvanceLock(1.5f);
  TestTrue(TEXT("측면 오프셋 복귀"), Boom->SocketOffset.Equals(Offset, .05f));
  TestTrue(TEXT("이동 회전 원복"), F.Player->GetCharacterMovement()->bOrientRotationToMovement);
  F.Boss->SetActorLocation(FVector(2600, 0, 0));
  TestFalse(TEXT("2500 밖 획득 불가"), F.Player->InputLockOn());
  F.Boss->SetActorLocation(FVector(-800, 0, 0));
  TestFalse(TEXT("뒤쪽 획득 불가"), F.Player->InputLockOn());
  F.Boss->SetActorLocation(FVector(1000, 0, 0));
  TestTrue(TEXT("재획득"), F.Player->InputLockOn());
  F.Boss->SetActorLocation(FVector(3100, 0, 0));
  F.AdvanceLock(.02f);
  TestNull(TEXT("3000 밖 해제"), Lock->GetTarget());
  F.Boss->SetActorLocation(FVector(1000, 0, 0));
  F.Player->InputLockOn();
  F.ASC()->ApplyDamage(F.Boss->GetDrASC(), 100000.f, 0.f, F.Player);
  F.AdvanceLock(.02f);
  TestNull(TEXT("대상 사망 해제"), Lock->GetTarget());
  TestFalse(TEXT("사망 대상 재획득 불가"), F.Player->InputLockOn());
  F.Boss->ResetBoss(FTransform(FVector(1000, 0, 0)));
  F.Player->InputLockOn();
  F.Player->Revive(FTransform::Identity);
  TestNull(TEXT("재도전 해제"), Lock->GetTarget());
  F.Player->InputLockOn();
  F.ASC()->ApplyDamage(F.ASC(), 100000.f, 0.f, nullptr);
  TestNull(TEXT("플레이어 사망 즉시 해제"), Lock->GetTarget());
  VM->Shutdown();
  return true;
}

DR_FEEL_TEST(FDrLockCameraTest, "DrSample.Combat.LockOn.CameraSwitchAndOcclusion")
bool FDrLockCameraTest::RunTest(const FString&) {
  DrLockOnTests::FFixture F;
  auto* Lock = F.Player->GetLockOn();
  TestTrue(TEXT("카메라 시험 획득"), F.Player->InputLockOn());
  F.Boss->SetActorLocation(FVector(1000, 400, 0));
  const float DesiredYaw = (Lock->GetTargetLocation() - F.Player->GetPawnViewLocation()).Rotation().Yaw;
  const float Before = FMath::Abs(FMath::FindDeltaAngleDegrees(F.PC->GetControlRotation().Yaw, DesiredYaw));
  F.AdvanceLock(.8f);
  const float After = FMath::Abs(FMath::FindDeltaAngleDegrees(F.PC->GetControlRotation().Yaw, DesiredYaw));
  TestTrue(TEXT("컨트롤 회전 수렴"), After < Before * .1f);
  auto* Boom = F.Player->FindComponentByClass<USpringArmComponent>();
  const float FarArm = Boom->TargetArmLength;
  const float FarSide = Boom->SocketOffset.Y;
  F.Boss->SetActorLocation(FVector(200, 0, 0));
  F.AdvanceLock(1.f);
  TestTrue(TEXT("근접 시 암 길이 증가"), Boom->TargetArmLength > FarArm);
  TestTrue(TEXT("근접 시 측면 오프셋 감소"), Boom->SocketOffset.Y < FarSide);
  TestEqual(TEXT("보스가 카메라 프로브 차단"), F.Boss->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Camera), ECR_Block);
  F.Boss->SetActorLocation(F.Player->GetActorLocation() + FVector(5, 0, 500));
  F.AdvanceLock(1.f);
  const float Pitch = FRotator::NormalizeAxis(F.PC->GetControlRotation().Pitch);
  TestTrue(TEXT("초근접 피치 제한"), Pitch >= -35.f && Pitch <= 10.1f);
  F.Boss->SetActorLocation(FVector(1000, 0, 0));
  F.PC->SetControlRotation(FRotator::ZeroRotator);
  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  auto* Other = F.Base.World->SpawnActor<ADsBossCharacter>(ADsBossCharacter::StaticClass(), FVector(1000, 600, 0), FRotator::ZeroRotator, Params);
  TestTrue(TEXT("오른쪽 대상 전환"), Lock->SwitchTarget(1.f));
  TestEqual(TEXT("다중 대상 선택"), Lock->GetTarget(), static_cast<AActor*>(Other));
  Lock->Clear();
  Other->Destroy();
  F.PC->SetControlRotation(FRotator::ZeroRotator);
  F.Player->InputLockOn();
  auto* Wall = F.Base.World->SpawnActor<AActor>();
  auto* Box = NewObject<UBoxComponent>(Wall);
  Wall->SetRootComponent(Box);
  Box->SetBoxExtent(FVector(40, 1000, 1000));
  Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  Box->SetCollisionResponseToAllChannels(ECR_Block);
  Box->RegisterComponent();
  Wall->SetActorLocation(FVector(500, 0, 0));
  F.AdvanceLock(1.8f);
  TestNotNull(TEXT("차단 유예 유지"), Lock->GetTarget());
  F.AdvanceLock(.3f);
  TestNull(TEXT("차단 2초 후 해제"), Lock->GetTarget());
  TestFalse(TEXT("벽 뒤 재획득 불가"), F.Player->InputLockOn());
  return true;
}

DR_FEEL_TEST(FDrHitFeedbackTest, "DrSample.Combat.Feel.HitStopOverlayAndRestore")
bool FDrHitFeedbackTest::RunTest(const FString&) {
  DrLockOnTests::FFixture F;
  auto* Feedback = F.Base.World->GetSubsystem<UDsHitFeedbackSubsystem>();
  TestNotNull(TEXT("월드 연출 시스템"), Feedback);
  auto* Data = NewObject<UDsCombatFeelData>();
  Data->HitOverlay = NewObject<UMaterial>();
  Feedback->Settings = Data;
  auto* Mesh = F.Boss->GetMesh();
  auto* Previous = NewObject<UMaterial>();
  Mesh->SetOverlayMaterial(Previous);
  F.Player->CustomTimeDilation = .75f;
  Feedback->PlayHit(F.Player, F.Boss, EDsHitWeight::Light);
  TestTrue(TEXT("공격자 개별 감속"), F.Player->CustomTimeDilation < .75f);
  TestTrue(TEXT("피격자 개별 감속"), F.Boss->CustomTimeDilation < 1.f);
  TestEqual(TEXT("피격 림 적용"), Mesh->GetOverlayMaterial(), Data->HitOverlay.Get());
  Feedback->Tick(.04f);
  Feedback->PlayHit(F.Player, F.Boss, EDsHitWeight::Heavy);
  Feedback->Tick(.07f);
  TestTrue(TEXT("강공격 재적중은 감속 연장"), F.Player->CustomTimeDilation < .75f);
  Feedback->Tick(.06f);
  TestEqual(TEXT("원래 감속 값 복구"), F.Player->CustomTimeDilation, .75f);
  TestEqual(TEXT("피격자 복구"), F.Boss->CustomTimeDilation, 1.f);
  TestEqual(TEXT("원래 오버레이 복구"), Mesh->GetOverlayMaterial(), static_cast<UMaterialInterface*>(Previous));
  Feedback->PlayHit(F.Boss, F.Player, EDsHitWeight::Boss);
  F.Player->Revive(FTransform::Identity);
  TestEqual(TEXT("재도전 감속 정리"), F.Player->CustomTimeDilation, .75f);
  Feedback->ResetActor(F.Boss);
  return true;
}

DR_FEEL_TEST(FDrHitIntegrationTest, "DrSample.Combat.Feel.ActualMeleeHitAndInvulnerability")
bool FDrHitIntegrationTest::RunTest(const FString&) {
  DrLockOnTests::FFixture F;
  // 경량 월드의 PC는 로컬 플레이어가 없다. 기존 전투 픽스처와 같은 무소유 권한 경로로 검사한다.
  F.PC->UnPossess();
  F.ASC()->InitAbilityActorInfo(F.Player, F.Player);
  F.Boss->SetActorLocation(FVector(150, 0, 0));
  F.ASC()->GiveAbility(FGameplayAbilitySpec(UDsGameplayAbility_AttackLight::StaticClass(), 1));
  TestTrue(TEXT("실제 약공격 발동"), F.Player->InputAttackLight());
  F.ASC()->TickComponent(.55f, LEVELTICK_All, nullptr);
  TestTrue(TEXT("실제 적중이 공격자 감속"), F.Player->CustomTimeDilation < 1.f);
  TestTrue(TEXT("실제 적중이 보스 감속"), F.Boss->CustomTimeDilation < 1.f);
  auto* Feedback = F.Base.World->GetSubsystem<UDsHitFeedbackSubsystem>();
  Feedback->Tick(.07f);
  TestEqual(TEXT("월드 틱으로 복구"), F.Player->CustomTimeDilation, 1.f);
  Feedback->Tick(.05f);
  F.ASC()->CancelAllAbilities();
  F.Boss->GetDrASC()->SetLooseState(DsTags::State_Invulnerable, true);
  F.Player->InputAttackLight();
  F.ASC()->TickComponent(.55f, LEVELTICK_All, nullptr);
  TestEqual(TEXT("무적 대상은 히트스톱 없음"), F.Player->CustomTimeDilation, 1.f);
  return true;
}
#endif
