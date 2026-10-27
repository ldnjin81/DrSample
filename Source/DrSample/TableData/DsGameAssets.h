#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

namespace DsGameAssets
{
  DRSAMPLE_API FSoftObjectPath Path(FName Key);

  template <typename T> T* Load(FName Key) {
    return Cast<T>(Path(Key).TryLoad());
  }
}
