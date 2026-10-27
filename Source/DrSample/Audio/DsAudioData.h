#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DsAudioData.generated.h"

class USoundBase;
class USoundControlBus;
class USoundAttenuation;

/** 음량을 따로 조절하는 오디오 채널이다. 컨트롤 버스 순서와 같다. */
UENUM(BlueprintType)
enum class EDsAudioChannel : uint8 { SFX, Voice, Music };

/** 저장된 참조로 패키징에서도 사운드와 버스를 유지한다. */
UCLASS(BlueprintType)
class DRSAMPLE_API UDsAudioData : public UDataAsset {
  GENERATED_BODY()
public:
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundBase>> Whooshes;
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundBase>> Impacts;
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundBase>> BossAttacks;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundBase> Music;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundBase> Distant;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundBase> Wind;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundBase> Fire;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundBase> Slide;
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundBase>> Footsteps;
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundBase>> PlayerVoices;
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundBase>> BossVoices;
  UPROPERTY(EditAnywhere) TArray<TObjectPtr<USoundControlBus>> Buses;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundAttenuation> BossAttenuation;
  UPROPERTY(EditAnywhere) TObjectPtr<USoundAttenuation> EnvironmentAttenuation;
};
