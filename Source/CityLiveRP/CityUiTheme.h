#pragma once
#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"
namespace CityUi
{
inline FLinearColor Orange(){return FLinearColor(1.f,.145f,.018f,1);}
inline FLinearColor Panel(){return FLinearColor(.012f,.014f,.019f,.97f);}
inline FLinearColor Muted(){return FLinearColor(.52f,.55f,.60f,1);}
inline FString Money(int32 Amount)
{
    const FString Digits=FString::FromInt(Amount);FString Result;
    for(int32 I=0;I<Digits.Len();++I){if(I>0&&(Digits.Len()-I)%3==0&&Digits[I-1]!=TEXT('-'))Result+=TEXT(" ");Result+=Digits[I];}
    return Result+TEXT(" ₽");
}
inline FButtonStyle Button(bool Primary=false)
{
    auto Style=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
    Style.SetNormal(FSlateRoundedBoxBrush(Primary?Orange():FLinearColor(.026f,.029f,.036f,1),12));
    Style.SetHovered(FSlateRoundedBoxBrush(Primary?FLinearColor(1,.24f,.04f):FLinearColor(.075f,.045f,.027f),12));
    Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.40f,.055f,.005f),12));
    Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.035f,.037f,.045f),12));
    Style.SetNormalForeground(FSlateColor(FLinearColor::White));Style.SetHoveredForeground(FSlateColor(FLinearColor::White));Style.SetPressedForeground(FSlateColor(FLinearColor::White));
    Style.SetNormalPadding(FMargin(22,15));Style.SetPressedPadding(FMargin(22,18,22,12));return Style;
}
}
