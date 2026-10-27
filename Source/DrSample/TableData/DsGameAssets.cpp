#include "TableData/DsGameAssets.h"

#include "TableData/Generated/DsGameAssetsRow.h"

FSoftObjectPath DsGameAssets::Path(FName Key) {
  const FDsGameAssetsRow* Row = FDsGameAssetsRow::Find(Key);
  if (!Row || Row->Path.IsNull()) {
    UE_LOG(LogTemp, Warning, TEXT("GameAssets 테이블에 경로가 없습니다: %s"), *Key.ToString());
    return FSoftObjectPath();
  }
  return Row->Path;
}
