#include "Combat/DsLightHitShake.h"

#include "Combat/DsHitShakePattern.h"

UDsLightHitShake::UDsLightHitShake(const FObjectInitializer& Initializer) : Super(Initializer) {
  bSingleInstance = true;
  auto* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Perlin"));
  DrHitShake::Configure(Pattern, 1.f, .16f);
  SetRootShakePattern(Pattern);
}
