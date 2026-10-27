#pragma once

#include "CoreMinimal.h"

/** 속성 이벤트로 목표를 바꾸고, 피해가 있을 때만 짧게 갱신하는 잔상이다. */
struct FDsHUDDamageTrail {
  float Value = 1.f;
  float Target = 1.f;
  float StartValue = 1.f;
  double FollowAt = 0.0;
  bool bActive = false;

  void Reset(float Rate) { Value = Target = StartValue = Rate; bActive = false; }
  void SetTarget(float Rate, double Now) {
    Rate = FMath::Clamp(Rate, 0.f, 1.f);
    if (Rate == Target) return;
    if (Rate >= Target) { Reset(Rate); return; }
    Target = Rate;
    StartValue = Value;
    FollowAt = Now + .4;
    bActive = true;
  }
  void Advance(double Now) {
    if (!bActive || Now < FollowAt) return;
    const float Alpha = FMath::Clamp(float((Now - FollowAt) / .3), 0.f, 1.f);
    Value = FMath::Lerp(StartValue, Target, Alpha);
    if (Alpha >= 1.f) { Value = Target; bActive = false; }
  }
};
