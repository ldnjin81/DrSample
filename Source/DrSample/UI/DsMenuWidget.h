#pragma once

#include "CoreMinimal.h"
#include "Flow/DrUIScreens.h"
#include "DsMenuWidget.generated.h"

class UDsGlassButton;
class UVerticalBox;
class UVerticalBoxSlot;

/** 유리 패널 메뉴. 입력 라우팅·첫 포커스·되돌아가기는 DrUIKit 메뉴 기반(UDrUIMenuWidget)이 맡는다. */
UCLASS(Abstract)
class DRSAMPLE_API UDsMenuWidget : public UDrUIMenuWidget {
  GENERATED_BODY()
public:
  using UDrUIMenuWidget::UDrUIMenuWidget;
protected:
  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void BuildMenu(UVerticalBox* Contents) PURE_VIRTUAL(UDsMenuWidget::BuildMenu, );
  UDsGlassButton* AddButton(UVerticalBox* Contents, const TCHAR* Key);
  void AddHeading(UVerticalBox* Contents, const TCHAR* Key);
  /** 메뉴 문자열 테이블에서 문구를 찾는다. */
  static FText MenuText(const TCHAR* Key);
  /** 메뉴 항목 사이 간격과 가운데 정렬을 맞춘다. */
  static void ApplyItemSpacing(UVerticalBoxSlot* LayoutSlot);

};
