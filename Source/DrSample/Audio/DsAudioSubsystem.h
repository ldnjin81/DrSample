#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Audio/DsAudioData.h"
#include "TableData/Generated/EDsBossAttackKind.h"
#include "Encounter/DsEncounterSubsystem.h"
#include "DsAudioSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

/** 전투 상태의 오디오 수명과 메뉴용 믹스 값을 월드마다 관리한다. */
UCLASS()
class DRSAMPLE_API UDsAudioSubsystem : public UTickableWorldSubsystem {
  GENERATED_BODY()
public:
  virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
  /** 엔카운터를 먼저 초기화하고 그 상태·배우 변경 이벤트를 구독한다. */
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void OnWorldBeginPlay(UWorld& InWorld) override;
  virtual void Deinitialize() override;
  virtual void Tick(float DeltaTime) override;
  virtual TStatId GetStatId() const override;
  const UDsAudioData* GetData();
  USoundBase* ResolveBossAttack(EDsBossAttackKind Kind);
  void PlayBossAttack(AActor* Boss, EDsBossAttackKind Kind);
  void PlayDamage(AActor* Victim, float Damage, float Health, float MaxHealth);
  void EncounterChanged(EDsEncounterState State, AActor* Player, AActor* Boss);
  void MoveDoor(AActor* Door, bool bMoving);
  void SetPhase(int32 InPhase);
  UFUNCTION(BlueprintCallable, Category="Ds|Audio") void SetChannelVolume(EDsAudioChannel Channel, float Value);
  UFUNCTION(BlueprintPure, Category="Ds|Audio") float GetChannelVolume(EDsAudioChannel Channel) const;
  UAudioComponent* GetMusicComponent() const { return MusicComponent; }
  int32 GetPhase() const { return Phase; }
  EDsEncounterState GetMusicState() const { return MusicState; }
private:
  void PlayAt(USoundBase* Sound, AActor* Actor, bool bBoss);
  void HandleEncounterStateChanged(EDsEncounterState OldState, EDsEncounterState NewState);
  /** 엔카운터가 묶은 플레이어·보스의 피해 이벤트로 피격 소리를 낸다. */
  void HandleEncounterActorsChanged();
  void UnbindDamageEvents();
  TWeakObjectPtr<UDsEncounterSubsystem> Encounter;
  TArray<TWeakObjectPtr<AActor>> DamageSources;
  void StopOwnedAudio();
  UPROPERTY(Transient) TObjectPtr<UDsAudioData> Data;
  UPROPERTY(Transient) TObjectPtr<UAudioComponent> MusicComponent;
  UPROPERTY(Transient) TObjectPtr<UAudioComponent> DoorComponent;
  UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> EnvironmentComponents;
  TWeakObjectPtr<AActor> BossActor;
  float Volumes[3] = {1.f, 1.f, 1.f};
  float DistantRemaining = 3.f;
  float FootstepRemaining = 0.f;
  int32 Phase = 1;
  EDsEncounterState MusicState = EDsEncounterState::Idle;
  bool bRecording = false;
};
