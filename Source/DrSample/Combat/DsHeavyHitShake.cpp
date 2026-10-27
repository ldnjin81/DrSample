#include "Combat/DsHeavyHitShake.h"

#include "Combat/DsHitShakePattern.h"

UDsHeavyHitShake::UDsHeavyHitShake(const FObjectInitializer& Initializer) : Super(Initializer) {
  bSingleInstance = true;
  auto* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Perlin"));
  DrHitShake::Configure(Pattern, 2.5f, .3f);
  SetRootShakePattern(Pattern);
}
