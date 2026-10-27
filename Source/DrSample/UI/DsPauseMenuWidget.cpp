#include "UI/DsPauseMenuWidget.h"

#include "UI/DsGlassButton.h"
#include "Game/DsPlayerController.h"

void UDsPauseMenuWidget::BuildMenu(UVerticalBox* Contents) {
  AddHeading(Contents, TEXT("Paused"));
  AddButton(Contents, TEXT("Continue"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->ClosePauseMenu();
  });
  AddButton(Contents, TEXT("Restart"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->RestartCheckpoint();
  });
  AddButton(Contents, TEXT("Settings"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->OpenSettings();
  });
  AddButton(Contents, TEXT("ToTitle"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->ReturnToTitle();
  });
}
