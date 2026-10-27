#pragma once

#include "CoreMinimal.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

namespace DrHitShake {
/** 타격 셰이크 두 종류가 같은 펄린 패턴을 세기와 길이만 바꿔 쓴다. */
inline void Configure(UPerlinNoiseCameraShakePattern* Pattern, float Strength, float Duration) {
  Pattern->Duration = Duration;
  Pattern->BlendInTime = .015f;
  Pattern->BlendOutTime = Duration * .7f;
  Pattern->X.Amplitude = .3f * Strength;
  Pattern->Y.Amplitude = .5f * Strength;
  Pattern->Z.Amplitude = .8f * Strength;
  Pattern->Pitch.Amplitude = .45f * Strength;
  Pattern->Yaw.Amplitude = .2f * Strength;
  Pattern->Roll.Amplitude = .15f * Strength;
  Pattern->FOV.Amplitude = 0.f;
  Pattern->LocationFrequencyMultiplier = 28.f;
  Pattern->RotationFrequencyMultiplier = 24.f;
}
} // namespace DrHitShake
