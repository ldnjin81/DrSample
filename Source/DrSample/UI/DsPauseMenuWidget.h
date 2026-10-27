#pragma once

#include "CoreMinimal.h"
#include "UI/DsMenuWidget.h"
#include "DsPauseMenuWidget.generated.h"

/** 일시정지 메뉴. 계속·체크포인트에서 다시·설정·타이틀로를 고른다. */
UCLASS()
class DRSAMPLE_API UDsPauseMenuWidget : public UDsMenuWidget {
  GENERATED_BODY()
protected:
  virtual void BuildMenu(UVerticalBox* Contents) override;
};
