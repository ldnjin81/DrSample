#include "Debug/DsPerfSubsystem.h"

#include "Game/DsPlayerController.h"
#include "Boss/DsBossCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Scalability.h"
#include "UnrealClient.h"

void UDsPerfSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
  Super::Initialize(Collection);
  if (!FParse::Param(FCommandLine::Get(), TEXT("DsPerfCapture"))) return;
  Started = FPlatformTime::Seconds();
  Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick));
}
void UDsPerfSubsystem::Deinitialize() {
  FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
  Super::Deinitialize();
}
namespace {
FVector RoadCamera(float X) {
  return FVector(X, 1800 * FMath::Sin(X * PI / 16000) - 150, X * .01875f + 280);
}

}
bool UDsPerfSubsystem::Tick(float) {
  const double Now = FPlatformTime::Seconds();
  if (Now - Started > 180) return Finish(TEXT("성능 측정 준비 시간 초과"));
  auto* World = GetGameInstance()->GetWorld();
  auto* PC = World ? Cast<ADsPlayerController>(World->GetFirstPlayerController()) : nullptr;
  if (!PC) return true;
  if (PC->IsTitleScreen()) {
    if (!bTravelStarted) { PC->StartGame(); bTravelStarted = true; }
    return true;
  }
  if (!World->HasBegunPlay() || !PC->GetPawn()) return true;
  if (!Camera.IsValid()) {
    TActorIterator<ADsBossCharacter> Boss(World);
    if (!Boss) return true;
    ArenaCenter = Boss->GetHomeTransform().GetLocation();
    Camera = World->SpawnActor<ACameraActor>();
    if (!Camera.IsValid()) return Finish(TEXT("성능 측정 카메라 생성 실패"));
    Camera->GetCameraComponent()->SetFieldOfView(90);
    PC->SetViewTarget(Camera.Get());
    // 이전 실행의 사용자 품질 저장값에 영향을 받지 않도록 엔진 기본값을 메모리에만 적용한다.
    Scalability::FQualityLevels DefaultQuality;
    DefaultQuality.SetDefaults();
    Scalability::SetQualityLevels(DefaultQuality);
    // FStatUnitData의 Raw 값은 stat unit이 그려질 때 갱신된다.
    PC->ConsoleCommand(TEXT("stat unit"));
    PC->ConsoleCommand(TEXT("t.MaxFPS 0"));
    PC->ConsoleCommand(TEXT("r.VSync 0"));
    WarmupStarted = Now;
  }
  const FVector Points[] = {RoadCamera(0), RoadCamera(14000), RoadCamera(30500),
    FVector(34000, 0, 880), ArenaCenter + FVector(-500, -500, 300)};
  if (Now - WarmupStarted < 10) {
    Camera->SetActorLocationAndRotation(Points[0], (Points[1] - Points[0]).Rotation());
    return true;
  }
  if (!CaptureStarted) { CaptureStarted = LastFrame = Now; }
  const double Elapsed = Now - CaptureStarted;
  if (Elapsed >= 60) return Finish();
  const int32 Segment = FMath::Clamp(FMath::FloorToInt(Elapsed / 15), 0, 3);
  const float Alpha = (Elapsed - Segment * 15) / 15;
  const FVector Location = FMath::Lerp(Points[Segment], Points[Segment + 1], Alpha);
  FVector Look = Points[Segment + 1] - Location;
  if (Segment == 3) Look = ArenaCenter + FVector(0, 0, 100) - Location;
  Camera->SetActorLocationAndRotation(Location, Look.Rotation());
  if (Now > LastFrame) {
    FSample Sample;
    Sample.Frame = (Now - LastFrame) * 1000;
    if (auto* Viewport = World->GetGameViewport()) if (const auto* Unit = Viewport->GetStatUnitData()) {
      Sample.Game = Unit->RawGameThreadTime;
      Sample.GPU = Unit->RawGPUFrameTime[0];
    }
    Samples[Segment].Add(Sample);
  }
  LastFrame = Now;
  return true;
}
bool UDsPerfSubsystem::Finish(const FString& Error) {
  auto Root = MakeShared<FJsonObject>();
  Root->SetStringField(TEXT("error"), Error);
  Root->SetNumberField(TEXT("duration_seconds"), CaptureStarted ? FPlatformTime::Seconds() - CaptureStarted : 0);
  Root->SetStringField(TEXT("thread_source"), TEXT("FStatUnitData.RawGameThreadTime / RawGPUFrameTime[0]"));
  Root->SetStringField(TEXT("quality"), TEXT("패키지 기본 품질, VSync 및 프레임 제한 해제"));
  const auto Quality = Scalability::GetQualityLevels();
  auto QualityJson = MakeShared<FJsonObject>();
  QualityJson->SetNumberField(TEXT("resolution_quality"), Quality.ResolutionQuality);
  QualityJson->SetNumberField(TEXT("view_distance"), Quality.ViewDistanceQuality);
  QualityJson->SetNumberField(TEXT("shadows"), Quality.ShadowQuality);
  QualityJson->SetNumberField(TEXT("global_illumination"), Quality.GlobalIlluminationQuality);
  QualityJson->SetNumberField(TEXT("reflections"), Quality.ReflectionQuality);
  QualityJson->SetNumberField(TEXT("foliage"), Quality.FoliageQuality);
  Root->SetObjectField(TEXT("scalability"), QualityJson);
  if (auto* World = GetGameInstance()->GetWorld()) if (auto* Viewport = World->GetGameViewport()) {
    const FIntPoint Size = Viewport->Viewport ? Viewport->Viewport->GetSizeXY() : FIntPoint::ZeroValue;
    Root->SetNumberField(TEXT("width"), Size.X); Root->SetNumberField(TEXT("height"), Size.Y);
  }
  const TCHAR* Names[] = {TEXT("진입로 앞"), TEXT("숲 가운데"), TEXT("관문"), TEXT("아레나")};
  TArray<TSharedPtr<FJsonValue>> Sections;
  bool bComplete = Error.IsEmpty();
  for (int32 I = 0; I < 4; ++I) {
    auto Section = MakeShared<FJsonObject>();
    Section->SetStringField(TEXT("name"), Names[I]);
    TArray<double> Frame, Game, GPU;
    for (const auto& Sample : Samples[I]) {
      Frame.Add(Sample.Frame);
      if (Sample.Game > 0) Game.Add(Sample.Game);
      if (Sample.GPU > 0) GPU.Add(Sample.GPU);
    }
    bComplete &= !Frame.IsEmpty() && !Game.IsEmpty() && !GPU.IsEmpty();
    auto Stats = Summarize(Frame);
    Section->SetObjectField(TEXT("frame"), Stats);
    Section->SetObjectField(TEXT("game_thread"), Summarize(Game));
    Section->SetObjectField(TEXT("gpu"), Summarize(GPU));
    const bool bOverBudget = !Frame.IsEmpty() && Stats->GetNumberField(TEXT("p95_ms")) > 16.7;
    Section->SetBoolField(TEXT("over_budget"), bOverBudget);
    if (bOverBudget) {
      Section->SetStringField(TEXT("candidate_causes"), TEXT("원인 후보(미확정): Nanite Foliage, 섀도, Lumen. GPU·게임 스레드 분포와 프로파일링으로 구분 필요"));
    }
    Sections.Add(MakeShared<FJsonValueObject>(Section));
  }
  Root->SetBoolField(TEXT("completed"), bComplete);
  Root->SetArrayField(TEXT("sections"), Sections);
  FString Json;
  FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
  const FString Directory = FPaths::ProjectSavedDir() / TEXT("Perf");
  IFileManager::Get().MakeDirectory(*Directory, true);
  const bool bSaved = FFileHelper::SaveStringToFile(Json, *(Directory / TEXT("result.json")));
  // 예산 초과는 측정 실패가 아니다. 표본 누락·저장 실패만 실패로 처리한다.
  FPlatformMisc::RequestExitWithStatus(false, bComplete && bSaved ? 0 : 1);
  return false;
}
