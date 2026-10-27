// 실제 샘플 맵과 입력·GAS·보스 판단을 함께 검증한다.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/PackageName.h"
#include "DsSampleContent.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Boss/DsBossBrainComponent.h"
#include "Characters/DsCharacter.h"
#include "Core/DsGameplayTags.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "InputAction.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UnrealClient.h"

namespace DrSampleTests {
class FBasicCombat : public IAutomationLatentCommand {
public:
 explicit FBasicCombat(FAutomationTestBase* InTest) : Test(InTest) {}
 bool Update() override;
private:
 bool Finish(const FString& Error = FString()) {
  if (!Error.IsEmpty()) Test->AddError(Error);
  FApp::SetUseFixedTimeStep(bPreviousFixed);
  FApp::SetFixedDeltaTime(PreviousDelta);
  return true;
 }
 void Next(const TCHAR* Name) {
  Test->AddInfo(FString::Printf(TEXT("샘플 통과: %s"), Name));
  ++Stage; Frames = 0;
 }
 bool Shot(const TCHAR* Name) {
  if (!FApp::CanEverRender()) return true;
  const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/DrSample") / Name);
  IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
  FScreenshotRequest::RequestScreenshot(Path, false, false);
  return true;
 }
 void FrameActors(UWorld* World, APlayerController* PC, ADsCharacter* Player, ADsSampleEnemy* Enemy) {
  if (!Camera.IsValid()) Camera = World->SpawnActor<ACameraActor>();
  const FVector Center = (Player->GetActorLocation() + Enemy->GetActorLocation()) * .5f;
  const float Distance = FMath::Max(650.f, FVector::Dist2D(Player->GetActorLocation(), Enemy->GetActorLocation()) * 1.6f);
  const FVector Location = Center + FVector(-.35f, -1.f, .65f).GetSafeNormal() * Distance;
  Camera->SetActorLocationAndRotation(Location, (Center - Location).Rotation());
  Camera->GetCameraComponent()->FieldOfView = 55.f;
  Camera->GetCameraComponent()->bConstrainAspectRatio = false;
  Camera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount = true;
  Camera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount = 0.f;
  PC->SetViewTarget(Camera.Get());
 }
 void CheckDirections(ADsCharacter* Player, ADsSampleEnemy* Enemy, USkeletalMeshComponent* Visible) {
  const FTransform PlayerStart = Player->GetActorTransform(), EnemyStart = Enemy->GetActorTransform();
  const TCHAR* Names[] = {TEXT("Front"), TEXT("Back"), TEXT("Left"), TEXT("Right")};
  const FVector Directions[] = {FVector(1,0,0), FVector(-1,0,0), FVector(0,-1,0), FVector(0,1,0)};
  for (int32 Index = 0; Index < 4; ++Index) {
   auto* Montage = LoadObject<UAnimMontage>(nullptr, *FString::Printf(TEXT("/Game/DrSample/Animations/AM_DrSample_Hit%s"), Names[Index]));
   Player->SetActorRotation(FRotator::ZeroRotator);
   Enemy->SetActorLocation(Player->GetActorLocation() + Directions[Index] * 250.f);
   Player->GetAttributeSet()->OnDamaged.Broadcast(1.f, Enemy, Enemy);
   Test->TestTrue(FString::Printf(TEXT("플레이어 %s 몽타주 재생"), Names[Index]), Montage && Visible->GetAnimInstance()->Montage_IsPlaying(Montage));
   Player->GetAttributeSet()->OnPoiseBroken.Broadcast(Enemy, Enemy);
   Test->TestTrue(FString::Printf(TEXT("플레이어 포이즈 붕괴 %s 몽타주 재생"), Names[Index]), Montage && Visible->GetAnimInstance()->Montage_IsPlaying(Montage));
   CastChecked<UDsAbilitySystemComponent>(Player->GetAbilitySystemComponent())->SetLooseState(DsTags::State_Staggered, false);
   Enemy->SetActorRotation(FRotator::ZeroRotator);
   Player->SetActorLocation(Enemy->GetActorLocation() + Directions[Index] * 250.f);
   Enemy->GetAttributes()->OnDamaged.Broadcast(1.f, Player, Player);
   Test->TestTrue(FString::Printf(TEXT("적 %s 몽타주 재생"), Names[Index]), Montage && Enemy->GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage));
   Test->AddInfo(FString::Printf(TEXT("4방향 재생 검증: 플레이어·적 %s → %s"), Names[Index], *GetNameSafe(Montage)));
  }
  Visible->GetAnimInstance()->Montage_Stop(0.f);
  Enemy->GetMesh()->GetAnimInstance()->Montage_Stop(0.f);
  Player->SetActorTransform(PlayerStart); Enemy->SetActorTransform(EnemyStart);
 }
 FAutomationTestBase* Test;
 int32 Stage = 0, Frames = 0, Total = 0;
 FVector Start;
 float EnemyHealth = 0.f, PlayerHealth = 0.f;
 bool bPreviousFixed = false;
 double PreviousDelta = 0;
 bool bStarted = false;
 bool bEnemyReaction = false, bPlayerReaction = false;
 TWeakObjectPtr<ACameraActor> Camera;
};
bool FBasicCombat::Update() {
 if (!bStarted) {
  bStarted = true;
  bPreviousFixed = FApp::UseFixedTimeStep(); PreviousDelta = FApp::GetFixedDeltaTime();
  FApp::SetFixedDeltaTime(1.0 / 64.0); FApp::SetUseFixedTimeStep(true);
 }
 if (++Total > 6400) return Finish(TEXT("샘플 PIE 준비/전투 시간 초과"));
 UWorld* World = GEditor ? GEditor->PlayWorld.Get() : nullptr;
 APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
 ADsCharacter* Player = PC ? Cast<ADsCharacter>(PC->GetPawn()) : nullptr;
 ADsSampleEnemy* Enemy = nullptr;
 if (World) for (TActorIterator<ADsSampleEnemy> It(World); It; ++It) { Enemy = *It; break; }
 if (!Player || !Enemy) return false;
 ++Frames;
 auto* Brain = Enemy->GetBrain();
 auto* Input = PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
 auto* Move = LoadObject<UInputAction>(nullptr, TEXT("/Game/DrSample/Input/IA_DrSample_Move.IA_DrSample_Move"));
 USkeletalMeshComponent* Visible = nullptr;
 for (USkeletalMeshComponent* Mesh : TInlineComponentArray<USkeletalMeshComponent*>(Player))
  if (Mesh->GetFName() == TEXT("MannyMesh")) Visible = Mesh;
 if (!Visible || !Visible->GetAnimInstance() || !Input || !Move)
  return Finish(TEXT("샘플 메시·애님 BP·입력 연결 누락"));
 FrameActors(World, PC, Player, Enemy);
 switch (Stage) {
 case 0:
  Brain->SetEngageAllowed(false); Brain->SetRandomSeed(64);
  if (Frames == 1) CheckDirections(Player, Enemy, Visible);
  if (Frames < 128) return false;
  Start = Player->GetActorLocation();
  Shot(TEXT("01_idle.png")); Next(TEXT("대기와 애님 BP")); break;
 case 1:
  Input->InjectInputForAction(Move, FInputActionValue(FVector2D(0,1)), {}, {});
  if (Frames < 32) return false;
  Test->TestTrue(TEXT("a: 이동 입력으로 위치 변화"), FVector::Dist2D(Start, Player->GetActorLocation()) > 20.f);
  EnemyHealth = Enemy->GetAttributes()->GetHealth();
  Player->SetActorLocation(Enemy->GetActorLocation() + FVector(-180,0,0));
  Player->SetActorRotation(FRotator::ZeroRotator);
  PC->SetControlRotation(FRotator::ZeroRotator);
  Test->TestTrue(TEXT("기본 공격 입력 수락"), Player->InputAttackLight());
  Next(TEXT("a 이동")); break;
 case 2:
  if (Enemy->GetMesh()->GetAnimInstance() && Enemy->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying()) bEnemyReaction = true;
  if (Enemy->GetAttributes()->GetHealth() < EnemyHealth) {
   Test->TestTrue(TEXT("b: 적 체력 감소"), true);
   Test->TestTrue(TEXT("b: 적 피격 몽타주"), bEnemyReaction);
   Shot(TEXT("02_attack_hit.png")); Next(TEXT("b 공격 명중·피격"));
  } else if (Frames > 192) return Finish(TEXT("b: 기본 공격이 적에게 명중하지 않음"));
  break;
 case 3:
  if (Frames < 128) return false;
  PlayerHealth = Player->GetAttributeSet()->GetHealth();
  Enemy->SetActorLocation(Player->GetActorLocation() + FVector(650,0,0));
  Enemy->SetActorRotation(FRotator(0,180,0));
  Start = Enemy->GetActorLocation();
  Brain->SetTarget(Player); Brain->SetEngageAllowed(true);
  Next(TEXT("적 추적 시작")); break;
 case 4:
  if (Visible->GetAnimInstance()->IsAnyMontagePlaying()) bPlayerReaction = true;
  if (Player->GetAttributeSet()->GetHealth() < PlayerHealth) {
   Test->TestTrue(TEXT("c: 적 추적 위치 변화"), FVector::Dist2D(Start, Enemy->GetActorLocation()) > 30.f);
   Test->TestTrue(TEXT("c: 플레이어 피격 몽타주"), bPlayerReaction);
   Shot(TEXT("03_player_hit.png")); Brain->SetEngageAllowed(false);
   Next(TEXT("c 적 추적·공격·플레이어 피격"));
  } else if (Frames > 1024) return Finish(TEXT("c: 적 추적/공격으로 플레이어 체력이 감소하지 않음"));
  break;
 case 5:
  if (Frames < 128) return false;
  Enemy->GetDrASC()->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), 1.f);
  Player->SetActorLocation(Enemy->GetActorLocation() + FVector(-180,0,0));
  Player->SetActorRotation(FRotator::ZeroRotator);
  Test->TestTrue(TEXT("마지막 기본 공격 입력"), Player->InputAttackLight());
  Next(TEXT("사망 검증 공격")); break;
 case 6:
  if (Enemy->GetAttributes()->GetHealth() <= 0.f) {
   Test->TestTrue(TEXT("d: 적 사망 상태"), Brain->GetState() == EDsBossState::Dead);
   auto* Single = Enemy->GetMesh()->GetSingleNodeInstance();
   Test->TestTrue(TEXT("d: 적 사망 애니메이션"), Single && Single->GetCurrentAsset() && Single->GetCurrentAsset()->GetName().Contains(TEXT("Death")));
   Next(TEXT("d 사망"));
  } else if (Frames > 192) return Finish(TEXT("d: 적이 사망하지 않음"));
  break;
 case 7:
  if (Frames < 32) return false;
  Shot(TEXT("04_death.png")); Next(TEXT("사망 포즈 캡처")); break;
 case 8:
  if (Frames < 8) return false;
  return Finish();
 }
 return false;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrSampleBasicCombat, "DrSample.PIE.BasicCombat",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDrSampleBasicCombat::RunTest(const FString&) {
 if (!FPackageName::DoesPackageExist(TEXT("/Game/DrSample/Maps/L_DrSample"))) {
  // 샘플 맵은 콘텐츠 작업에서 만든다. 맵이 생기기 전에는 경고만 남기고 건너뛴다.
  AddWarning(TEXT("샘플 맵 /Game/DrSample/Maps/L_DrSample 없음: 콘텐츠 작업 전이라 건너뜀"));
  return true;
 }
 AutomationOpenMap(TEXT("/Game/DrSample/Maps/L_DrSample"));
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(DrSampleTests::FBasicCombat(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
#endif
