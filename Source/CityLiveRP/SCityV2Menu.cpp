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

void SCityV2Menu::Construct(const FArguments& Args)
{
    SetForegroundColor(TAttribute<FSlateColor>(FSlateColor(FLinearColor::White)));
    Controller=Args._Controller;bLoginLayout=!Args._District&&!Args._Garage;
    BackgroundImage.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/CityLiveUI/T_LoginCity.T_LoginCity")));
    if(!BackgroundImage.Get())BackgroundImage.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/CityLiveUI/T_LoadingCity.T_LoadingCity")));
    BackgroundBrush.SetResourceObject(BackgroundImage.Get());BackgroundBrush.ImageSize=FVector2D(1600,900);
    ActionStyle=CityUi::Button(true);SecondaryStyle=CityUi::Button();RowStyle=SecondaryStyle;KeyStyle=SecondaryStyle;
    ScrollStyle=FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar");
    ScrollStyle.SetNormalThumbImage(FSlateRoundedBoxBrush(CityUi::Orange(),4));
    ScrollStyle.SetHoveredThumbImage(FSlateRoundedBoxBrush(CityUi::Orange(),4));
    ScrollStyle.SetDraggedThumbImage(FSlateRoundedBoxBrush(CityUi::Orange(),4));
    RowStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White,10));RowStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.85f,.85f,.85f),10));
    KeyStyle.SetNormalPadding(FMargin(3,10));KeyStyle.SetPressedPadding(FMargin(3,12,3,8));
    FrameBrush=FSlateRoundedBoxBrush(CityUi::Panel(),18);
    InputStyle=FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    InputStyle.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(.027f,.029f,.035f),10,FLinearColor(.15f,.16f,.18f),1));
    InputStyle.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(.04f,.042f,.05f),10));
    InputStyle.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(.04f,.03f,.024f),10,CityUi::Orange(),1));
    InputStyle.SetForegroundColor(FLinearColor::White);InputStyle.SetPadding(FMargin(20,18));
    if(Args._District){if(Controller->DistrictGameplay->Context==TEXT("Chat"))ChatOverlay();else DistrictPage();}
    else if(Args._Garage)GaragePage();else IdentityPage();
}
void SCityV2Menu::BuildFrame(int32 Mode)
{
    bChatLayout=false;Fields=SNew(SVerticalBox);Actions=Fields;
    const bool Identity=Mode==0,District=Mode==3;
    TSharedRef<SVerticalBox> Dock=SNew(SVerticalBox);if(Identity)Actions=Dock;
    const float Width=Identity?390:District?540:460;
    TSharedRef<SWidget> Panel=SNew(SBox).WidthOverride(Width).HeightOverride(Identity?330:District?650:Mode==1?630:570)
        [SNew(SBorder).BorderImage(&FrameBrush).Padding(26)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                [SNew(STextBlock).Text(FText::FromString(District?(Controller->DistrictGameplay->Context.StartsWith(TEXT("Gps"))?TEXT("CITYLIVE RP  |  GPS"):TEXT("CITYLIVE RP  |  ГОРОД")):TEXT("CITYLIVE RP"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(CityUi::Orange())]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)
                [SNew(STextBlock).Text_Lambda([this](){return Title;}).Font(FCoreStyle::GetDefaultFontStyle("Bold",29)).AutoWrapText(true)]
                +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox).ScrollBarStyle(&ScrollStyle)+SScrollBox::Slot()[Fields.ToSharedRef()]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
                [SNew(STextBlock).Text_Lambda([this](){return Message;}).Font(FCoreStyle::GetDefaultFontStyle("Regular",17)).AutoWrapText(true).ColorAndOpacity(FLinearColor(1,.43f,.15f))]
            ]];
    ChildSlot[SNew(SOverlay)
        +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundBrush).Visibility(bLoginLayout?EVisibility::HitTestInvisible:EVisibility::Collapsed)]]
        +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.002f,.003f,.005f,bLoginLayout?.20f:.70f))]
        +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1280).HeightOverride(720)
            [SNew(SOverlay)
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(48,28)
                [SNew(STextBlock).Visibility(District?EVisibility::Collapsed:EVisibility::HitTestInvisible).Text(FText::FromString(TEXT("CITYLIVE  RP"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",25)).ColorAndOpacity(CityUi::Orange())]
                +SOverlay::Slot().HAlign(District?HAlign_Center:Identity?HAlign_Left:HAlign_Right).VAlign(VAlign_Center).Padding(48,22)[Panel]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,48,44)
                [SNew(SBox).WidthOverride(280).Visibility(Identity?EVisibility::Visible:EVisibility::Collapsed)[Dock]]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(54,130,0,0)
                [SNew(SBox).WidthOverride(390).Visibility(!District&&!Identity?EVisibility::HitTestInvisible:EVisibility::Collapsed)
                    [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(LoginName);}).Font(FCoreStyle::GetDefaultFontStyle("Bold",27))]]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(48,0,0,26)
                [SNew(STextBlock).Visibility(District?EVisibility::Collapsed:EVisibility::HitTestInvisible).Text(FText::FromString(TEXT("ПОЛИЦЕЙСКИЙ РАЙОН"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(CityUi::Muted())]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,24,28,0)
                [SNew(SButton).Visibility(District?EVisibility::Visible:EVisibility::Collapsed).ButtonStyle(&SecondaryStyle)
                    .OnClicked_Lambda([this](){Controller->CloseCityMenu();return FReply::Handled();})
                    [SNew(STextBlock).Text(FText::FromString(TEXT("×"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",25))]]
            ]]]];
}
void SCityV2Menu::Button(const FText& Text,TFunction<FReply()> Action,bool bPrimary)
{
    Actions->AddSlot().AutoHeight().Padding(0,5)
        [SNew(SButton).ButtonStyle(bPrimary?&ActionStyle:&SecondaryStyle).HAlign(HAlign_Center).ForegroundColor(FLinearColor::White)
            .OnClicked_Lambda([Action=MoveTemp(Action)](){return Action();})
            [SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Bold",22))]];
}
void SCityV2Menu::IdentityPage()
{
    const FString SavedFirst=First?First->GetText().ToString():Controller->RememberedFirst;
    const FString SavedLast=Last?Last->GetText().ToString():Controller->RememberedLast;
    if(Password)Password->SetText(FText::GetEmpty());if(Confirmation)Confirmation->SetText(FText::GetEmpty());
    Title=FText::FromString(TEXT("Вход в город"));Message=FText::GetEmpty();BuildFrame(0);
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(First,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT("Имя")))
            .Text(FText::FromString(SavedFirst)).Font(FCoreStyle::GetDefaultFontStyle("Regular",25))];
    Fields->AddSlot().AutoHeight().Padding(0,8)
        [SAssignNew(Last,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT("Фамилия")))
            .Text(FText::FromString(SavedLast)).Font(FCoreStyle::GetDefaultFontStyle("Regular",25))];
    Button(FText::FromString(TEXT("ДАЛЕЕ  →")),[this](){
        if(Controller.IsValid()&&Controller->SelectIdentity(First->GetText().ToString(),Last->GetText().ToString(),bNewProfile,Message))
        {LoginName=First->GetText().ToString().TrimStartAndEnd()+TEXT("_")+Last->GetText().ToString().TrimStartAndEnd();PasswordPage();}
        return FReply::Handled();},true);
}
void SCityV2Menu::PasswordPage()
{
    Title=FText::FromString(bNewProfile?TEXT("Регистрация"):TEXT("Авторизация"));Message=FText::GetEmpty();bPasswordVisible=false;BuildFrame(1);
    Fields->AddSlot().AutoHeight().Padding(0,0,0,18)
        [SNew(STextBlock).Text(FText::FromString(bNewProfile?TEXT("Придумайте пароль для своего персонажа."):TEXT("С возвращением в CityLive RP.")))
            .AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",19)).ColorAndOpacity(CityUi::Muted())];
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(Password,SEditableTextBox).Style(&InputStyle).IsPassword_Lambda([this](){return !bPasswordVisible;})
            .HintText(FText::FromString(TEXT("Пароль"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",25))];
    if(bNewProfile)
    {
        Fields->AddSlot().AutoHeight().Padding(0,5)
            [SAssignNew(Confirmation,SEditableTextBox).Style(&InputStyle).IsPassword_Lambda([this](){return !bPasswordVisible;})
                .HintText(FText::FromString(TEXT("Повторите пароль"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",25))];
        Fields->AddSlot().AutoHeight().Padding(0,12)
            [SNew(STextBlock).Text(FText::FromString(TEXT("От 6 до 64 символов. Регистр учитывается.\nНачальный баланс — 20 000 ₽.")))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular",17)).AutoWrapText(true).ColorAndOpacity(CityUi::Muted())];
    }
    Button(FText::FromString(TEXT("Показать / скрыть пароль")),[this](){bPasswordVisible=!bPasswordVisible;return FReply::Handled();});
    Button(FText::FromString(bNewProfile?TEXT("ЗАРЕГИСТРИРОВАТЬСЯ"):TEXT("ВОЙТИ")),[this](){
        const FString Secret=Password->GetText().ToString();
        if(bNewProfile&&Secret!=Confirmation->GetText().ToString()){Message=FText::FromString(TEXT("Пароли не совпадают."));return FReply::Handled();}
        if(Controller.IsValid()&&Controller->Authenticate(Secret,Message))
        {Password->SetText(FText::GetEmpty());if(Confirmation)Confirmation->SetText(FText::GetEmpty());SpawnPage();}
        return FReply::Handled();},true);
    Button(FText::FromString(TEXT("← Другое имя")),[this](){IdentityPage();return FReply::Handled();});
}
void SCityV2Menu::SpawnPage()
{
    Title=FText::FromString(TEXT("Место появления"));Message=FText::GetEmpty();bLastSpawn=false;BuildFrame(2);
    for(bool LastPlace:{false,true})
    {
        Fields->AddSlot().AutoHeight().Padding(0,8)
            [SNew(SButton).ButtonStyle(&RowStyle).HAlign(HAlign_Left).IsEnabled(!LastPlace||Controller->HasSavedPosition())
                .ButtonColorAndOpacity_Lambda([this,LastPlace](){return bLastSpawn==LastPlace?FLinearColor(.32f,.065f,.008f):FLinearColor(.026f,.029f,.038f);})
                .OnClicked_Lambda([this,LastPlace](){bLastSpawn=LastPlace;return FReply::Handled();})
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(LastPlace?TEXT("Последний выход"):TEXT("Начальное место"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",23))]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,6)
                    [SNew(STextBlock).Text(FText::FromString(LastPlace?(Controller->HasSavedPosition()?TEXT("Продолжить с сохранённой позиции"):TEXT("Пока нет сохранённого места")):TEXT("Появиться в стартовой точке города"))).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(CityUi::Muted())]
                ]];
    }
    Button(FText::FromString(TEXT("ДАЛЕЕ  →")),[this](){Controller->EnterCity(bLastSpawn,Message);return FReply::Handled();},true);
}
void SCityV2Menu::GaragePage()
{
    Title=FText::FromString(TEXT("Парковка"));Message=FText::GetEmpty();BuildFrame(3);
    Button(FText::FromString(TEXT("Седан · бесплатно")),[this](){bCarSelected=true;Message=FText::FromString(TEXT("Выбран седан."));return FReply::Handled();});
    Button(FText::FromString(TEXT("ВЫЗВАТЬ АВТОМОБИЛЬ")),[this](){if(!bCarSelected)Message=FText::FromString(TEXT("Сначала выберите автомобиль."));else Controller->SpawnParkingCar(Message);return FReply::Handled();},true);
    Button(FText::FromString(TEXT("Закрыть")),[this](){Controller->CloseCityMenu();return FReply::Handled();});
}
