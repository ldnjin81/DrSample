#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DsCharacter.generated.h"

class UDsCombatFXComponent;
class UDsLockOnComponent;
class UAnimMontage;
class UAnimSequence;
class UDsAbilitySystemComponent;
class UDsGameplayAbility;
class UDsAttributeSet;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UScriptStruct;
struct FInputActionValue;

/** GASP 로코모션을 구동하는 프로젝트 기본 캐릭터이다. */
UCLASS(Blueprintable)
class DRSAMPLE_API ADsCharacter : public ACharacter, public IAbilitySystemInterface {
  GENERATED_BODY()

public:
  ADsCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
  virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
  UFUNCTION(BlueprintPure, Category = "Ds|Abilities")
  UDsAttributeSet *GetAttributeSet() const { return Attributes; }
  virtual void PostInitializeComponents() override;
  virtual void PostActorCreated() override;
  virtual void PossessedBy(AController *NewController) override;
  virtual void OnRep_Controller() override;
  virtual void Landed(const FHitResult &Hit) override;
  virtual void SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;
  virtual void Tick(float DeltaSeconds) override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void FellOutOfWorld(const UDamageType &DamageType) override;
  /** 공격·막기·튕겨내기·구르기·경직·사망 중에는 점프하지 않는다(이동 입력 차단과 같은 조건). */
  virtual bool CanJumpInternal_Implementation() const override;
  void Revive(const FTransform &Where);

  /** 스폰될 때 한 번 부여하는 전투 어빌리티이다. */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ds|Abilities")
  TArray<TSubclassOf<UDsGameplayAbility>> DefaultAbilities;

  // 전투 입력 진입점. 입력 바인딩과 AI·테스트가 같은 경로를 쓴다.
  /** 약공격을 시작하거나, 진행 중이면 다음 타를 예약한다. */
  UFUNCTION(BlueprintCallable, Category = "Ds|Combat")
  bool InputAttackLight();
  UFUNCTION(BlueprintCallable, Category = "Ds|Combat")
  bool InputAttackHeavy();
  UFUNCTION(BlueprintCallable, Category = "Ds|Combat")
  bool InputGuard();
  UFUNCTION(BlueprintCallable, Category = "Ds|Combat")
  bool InputDodge();
  UFUNCTION(BlueprintCallable, Category = "Ds|Combat")
  bool InputLockOn();
  UDsLockOnComponent* GetLockOn() const { return LockOn; }

  /** 표시 메시에 전투 모션을 재생할 수 있는 상태인지 확인한다. */
  bool CanPlayCombatMontage() const;
  /** 표시 메시(Manny)에 전투 몽타주를 재생한다. */
  void PlayCombatMontage(UAnimMontage *Montage, bool bHoldLastPose = false,
                         float PlayRate = 1.f);
  void StopCombatMontage(UAnimMontage *Montage);

  UFUNCTION(BlueprintPure, Category = "Ds|Movement")
  bool IsWalking() const { return bWantsToWalk; }
  UFUNCTION(BlueprintPure, Category = "Ds|Movement")
  bool IsSprinting() const { return bWantsToSprint; }
  UFUNCTION(BlueprintPure, Category = "Ds|Movement")
  float GetWalkSpeed() const;
  UFUNCTION(BlueprintPure, Category = "Ds|Movement")
  float GetRunSpeed() const;
  UFUNCTION(BlueprintPure, Category = "Ds|Movement")
  float GetSprintSpeed() const;
  UFUNCTION(BlueprintCallable, Category = "Ds|Movement")
  void SetWalking(bool bInWalking);
  UFUNCTION(BlueprintCallable, Category = "Ds|Movement")
  void SetSprinting(bool bInSprinting);
  UFUNCTION(BlueprintCallable, CustomThunk, Category = "Ds|Animation",
            meta = (CustomStructureParam = "OutProps"))
  void FillAnimationProperties(int32 &OutProps);
  DECLARE_FUNCTION(execFillAnimationProperties);

protected:
  /** 복제되는 ASC 하위 객체. 상태 효과·어빌리티 틱·쿨다운을 진행한다. */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ds|Abilities")
  TObjectPtr<UDsAbilitySystemComponent> AbilitySystem;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ds|Combat")
  TObjectPtr<UDsLockOnComponent> LockOn;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> LockOnAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> GamepadLookAction;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ds|Abilities")
  TObjectPtr<UDsAttributeSet> Attributes;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ds|Appearance")
  TObjectPtr<USkeletalMeshComponent> MannyMesh;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ds|Camera")
  TObjectPtr<USpringArmComponent> CameraBoom;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ds|Camera")
  TObjectPtr<UCameraComponent> FollowCamera;


  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputMappingContext> InputMapping;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> MoveAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> LookAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> JumpAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> WalkAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> SprintAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> CrouchAction;

