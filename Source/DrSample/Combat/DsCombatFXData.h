#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DsCombatFXData.generated.h"
class UNiagaraSystem;
class UNiagaraRendererProperties;

/** 보스 공격 테이블의 Id를 키로 연출을 지정한다. */
USTRUCT(BlueprintType)
struct FDsAttackFX {
  GENERATED_BODY()
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> Telegraph;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> Impact;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Socket = TEXT("hand_r");
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float Scale = 1.f;
};

UCLASS(BlueprintType)
class DRSAMPLE_API UDsCombatFXData : public UDataAsset {
  GENERATED_BODY()
public:
  static const UDsCombatFXData* Get();
  // 변환 스크립트가 에셋을 삭제하지 않고 같은 시스템을 다시 채우도록 한다.
  UFUNCTION(BlueprintCallable, Category="Ds|FX", meta=(DevelopmentOnly))
  static void ClearConversionEmitters(UNiagaraSystem* InSystem);
  // 변환기의 비동기 컴파일이 끝나기 전에 저장·시뮬레이션하지 않게 한다.
  UFUNCTION(BlueprintCallable, Category="Ds|FX", meta=(DevelopmentOnly))
  static bool WaitForConversionCompilation(UNiagaraSystem* InSystem);
  // 변환 결과의 렌더러를 진단하고 원본을 건드리지 않고 보정한다.
  UFUNCTION(BlueprintCallable, Category="Ds|FX", meta=(DevelopmentOnly))
  static TArray<UNiagaraRendererProperties*> GetConversionRenderers(UNiagaraSystem* InSystem);
  // 변환기는 메시 렌더러의 덮어쓰기 머티리얼 배열만 채우고 켜짐 플래그는 두지 않는다.
  // 플래그(bOverrideMaterials)는 파이썬 이름이 배열(override_materials)과 겹쳐 스크립트에서 바꿀 수 없다.
  UFUNCTION(BlueprintCallable, Category="Ds|FX", meta=(DevelopmentOnly))
  static bool EnableMeshMaterialOverrides(UNiagaraRendererProperties* Renderer);
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<int32, FDsAttackFX> BossAttacks;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> DashWind;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> DashBlur;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> DashDust;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> SwordLeft;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> SwordRight;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> PlayerImpact;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> PlayerSparks;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> BossImpact;
  /** 보스 공격 예고 바닥 원. 지면에 투영하는 디퍼드 데칼(스칼라 파라미터 Progress 0~1). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UMaterialInterface> GroundCircleDecal;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> PhaseBurst;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> PhaseLoop;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UMaterialInterface> GhostMaterial;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UMaterialInterface> TelegraphOverlay;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BodySocket = TEXT("pelvis");
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FName WeaponSocket = TEXT("hand_r");
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector WeaponCenterOffset = FVector(0, 0, -60);
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeaponGlowRadius = 85.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float GhostInterval = .2f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float GhostLifetime = .45f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) float GhostCameraHideDistance = 100.f;
  /** 어두운 지형에서도 충격 위치를 읽게 하는 짧은 점광원(칸델라). */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ImpactLightIntensity = 80.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float ImpactLightRadius = 250.f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ImpactLightDuration = .12f;
  UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor ImpactLightColor = FLinearColor(1.f, .35f, .08f);
};
