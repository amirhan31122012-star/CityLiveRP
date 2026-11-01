#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class UCityDistrictGameplayComponent;
class SCityChatLog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCityChatLog):_Lines(5),_FontSize(17){}
        SLATE_ARGUMENT(UCityDistrictGameplayComponent*,District)
        SLATE_ARGUMENT(int32,Lines)
        SLATE_ARGUMENT(int32,FontSize)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual void Tick(const FGeometry&,double,float) override;
private:
    TWeakObjectPtr<UCityDistrictGameplayComponent> District;
    TArray<FString> VisibleLines;
    FString Previous;
    int32 Count=5;
    FText Line(int32 Index) const;
    FSlateColor Colour(int32 Index) const;
};
