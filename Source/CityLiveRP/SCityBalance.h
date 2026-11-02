#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"
#include "CityLiveRPPlayerController.h"
#include "CityUiTheme.h"

class SCityBalance : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCityBalance){} SLATE_ARGUMENT(ACityLiveRPPlayerController*,Controller) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Controller=Args._Controller;
        ChildSlot[SNew(SBox).WidthOverride(148).HeightOverride(66)
            [SNew(SOverlay)
                +SOverlay::Slot().VAlign(VAlign_Top)
                [SNew(SBorder).BorderImage(&Plate).Padding(FMargin(14,9))
                    [SNew(STextBlock).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("BoldItalic",22))
                        .ColorAndOpacity(FLinearColor::White).Text_Lambda([this](){return FText::FromString(CityUi::Money(Controller.IsValid()?Controller->GetBalance():0,false));})]]]];
    }
    virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override
    {
        for(int I=0;I<18;++I)
        {
            const double Y=I/18.;const double Half=65*FMath::Sqrt(1-Y*Y);
            TArray<FVector2D> Line;Line.Add(FVector2D(74-Half,46+I));Line.Add(FVector2D(74+Half,46+I));
            FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Line,ESlateDrawEffect::None,FLinearColor(.8f,.012f,.004f,.38f),true,1.3f);
        }
        return SCompoundWidget::OnPaint(Args,G,Clip,Out,Layer+1,Style,Enabled);
    }
private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    FSlateRoundedBoxBrush Plate{FLinearColor(0,0,0,.58f),8};
};
