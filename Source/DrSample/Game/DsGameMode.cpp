#include "Game/DsGameMode.h"
#include "TableData/DsGameAssets.h"

#include "Characters/DsCharacter.h"
#include "Game/DsPlayerController.h"
#include "UObject/SoftObjectPath.h"

ADsGameMode::ADsGameMode() {
  DefaultPawnClass = ADsCharacter::StaticClass();
  PlayerControllerClass = ADsPlayerController::StaticClass();
}

void ADsGameMode::InitGame(const FString &MapName, const FString &Options,
                           FString &ErrorMessage) {
  Super::InitGame(MapName, Options, ErrorMessage);
  const FSoftObjectPath Path = DsGameAssets::Path(TEXT("Player"));
  if (UClass* CharacterClass = FSoftClassPath(Path.ToString()).TryLoadClass<ADsCharacter>()) {
    DefaultPawnClass = CharacterClass;
  } else {
    UE_LOG(LogTemp, Error, TEXT("GameAssets.Player 플레이어 클래스를 불러올 수 없습니다: %s"), *Path.ToString());
  }
}
