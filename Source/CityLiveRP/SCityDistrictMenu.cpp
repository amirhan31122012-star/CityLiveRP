#include "SCityV2Menu.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "CityDistrictMapData.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"

void SCityV2Menu::DistrictPage()
{
    auto* District=Controller->DistrictGameplay.Get();const FString Mode=District->Context;
    if(Mode==TEXT("Map")){MapPage();return;}
    if(Mode==TEXT("Chat")){ChatOverlay();return;}
    if(Mode.StartsWith(TEXT("Gps"))){GpsPage();return;}
    BuildFrame(3);Title=FText::FromString(TEXT("Город"));
    if(Mode==TEXT("Police"))Title=FText::FromString(TEXT("Дежурная часть"));
    else if(Mode==TEXT("Hospital"))Title=FText::FromString(TEXT("Регистратура"));
    else if(Mode==TEXT("Courier"))Title=FText::FromString(TEXT("Служба доставки"));
    if(Mode==TEXT("Residence"))
    {
        Title=FText::FromString(District->CurrentBuilding.ToString()+FString::Printf(TEXT(" · этаж %d · %d ₽"),District->CurrentFloor,District->ApartmentPrice()));
        for(const auto& Place:CityDistrictMapData::Places())if(District->CurrentBuilding==FName(UTF8_TO_TCHAR(Place.Id)))
            Title=FText::FromString(FString(UTF8_TO_TCHAR(Place.Label))+FString::Printf(TEXT(" · этаж %d · %d ₽"),District->CurrentFloor,District->ApartmentPrice()));
        for(int32 Floor=1;Floor<=District->MaxFloor();++Floor)
            Button(FText::FromString(FString::Printf(TEXT("Лифт → %d этаж"),Floor)),[this,District,Floor](){District->ChangeFloor(Floor,Message);DistrictPage();return FReply::Handled();});
        for(int32 Door=1;Door<=4;++Door)
        {
            Button(FText::FromString(FString::Printf(TEXT("Квартира %d · купить"),(District->CurrentFloor-1)*4+Door)),[this,District,Door](){
                if(District->BuyApartment(District->CurrentBuilding,District->CurrentFloor,Door,Message))Message=FText::FromString(TEXT("Квартира куплена."));return FReply::Handled();});
            Button(FText::FromString(FString::Printf(TEXT("Войти в квартиру %d"),(District->CurrentFloor-1)*4+Door)),[this,District,Door](){if(District->EnterApartment(Door,Message))Controller->CloseCityMenu();return FReply::Handled();});
        }
    }
    else if(Mode==TEXT("Police")||Mode==TEXT("Hospital"))
    {
        Fields->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Здравствуйте. Вы точно хотите устроиться? Потребуется пройти вводный опрос."))).Font(FCoreStyle::GetDefaultFontStyle("Regular",21)).AutoWrapText(true)];
        Button(FText::FromString(TEXT("Да, пройти опрос")),[this,Mode](){QuizPage(FName(*Mode));return FReply::Handled();});
        Button(FText::FromString(TEXT("Нет")),[this](){Controller->CloseCityMenu();return FReply::Handled();});
    }
    else if(Mode==TEXT("Courier"))
    {
        Button(FText::FromString(TEXT("Устроиться курьером")),[this,District](){if(District->SetCourierEmployment(true,Message))Message=FText::FromString(TEXT("Вы приняты. Возьмите рабочий скутер."));return FReply::Handled();});
        Button(FText::FromString(TEXT("Выдать рабочий скутер")),[this,District](){District->SpawnWorkScooter(Message);return FReply::Handled();});
        Button(FText::FromString(TEXT("Получить заказ · 5 000 ₽")),[this,District](){if(District->BeginDelivery(Message))Message=FText::FromString(TEXT("Адрес отмечен красным. Остановитесь у метки на рабочем скутере."));return FReply::Handled();});
        Button(FText::FromString(TEXT("Завершить смену")),[this,District](){District->SetCourierEmployment(false,Message);return FReply::Handled();});
    }
    Button(FText::FromString(TEXT("Закрыть")),[this](){Controller->CloseCityMenu();return FReply::Handled();});
}
void SCityV2Menu::QuizPage(FName Organization,int32 Question,int32 Score)
{
    struct FQuiz{const TCHAR* Prompt;const TCHAR* Good;const TCHAR* Bad;};
    static const FQuiz Police[]={
        {TEXT("Можно применять силу без законного основания?"),TEXT("Нет, сначала законное основание"),TEXT("Да, по желанию")},
        {TEXT("Первое действие при обращении гражданина?"),TEXT("Представиться и выслушать"),TEXT("Сразу задержать")},
        {TEXT("Доклад о происшествии должен быть…"),TEXT("Точным, без выдуманных сведений"),TEXT("Каким угодно")}};
    static const FQuiz Hospital[]={
        {TEXT("У пострадавшего нет сознания: сначала…"),TEXT("Убедиться в безопасности и вызвать помощь"),TEXT("Дать воду")},
        {TEXT("Можно разглашать сведения пациента?"),TEXT("Нет, нужна законная причина"),TEXT("Да, всем знакомым")},
        {TEXT("Нет подготовки для сложной процедуры: что делать?"),TEXT("Позвать квалифицированного сотрудника"),TEXT("Экспериментировать")}};
    if(Question>=3){Controller->DistrictGameplay->JoinOrganization(Organization,Score,Message);DistrictPage();return;}
    Fields->ClearChildren();Title=FText::FromString(FString::Printf(TEXT("Вводный опрос · %d/3"),Question+1));
    const FQuiz Item=Organization==TEXT("Police")?Police[Question]:Hospital[Question];
    Fields->AddSlot().AutoHeight()[SNew(SBorder).Padding(24).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.20f,.055f,.012f,1))
        [SNew(STextBlock).Text(FText::FromString(Item.Prompt)).Font(FCoreStyle::GetDefaultFontStyle("Bold",27)).AutoWrapText(true)]];
    const bool Swap=Question%2!=0;
    for(int32 Index=0;Index<2;++Index)
    {const bool Correct=(Index==0)!=Swap;Button(FText::FromString(Correct?Item.Good:Item.Bad),[this,Organization,Question,Score,Correct](){QuizPage(Organization,Question+1,Score+(Correct?1:0));return FReply::Handled();});}
}