  /** 전투 키 매핑. 로코모션 매핑보다 높은 우선순위로 추가한다. */
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputMappingContext> CombatInputMapping;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> AttackLightAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> AttackHeavyAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> GuardAction;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Input")
  TObjectPtr<UInputAction> DodgeAction;

  UPROPERTY(EditDefaultsOnly, Category = "Ds|Combat")
  TObjectPtr<UAnimMontage> HitReactFrontMontage;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Combat")
  TObjectPtr<UAnimMontage> HitReactBackMontage;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Combat")
  TObjectPtr<UAnimMontage> HitReactLeftMontage;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Combat")
  TObjectPtr<UAnimMontage> HitReactRightMontage;
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Combat")
  TObjectPtr<UAnimMontage> DeathMontage;
  /** 전투 모션에서 이 뼈의 수평 이동만큼 캡슐을 함께 옮긴다. */
  UPROPERTY(EditDefaultsOnly, Category = "Ds|Combat")
  FName CombatMotionBone = TEXT("pelvis");

private:
  UPROPERTY(VisibleAnywhere) TObjectPtr<UDsCombatFXComponent> CombatFX;
  void FillAnimationPropertiesRaw(const UScriptStruct *Struct, void *Data) const;
  void InitializeAbilities();
  void GrantDefaultAbilities();
  /** 표시 메시에 붙은 보조 메시(의상 등)가 표시 메시 포즈를 따르게 한다. */
  void FollowVisibleMeshPose();
  void BindCombatEvents();
  void HandleDamaged(float Damage, AActor *DamageInstigator, AActor *Causer);
  UAnimMontage* GetHitReactMontage(const AActor* Attacker) const;
  void HandlePoiseBroken(AActor *DamageInstigator, AActor *Causer);
  void HandleOutOfHealth(AActor *DamageInstigator, AActor *Causer);
  /** 전투 동작·경직·사망 중에는 이동 입력을 받지 않는다. */
  bool IsMoveInputBlocked() const;
  /** 표시 메시의 애님 BP에 몽타주 슬롯 노드가 있는지 확인한다. */
  bool HasMontageSlot() const;
  void RestoreAnimBlueprint();
  /** 재생 중인 전투 모션의 지정 시각에서 이동 기준 뼈의 메시 공간 수평 위치를 구한다. */
  FVector SampleCombatMotion(float Time) const;
  /** 모션이 진행한 만큼 캡슐을 옮기고 표시 메시를 캡슐 위로 되돌려 놓는다. */
  void FollowCombatMotion(float DeltaSeconds);
  void OnInputAttackLight() { InputAttackLight(); }
  void OnInputAttackHeavy() { InputAttackHeavy(); }
  void OnInputGuard() { InputGuard(); }
  void OnInputDodge() { InputDodge(); }
  void OnInputLockOn() { InputLockOn(); }
  bool bAbilitiesGranted = false;
  /** 플레이어 전투 데이터의 스탯·특성을 속성에 한 번 적용했는지. */
  bool bStatsInitialized = false;
  bool bCombatEventsBound = false;
  float StaggerRemaining = 0.f;
  /** 슬롯이 없어 표시 메시를 단일 애니 재생으로 덮어쓴 남은 시간이다. 음수면 유지한다. */
  float PoseOverrideRemaining = 0.f;
  bool bPoseOverridden = false;
  UPROPERTY(Transient)
  TObjectPtr<UAnimSequence> MotionSequence;
  float MotionTime = 0.f;
  float MotionPlayRate = 1.f;
  FVector MotionLastOffset = FVector::ZeroVector;
  FVector MotionMeshBaseLocation = FVector::ZeroVector;
  void RefreshMaxSpeed();
  void Move(const FInputActionValue &Value);
  void Look(const FInputActionValue &Value);
  void LookCompleted() { ResetLockOnLook(); }
  void ResetLockOnLook();
  void StartWalk();
  void StopWalk();
  void StartSprint();
  void StopSprint();
  void ToggleCrouch();
  bool bWantsToWalk = false;
  bool bWantsToSprint = false;
  FVector LastLandVelocity = FVector::ZeroVector;
  float LastLandingTime = -1.f;
};
