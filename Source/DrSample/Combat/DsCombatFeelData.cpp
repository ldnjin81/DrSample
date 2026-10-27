#include "Combat/DsCombatFeelData.h"
#include "TableData/DsGameAssets.h"

const UDsCombatFeelData* UDsCombatFeelData::Get() {
  static TWeakObjectPtr<UDsCombatFeelData> Cached;
  if (Cached.IsValid()) return Cached.Get();
  const auto* Data = LoadObject<UDsCombatFeelData>(nullptr, *DsGameAssets::Path(TEXT("DA_CombatFeel")).ToString());
  Cached = const_cast<UDsCombatFeelData*>(Data);
  return Data ? Data : GetDefault<UDsCombatFeelData>();
}
