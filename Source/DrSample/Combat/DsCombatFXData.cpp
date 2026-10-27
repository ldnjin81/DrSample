#include "Combat/DsCombatFXData.h"
#include "TableData/DsGameAssets.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRendererProperties.h"

TArray<UNiagaraRendererProperties*> UDsCombatFXData::GetConversionRenderers(UNiagaraSystem* InSystem) {
  TArray<UNiagaraRendererProperties*> Result;
#if WITH_EDITOR
  if (InSystem) {
    for (const auto& Handle : InSystem->GetEmitterHandles()) {
      if (const auto* Data = Handle.GetEmitterData()) Result.Append(Data->GetRenderers());
    }
  }
#endif
  return Result;
}

bool UDsCombatFXData::EnableMeshMaterialOverrides(UNiagaraRendererProperties* Renderer) {
#if WITH_EDITOR
  if (auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer)) {
    Mesh->Modify();
    Mesh->bOverrideMaterials = true;
    FPropertyChangedEvent Event(UNiagaraMeshRendererProperties::StaticClass()->FindPropertyByName(TEXT("bOverrideMaterials")));
    Mesh->PostEditChangeProperty(Event);
    return true;
  }
#endif
  return false;
}

bool UDsCombatFXData::WaitForConversionCompilation(UNiagaraSystem* InSystem) {
#if WITH_EDITOR
  if (!InSystem) return false;
  InSystem->WaitForCompilationComplete(true, false);
  return InSystem->IsReadyToRun();
#else
  return false;
#endif
}

void UDsCombatFXData::ClearConversionEmitters(UNiagaraSystem* InSystem) {
#if WITH_EDITOR
  if (!InSystem) return;
  InSystem->Modify();
  TSet<FGuid> Handles;
  for (const auto& Handle : InSystem->GetEmitterHandles()) Handles.Add(Handle.GetId());
  InSystem->RemoveEmitterHandlesById(Handles);
#endif
}
const UDsCombatFXData* UDsCombatFXData::Get() {
  static TWeakObjectPtr<UDsCombatFXData> Cached;
  if (!Cached.IsValid()) {
    Cached = DsGameAssets::Load<UDsCombatFXData>(TEXT("CombatFX"));
    if (!Cached.IsValid()) UE_LOG(LogTemp, Warning, TEXT("전투 FX 데이터가 없어 기본값을 사용합니다."));
  }
  return Cached.IsValid() ? Cached.Get() : GetDefault<UDsCombatFXData>();
}
