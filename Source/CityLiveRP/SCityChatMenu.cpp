#include "SCityV2Menu.h"
#include "SCityMap.h"
#include "SCityChatLog.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "CityChatInput.h"
#include "CityUiTheme.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

void SCityV2Menu::FocusChatInput(){FSlateApplication::Get().SetKeyboardFocus(SharedThis(this),EFocusCause::SetDirectly);}
void SCityV2Menu::TypeChat(const FString& Value)
{
    const int32 Count=static_cast<int32>(CityChatInput::Fit(*Value,Value.Len(),FMath::Max(0,180-ChatDraft.Len())));
    ChatDraft+=Value.Left(Count);Controller->DistrictGameplay->ChatUnsentDraft=ChatDraft;
}
void SCityV2Menu::BackspaceChat()
{
    ChatDraft.LeftChopInline(static_cast<int32>(CityChatInput::EraseCount(*ChatDraft,ChatDraft.Len())));
    Controller->DistrictGameplay->ChatUnsentDraft=ChatDraft;
}
void SCityV2Menu::SendChat()
{
    auto* Player=Controller.Get();if(!Player)return;auto* District=Player->DistrictGameplay.Get();
    const FString Text=ChatDraft.TrimStartAndEnd();
    if(Text.IsEmpty()){Player->CloseCityMenu();return;}
    District->SentMessages.Add(Text);if(District->SentMessages.Num()>30)District->SentMessages.RemoveAt(0);
    FString Submitted=Text;
    if(!Text.StartsWith(TEXT("/")))
    {if(ChatChannel==1)Submitted=TEXT("/b ")+Text;else if(ChatChannel==2)Submitted=TEXT("/s ")+Text;else if(ChatChannel==3)Submitted=TEXT("/me ")+Text;}
    const FText Response=District->ExecuteChat(Submitted);
    if(District->Context==TEXT("Chat")&&Response.ToString()!=District->ChatHistory)
        District->ChatHistory=(District->ChatHistory+TEXT("\n")+Response.ToString()).Right(1200);
    District->ChatUnsentDraft.Empty();
    const bool OpenNext=District->Context!=TEXT("Chat");Player->CloseCityMenu();if(OpenNext)District->OpenCurrentMenu();
}
FReply SCityV2Menu::OnKeyChar(const FGeometry& G,const FCharacterEvent& E)
{
    if(!bChatLayout)return SCompoundWidget::OnKeyChar(G,E);
    const TCHAR C=E.GetCharacter();if(C>=32&&C!=127&&!E.IsControlDown())TypeChat(FString::Chr(C));return FReply::Handled();
}
FReply SCityV2Menu::OnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    if(E.GetKey()==EKeys::Escape&&!bLoginLayout){Controller->CloseCityMenu();return FReply::Handled();}
    if(!bChatLayout)return SCompoundWidget::OnKeyDown(G,E);
    if(E.GetKey()==EKeys::BackSpace){BackspaceChat();return FReply::Handled();}
    if(E.GetKey()==EKeys::Enter){SendChat();return FReply::Handled();}
    return SCompoundWidget::OnKeyDown(G,E);
}
void SCityV2Menu::ChatOverlay()
{
    bChatLayout=true;auto* District=Controller->DistrictGameplay.Get();ChatDraft=District->ChatUnsentDraft;
    ChatChannel=District->ChatChannel;HistoryIndex=District->SentMessages.Num();
    ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride_Lambda([this](){return FOptionalSize(CityUi::CanvasWidth(GetCachedGeometry().GetLocalSize()));}).HeightOverride(720)
        [SNew(SOverlay)
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,18,22,0)
            [SNew(SBox).WidthOverride(218).HeightOverride(218).Visibility(EVisibility::HitTestInvisible)[SNew(SCityMap).Controller(Controller.Get()).Mini(true)]]
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(26,22,0,0)
            [SNew(SBox).WidthOverride(850).HeightOverride(218).Clipping(EWidgetClipping::ClipToBounds)
                [SNew(SBorder).BorderImage(&FrameBrush).Padding(14)
                    [SNew(SCityChatLog).District(District).Lines(8).FontSize(18)]]]
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,244,32,0)
            [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(CityUi::Money(Controller->GetBalance(),false));})
                .Font(FCoreStyle::GetDefaultFontStyle("BoldItalic",30)).ShadowOffset(FVector2D(2,2)).ShadowColorAndOpacity(FLinearColor(0,0,0,.8f))]
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(900,22,0,0)
            [SNew(SButton).IsFocusable(false).ButtonStyle(&SecondaryStyle).OnClicked_Lambda([this](){Controller->CloseCityMenu();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("×"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",27))]]
            +SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(16,0,300,418)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).IsFocusable(false).ButtonStyle(&RowStyle).ButtonColorAndOpacity_Lambda([this](){return ChatChannel==0?CityUi::Orange():CityUi::Panel();}).OnClicked_Lambda([this,District](){ChatChannel=District->ChatChannel=0;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("RP чат"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",16))]]
                    +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).IsFocusable(false).ButtonStyle(&RowStyle).ButtonColorAndOpacity_Lambda([this](){return ChatChannel==1?CityUi::Orange():CityUi::Panel();}).OnClicked_Lambda([this,District](){ChatChannel=District->ChatChannel=1;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("NRP чат"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",16))]]
                    +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).IsFocusable(false).ButtonStyle(&RowStyle).ButtonColorAndOpacity_Lambda([this](){return ChatChannel==2?CityUi::Orange():CityUi::Panel();}).OnClicked_Lambda([this,District](){ChatChannel=District->ChatChannel=2;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("Крик"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",16))]]
                    +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).IsFocusable(false).ButtonStyle(&RowStyle).ButtonColorAndOpacity_Lambda([this](){return ChatChannel==3?CityUi::Orange():CityUi::Panel();}).OnClicked_Lambda([this,District](){ChatChannel=District->ChatChannel=3;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("Действие"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",16))]]]
                +SHorizontalBox::Slot().AutoWidth().Padding(4,0)[SNew(SButton).IsFocusable(false).ButtonStyle(&SecondaryStyle).OnClicked_Lambda([this,District](){if(HistoryIndex>0)ChatDraft=District->SentMessages[--HistoryIndex];District->ChatUnsentDraft=ChatDraft;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("↑"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))]]
                +SHorizontalBox::Slot().AutoWidth().Padding(4,0)[SNew(SButton).IsFocusable(false).ButtonStyle(&SecondaryStyle).OnClicked_Lambda([this,District](){HistoryIndex=FMath::Min(HistoryIndex+1,District->SentMessages.Num());ChatDraft=HistoryIndex<District->SentMessages.Num()?District->SentMessages[HistoryIndex]:FString();District->ChatUnsentDraft=ChatDraft;return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("↓"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))]]]
            +SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(12,0,12,340)
            [SNew(SBox).HeightOverride(66)[SNew(SBorder).BorderImage(&FrameBrush).Padding(FMargin(20,14))
                [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(ChatDraft+TEXT("│"));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",27))]]]
            +SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(0,0,0,0)
            [SNew(SBox).HeightOverride(330)[SNew(SBorder).BorderImage(&FrameBrush).Padding(10)[SAssignNew(KeyboardHost,SVerticalBox)]]]
        ]]];
    KeyboardPage();
}
void SCityV2Menu::KeyboardPage()
{
    KeyboardHost->ClearChildren();
    const TArray<FString> Rows=bSymbols?TArray<FString>{TEXT("1234567890"),TEXT("@#₽%&*()-+"),TEXT("_=:;\"'?!.")}:bRussian?
        TArray<FString>{TEXT("йцукенгшщзхъ"),TEXT("фывапролджэ"),TEXT("ячсмитьбю")}:
        TArray<FString>{TEXT("qwertyuiop"),TEXT("asdfghjkl"),TEXT("zxcvbnm")};
    auto Special=[&](TSharedRef<SHorizontalBox> Row,const FString& Label,TFunction<void()> Action,float Weight=1.f,bool Primary=false)
    {Row->AddSlot().FillWidth(Weight).Padding(3,3)[SNew(SButton).IsFocusable(false).ButtonStyle(Primary?&ActionStyle:&KeyStyle).HAlign(HAlign_Center).VAlign(VAlign_Center)
        .OnClicked_Lambda([Action=MoveTemp(Action)](){Action();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(Label)).Font(FCoreStyle::GetDefaultFontStyle("Bold",22))]];};
    for(int32 Index=0;Index<3;++Index)
    {
        TSharedRef<SHorizontalBox> Row=SNew(SHorizontalBox);KeyboardHost->AddSlot().FillHeight(1)[Row];
        if(Index==2)Special(Row,bShift?TEXT("⇧ ON"):TEXT("⇧"),[this](){bShift=!bShift;KeyboardPage();});
        for(int32 Letter=0;Letter<Rows[Index].Len();++Letter)
        {const FString Value=bShift?Rows[Index].Mid(Letter,1).ToUpper():Rows[Index].Mid(Letter,1);
            Special(Row,Value,[this,Value](){TypeChat(Value);if(bShift&&!bSymbols){bShift=false;KeyboardPage();}});}
        if(Index==2)Special(Row,TEXT("⌫"),[this](){BackspaceChat();});
    }
    TSharedRef<SHorizontalBox> Bottom=SNew(SHorizontalBox);KeyboardHost->AddSlot().FillHeight(1)[Bottom];
    Special(Bottom,TEXT("/"),[this](){TypeChat(TEXT("/"));});
    Special(Bottom,bRussian?TEXT("RU"):TEXT("EN"),[this](){bRussian=!bRussian;bSymbols=false;KeyboardPage();});
    Special(Bottom,bSymbols?TEXT("ABC"):TEXT("123"),[this](){bSymbols=!bSymbols;KeyboardPage();});
    Special(Bottom,TEXT("Пробел"),[this](){TypeChat(TEXT(" "));},5);
    Special(Bottom,TEXT(","),[this](){TypeChat(TEXT(","));});Special(Bottom,TEXT("."),[this](){TypeChat(TEXT("."));});
    Special(Bottom,TEXT("→"),[this](){SendChat();},1.6f,true);
}
