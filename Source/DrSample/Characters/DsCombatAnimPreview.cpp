#include "Characters/DsCombatAnimPreview.h"

#include "Animation/AnimSequence.h"
#include "AnimSequencerInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

ADsCombatAnimPreview::ADsCombatAnimPreview()
{
	PrimaryActorTick.bCanEverTick = true;
	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
	SetRootComponent(PreviewMesh);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADsCombatAnimPreview::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PreviewMesh->SetSkeletalMeshAsset(MeshAsset);
}

void ADsCombatAnimPreview::BeginPlay()
{
	Super::BeginPlay();
	PreviewMesh->SetAnimInstanceClass(UAnimSequencerInstance::StaticClass());
	CurrentIndex = 0;
	PlayCurrentAnimation();
}

void ADsCombatAnimPreview::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Animations.IsEmpty())
	{
		return;
	}
	RemainingSeconds -= DeltaSeconds;
	AnimationSeconds += DeltaSeconds;
	if (RemainingSeconds <= 0.0f)
	{
		CurrentIndex = (CurrentIndex + 1) % Animations.Num();
		PlayCurrentAnimation();
	}
	else
	{
		UpdateAnimationPose();
	}
}

void ADsCombatAnimPreview::PlayCurrentAnimation()
{
	if (!Animations.IsValidIndex(CurrentIndex) || !Animations[CurrentIndex])
	{
		RemainingSeconds = FMath::Max(PauseSeconds, 0.1f);
		return;
	}
	UAnimSequence* Animation = Animations[CurrentIndex];
	AnimationSeconds = 0.0f;
	UpdateAnimationPose();
	RemainingSeconds = FMath::Max(Animation->GetPlayLength() + PauseSeconds, 0.1f);
	UE_LOG(LogTemp, Display, TEXT("전투 애니메이션 프리뷰: %s / %s"), *GetName(), *Animation->GetName());
}

void ADsCombatAnimPreview::UpdateAnimationPose()
{
	UAnimSequencerInstance* Instance = Cast<UAnimSequencerInstance>(PreviewMesh->GetAnimInstance());
	if (!Instance || !Animations.IsValidIndex(CurrentIndex) || !Animations[CurrentIndex])
	{
		return;
	}
	UAnimSequence* Animation = Animations[CurrentIndex];
	Instance->ResetNodes();
	Instance->UpdateAnimTrack(Animation, CurrentIndex, FMath::Min(AnimationSeconds, Animation->GetPlayLength()), 1.0f, false);
}
