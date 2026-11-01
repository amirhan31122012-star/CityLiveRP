#include "SCityV2Menu.h"
#include "SCityMap.h"
#include "CityLiveRPPlayerController.h"
#include "CityDistrictGameplayComponent.h"
#include "CityDistrictMapData.h"
#include "CityUiTheme.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

void SCityV2Menu::GpsPage()
{
    SecondaryStyle.SetNormalPadding(FMargin(16,9));SecondaryStyle.SetPressedPadding(FMargin(16,12,16,6));
    BuildFrame(3);auto* District=Controller->DistrictGameplay.Get();const FString Mode=District->Context;
    Title=FText::FromString(TEXT("GPS · навигация"));Message=FText::GetEmpty();
    if(Mode==TEXT("Gps"))
    {
        const TPair<const TCHAR*,const TCHAR*> Groups[]={
            {TEXT("Важные места"),TEXT("GpsImportant")},{TEXT("Работы для новичков"),TEXT("GpsJobs")},
            {TEXT("Государственные организации"),TEXT("GpsGovernment")},{TEXT("Транспорт и парковки"),TEXT("GpsTransport")},
            {TEXT("Магазины"),TEXT("GpsShops")},{TEXT("Жилые дома"),TEXT("GpsHomes")},{TEXT("Ближайшие места"),TEXT("GpsNearest")}};
        int Number=0;for(const auto& Group:Groups)
        {const FString Context=Group.Value;Button(FText::FromString(FString::Printf(TEXT("%d.  %s"),++Number,Group.Key)),[this,District,Context](){District->Context=Context;SelectedGps.Empty();GpsPage();return FReply::Handled();});}
    }
    else
    {
        const auto PageFields=Fields;TSharedRef<SVerticalBox> PlacesList=SNew(SVerticalBox);
        Fields->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(310)[SNew(SScrollBox).ScrollBarStyle(&ScrollStyle)+SScrollBox::Slot()[PlacesList]]];
        Fields=PlacesList;
        auto Places=CityDistrictMapData::Places();const auto Here=District->MapPosition();
        if(Mode==TEXT("GpsNearest"))std::sort(Places.begin(),Places.end(),[&](const auto& A,const auto& B){return CityMap::Distance(A.Position,{Here.X/100,Here.Y/100})<CityMap::Distance(B.Position,{Here.X/100,Here.Y/100});});
        for(const auto& Place:Places)
        {
            const FString Group=UTF8_TO_TCHAR(Place.Group);
            if(Mode!=TEXT("GpsNearest") && !(Mode==TEXT("GpsImportant")&&(Group==TEXT("Government")||Group==TEXT("Jobs")||Group==TEXT("Transport"))) &&
                !(Mode==TEXT("GpsGovernment")&&Group==TEXT("Government"))&&!(Mode==TEXT("GpsJobs")&&Group==TEXT("Jobs"))&&
                !(Mode==TEXT("GpsTransport")&&Group==TEXT("Transport"))&&!(Mode==TEXT("GpsShops")&&Group==TEXT("Shops"))&&!(Mode==TEXT("GpsHomes")&&Group==TEXT("Homes")))continue;
            const FString Id=UTF8_TO_TCHAR(Place.Id),Name=UTF8_TO_TCHAR(Place.Label);
            Fields->AddSlot().AutoHeight().Padding(0,4)
                [SNew(SButton).ButtonStyle(&RowStyle).HAlign(HAlign_Left)
                    .ButtonColorAndOpacity_Lambda([this,Id](){return SelectedGps==Id?FLinearColor(.36f,.065f,.006f):FLinearColor(.025f,.028f,.034f);})
                    .OnClicked_Lambda([this,Id](){SelectedGps=Id;return FReply::Handled();})
                    [SNew(STextBlock).Text(FText::FromString(Name)).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Bold",21))]];
        }
        Fields=PageFields;
        Button(FText::FromString(TEXT("ПРОЛОЖИТЬ МАРШРУТ")),[this,District](){if(SelectedGps.IsEmpty())Message=FText::FromString(TEXT("Выберите место из списка."));else if(District->SelectMapPlace(FName(*SelectedGps)))Controller->CloseCityMenu();return FReply::Handled();},true);
        Button(FText::FromString(TEXT("← Разделы GPS")),[this,District](){District->Context=TEXT("Gps");GpsPage();return FReply::Handled();});
    }
    Button(FText::FromString(TEXT("Закрыть")),[this](){Controller->CloseCityMenu();return FReply::Handled();});
}
void SCityV2Menu::MapPage()
{
    bChatLayout=false;bMapSelection=false;auto* District=Controller->DistrictGameplay.Get();MapLabel.Empty();
    FVector Existing;FString Name;if(District->NavigationTarget(Existing,Name)){MapSelection=Existing;MapLabel=Name;}
    ChildSlot[SNew(SOverlay)
        +SOverlay::Slot()[SAssignNew(DistrictMap,SCityMap).Controller(Controller.Get()).OnSelection_Lambda([this](FVector Point,FString Label){MapSelection=Point;MapLabel=Label;bMapSelection=true;DistrictMap->SetPendingPoint(Point);})]
        +SOverlay::Slot()[SNew(SScaleBox).Visibility(EVisibility::SelfHitTestInvisible).Stretch(EStretch::ScaleToFit)[SNew(SBox).Visibility(EVisibility::SelfHitTestInvisible).WidthOverride(1280).HeightOverride(720)
            [SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(32,26)
                [SNew(STextBlock).Visibility(EVisibility::HitTestInvisible).Text(FText::FromString(TEXT("КАРТА  |  ПОЛИЦЕЙСКИЙ РАЙОН"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",24))]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30,78,0,0)
                [SNew(SButton).ButtonStyle(&SecondaryStyle).OnClicked_Lambda([this](){bLegendOpen=!bLegendOpen;return FReply::Handled();})
                    [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(bLegendOpen?TEXT("Легенда  ↑"):TEXT("Легенда  ↓"));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",17))]]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,22,28,0)
                [SNew(SButton).ButtonStyle(&SecondaryStyle).OnClicked_Lambda([this](){Controller->CloseCityMenu();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("×"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",28))]]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(28,0,0,28)
                [SNew(SBorder).Visibility_Lambda([this](){return bLegendOpen?EVisibility::HitTestInvisible:EVisibility::Collapsed;}).Padding(18).BorderImage(&FrameBrush).BorderBackgroundColor(FLinearColor::White)
                    [SNew(STextBlock).Text(FText::FromString(TEXT("ЛЕГЕНДА\nПолиция · синяя метка\nБольница · красная метка\nКурьер · оранжевая метка\nДома · зелёная метка\nP · парковка    Сумка · магазины\nОранжевая линия — маршрут\nПунктир — подход к точке"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(FLinearColor(.75f,.78f,.82f))]]
                +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30,142,0,0)
                [SNew(SButton).ButtonStyle(&SecondaryStyle).OnClicked_Lambda([this,District](){District->Context=TEXT("Gps");GpsPage();return FReply::Handled();})
                    [SNew(STextBlock).Text(FText::FromString(TEXT("GPS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",20))]]
                +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,28,28)
                [SNew(SBox).WidthOverride(370).Visibility_Lambda([this,District](){FVector Point;FString Label;return bMapSelection||District->NavigationTarget(Point,Label)?EVisibility::Visible:EVisibility::Collapsed;})[SNew(SBorder).BorderImage(&FrameBrush).Padding(18)
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(MapLabel);}).Font(FCoreStyle::GetDefaultFontStyle("Bold",22)).AutoWrapText(true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text_Lambda([this,District](){
                            if(bMapSelection)return FText::FromString(FString::Printf(TEXT("До точки: %.0f м"),FVector::Dist2D(MapSelection,District->MapPosition())/100));
                            return FText::FromString(District->MapRoute().Valid?FString::Printf(TEXT("Маршрут: %.0f м"),District->MapRoute().Metres):District->NavigationText());
                        }).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).AutoWrapText(true).ColorAndOpacity(CityUi::Orange())]
                        +SVerticalBox::Slot().AutoHeight()[SNew(SButton).ButtonStyle(&ActionStyle).OnClicked_Lambda([this,District](){if(bMapSelection){District->SetMapDestination(MapSelection,MapLabel);bMapSelection=false;DistrictMap->ClearPendingPoint();}else{FVector Point;FString Label;if(District->NavigationTarget(Point,Label)){District->ClearGps();MapLabel.Empty();}}return FReply::Handled();})
                            [SNew(STextBlock).Text_Lambda([this,District](){FVector Point;FString Label;return FText::FromString(bMapSelection?TEXT("ПОСТРОИТЬ МАРШРУТ"):District->NavigationTarget(Point,Label)?TEXT("УБРАТЬ МАРШРУТ"):TEXT(""));}).Font(FCoreStyle::GetDefaultFontStyle("Bold",18))]]
                    ]]]
            ]]]];
}
