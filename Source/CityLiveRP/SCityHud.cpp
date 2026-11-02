#include "SCityHud.h"
#include "SCityMap.h"
#include "SCityChatLog.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "CitySedan.h"
#include "CityUiTheme.h"
#include "SCityBalance.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Rendering/DrawElements.h"

void SCityHud::Construct(const FArguments& Args)
{
    SetForegroundColor(TAttribute<FSlateColor>(FSlateColor(FLinearColor::White)));
    Controller=Args._Controller;SmallStyle=CityUi::Button();SmallStyle.SetNormalPadding(FMargin(16,9));SmallStyle.SetPressedPadding(FMargin(16,11,16,7));
    ChatStyle=SmallStyle;ChatStyle.SetNormalPadding(FMargin(12));ChatStyle.SetPressedPadding(FMargin(12));
    ChatStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.008f,.009f,.014f,.42f),10));
    ChildSlot[SNew(SScaleBox).Visibility(EVisibility::SelfHitTestInvisible).Stretch(EStretch::ScaleToFit)
        [SNew(SBox).Visibility(EVisibility::SelfHitTestInvisible).WidthOverride_Lambda([this](){return FOptionalSize(CityUi::CanvasWidth(GetCachedGeometry().GetLocalSize()));}).HeightOverride(720)
            [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(0,0,0,0)
                [SNew(SBox).WidthOverride(218).HeightOverride(218)[SNew(SCityMap).Controller(Controller.Get()).Mini(true)]]
                +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0,18,0,0)
                [SNew(SBox).WidthOverride(740).HeightOverride(218).Clipping(EWidgetClipping::ClipToBounds)
                    [SNew(SButton).ButtonStyle(&ChatStyle).HAlign(HAlign_Left).VAlign(VAlign_Top)
                        .OnClicked_Lambda([this](){Controller->DistrictGameplay->OpenChat();return FReply::Handled();})
                        [SNew(SCityChatLog).District(Controller->DistrictGameplay.Get()).Lines(8).FontSize(18)]]]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,32,28,0)
                [SNew(SCityBalance).Controller(Controller.Get())]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,110,30,0)
                [SNew(SBox).WidthOverride(225)[SNew(STextBlock).Visibility(EVisibility::HitTestInvisible).AutoWrapText(true).ColorAndOpacity(CityUi::Orange()).Font(FCoreStyle::GetDefaultFontStyle("Bold",16))
                    .Text_Lambda([this](){return FText::FromString(Controller->DistrictGameplay->NavigationText());})]]
                +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0,0,0,24)
                [SNew(SBorder).Visibility_Lambda([this](){return Controller->IsSedanDriver()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
                    .Padding(FMargin(24,8)).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(CityUi::Panel())
                    [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",30)).ColorAndOpacity(CityUi::Orange())
                        .Text_Lambda([this](){const auto* Car=Controller->GetSeatedSedan();return FText::FromString(FString::Printf(TEXT("%03d  км/ч"),Car?FMath::RoundToInt(Car->GetSpeedKmh()):0));})]]
            ]]];
}
int32 SCityHud::OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
    const int32 Last=SCompoundWidget::OnPaint(Args,G,Clip,Out,Layer,Style,Enabled);
    if(!Controller.IsValid()||Controller->DistrictGameplay->IsInside())return Last;
    FVector Target;FString Name;if(!Controller->DistrictGameplay->NavigationTarget(Target,Name))return Last;
    FVector2D Screen;if(!Controller->ProjectWorldLocationToScreen(Target+FVector(0,0,180),Screen))return Last;
    int32 Width=0,Height=0;Controller->GetViewportSize(Width,Height);if(!Width||!Height)return Last;
    const auto Size=G.GetLocalSize();Screen=FVector2D(Screen.X/Width*Size.X,Screen.Y/Height*Size.Y);
    if(Screen.X<30||Screen.X>Size.X-30||Screen.Y<Size.Y*.24||Screen.Y>Size.Y*.84)return Last;
    const FSlateRoundedBoxBrush Pin(FLinearColor::White,10);
    FSlateDrawElement::MakeBox(Out,Last+1,G.ToPaintGeometry(FVector2D(20,20),FSlateLayoutTransform(Screen-FVector2D(10,10))),&Pin,ESlateDrawEffect::None,FLinearColor(1,.045f,.01f));
    const FString Metres=FString::Printf(TEXT("%.0f м"),FVector::Dist2D(Controller->DistrictGameplay->MapPosition(),Target)/100);
    FSlateDrawElement::MakeText(Out,Last+1,G.ToPaintGeometry(FVector2D(120,26),FSlateLayoutTransform(Screen+FVector2D(-25,16))),Metres,FCoreStyle::GetDefaultFontStyle("Bold",17),ESlateDrawEffect::None,FLinearColor::White);
    return Last+1;
}
