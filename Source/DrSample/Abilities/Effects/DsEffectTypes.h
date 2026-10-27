// 데이터 테이블과 런타임에서 공유하는 상태 효과 타입을 선언한다.
#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TableData/Generated/EDsEffectStackRule.h"
#include "DsEffectTypes.generated.h"

struct FDsEffectDefsRow;
/** 어빌리티 시스템 컴포넌트가 적용하는 런타임 효과 값이다. */
USTRUCT()
struct FDsResolvedEffect {
  GENERATED_BODY()
  UPROPERTY() int32 DefinitionId = 0;

  UPROPERTY()
  FGameplayTag StateTag;
  UPROPERTY()
  float Duration = 0.f;
  UPROPERTY()
  float DamagePerTick = 0.f;
  UPROPERTY()
  float Period = 0.f;
  /** 효과 크기이며 보호막 흡수 풀 등에 사용한다. */
  UPROPERTY()
  float Magnitude = 0.f;
  // 활성 중 적용되는 데이터 기반 배수
  UPROPERTY()
  float MoveSpeedMultiplier = 1.f;
  UPROPERTY()
  float DamageTakenMultiplier = 1.f;
  UPROPERTY()
  float DamageDealtMultiplier = 1.f;
  // 적용 즉시 한 번 실행되는 효과
  UPROPERTY()
  float ProcInstantDamageCurHpPct = 0.f;
  UPROPERTY()
  EDsEffectStackRule StackRule = EDsEffectStackRule::Refresh;
  /** 1단계부터 적용하는 주 수치의 백분율 보너스이다. */
  UPROPERTY()
  TArray<int32> StackBonus;
};

namespace DsEffects {
/** 구운 효과 정의 행을 어빌리티 시스템이 소비하는 값으로 변환한다. */
DRSAMPLE_API FDsResolvedEffect Resolve(const FDsEffectDefsRow &Row);
}
