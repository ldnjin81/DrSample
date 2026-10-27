#include "UI/DsTitleMenuWidget.h"

#include "UI/DsGlassButton.h"
#include "Game/DsPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

void UDsTitleMenuWidget::BuildMenu(UVerticalBox* Contents) {
  AddHeading(Contents, TEXT("Title"));
  AddButton(Contents, TEXT("Start"))->OnClicked().AddUObject(this, &ThisClass::StartGame);
  AddButton(Contents, TEXT("Settings"))->OnClicked().AddWeakLambda(this, [this] {
    if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->OpenSettings();
  });
  AddButton(Contents, TEXT("Quit"))->OnClicked().AddWeakLambda(this, [this] {
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
  });
}
void UDsTitleMenuWidget::StartGame() {
  if (auto* PC = GetOwningPlayer<ADsPlayerController>()) PC->StartGame();
}
