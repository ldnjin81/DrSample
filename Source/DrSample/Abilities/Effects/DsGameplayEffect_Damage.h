// C++만으로 정의되는 즉시형 피해 GameplayEffect를 선언한다.
#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "DsGameplayEffect_Damage.generated.h"

/**
 * SetByCaller 크기로 Data.Damage와 Data.PoiseDamage 메타 속성을 구동한다.
 * 속성 세트가 두 값을 Health와 Poise 변화로 변환한다.
 */
UCLASS()
class DRSAMPLE_API UDsGameplayEffect_Damage : public UGameplayEffect {
  GENERATED_BODY()

public:
  UDsGameplayEffect_Damage();
};
