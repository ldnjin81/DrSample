#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DsGlassButton.generated.h"

class UTextBlock;
class UMaterialInstanceDynamic;

/** 키보드·패드 포커스와 마우스 호버를 같은 유리 림으로 표시한다. */
UCLASS()
class DRSAMPLE_API UDsGlassButton : public UCommonButtonBase {
  GENERATED_BODY()
public:
  UDsGlassButton();
  void SetLabel(const FText& InLabel);
protected:
  virtual void NativeOnInitialized() override;
  virtual void NativeOnHovered() override;
  virtual void NativeOnUnhovered() override;
  virtual void NativeOnClicked() override;
  virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
  virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
private:
  void RefreshGlass();
  FText Label;
  bool bFocused = false;
  UPROPERTY(Transient) TObjectPtr<UTextBlock> Text;
  UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Glass;
};
