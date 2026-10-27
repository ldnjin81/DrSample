#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Boss/DsBossCharacter.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DsSampleContent.generated.h"

/** 샘플 이동 블렌드의 입력이다. 게임 전투 규칙은 프레임워크가 처리한다. */
UCLASS(Transient, Blueprintable)
class DRSAMPLE_API UDsSampleAnimInstance : public UAnimInstance {
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadOnly, Category="샘플") float Speed = 0.f;
 virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};

/** 기존 보스 판단을 유지하면서 샘플의 일반 피격 연출만 연결한다. */
UCLASS()
class DRSAMPLE_API ADsSampleEnemy : public ADsBossCharacter {
 GENERATED_BODY()
public:
 ADsSampleEnemy();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(EditDefaultsOnly, Category="샘플") TObjectPtr<UAnimMontage> HitMontage;
 UPROPERTY(EditDefaultsOnly, Category="샘플") TObjectPtr<UAnimMontage> HitBackMontage;
 UPROPERTY(EditDefaultsOnly, Category="샘플") TObjectPtr<UAnimMontage> HitLeftMontage;
 UPROPERTY(EditDefaultsOnly, Category="샘플") TObjectPtr<UAnimMontage> HitRightMontage;
private:
 void OnSampleDamaged(float Damage, AActor* InstigatorActor, AActor* Causer);
 float ReactionRemaining = 0.f;
};

/** 에셋 생성 스크립트가 사용하는 샘플 전용 에디터 함수이다. */
UCLASS()
class DRSAMPLE_API UDsSampleContent : public UBlueprintFunctionLibrary {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="샘플")
 static bool BuildAnimationBlueprint(class UAnimBlueprint* Blueprint, class UBlendSpace* Movement);
 UFUNCTION(BlueprintCallable, Category="샘플")
 static void CleanPhysics(class UPhysicsAsset* Physics);
 UFUNCTION(BlueprintCallable, Category="샘플")
 static int32 CleanAnimation(class UAnimSequence* Sequence);
};
