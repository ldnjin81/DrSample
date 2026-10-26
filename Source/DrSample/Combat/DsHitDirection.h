#pragma once

#include "CoreMinimal.h"

/** 피격자의 수평 좌표계에서 본 공격자 방향이다. */
enum class EDsHitDirection : uint8 { Front, Back, Left, Right };

/** 높이는 무시하며 대각선 경계는 앞/뒤에 포함한다. 겹친 위치는 정면이다. */
inline EDsHitDirection DsHitDirection(const FRotator& VictimRotation, const FVector& ToAttacker) {
  const FVector Local = FRotator(0, VictimRotation.Yaw, 0).UnrotateVector(FVector(ToAttacker.X, ToAttacker.Y, 0).GetSafeNormal());
  // 회전의 삼각함수 근사 오차가 정확한 대각선의 포함 방향을 뒤집지 않게 한다.
  if (FMath::Abs(Local.X) + UE_KINDA_SMALL_NUMBER >= FMath::Abs(Local.Y))
    return Local.X >= 0 ? EDsHitDirection::Front : EDsHitDirection::Back;
  return Local.Y < 0 ? EDsHitDirection::Left : EDsHitDirection::Right;
}

template<typename T>
T* DsSelectHitMontage(EDsHitDirection Direction, T* Front, T* Back, T* Left, T* Right) {
  T* Selected = Direction == EDsHitDirection::Back ? Back :
    Direction == EDsHitDirection::Left ? Left : Direction == EDsHitDirection::Right ? Right : Front;
  return Selected ? Selected : Front;
}
