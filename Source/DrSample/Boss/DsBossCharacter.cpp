#include "Boss/DsBossCharacter.h"
#include "TableData/DsGameAssets.h"
#include "Combat/DsCombatFXComponent.h"

#include "Boss/DsBossBrainComponent.h"
#include "Boss/DsBossData.h"
#include "Combat/DsLockOnTargetComponent.h"
#include "Combat/DsHitFeedbackSubsystem.h"
#include "Abilities/DsAbilitySystemComponent.h"
#include "Abilities/DsAttributeSet.h"
#include "Core/DsGameplayTags.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogDsBoss, Log, All);

ADsBossCharacter::ADsBossCharacter() {
  CombatFX = CreateDefaultSubobject<UDsCombatFXComponent>(TEXT("CombatFX"));
  LockOnTarget = CreateDefaultSubobject<UDsLockOnTargetComponent>(TEXT("LockOnTarget"));
  AbilitySystem = CreateDefaultSubobject<UDsAbilitySystemComponent>(TEXT("AbilitySystem"));
  Attributes = CreateDefaultSubobject<UDsAttributeSet>(TEXT("Attributes"));
  Brain = CreateDefaultSubobject<UDsBossBrainComponent>(TEXT("Brain"));
  AbilitySystem->SetIsReplicated(true);
  AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
  GetCapsuleComponent()->InitCapsuleSize(65.f, 100.f);
  GetCapsuleComponent()->SetCanEverAffectNavigation(false);
  // 근접 락온 중 스프링암이 보스 몸 안으로 들어가지 않게 한다.
  GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
  GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-100), FRotator(0,-90,0));
  GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  GetCharacterMovement()->bRunPhysicsWithNoController = true;
  GetCharacterMovement()->bOrientRotationToMovement = false;
  // 이동 속도 기본값. 생성 뒤 보스 데이터로 맞춘다.
  GetCharacterMovement()->MaxWalkSpeed = 350.f;
  // 상태 판단·루트 모션 등록이 이동 컴포넌트보다 먼저 진행된다.
  GetCharacterMovement()->AddTickPrerequisiteComponent(Brain);
  bUseControllerRotationYaw = false;
}
UAbilitySystemComponent* ADsBossCharacter::GetAbilitySystemComponent() const { return AbilitySystem; }
const UDsBossData* ADsBossCharacter::GetData() const {
  if (!LoadedData) {
    LoadedData = BossData.IsNull() ? DsGameAssets::Load<UDsBossData>(BossAssetKey) : BossData.LoadSynchronous();
    if (!LoadedData) {
      UE_LOG(LogDsBoss, Warning, TEXT("보스 데이터 에셋이 없어 기본값을 사용합니다."));
      LoadedData = GetDefault<UDsBossData>();
    }
  }
  return LoadedData;
}
void ADsBossCharacter::BeginPlay() {
  HomeTransform = GetActorTransform();
  Super::BeginPlay();
}

void ADsBossCharacter::ResetBoss(const FTransform &Home) {
  GetWorld()->GetSubsystem<UDsHitFeedbackSubsystem>()->ResetActor(this);
  AbilitySystem->CancelAllAbilities();
  AbilitySystem->SetNumericAttributeBase(UDsAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
  AbilitySystem->SetNumericAttributeBase(UDsAttributeSet::GetPoiseAttribute(), Attributes->GetMaxPoise());
  Brain->ResetEncounter();
  SetActorLocationAndRotation(Home.GetLocation(), Home.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
  GetCharacterMovement()->SetMovementMode(MOVE_Walking);
  GetCharacterMovement()->MaxWalkSpeed = GetData()->BaseMoveSpeed;
  GetCharacterMovement()->StopMovementImmediately();
  PlaySequence(GetData()->IdleAnim, true);
}
void ADsBossCharacter::PostInitializeComponents() {
  Super::PostInitializeComponents();
  if (!AbilitySystem->GetSet<UDsAttributeSet>()) AbilitySystem->AddAttributeSetSubobject(Attributes.Get());
  AbilitySystem->InitAbilityActorInfo(this, this);
  if (IsTemplate() || !GetWorld() || !GetWorld()->IsGameWorld()) return;
  // 기본 스탯·특성은 보스 데이터에 있다. 포이즈 자동 재생은 0(그로기 종료 시 한 번에 채운다).
  Attributes->InitFromStats(GetData()->Stats);
  GetCharacterMovement()->MaxWalkSpeed = GetData()->BaseMoveSpeed;
  if (!bEventsBound) {
    Attributes->OnPoiseBroken.AddUObject(this, &ADsBossCharacter::HandlePoiseBroken);
    Attributes->OnOutOfHealth.AddUObject(this, &ADsBossCharacter::HandleOutOfHealth);
    Attributes->OnDamaged.AddUObject(this, &ADsBossCharacter::HandleDamaged);
    bEventsBound = true;
  }
}
void ADsBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Attributes->OnPoiseBroken.RemoveAll(this);
  Attributes->OnOutOfHealth.RemoveAll(this);
  Attributes->OnDamaged.RemoveAll(this);
  bEventsBound = false;
  Super::EndPlay(EndPlayReason);
}
void ADsBossCharacter::HandlePoiseBroken(AActor*, AActor*) { Brain->EnterGroggy(); }
void ADsBossCharacter::HandleOutOfHealth(AActor*, AActor*) { Brain->EnterDead(); }
void ADsBossCharacter::HandleDamaged(float, AActor*, AActor*) { Brain->CheckPhaseThreshold(); }
void ADsBossCharacter::PlaySequence(FName InName, bool bLoop) {
  // 메시가 없는 경량 테스트에서는 연출을 건너뛰고 로직만 진행한다.
  if (InName.IsNone() || !GetMesh()->GetSkeletalMeshAsset()) return;
  UAnimSequence* Sequence = Animations.FindRef(InName);
  if (!Sequence && !MissingAnimations.Contains(InName)) {
    const FString Name = InName.ToString();
    const FString Path = DsGameAssets::Path(FName(TEXT("BossAnim_") + Name)).ToString();
    Sequence = LoadObject<UAnimSequence>(nullptr, *Path);
    if (Sequence) Animations.Add(InName, Sequence);
    else {
      MissingAnimations.Add(InName);
      UE_LOG(LogDsBoss, Warning, TEXT("보스 애니메이션이 없습니다: %s. 판정은 계속 진행합니다."), *Path);
    }
  }
  if (Sequence) GetMesh()->PlayAnimation(Sequence, bLoop);
}
