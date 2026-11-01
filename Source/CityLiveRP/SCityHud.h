#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
class ACityLiveRPPlayerController;
class SCityHud : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCityHud){} SLATE_ARGUMENT(ACityLiveRPPlayerController*,Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    FButtonStyle SmallStyle,ChatStyle;
};
