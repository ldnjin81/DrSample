#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DsCombatAnimPreview.generated.h"

class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;

/** 지정한 애니메이션을 순서대로 반복 재생하는 확인용 액터. */
UCLASS(Blueprintable)
class DRSAMPLE_API ADsCombatAnimPreview : public AActor
{
	GENERATED_BODY()

public:
	ADsCombatAnimPreview();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "전투 애니메이션")
	TObjectPtr<USkeletalMeshComponent> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "전투 애니메이션")
	TObjectPtr<USkeletalMesh> MeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "전투 애니메이션")
	TArray<TObjectPtr<UAnimSequence>> Animations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "전투 애니메이션", meta = (ClampMin = "0.0"))
	float PauseSeconds = 0.5f;

private:
	void PlayCurrentAnimation();
	int32 CurrentIndex = 0;
	float RemainingSeconds = 0.0f;
	float AnimationSeconds = 0.0f;
	void UpdateAnimationPose();
};
