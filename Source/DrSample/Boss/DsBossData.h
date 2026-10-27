#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Abilities/DsAttributeSet.h"
#include "DsBossData.generated.h"

/** 보스 한 종의 기본 수치·애니 이름·스탯. 공격 패턴은 BossPhase/BossAttack 테이블에 있다. 코드의 값은 에셋이 없을 때의 기본값이다. */
UCLASS(BlueprintType)
class DRSAMPLE_API UDsBossData : public UDataAsset {
  GENERATED_BODY()
public:
  UDsBossData();
  /** BossPhase.Boss와 맞춰 패턴을 찾는 이름. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") FName BossName = TEXT("Boss");
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") FText DisplayName;
  /** 기본 스탯. 보스 포이즈는 그로기가 끝날 때 한 번에 채우므로 자동 재생은 0이다. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Stats") FDsStatBlock Stats;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float BaseMoveSpeed = 350.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float AggroRadius = 2500.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float GroggyDuration = 2.67f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float PhaseTransitionDuration = 2.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float RecoverDuration = .6f;
  /** 초당 회전 각도와, 정면으로 보는 반각(도). */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float TurnRate = 180.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Boss") float AttackFacingAngle = 60.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Animation") FName IdleAnim = TEXT("Idle");
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Animation") FName MoveAnim = TEXT("Jog_Fwd");
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Animation") FName GroggyAnim = TEXT("Knock_back");
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Animation") FName DeathAnim = TEXT("Death_front");
};
