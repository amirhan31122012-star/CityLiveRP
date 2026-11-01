#include "SCityV2Menu.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"

void SCityV2Menu::Construct(const FArguments& Args)
{
    Controller=Args._Controller;
    bLoginLayout=!Args._District&&!Args._Garage;
    BackgroundImage.Reset(LoadObject<UTexture2D>(nullptr,TEXT("/Game/CityLiveUI/T_LoadingCity.T_LoadingCity")));
    BackgroundBrush.SetResourceObject(BackgroundImage.Get());BackgroundBrush.ImageSize=FVector2D(1280,720);
    ActionStyle=FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
    ActionStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(.015f,.38f,.47f,1),12));
    ActionStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(.02f,.55f,.64f,1),12));
    ActionStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.01f,.22f,.28f,1),12));
    ActionStyle.SetNormalPadding(FMargin(24,16));ActionStyle.SetPressedPadding(FMargin(24,20,24,12));
    InputStyle=FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    InputStyle.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(.035f,.055f,.09f,1),10));
    InputStyle.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(.06f,.09f,.14f,1),10));
    InputStyle.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(.04f,.16f,.21f,1),10));
    InputStyle.SetForegroundColor(FLinearColor::White);InputStyle.SetPadding(FMargin(22,18));
    if(Args._District && Controller->DistrictGameplay->Context==TEXT("Chat")){ChatOverlay();return;}
    ChildSlot[SNew(SOverlay)
        +SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundBrush).Visibility(bLoginLayout?EVisibility::HitTestInvisible:EVisibility::Collapsed)]]
        +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(.005f,.01f,.025f,bLoginLayout?.25f:.8f))]
        +SOverlay::Slot().HAlign(bLoginLayout?HAlign_Left:HAlign_Center).VAlign(bLoginLayout?VAlign_Top:VAlign_Center).Padding(48,28)
        [SNew(SBox).WidthOverride(bLoginLayout?500:670)
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(.008f,.02f,.04f,.88f)).Padding(28)
                [SNew(SScrollBox)+SScrollBox::Slot()
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)
                        [SNew(STextBlock).Text(FText::FromString(TEXT("CITYLIVE RP"))).ColorAndOpacity(FLinearColor(.2f,.88f,.95f)).Font(FCoreStyle::GetDefaultFontStyle("Bold",34))]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,22)
                        [SNew(STextBlock).Text_Lambda([this](){return Title;}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Bold",28))]
                        +SVerticalBox::Slot().AutoHeight()[SAssignNew(Fields,SVerticalBox)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,14)
                        [SNew(STextBlock).Text_Lambda([this](){return Message;}).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",26)).ColorAndOpacity(FLinearColor(1,.72f,.35f))]
                    ]]]]];
    if(Args._District) DistrictPage(); else if (Args._Garage) GaragePage(); else IdentityPage();
}
void SCityV2Menu::Button(const FText& Text,TFunction<FReply()> Action)
{
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SNew(SButton).ButtonStyle(&ActionStyle).HAlign(HAlign_Center).ForegroundColor(FLinearColor::White)
            .OnClicked_Lambda([Action=MoveTemp(Action)](){return Action();})
            [SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Bold",24))]];
}
void SCityV2Menu::IdentityPage()
{
    Title=FText::FromString(TEXT("Вход в город")); Message=FText::GetEmpty();
    Fields->ClearChildren();
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(First,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT("Имя")))
            .Text(FText::FromString(Controller->RememberedFirst))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",26)).MinDesiredWidth(320)];
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(Last,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT("Фамилия")))
            .Text(FText::FromString(Controller->RememberedLast))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",26))];
    Button(FText::FromString(TEXT("Играть")),[this](){
        if (Controller.IsValid() && Controller->SelectIdentity(First->GetText().ToString(),Last->GetText().ToString(),bNewProfile,Message))
            PasswordPage();
        return FReply::Handled();
    });
}
void SCityV2Menu::PasswordPage()
{
    Fields->ClearChildren(); Message=FText::GetEmpty();
    Title=FText::FromString(bNewProfile ? TEXT("Новый профиль — придумайте пароль") : TEXT("Введите пароль профиля"));
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(Password,SEditableTextBox).Style(&InputStyle).IsPassword(true)
            .HintText(FText::FromString(TEXT("Пароль")))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",26))];
    if (bNewProfile)
    {
        Fields->AddSlot().AutoHeight().Padding(0,5)
            [SAssignNew(Confirmation,SEditableTextBox).Style(&InputStyle).IsPassword(true)
                .HintText(FText::FromString(TEXT("Повторите пароль")))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular",26))];
        Fields->AddSlot().AutoHeight().Padding(0,8)
            [SNew(STextBlock).Text(FText::FromString(TEXT("Начальный баланс: 20 000 ₽")))];
    }
    Button(FText::FromString(bNewProfile ? TEXT("Создать профиль") : TEXT("Войти")),[this](){
        const FString Secret=Password->GetText().ToString();
        if (bNewProfile && Secret!=Confirmation->GetText().ToString())
        { Message=FText::FromString(TEXT("Пароли не совпадают.")); return FReply::Handled(); }
        if (Controller.IsValid() && Controller->Authenticate(Secret,Message))
        {
            Password->SetText(FText::GetEmpty());
            if (Confirmation) Confirmation->SetText(FText::GetEmpty());
            if (bNewProfile)
            {
                if (!Controller->EnterCity(false,Message))
                {
                    const FText SpawnError=Message;
                    SpawnPage(); Message=SpawnError;
                }
            }
            else SpawnPage();
        }
        return FReply::Handled();
    });
    Button(FText::FromString(TEXT("Назад")),[this](){IdentityPage();return FReply::Handled();});
}
void SCityV2Menu::SpawnPage()
{
    Fields->ClearChildren(); Message=FText::GetEmpty(); bLastSpawn=false;
    Title=FText::FromString(TEXT("Место появления"));
    Button(FText::FromString(TEXT("Начальное место спавна")),[this](){
        bLastSpawn=false; Message=FText::FromString(TEXT("Выбрано начальное место."));return FReply::Handled();});
    Button(FText::FromString(TEXT("Место последнего выхода")),[this](){
        if (!Controller->HasSavedPosition()) Message=FText::FromString(TEXT("Для этой карты ещё нет сохранённой позиции."));
        else {bLastSpawn=true;Message=FText::FromString(TEXT("Выбрано место последнего выхода."));}
        return FReply::Handled();});
    Button(FText::FromString(TEXT("Далее")),[this](){
        if(Controller.IsValid()) Controller->EnterCity(bLastSpawn,Message);
        return FReply::Handled();});
}
void SCityV2Menu::GaragePage()
{
    Fields->ClearChildren(); Message=FText::GetEmpty();
    Title=FText::FromString(TEXT("Парковка · вызов автомобиля"));
    Button(FText::FromString(TEXT("Седан · бесплатно")),[this](){
        bCarSelected=true;Message=FText::FromString(TEXT("Выбран седан. Персонаж подойдёт к двери и сядет за руль."));return FReply::Handled();});
    Button(FText::FromString(TEXT("Вызвать автомобиль")),[this](){
        if(!bCarSelected) Message=FText::FromString(TEXT("Сначала выберите автомобиль."));
        else if(Controller.IsValid()) Controller->SpawnParkingCar(Message);
        return FReply::Handled();});
    Button(FText::FromString(TEXT("Отмена")),[this](){
        if(Controller.IsValid()) Controller->CloseCityMenu();return FReply::Handled();});
}
