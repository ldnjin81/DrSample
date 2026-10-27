#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "DsBossCharacter.generated.h"

class UDsAbilitySystemComponent;
class UDsAttributeSet;
class UDsBossBrainComponent;
class UDsCombatFXComponent;
class UDsLockOnTargetComponent;
class UAnimSequence;
class UDsBossData;

/** 플레이어와 같은 GAS 피해 경로를 사용하는 Boss 보스이다. */
UCLASS()
class DRSAMPLE_API ADsBossCharacter : public ACharacter, public IAbilitySystemInterface {
  GENERATED_BODY()
public:
  ADsBossCharacter();
  virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
  virtual void PostInitializeComponents() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  UDsAbilitySystemComponent* GetDrASC() const { return AbilitySystem; }
  UDsAttributeSet* GetAttributes() const { return Attributes; }
  UDsBossBrainComponent* GetBrain() const { return Brain; }
  void PlaySequence(FName InName, bool bLoop);
  virtual void BeginPlay() override;
  void ResetBoss(const FTransform &Home);
  const FTransform &GetHomeTransform() const { return HomeTransform; }

  /** 이 보스의 데이터 에셋. 비어 있거나 못 불러오면 클래스 기본값을 쓴다. */
  UPROPERTY(EditAnywhere, Category="Ds|Boss") TSoftObjectPtr<UDsBossData> BossData;
  /** GameAssets에서 조회할 게임별 보스 데이터 키. */
  UPROPERTY(EditDefaultsOnly, Category="Ds|Boss") FName BossAssetKey = TEXT("Boss");
  const UDsBossData* GetData() const;
private:
  UPROPERTY(VisibleAnywhere) TObjectPtr<UDsCombatFXComponent> CombatFX;
  UPROPERTY(VisibleAnywhere) TObjectPtr<UDsLockOnTargetComponent> LockOnTarget;
  void HandlePoiseBroken(AActor* InstigatorActor, AActor* Causer);
  void HandleOutOfHealth(AActor* InstigatorActor, AActor* Causer);
  void HandleDamaged(float Damage, AActor* InstigatorActor, AActor* Causer);
  UPROPERTY(VisibleAnywhere, Category="Ds|Boss") TObjectPtr<UDsAbilitySystemComponent> AbilitySystem;
  UPROPERTY(VisibleAnywhere, Category="Ds|Boss") TObjectPtr<UDsAttributeSet> Attributes;
  UPROPERTY(VisibleAnywhere, Category="Ds|Boss") TObjectPtr<UDsBossBrainComponent> Brain;
  UPROPERTY(Transient) TMap<FName, TObjectPtr<UAnimSequence>> Animations;
  TSet<FName> MissingAnimations;
  bool bEventsBound = false;
  UPROPERTY(Transient) mutable TObjectPtr<const UDsBossData> LoadedData;
  FTransform HomeTransform = FTransform::Identity;
};
