#include "UI/DsPauseMenuWidget.h"

#include "Game/DsPlayerController.h"
#include "Flow/DrUIFlowSubsystem.h"
#include "Widgets/DrButton.h"

#define LOCTEXT_NAMESPACE "DsMenu"

void UDsPauseMenuWidget::BuildMenu(UVerticalBox* Contents) {
  AddHeading(Contents, LOCTEXT("Paused", "일시정지"));
  AddButton(Contents, LOCTEXT("Continue", "계속하기"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* Flow = UDrUIFlowSubsystem::Get(this)) Flow->ClosePauseMenu();
  });
  AddButton(Contents, LOCTEXT("Restart", "체크포인트에서 다시"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->RestartCheckpoint();
  });
  AddButton(Contents, LOCTEXT("Settings", "설정"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* Flow = UDrUIFlowSubsystem::Get(this)) Flow->OpenSettings();
  });
  AddButton(Contents, LOCTEXT("ToTitle", "타이틀로"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->ReturnToTitle();
  });
}

#undef LOCTEXT_NAMESPACE
