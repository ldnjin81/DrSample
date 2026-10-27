#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Abilities/DsAttributeSet.h"
#include "DsPlayerCombatData.generated.h"

class UAnimMontage;

/** 콤보 한 타의 길이·판정 구간·수치이다. 시간 단위는 초다. */
USTRUCT(BlueprintType)
struct FDsMeleeStage {
  GENERATED_BODY()

  /** 연출용 몽타주. 없어도 판정은 동작한다. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  TSoftObjectPtr<UAnimMontage> Montage;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float Duration = 1.f;
  /** 타격 판정이 켜지는 시각과 꺼지는 시각이다. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float ActiveStart = .3f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float ActiveEnd = .5f;
  /** 공격자 AttackPower에 곱하는 배수이다. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float DamageRate = 1.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float PoiseDamage = 20.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float StaminaCost = 12.f;
  /** 판정 시작 전까지 전방으로 미는 거리(cm)이다. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee")
  float LungeDistance = 80.f;

  /** 몽타주 경로와 수치로 한 타를 만든다. */
  static FDsMeleeStage Make(const TCHAR *MontagePath, float Duration,
                            float ActiveStart, float ActiveEnd, float DamageRate,
                            float PoiseDamage, float StaminaCost,
                            float LungeDistance);
};

/** 회피(구르기) 수치. 시간 단위는 초, 거리는 cm. */
USTRUCT(BlueprintType)
struct FDsDodgeTuning {
  GENERATED_BODY()
  /** 표시 메시가 없을 때의 대시 길이·거리. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") float Duration = .35f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") float Distance = 450.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") float InvulnerableStart = .05f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") float InvulnerableEnd = .3f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") float StaminaCost = 20.f;
  /** 캐릭터 기준 방향별 구르기 몽타주. 앞부터 시계 방향 45도씩. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") TArray<TSoftObjectPtr<UAnimMontage>> RollMontages;
  /** 구르기 모션 재생 속도. 원본(1.17초)을 회피 템포에 맞춰 줄인다. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") float RollPlayRate = 1.6f;
};

/** 막기·쳐내기 수치. */
USTRUCT(BlueprintType)
struct FDsGuardTuning {
  GENERATED_BODY()
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") TSoftObjectPtr<UAnimMontage> Montage;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") float Duration = 1.13f;
  /** 발동 직후 쳐내기 구간 길이. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") float ParryWindow = .3f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") float StaminaCost = 8.f;
  /** 쳐내기 성공 시 공격자에게 돌려주는 포이즈 피해. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") float ParryPoiseReturn = 40.f;
  /** 막기 중 통과하는 피해 비율과, 막은 원 피해 대비 기력 소모 비율. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") float GuardDamageRate = .3f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") float GuardStaminaRate = .5f;
};

/** 플레이어 전투 밸런스를 한 곳에 모은 데이터 에셋. 코드의 값은 에셋이 없을 때의 기본값이다. */
UCLASS(BlueprintType)
class DRSAMPLE_API UDsPlayerCombatData : public UDataAsset {
  GENERATED_BODY()
public:
  UDsPlayerCombatData();
  /** GameAssets.PlayerCombat. 없으면 클래스 기본값. */
  static const UDsPlayerCombatData* Get();

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Stats") FDsStatBlock Stats;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee") TArray<FDsMeleeStage> LightStages;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee") TArray<FDsMeleeStage> HeavyStages;
  /** 전방 판정 구의 반경과 캐릭터 중심에서의 거리(cm). */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee") float MeleeHitRadius = 90.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Melee") float MeleeHitForwardOffset = 130.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Dodge") FDsDodgeTuning Dodge;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Guard") FDsGuardTuning Guard;
  /** 포이즈가 무너졌을 때 행동이 막히는 시간. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Reaction") float StaggerDuration = .8f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Movement") float WalkSpeed = 200.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Movement") float RunSpeed = 500.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Movement") float SprintSpeed = 700.f;
  /** 기력을 쓴 뒤, 맞은 뒤 재생이 멈추는 시간. */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Stamina") float StaminaRegenDelayAfterSpend = 1.f;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ds|Stamina") float StaminaRegenDelayAfterHit = 1.5f;
};
