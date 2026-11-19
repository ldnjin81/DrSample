#pragma once

#include "CoreMinimal.h"
#include "Flow/DrUIScreens.h"
#include "DsMenuWidget.generated.h"

class UBorder;
class UDrActionBar;
class UDrButton;
class UVerticalBox;

/**
 * 메뉴 공통 틀: 화면을 덮는 막 + 가운데 표면(UDrSurface) + 아래 행동 안내 바(UDrActionBar).
 * 모양은 모두 DrUIKit 테마 토큰으로 칠하므로 스타일을 바꾸면 메뉴도 같이 바뀐다.
 * 입력 라우팅·첫 포커스·뒤로는 DrUIKit 메뉴 기반(UDrUIMenuWidget)이 맡는다.
 */
UCLASS(Abstract)
class DRSAMPLE_API UDsMenuWidget : public UDrUIMenuWidget {
  GENERATED_BODY()
public:
  using UDrUIMenuWidget::UDrUIMenuWidget;
protected:
  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;
  virtual void BuildMenu(UVerticalBox* Contents) PURE_VIRTUAL(UDsMenuWidget::BuildMenu, );
  /** 메뉴 버튼을 더한다. 첫 버튼이 첫 포커스가 된다. */
  UDrButton* AddButton(UVerticalBox* Contents, const FText& Label);
  void AddHeading(UVerticalBox* Contents, const FText& Text);
  /** 표면 너비. 설정처럼 줄이 긴 메뉴는 넓힌다. */
  virtual float GetPanelWidth() const { return 440.f; }
  /** 아래 안내에 '뒤로'를 보일지. 뒤로가 없는 타이틀은 끈다. */
  virtual bool HasBackAction() const { return true; }
private:
  void ApplyScrim();
  UPROPERTY(Transient) TObjectPtr<UBorder> Scrim;
  UPROPERTY(Transient) TObjectPtr<UDrActionBar> ActionBar;
  FDelegateHandle ThemeHandle;
};
