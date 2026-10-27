#include "Game/DsTitleGameMode.h"

#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Game/DsPlayerController.h"
#include "Engine/World.h"

ADsTitleGameMode::ADsTitleGameMode() {
  DefaultPawnClass = nullptr;
  PlayerControllerClass = ADsPlayerController::StaticClass();
  PrimaryActorTick.bCanEverTick = true;
}
void ADsTitleGameMode::BeginPlay() {
  Super::BeginPlay();
  Camera = GetWorld()->SpawnActor<ACineCameraActor>();
  if (!Camera) return;
  // 관문 바깥에서 숲길을 돌아본다. 아레나 외벽·문 블록은 카메라 뒤에 둔다.
  const FVector Position(30900, -750, 1100);
  BaseRotation = (FVector(26300, -1450, 700) - Position).Rotation();
  Camera->SetActorLocationAndRotation(Position, BaseRotation);
  Camera->GetCineCameraComponent()->SetCurrentFocalLength(32.f);
  Camera->GetCineCameraComponent()->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
  if (auto* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
}
void ADsTitleGameMode::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  if (Camera) {
    FRotator Rotation = BaseRotation;
    Rotation.Yaw += FMath::Sin(GetWorld()->GetTimeSeconds() * .16f) * .4f;
    Camera->SetActorRotation(Rotation);
  }
}
