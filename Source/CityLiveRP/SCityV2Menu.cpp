#include "SCityV2Menu.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "CityUiTheme.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

namespace
{
// Draw the eye ourselves: the mobile font need not contain an icon glyph.
class SCityPasswordEye : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCityPasswordEye){} SLATE_END_ARGS()
    void Construct(const FArguments&){}
    virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(28,24);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
    {
        const auto C=G.GetLocalSize()*.5;TArray<FVector2D> Path;
        for(int I=0;I<=32;++I){const float A=I*2*PI/32;Path.Add(C+FVector2D(FMath::Cos(A)*12,FMath::Sin(A)*7));}
        FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Path,ESlateDrawEffect::None,FLinearColor::White,true,2);
        const FSlateRoundedBoxBrush Dot(FLinearColor::White,4);
        FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2D(8,8),FSlateLayoutTransform(C-FVector2D(4,4))),&Dot);
        return Layer;
    }
};
}
void SCityV2Menu::Construct(const FArguments& Args)
{
    SetForegroundColor(TAttribute<FSlateColor>(FSlateColor(FLinearColor::White)));
    Controller=Args._Controller;bLoginLayout=!Args._District&&!Args._Garage;
    BackgroundImage.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/CityLiveUI/T_LoginCity.T_LoginCity")));
    if(!BackgroundImage.Get())BackgroundImage.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/CityLiveUI/T_LoadingCity.T_LoadingCity")));
    AccountImage.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/CityLiveUI/T_AccountCity.T_AccountCity")));
    BackgroundBrush.SetResourceObject(BackgroundImage.Get());BackgroundBrush.ImageSize=FVector2D(1600,900);
    AccountBrush.SetResourceObject(AccountImage.Get()?AccountImage.Get():BackgroundImage.Get());AccountBrush.ImageSize=FVector2D(1600,900);
    const TCHAR* Images[]={TEXT("Organization"),TEXT("Start"),TEXT("Last"),TEXT("Parking"),TEXT("Home")};
    for(int32 I=0;I<5;++I)
    {
        const FString Path=FString::Printf(TEXT("/Game/CityLiveUI/T_Spawn%s.T_Spawn%s"),Images[I],Images[I]);
        SpawnImages[I].Reset(LoadObject<UTexture2D>(nullptr,*Path));
        SpawnBrushes[I].SetResourceObject(SpawnImages[I].Get()?SpawnImages[I].Get():BackgroundImage.Get());SpawnBrushes[I].ImageSize=FVector2D(400,640);
    }
    ActionStyle=CityUi::Button(true);SecondaryStyle=CityUi::Button();RowStyle=SecondaryStyle;KeyStyle=SecondaryStyle;
    ScrollStyle=FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar");
    ScrollStyle.SetNormalThumbImage(FSlateRoundedBoxBrush(CityUi::Red(),4));
    ScrollStyle.SetHoveredThumbImage(FSlateRoundedBoxBrush(CityUi::Red(),4));
    ScrollStyle.SetDraggedThumbImage(FSlateRoundedBoxBrush(CityUi::Red(),4));
    RowStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White,10));RowStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.85f,.85f,.85f),10));
    KeyStyle.SetNormalPadding(FMargin(3,10));KeyStyle.SetPressedPadding(FMargin(3,12,3,8));
    FrameBrush=FSlateRoundedBoxBrush(CityUi::Panel(),12);
    InputStyle=FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    InputStyle.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(.016f,.018f,.022f,.96f),9,FLinearColor(.34f,.35f,.38f),1));
    InputStyle.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(.025f,.028f,.034f),9,FLinearColor(.52f,.53f,.56f),1));
    InputStyle.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(.028f,.021f,.021f),9,CityUi::Red(),1));
    InputStyle.SetForegroundColor(FLinearColor::White);InputStyle.SetPadding(FMargin(22,17));
    if(Args._District){if(Controller->DistrictGameplay->Context==TEXT("Chat"))ChatOverlay();else DistrictPage();}
    else if(Args._Garage)GaragePage();else IdentityPage();
}
void SCityV2Menu::BuildFrame(int32 Mode)
{
    bChatLayout=false;Fields=SNew(SVerticalBox);Actions=Fields;
    const bool Identity=Mode==0,District=Mode==3,Spawn=Mode==2;
    TSharedRef<SVerticalBox> Dock=SNew(SVerticalBox);if(Identity||Spawn)Actions=Dock;
    TSharedRef<SWidget> Panel=Fields.ToSharedRef();
    if(District)
    {
        Panel=SNew(SBox).WidthOverride(560).HeightOverride(650)
            [SNew(SBorder).BorderImage(&FrameBrush).Padding(26)
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)
                    [SNew(STextBlock).Text_Lambda([this](){return Title;}).Font(FCoreStyle::GetDefaultFontStyle("Bold",29)).AutoWrapText(true)]
                    +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox).ScrollBarStyle(&ScrollStyle)+SScrollBox::Slot()[Fields.ToSharedRef()]]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
                    [SNew(STextBlock).Text_Lambda([this](){return Message;}).Font(FCoreStyle::GetDefaultFontStyle("Regular",18)).AutoWrapText(true).ColorAndOpacity(CityUi::Red())]
                ]];
    }
    else Panel=SNew(SBox).WidthOverride_Lambda([this,Identity,Spawn](){return FOptionalSize(Identity?430.f:Spawn?CityUi::CanvasWidth(GetCachedGeometry().GetLocalSize())-104.f:500.f);})[Fields.ToSharedRef()];
    ChildSlot[SNew(SOverlay)
        +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)
            [SNew(SImage).Image(Mode==1?&AccountBrush:&BackgroundBrush).Visibility(bLoginLayout?EVisibility::HitTestInvisible:EVisibility::Collapsed)]]
        +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.002f,.003f,.005f,District?.70f:Spawn?.88f:.12f))]
        +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride_Lambda([this](){return FOptionalSize(CityUi::CanvasWidth(GetCachedGeometry().GetLocalSize()));}).HeightOverride(720)
            [SNew(SOverlay)
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(52,40,0,0)
                [SNew(STextBlock).Visibility(Identity||District?EVisibility::Collapsed:EVisibility::HitTestInvisible).Text_Lambda([this](){return Title;}).Font(FCoreStyle::GetDefaultFontStyle("Bold",34))]
                +SOverlay::Slot().HAlign(District||Spawn?HAlign_Center:Identity?HAlign_Left:HAlign_Right).VAlign(VAlign_Center).Padding(52,Mode==1?40:0,Mode==1?82:52,0)[Panel]
                +SOverlay::Slot().HAlign(Spawn?HAlign_Center:HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,Spawn?0:52,44)
                [SNew(SBox).WidthOverride(Spawn?270:300).Visibility(Identity||Spawn?EVisibility::Visible:EVisibility::Collapsed)[Dock]]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,156,82,0)
                [SNew(SBox).WidthOverride(500).Visibility(Mode==1?EVisibility::HitTestInvisible:EVisibility::Collapsed)
                    [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(LoginName);}).Font(FCoreStyle::GetDefaultFontStyle("Bold",24))]]
                +SOverlay::Slot().HAlign(Identity?HAlign_Left:HAlign_Center).VAlign(VAlign_Bottom).Padding(52,0,52,Spawn?137:Identity?142:36)
                [SNew(SBox).WidthOverride(Spawn?1000:Identity?430:500).Visibility(District?EVisibility::Collapsed:EVisibility::HitTestInvisible)
                    [SNew(STextBlock).Text_Lambda([this](){return Message;}).Font(FCoreStyle::GetDefaultFontStyle("Regular",19)).AutoWrapText(true).ColorAndOpacity(FLinearColor(1,.24f,.15f))]]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,24,28,0)
                [SNew(SButton).Visibility(District?EVisibility::Visible:EVisibility::Collapsed).ButtonStyle(&SecondaryStyle)
                    .OnClicked_Lambda([this](){Controller->CloseCityMenu();return FReply::Handled();})
                    [SNew(STextBlock).Text(FText::FromString(TEXT("×"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",25))]]
            ]]]];
}
void SCityV2Menu::Button(const FText& Text,TFunction<FReply()> Action,bool bPrimary)
{
    Actions->AddSlot().AutoHeight().Padding(0,5)
        [SNew(SBox).MinDesiredHeight(bPrimary&&bLoginLayout?72:0)
            [SNew(SButton).ButtonStyle(bPrimary?&ActionStyle:&SecondaryStyle).HAlign(HAlign_Center).VAlign(VAlign_Center).ForegroundColor(FLinearColor::White)
                .OnClicked_Lambda([Action=MoveTemp(Action)](){return Action();})
                [SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Bold",bPrimary&&bLoginLayout?26:22))]]];
}
void SCityV2Menu::IdentityPage()
{
    const FString SavedFirst=First?First->GetText().ToString():Controller->RememberedFirst;
    const FString SavedLast=Last?Last->GetText().ToString():Controller->RememberedLast;
    if(Password)Password->SetText(FText::GetEmpty());if(Confirmation)Confirmation->SetText(FText::GetEmpty());
    Title=FText::GetEmpty();Message=FText::GetEmpty();BuildFrame(0);
    Fields->AddSlot().AutoHeight().Padding(0,0,0,16)
        [SNew(SBox).HeightOverride(76)[SAssignNew(First,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT("Введите имя")))
            .Text(FText::FromString(SavedFirst)).Font(FCoreStyle::GetDefaultFontStyle("Regular",27))]];
    Fields->AddSlot().AutoHeight()
        [SNew(SBox).HeightOverride(76)[SAssignNew(Last,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT("Введите фамилию")))
            .Text(FText::FromString(SavedLast)).Font(FCoreStyle::GetDefaultFontStyle("Regular",27))]];
    Button(FText::FromString(TEXT("ИГРАТЬ")),[this](){
        if(Controller.IsValid()&&Controller->SelectIdentity(First->GetText().ToString(),Last->GetText().ToString(),bNewProfile,Message))
        {LoginName=First->GetText().ToString().TrimStartAndEnd()+TEXT("_")+Last->GetText().ToString().TrimStartAndEnd();PasswordPage();}
        return FReply::Handled();},true);
}
void SCityV2Menu::PasswordPage()
{
    Title=FText::FromString(bNewProfile?TEXT("РЕГИСТРАЦИЯ"):TEXT("АВТОРИЗАЦИЯ"));Message=FText::GetEmpty();bPasswordVisible=false;BuildFrame(1);
    auto PasswordField=[this](TSharedPtr<SEditableTextBox>& Target,const TCHAR* Hint)
    {
        Fields->AddSlot().AutoHeight().Padding(0,0,0,18)
            [SNew(SBox).HeightOverride(76)[SNew(SOverlay)
                +SOverlay::Slot()[SAssignNew(Target,SEditableTextBox).Style(&InputStyle).IsPassword_Lambda([this](){return !bPasswordVisible;})
                    .Padding(FMargin(22,17,78,17)).HintText(FText::FromString(Hint)).Font(FCoreStyle::GetDefaultFontStyle("Regular",27))]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(10)
                    [SNew(SButton).ButtonStyle(&SecondaryStyle).ContentPadding(FMargin(9,8))
                        .OnClicked_Lambda([this](){bPasswordVisible=!bPasswordVisible;return FReply::Handled();})[SNew(SCityPasswordEye)]]
            ]];
    };
    PasswordField(Password,TEXT("Пароль"));
    if(bNewProfile)PasswordField(Confirmation,TEXT("Повторите пароль"));
    Button(FText::FromString(TEXT("ВОЙТИ")),[this](){
        const FString Secret=Password->GetText().ToString();
        if(bNewProfile&&Secret!=Confirmation->GetText().ToString()){Message=FText::FromString(TEXT("Пароли не совпадают."));return FReply::Handled();}
        if(Controller.IsValid()&&Controller->Authenticate(Secret,Message))
        {Password->SetText(FText::GetEmpty());if(Confirmation)Confirmation->SetText(FText::GetEmpty());SpawnPage();}
        return FReply::Handled();},true);
    Fields->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,12)
        [SNew(SButton).ButtonStyle(&SecondaryStyle).ContentPadding(FMargin(16,8)).OnClicked_Lambda([this](){IdentityPage();return FReply::Handled();})
            [SNew(STextBlock).Text(FText::FromString(TEXT("Назад"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",17))]];
}
void SCityV2Menu::SpawnPage()
{
    Title=FText::FromString(TEXT("Выберите место появления"));Message=FText::GetEmpty();SelectedSpawn=TEXT("Start");BuildFrame(2);
    const TCHAR* Ids[]={TEXT("Organization"),TEXT("Start"),TEXT("Last"),TEXT("Parking"),TEXT("Home")};
    const TCHAR* Names[]={TEXT("Организация"),TEXT("Площадь"),TEXT("Место выхода"),TEXT("Парковка"),TEXT("Дом")};
    TSharedRef<SHorizontalBox> Cards=SNew(SHorizontalBox);Fields->AddSlot().AutoHeight()[Cards];
    for(int32 I=0;I<5;++I)
    {
        const FName Place(Ids[I]);const bool Available=Controller->CanSpawnAt(Place);
        Cards->AddSlot().FillWidth(1).Padding(7,0)
            [SNew(SBox).HeightOverride(342)
                [SNew(SBorder).Padding(3).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor_Lambda([this,Place](){return SelectedSpawn==Place?CityUi::Red():FLinearColor(.16f,.17f,.19f);})
                    [SNew(SButton).ButtonStyle(&RowStyle).ContentPadding(0).IsEnabled(Available)
                        .OnClicked_Lambda([this,Place](){SelectedSpawn=Place;return FReply::Handled();})
                        [SNew(SOverlay)
                            +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&SpawnBrushes[I])]]
                            +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,Available?.48f:.82f))]
                            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                                [SNew(STextBlock).Text(FText::FromString(Names[I])).Font(FCoreStyle::GetDefaultFontStyle("Bold",23))]
                        ]]]];
    }
    Button(FText::FromString(TEXT("ВОЙТИ")),[this](){Controller->EnterCityAt(SelectedSpawn,Message);return FReply::Handled();},true);
}
void SCityV2Menu::GaragePage()
{
    Title=FText::FromString(TEXT("Парковка"));Message=FText::GetEmpty();BuildFrame(3);
    Button(FText::FromString(TEXT("Седан · бесплатно")),[this](){bCarSelected=true;Message=FText::FromString(TEXT("Выбран седан."));return FReply::Handled();});
    Button(FText::FromString(TEXT("ВЫЗВАТЬ АВТОМОБИЛЬ")),[this](){if(!bCarSelected)Message=FText::FromString(TEXT("Сначала выберите автомобиль."));else Controller->SpawnParkingCar(Message);return FReply::Handled();},true);
    Button(FText::FromString(TEXT("Закрыть")),[this](){Controller->CloseCityMenu();return FReply::Handled();});
}
