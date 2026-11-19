#include "UI/DsTitleMenuWidget.h"

#include "Game/DsPlayerController.h"
#include "Flow/DrUIFlowSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/DrButton.h"

#define LOCTEXT_NAMESPACE "DsMenu"

void UDsTitleMenuWidget::BuildMenu(UVerticalBox* Contents) {
  AddHeading(Contents, LOCTEXT("Title", "DrSample"));
  AddButton(Contents, LOCTEXT("Start", "시작"))->OnClicked().AddUObject(this, &ThisClass::StartGame);
  AddButton(Contents, LOCTEXT("Settings", "설정"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* Flow = UDrUIFlowSubsystem::Get(this)) Flow->OpenSettings();
  });
  AddButton(Contents, LOCTEXT("Quit", "종료"))->OnClicked().AddWeakLambda(this, [this] {
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
  });
}
void UDsTitleMenuWidget::StartGame() {
  if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->StartGame();
}

#undef LOCTEXT_NAMESPACE
