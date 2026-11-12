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
// -DrSmokeTest로 실행되면 기반(UDrSmokeSubsystem::Initialize)이 부른다.
void UDsSmokeSubsystem::RegisterSmokeSteps() {
 auto Controller=[this]() { auto* World=GetGameInstance()->GetWorld(); return World ? Cast<ADsPlayerController>(World->GetFirstPlayerController()) : nullptr; };
 // 컨트롤러가 타이틀 메뉴보다 먼저 생기므로 메뉴 입력이 될 때까지 기다린다(타이틀이 아니면 시간 초과로 실패).
 RegisterStep(TEXT("title"),[Controller](float) { auto* PC=Controller(); return PC && PC->IsTitleScreen() && PC->IsMenuInput() ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("travel"),[Controller](float StepSeconds) { if(StepSeconds<2) return EDrSmokeResult::Pending; auto* PC=Controller(); if(!PC) return EDrSmokeResult::Pending; if(PC->IsTitleScreen()) { PC->StartGame(); return EDrSmokeResult::Pending; } return EDrSmokeResult::Passed; });
 RegisterStep(TEXT("player"),[Controller](float) { auto* PC=Controller(); return PC && Cast<ADsCharacter>(PC->GetPawn()) ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("boss"),[this](float) { auto* World=GetGameInstance()->GetWorld(); return World && TActorIterator<ADsBossCharacter>(World) ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("tables"),[](float) { auto* Tables=UDrTableSubsystem::Get(); return Tables && Tables->AreAllTablesLoaded() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 // 빈 경로는 의도된 "없음"(DrTableSystem이 허용, 샘플은 CombatFX가 비어 있음)이라 건너뛰고, 값이 있는데 못 불러오면 실패.
 RegisterStep(TEXT("assets"),[](float) { for(const auto& Row:FDsGameAssetsRow::GetAll()) if(!Row.Path.IsNull() && !Row.Path.TryLoad()) { UE_LOG(LogTemp,Error,TEXT("스모크 assets: 불러오지 못함 %s (%s)"),*Row.Path.ToString(),*Row.Name.ToString()); return EDrSmokeResult::Failed; } return EDrSmokeResult::Passed; });
 RegisterStep(TEXT("hud"),[Controller](float) { auto* PC=Controller(); return PC && PC->GetCombatHUD() ? EDrSmokeResult::Passed : EDrSmokeResult::Pending; });
 RegisterStep(TEXT("pause_open"),[Controller](float StepSeconds) { if(StepSeconds<2) return EDrSmokeResult::Pending; auto* PC=Controller(); return PC && PC->OpenPauseMenu() && PC->IsPaused() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 RegisterStep(TEXT("settings"),[Controller](float) { auto* PC=Controller(); if(!PC) return EDrSmokeResult::Pending; PC->OpenSettings(); return PC->GetSettingsMenu() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
 RegisterStep(TEXT("pause_close"),[Controller](float) { auto* PC=Controller(); if(!PC) return EDrSmokeResult::Pending; PC->BackMenu(); PC->ClosePauseMenu(); return !PC->IsPaused() && !PC->IsMenuInput() ? EDrSmokeResult::Passed : EDrSmokeResult::Failed; });
}
