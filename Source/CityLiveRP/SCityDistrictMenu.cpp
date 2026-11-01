#include "SCityV2Menu.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
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
    Fields->ClearChildren();auto* District=Controller->DistrictGameplay.Get();Title=FText::FromString(TEXT("ГОРОД · ")+District->Context);
    const FString Mode=District->Context;
    if(Mode==TEXT("Police"))Title=FText::FromString(TEXT("Дежурная часть"));
    else if(Mode==TEXT("Hospital"))Title=FText::FromString(TEXT("Регистратура"));
    else if(Mode==TEXT("Courier"))Title=FText::FromString(TEXT("Служба доставки"));
    else if(Mode.StartsWith(TEXT("Gps")))Title=FText::FromString(TEXT("Навигация"));
    if(Mode==TEXT("Chat"))
    {
        Fields->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(District->ChatHistory)).AutoWrapText(true)];
        Fields->AddSlot().AutoHeight()[SAssignNew(First,SEditableTextBox).Style(&InputStyle).HintText(FText::FromString(TEXT(""))).Font(FCoreStyle::GetDefaultFontStyle("Regular",20))];
        Button(FText::FromString(TEXT("Отправить")),[this,District](){Message=District->ExecuteChat(First->GetText().ToString());First->SetText(FText::GetEmpty());if(District->Context!=TEXT("Chat"))DistrictPage();return FReply::Handled();});
        Button(FText::FromString(TEXT("GPS")),[this,District](){District->Context=TEXT("Gps");DistrictPage();return FReply::Handled();});
    }
    else if(Mode==TEXT("Gps"))
    {
        Button(FText::FromString(TEXT("Государственные учреждения")),[this,District](){District->Context=TEXT("GpsGovernment");DistrictPage();return FReply::Handled();});
        Button(FText::FromString(TEXT("Работы")),[this,District](){District->Context=TEXT("GpsJobs");DistrictPage();return FReply::Handled();});
        Button(FText::FromString(TEXT("Жилые дома")),[this,District](){District->Context=TEXT("GpsHomes");DistrictPage();return FReply::Handled();});
    }
    else if(Mode.StartsWith(TEXT("Gps")))
    {
        auto Destination=[this,District](const TCHAR* Label,const TCHAR* Id){
            Button(FText::FromString(Label),[this,District,Id](){District->SetGps(FName(Id));Controller->CloseCityMenu();return FReply::Handled();});};
        if(Mode==TEXT("GpsGovernment")){Destination(TEXT("Полиция"),TEXT("Police"));Destination(TEXT("Больница"),TEXT("Hospital"));}
        if(Mode==TEXT("GpsJobs"))Destination(TEXT("Курьер"),TEXT("Courier"));
        if(Mode==TEXT("GpsHomes"))for(const TCHAR* Id:{TEXT("North_A"),TEXT("North_B"),TEXT("South_A"),TEXT("South_B"),TEXT("Tower_A1"),TEXT("Tower_A2"),TEXT("Tower_B1"),TEXT("Tower_B2")})Destination(Id,Id);
    }
    else if(Mode==TEXT("Residence"))
    {
        Title=FText::FromString(District->CurrentBuilding.ToString()+FString::Printf(TEXT(" · этаж %d · %d ₽"),District->CurrentFloor,District->ApartmentPrice()));
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
        Fields->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Здравствуйте. Вы точно хотите устроиться? Потребуется пройти вводный опрос."))).AutoWrapText(true)];
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
    Fields->AddSlot().AutoHeight()[SNew(SBorder).Padding(24).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.025f,.16f,.23f,1))
        [SNew(STextBlock).Text(FText::FromString(Item.Prompt)).Font(FCoreStyle::GetDefaultFontStyle("Bold",27)).AutoWrapText(true)]];
    const bool Swap=Question%2!=0;
    for(int32 Index=0;Index<2;++Index)
    {const bool Correct=(Index==0)!=Swap;Button(FText::FromString(Correct?Item.Good:Item.Bad),[this,Organization,Question,Score,Correct](){QuizPage(Organization,Question+1,Score+(Correct?1:0));return FReply::Handled();});}
}

void SCityV2Menu::FocusChatInput()
{
    if(First)FSlateApplication::Get().SetKeyboardFocus(First,EFocusCause::SetDirectly);
}
void SCityV2Menu::SendChat()
{
    auto* District=Controller->DistrictGameplay.Get();
    const FString Text=First->GetText().ToString().TrimStartAndEnd();
    if(!Text.IsEmpty())
    {
        const FText Response=District->ExecuteChat(Text);
        if(District->Context==TEXT("Chat") && Response.ToString()!=District->ChatHistory)
            District->ChatHistory=(District->ChatHistory+TEXT("\n")+Response.ToString()).Right(1200);
    }
    Controller->CloseCityMenu();
    if(District->Context!=TEXT("Chat"))District->OpenCurrentMenu();
}
void SCityV2Menu::ChatOverlay()
{
    ChildSlot[SNew(SOverlay)
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0,14)
        [SNew(SBox).WidthOverride(700).HeightOverride(115)
            [SNew(SBorder).Padding(14).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.015f,.025f,.04f,.55f))
                [SNew(STextBlock).Text(FText::FromString(Controller->DistrictGameplay->ChatHistory.Right(280))).Font(FCoreStyle::GetDefaultFontStyle("Regular",19)).AutoWrapText(true)]]]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0,140,0,0))
        [SNew(SBox).WidthOverride(700)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)
                [SAssignNew(First,SEditableTextBox).Style(&InputStyle).Font(FCoreStyle::GetDefaultFontStyle("Regular",24))
                    .VirtualKeyboardTrigger(EVirtualKeyboardTrigger::OnAllFocusEvents)
                    .OnTextCommitted_Lambda([this](const FText&,ETextCommit::Type Reason){if(Reason==ETextCommit::OnEnter)SendChat();})]
                +SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                [SNew(SButton).ButtonStyle(&ActionStyle).OnClicked_Lambda([this](){SendChat();return FReply::Handled();})
                    [SNew(STextBlock).Text(FText::FromString(TEXT("→"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",28))]]]]];
}
