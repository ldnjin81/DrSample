#include "Debug/DsSmokeSubsystem.h"
#include "Game/DsPlayerController.h"
#include "Characters/DsCharacter.h"
#include "Boss/DsBossCharacter.h"
#include "UI/DsCombatHUDWidget.h"
#include "UI/DrUISettings.h"
#include "DrTableSubsystem.h"
#include "TableData/Generated/DsGameAssetsRow.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
void UDsSmokeSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
 Super::Initialize(Collection);
 if(!FParse::Param(FCommandLine::Get(),TEXT("DsSmokeTest"))) return;
 auto Controller=[this]() { auto* World=GetGameInstance()->GetWorld(); return World ? Cast<ADsPlayerController>(World->GetFirstPlayerController()) : nullptr; };
 RegisterStep(TEXT("title"),[Controller](float) { auto* PC=Controller(); return !PC ? EDrSmokeResult::Pending : PC->IsTitleScreen() && PC->IsMenuInput() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 RegisterStep(TEXT("travel"),[Controller](float StepSeconds) { if(StepSeconds<2) return EDrSmokeResult::Pending; auto* PC=Controller(); if(!PC) return EDrSmokeResult::Pending; if(PC->IsTitleScreen()) { PC->StartGame(); return EDrSmokeResult::Pending; } return EDrSmokeResult::Passed; });
 RegisterStep(TEXT("player"),[Controller](float) { auto* PC=Controller(); return PC && Cast<ADsCharacter>(PC->GetPawn()) ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("boss"),[this](float) { auto* World=GetGameInstance()->GetWorld(); return World && TActorIterator<ADsBossCharacter>(World) ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("tables"),[](float) { auto* Tables=UDrTableSubsystem::Get(); return Tables && Tables->AreAllTablesLoaded() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 RegisterStep(TEXT("assets"),[](float) { for(const auto& Row:FDsGameAssetsRow::GetAll()) if(Row.Path.IsNull() || !Row.Path.TryLoad()) return EDrSmokeResult::Failed; return EDrSmokeResult::Passed; });
 RegisterStep(TEXT("hud"),[Controller](float) { auto* PC=Controller(); return PC && PC->GetCombatHUD() ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("pause_open"),[Controller](float StepSeconds) { if(StepSeconds<2) return EDrSmokeResult::Pending; auto* PC=Controller(); return PC && PC->OpenPauseMenu() && PC->IsPaused() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 RegisterStep(TEXT("settings"),[Controller](float) { auto* PC=Controller(); if(!PC) return EDrSmokeResult::Pending; PC->OpenSettings(); return PC->GetSettingsMenu() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 RegisterStep(TEXT("pause_close"),[Controller](float) { auto* PC=Controller(); if(!PC) return EDrSmokeResult::Pending; PC->BackMenu(); PC->ClosePauseMenu(); return !PC->IsPaused() && !PC->IsMenuInput() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 StartSmoke();
}
