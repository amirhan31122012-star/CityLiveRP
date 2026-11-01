#include "SCityChatLog.h"
#include "CityDistrictGameplayComponent.h"
#include "CityUiTheme.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
void SCityChatLog::Construct(const FArguments& Args)
{
    District=Args._District;Count=Args._Lines;TSharedRef<SVerticalBox> Rows=SNew(SVerticalBox);
    for(int32 I=0;I<Count;++I)Rows->AddSlot().AutoHeight().Padding(0,1)
        [SNew(STextBlock).Text_Lambda([this,I](){return Line(I);}).ColorAndOpacity_Lambda([this,I](){return Colour(I);})
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",Args._FontSize)).AutoWrapText(false)];
    ChildSlot[Rows];
}
void SCityChatLog::Tick(const FGeometry& G,double Time,float Delta)
{
    SCompoundWidget::Tick(G,Time,Delta);if(!District.IsValid()||Previous==District->ChatHistory)return;Previous=District->ChatHistory;
    TArray<FString> All;Previous.ParseIntoArrayLines(All);VisibleLines.Reset();
    for(int32 I=FMath::Max(0,All.Num()-Count);I<All.Num();++I)VisibleLines.Add(All[I]);
}
FText SCityChatLog::Line(int32 Index) const
{
    if(!VisibleLines.IsValidIndex(Index))return FText::GetEmpty();const auto& Value=VisibleLines[Index];
    return FText::FromString(Value.Len()>100?Value.Left(99)+TEXT("…"):Value);
}
FSlateColor SCityChatLog::Colour(int32 Index) const
{
    if(!VisibleLines.IsValidIndex(Index))return FSlateColor(FLinearColor::White);const auto& Value=VisibleLines[Index];
    if(Value.StartsWith(TEXT("* ")))return FSlateColor(FLinearColor(.75f,.35f,1));
    if(Value.StartsWith(TEXT("((")))return FSlateColor(CityUi::Muted());
    if(Value.Contains(TEXT(" кричит:")))return FSlateColor(FLinearColor(1,.8f,.18f));
    return FSlateColor(Value.Contains(TEXT(": "))?FLinearColor::White:CityUi::Orange());
}
