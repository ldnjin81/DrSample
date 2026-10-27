#pragma once
#include "Network/DrNetTypes.h"
#include "DsNetTypes.generated.h"
/** 로그인에 사용할 사용자 식별자이다. */
USTRUCT()
struct FDsLoginRequest {
  GENERATED_BODY()
  UPROPERTY()
  FString UserId;
};
/** 로그인 성공 시 발급되는 세션 토큰이다. */
USTRUCT()
struct FDsLoginResponse {
  GENERATED_BODY()
  UPROPERTY()
  FString SessionToken;
};

namespace DsMessages {
 inline const FName Ping(TEXT("Ping"));
 inline const FName Login(TEXT("Login"));
 inline const FName RequestPlayerProfile(TEXT("RequestPlayerProfile"));
 inline const FName ServerPush_ProfileUpdated(TEXT("ServerPush_ProfileUpdated"));
}

