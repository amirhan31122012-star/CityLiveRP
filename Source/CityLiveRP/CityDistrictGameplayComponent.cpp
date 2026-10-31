#include "CityDistrictGameplayComponent.h"
#include "CityDistrictRules.h"
#include "CityLiveRPPlayerController.h"
#include "CityPlayerProfile.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
namespace
{
struct FBuilding
{
    const TCHAR* Id;int32 Floors;CityDistrictRules::HousingClass Class;FVector Entrance;
};
const TArray<FBuilding>& Buildings()
{
    using C=CityDistrictRules::HousingClass;
    // Authored district coordinates: Blender metres -> Unreal centimetres, mirrored Y.
    static const TArray<FBuilding> Data={
        {TEXT("North_A"),5,C::Standard,FVector(5620,-6400,100)},
        {TEXT("North_B"),5,C::Standard,FVector(9780,-6400,100)},
        {TEXT("South_A"),5,C::Economy,FVector(-9980,7700,100)},
        {TEXT("South_B"),5,C::Economy,FVector(-5820,7700,100)},
        {TEXT("Tower_A1"),6,C::Premium,FVector(4698,7900,100)},
        {TEXT("Tower_A2"),6,C::Premium,FVector(6102,7900,100)},
        {TEXT("Tower_B1"),6,C::Premium,FVector(9672,8400,100)},
        {TEXT("Tower_B2"),6,C::Premium,FVector(11128,8400,100)}};
    return Data;
}
bool Persist(UCityPlayerProfile* Profile)
{return UGameplayStatics::SaveGameToSlot(Profile,UCityPlayerProfile::Slot(Profile->FirstName,Profile->Surname),0);}
}
UCityDistrictGameplayComponent::UCityDistrictGameplayComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UCityDistrictGameplayComponent::BuyApartment(FName Building,int32 Floor,int32 Door,FText& Error)
{
    ACityLiveRPPlayerController* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(!Player || !Player->bEnteredCity || !Player->ActiveProfile)return false;
    UCityPlayerProfile* Profile=Player->ActiveProfile;
    const FBuilding* Definition=Buildings().FindByPredicate([&](const FBuilding& B){return Building==FName(B.Id);});
    if(!Definition || Floor<1 || Floor>Definition->Floors || Door<1 || Door>4)
    {Error=FText::FromString(TEXT("Квартира не найдена."));return false;}
    const FString Id=Building.ToString()+FString::Printf(TEXT(":%d:%d"),Floor,Door);
    const int32 Cost=CityDistrictRules::Price(Definition->Class);
    if(!CityDistrictRules::CanPurchase(Profile->Balance,Cost,Profile->Apartments.Contains(Id)))
    {Error=FText::FromString(TEXT("Квартира уже ваша или недостаточно денег."));return false;}
    Profile->Balance-=Cost;Profile->Apartments.Add(Id);
    if(!Persist(Profile))
    {Profile->Balance+=Cost;Profile->Apartments.Remove(Id);Error=FText::FromString(TEXT("Покупка отменена: не удалось сохранить профиль."));return false;}
    return true;
}
bool UCityDistrictGameplayComponent::SetCourierEmployment(bool Employed,FText& Error)
{
    ACityLiveRPPlayerController* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(!Player || !Player->bEnteredCity || !Player->ActiveProfile)return false;
    UCityPlayerProfile* Profile=Player->ActiveProfile;
    const bool Previous=Profile->bCourierEmployed;Profile->bCourierEmployed=Employed;
    if(!Persist(Profile)){Profile->bCourierEmployed=Previous;Error=FText::FromString(TEXT("Не удалось сохранить трудоустройство."));return false;}
    if(!Employed)bDeliveryActive=false;
    return true;
}
bool UCityDistrictGameplayComponent::BeginDelivery(FText& Error)
{
    ACityLiveRPPlayerController* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(!Player || !Player->bEnteredCity || !Player->ActiveProfile || !Player->ActiveProfile->bCourierEmployed)
    {Error=FText::FromString(TEXT("Сначала устройтесь курьером."));return false;}
    if(bDeliveryActive){Error=FText::FromString(TEXT("Сначала завершите текущую доставку."));return false;}
    const auto& All=Buildings();const FBuilding& Target=All[NextDestination%All.Num()];
    DeliveryTarget=Target.Entrance;DeliveryBuilding=FName(Target.Id);bDeliveryActive=true;
    NextDestination=(NextDestination+1)%All.Num();return true;
}
bool UCityDistrictGameplayComponent::CompleteDelivery(FText& Error)
{
    ACityLiveRPPlayerController* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(!Player || !Player->bEnteredCity || !Player->ActiveProfile || !Player->GetPawn())return false;
    UCityPlayerProfile* Profile=Player->ActiveProfile;
    APawn* Vehicle=Player->GetPawn();
    const bool Scooter=Vehicle->ActorHasTag(TEXT("CityCourierScooter"));
    if(!CityDistrictRules::CanDeliver(Profile->bCourierEmployed,bDeliveryActive,Scooter,
        FVector::Distance(Vehicle->GetActorLocation(),DeliveryTarget),Vehicle->GetVelocity().Size(),Profile->Balance) ||
        Profile->CompletedDeliveries==MAX_int32)
    {Error=FText::FromString(TEXT("Остановитесь у адреса доставки на рабочем скутере."));return false;}
    Profile->Balance+=CityDistrictRules::CourierReward;++Profile->CompletedDeliveries;
    if(!Persist(Profile))
    {Profile->Balance-=CityDistrictRules::CourierReward;--Profile->CompletedDeliveries;
        Error=FText::FromString(TEXT("Не удалось сохранить выплату. Доставка остаётся активной."));return false;}
    // Consumed only after saving: crossing the same marker twice cannot pay twice.
    bDeliveryActive=false;return true;
}
