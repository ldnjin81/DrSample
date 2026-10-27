#include "UI/DsGlassButtonStyle.h"

UDsGlassButtonStyle::UDsGlassButtonStyle() {
  for (auto* Brush : {&NormalBase, &NormalHovered, &NormalPressed, &SelectedBase,
      &SelectedHovered, &SelectedPressed, &Disabled}) Brush->DrawAs = ESlateBrushDrawType::NoDrawType;
  ButtonPadding = FMargin(0);
  CustomPadding = FMargin(0);
}
