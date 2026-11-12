#include "Debug/DsPerfSubsystem.h"

#include "Game/DsPlayerController.h"
#include "Boss/DsBossCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace {
FVector RoadCamera(float X) {
  return FVector(X, 1800 * FMath::Sin(X * PI / 16000) - 150, X * .01875f + 280);
}
}

UDsPerfSubsystem::UDsPerfSubsystem() {
  SectionNames = {TEXT("진입로 앞"), TEXT("숲 가운데"), TEXT("관문"), TEXT("아레나")};
  OverBudgetHint = TEXT("원인 후보(미확정): Nanite Foliage, 섀도, Lumen. GPU·게임 스레드 분포와 프로파일링으로 구분 필요");
}

bool UDsPerfSubsystem::PrepareCapture(UWorld& World, FString& OutError) {
  auto* PC = Cast<ADsPlayerController>(World.GetFirstPlayerController());
  if (!PC) return false;
  if (PC->IsTitleScreen()) {
    if (!bTravelStarted) { PC->StartGame(); bTravelStarted = true; }
    return false;
  }
  if (!PC->GetPawn()) return false;
  TActorIterator<ADsBossCharacter> Boss(&World);
  if (!Boss) return false;
  ArenaCenter = Boss->GetHomeTransform().GetLocation();
  Camera = World.SpawnActor<ACameraActor>();
  if (!Camera.IsValid()) { OutError = TEXT("성능 측정 카메라 생성 실패"); return false; }
  Camera->GetCameraComponent()->SetFieldOfView(90);
  PC->SetViewTarget(Camera.Get());
  return true;
}

void UDsPerfSubsystem::UpdateCapture(UWorld&, int32 Section, float Alpha) {
  if (!Camera.IsValid()) return;
  const FVector Points[] = {RoadCamera(0), RoadCamera(14000), RoadCamera(30500),
    FVector(34000, 0, 880), ArenaCenter + FVector(-500, -500, 300)};
  // 예열 중에는 출발점에서 첫 구간 방향을 본다.
  if (Section == INDEX_NONE) {
    Camera->SetActorLocationAndRotation(Points[0], (Points[1] - Points[0]).Rotation());
    return;
  }
  const FVector Location = FMath::Lerp(Points[Section], Points[Section + 1], Alpha);
  FVector Look = Points[Section + 1] - Location;
  if (Section == 3) Look = ArenaCenter + FVector(0, 0, 100) - Location;
  Camera->SetActorLocationAndRotation(Location, Look.Rotation());
}
