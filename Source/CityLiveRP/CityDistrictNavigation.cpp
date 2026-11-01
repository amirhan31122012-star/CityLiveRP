#include "CityDistrictGameplayComponent.h"
#include "CityDistrictMapData.h"
#include "CityLiveRPPlayerController.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

FVector UCityDistrictGameplayComponent::MapPosition() const
{
    if(bInside)return ReturnPoint;
    const auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    return Player&&Player->GetPawn()?Player->GetPawn()->GetActorLocation():FVector::ZeroVector;
}
bool UCityDistrictGameplayComponent::NavigationTarget(FVector& Point,FString& Name) const
{
    if(bTrackDelivery&&bDeliveryActive){Point=DeliveryTarget;Name=TEXT("Доставка · подъезд");return true;}
    if(!bGpsActive)return false;Point=GpsTarget;Name=GpsLabel;return true;
}
void UCityDistrictGameplayComponent::ClearGps()
{
    bGpsActive=false;bTrackDelivery=false;GpsLabel.Empty();CachedRoute={};bRouteDirty=true;
    if(IsValid(NavigationMarker))NavigationMarker->Destroy();NavigationMarker=nullptr;
}
bool UCityDistrictGameplayComponent::SelectMapPlace(FName Id)
{
    for(const auto& Place:CityDistrictMapData::Places())if(Id==FName(UTF8_TO_TCHAR(Place.Id)))
    {SetMapDestination(FVector(Place.Position.X*100,Place.Position.Y*100,100),UTF8_TO_TCHAR(Place.Label));return true;}
    return false;
}
void UCityDistrictGameplayComponent::SetMapDestination(FVector Point,const FString& Name)
{
    if(Point.ContainsNaN())return;
    CityMap::Point Position{FMath::Clamp(Point.X/100.,-209.,209.),FMath::Clamp(Point.Y/100.,-169.,169.)};
    Position=CityMap::Outdoors(Position,CityDistrictMapData::Buildings());
    GpsTarget=FVector(Position.X*100,Position.Y*100,100);GpsLabel=Name;bGpsActive=true;bTrackDelivery=false;bRouteDirty=true;
    if(IsValid(NavigationMarker))NavigationMarker->Destroy();
    NavigationMarker=GetWorld()->SpawnActor<AActor>();
    if(NavigationMarker)
    {
        auto* Sign=NewObject<UTextRenderComponent>(NavigationMarker);NavigationMarker->SetRootComponent(Sign);NavigationMarker->AddInstanceComponent(Sign);
        Sign->SetText(FText::FromString(TEXT("▼")));Sign->SetTextRenderColor(FColor(255,75,35));Sign->SetWorldSize(90);
        Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetWorldLocation(GpsTarget+FVector(0,0,130));Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sign->RegisterComponent();
    }
    RefreshNavigation();
}
void UCityDistrictGameplayComponent::RefreshNavigation()
{
    FVector Target;FString Name;if(!NavigationTarget(Target,Name)){CachedRoute={};return;}
    const FVector Start=MapPosition();
    if(!bRouteDirty&&FVector::DistSquared2D(Start,RouteStart)<FMath::Square(250.f)&&FVector::DistSquared2D(Target,RouteTarget)<1)return;
    RouteStart=Start;RouteTarget=Target;bRouteDirty=false;
    CachedRoute=CityMap::Build({Start.X/100,Start.Y/100},{Target.X/100,Target.Y/100},CityDistrictMapData::Roads(),CityDistrictMapData::Buildings());
}
void UCityDistrictGameplayComponent::OpenMap()
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(Player&&Player->IsInCity()&&!Player->IsCityMenuOpen()){Context=TEXT("Map");Player->OpenCityMenu(false,true);}
}
void UCityDistrictGameplayComponent::OpenGps()
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(Player&&Player->IsInCity()&&!Player->IsCityMenuOpen()){Context=TEXT("Gps");Player->OpenCityMenu(false,true);}
}
