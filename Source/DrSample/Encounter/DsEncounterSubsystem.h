#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DsEncounterSubsystem.generated.h"

class ADsCharacter;
class ADsBossCharacter;

UENUM(BlueprintType)
enum class EDsEncounterState : uint8 { Idle, Fighting, PlayerDead, Victory };

DECLARE_MULTICAST_DELEGATE_TwoParams(FDsEncounterStateChangedDelegate, EDsEncounterState, EDsEncounterState);
DECLARE_MULTICAST_DELEGATE(FDsEncounterActorsChangedDelegate);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDsEncounterDoorMovingDelegate, AActor* /*Door*/, bool /*bMoving*/);

/** 월드 안에서만 체크포인트·문·사망·승리를 관리한다. */
UCLASS()
class DRSAMPLE_API UDsEncounterSubsystem : public UTickableWorldSubsystem {
  GENERATED_BODY()
public:
  virtual void OnWorldBeginPlay(UWorld &InWorld) override;
  virtual void Deinitialize() override;
  virtual void Tick(float DeltaTime) override;
  virtual TStatId GetStatId() const override;
  void BindActors(ADsCharacter *InPlayer, ADsBossCharacter *InBoss, AActor *InCheckpoint,
                  AActor *InDoor, AActor *InStart);
  void Advance(float DeltaSeconds);
  void SetDoorClosedZ(float InZ);
  EDsEncounterState GetState() const { return State; }
  bool IsCheckpointActive() const { return bCheckpointActive; }
  FTransform GetRespawnTransform() const;
  void RestartCheckpoint();
  bool IsBoundPlayer(const ADsCharacter *InPlayer) const;
  FDsEncounterStateChangedDelegate OnStateChanged;
  FDsEncounterActorsChangedDelegate OnActorsChanged;
  /** 아레나 문이 움직이기 시작하거나 멈출 때 알린다. */
  FDsEncounterDoorMovingDelegate OnDoorMoving;
  ADsBossCharacter *GetBoss() const;
  ADsCharacter *GetPlayer() const { return Player.Get(); }

  UPROPERTY(EditAnywhere, Category = "Ds|Encounter") float CheckpointRadius = 400.f;
  UPROPERTY(EditAnywhere, Category = "Ds|Encounter") float RespawnDelay = 3.f;
  UPROPERTY(EditAnywhere, Category = "Ds|Encounter") float DoorMoveDuration = 1.f;
  UPROPERTY(EditAnywhere, Category = "Ds|Encounter") float ArenaEnterDepth = 300.f;
protected:
  virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
  void BindPlayer(ADsCharacter *InPlayer);
  void UnbindActors();
  void SetState(EDsEncounterState InState);
  void MoveDoor(bool bClose);
  void AdvanceDoor(float DeltaSeconds);
  void HandlePlayerDeath(AActor *InstigatorActor, AActor *Causer);
  void HandleBossDeath(AActor *InstigatorActor, AActor *Causer);
  UFUNCTION() void HandleActorEndPlay(AActor *Actor, EEndPlayReason::Type Reason);

  TWeakObjectPtr<ADsCharacter> Player;
  TWeakObjectPtr<ADsBossCharacter> Boss;
  TWeakObjectPtr<AActor> Checkpoint;
  TWeakObjectPtr<AActor> Door;
  TWeakObjectPtr<AActor> Start;
  EDsEncounterState State = EDsEncounterState::Idle;
  bool bCheckpointActive = false;
  bool bRespawnPending = false;
  float DeathElapsed = 0.f;
  float DoorElapsed = 0.f;
  FVector DoorOpen = FVector::ZeroVector;
  FVector DoorClosed = FVector::ZeroVector;
  FVector DoorFrom = FVector::ZeroVector;
  FVector DoorTo = FVector::ZeroVector;
  FVector ArenaDirection = FVector::ForwardVector;
  bool bDoorMoving = false;
  FTransform BossHome = FTransform::Identity;
};
