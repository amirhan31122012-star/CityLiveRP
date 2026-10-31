#include "SCityV2Menu.h"
#include "CityLiveRPPlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SCityV2Menu::Construct(const FArguments& Args)
{
    Controller=Args._Controller;
    ChildSlot
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(.008f,.015f,.03f,.95f)).Padding(24)
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(460)
            [SNew(SScrollBox)
                +SScrollBox::Slot()
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                    [SNew(STextBlock).Text(FText::FromString(TEXT("CITYLIVE RP · V2")))
                        .ColorAndOpacity(FLinearColor(.35f,.8f,1.f))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold",28))]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)
                    [SNew(STextBlock).Text_Lambda([this](){return Title;})
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular",20))]
                    +SVerticalBox::Slot().AutoHeight()[SAssignNew(Fields,SVerticalBox)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,12)
                    [SNew(STextBlock).Text_Lambda([this](){return Message;})
                        .AutoWrapText(true).ColorAndOpacity(FLinearColor(1.f,.68f,.35f))]]]]];
    if (Args._Garage) GaragePage(); else IdentityPage();
}
void SCityV2Menu::Button(const FText& Text,TFunction<FReply()> Action)
{
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(18,13))
            .OnClicked_Lambda([Action=MoveTemp(Action)](){return Action();})
            [SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))]];
}
void SCityV2Menu::IdentityPage()
{
    Title=FText::FromString(TEXT("Вход в город")); Message=FText::GetEmpty();
    Fields->ClearChildren();
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(First,SEditableTextBox).HintText(FText::FromString(TEXT("Имя латиницей — Ivan")))
            .Text(FText::FromString(Controller->RememberedFirst))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",20)).MinDesiredWidth(320)];
    Fields->AddSlot().AutoHeight().Padding(0,5)
        [SAssignNew(Last,SEditableTextBox).HintText(FText::FromString(TEXT("Фамилия латиницей — Ivanov")))
            .Text(FText::FromString(Controller->RememberedLast))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",20))];
    Fields->AddSlot().AutoHeight().Padding(0,8)
        [SNew(STextBlock).Text(FText::FromString(TEXT("Профиль сохраняется на этом устройстве."))).AutoWrapText(true)];
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
        [SAssignNew(Password,SEditableTextBox).IsPassword(true)
            .HintText(FText::FromString(TEXT("Пароль — от 6 до 64 символов")))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular",20))];
    if (bNewProfile)
    {
        Fields->AddSlot().AutoHeight().Padding(0,5)
            [SAssignNew(Confirmation,SEditableTextBox).IsPassword(true)
                .HintText(FText::FromString(TEXT("Повторите пароль")))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular",20))];
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
