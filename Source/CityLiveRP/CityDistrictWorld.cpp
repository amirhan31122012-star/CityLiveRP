#include "CityDistrictGameplayComponent.h"
#include "CityLiveRPPlayerController.h"
#include "CityPlayerProfile.h"
#include "CityDistrictRules.h"
#include "CityCourierScooter.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
namespace
{
const FVector Lobby(0,50000,200);
const FVector PoliceDoor(-7800,-4200,100),HospitalDoor(14000,-9700,100),CourierDesk(-11600,-1900,100);
struct FEntry{const TCHAR* Id;FVector Position;int32 Floors;int32 Price;};
const TArray<FEntry>& Entries()
{
    static const TArray<FEntry> All={
        {TEXT("North_A"),FVector(5620,-6200,100),5,350000},{TEXT("North_B"),FVector(9780,-6200,100),5,350000},
        {TEXT("South_A"),FVector(-9980,7900,100),5,150000},{TEXT("South_B"),FVector(-5820,7900,100),5,150000},
        {TEXT("Tower_A1"),FVector(4698,8100,100),6,500000},{TEXT("Tower_A2"),FVector(6102,8100,100),6,500000},
        {TEXT("Tower_B1"),FVector(9672,8600,100),6,500000},{TEXT("Tower_B2"),FVector(11128,8600,100),6,500000}};
    return All;
}
AActor* Box(UWorld* World,FVector Center,FVector Size,FLinearColor Color)
{
    AActor* Prop=World->SpawnActor<AActor>();
    UStaticMeshComponent* Geometry=NewObject<UStaticMeshComponent>(Prop);
    Prop->SetRootComponent(Geometry);Prop->AddInstanceComponent(Geometry);
    Geometry->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Geometry->SetWorldLocation(Center);Geometry->SetWorldScale3D(Size/100.f);
    Geometry->SetCollisionProfileName(TEXT("BlockAll"));Geometry->RegisterComponent();
    if(UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {auto* Material=UMaterialInstanceDynamic::Create(Base,Prop);Material->SetVectorParameterValue(TEXT("Color"),Color);Geometry->SetMaterial(0,Material);}
    return Prop;
}
AActor* Label(UWorld* World,FVector Point,const FString& Text,FColor Color=FColor::White,float Size=35)
{
    AActor* Prop=World->SpawnActor<AActor>();auto* Sign=NewObject<UTextRenderComponent>(Prop);
    Prop->SetRootComponent(Sign);Prop->AddInstanceComponent(Sign);Sign->SetText(FText::FromString(Text));
    Sign->SetWorldSize(Size);Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetTextRenderColor(Color);
    Sign->SetWorldLocationAndRotation(Point,FRotator(0,-90,0));Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sign->SetCullDistance(10000);Sign->RegisterComponent();
    auto* Back=NewObject<UTextRenderComponent>(Prop);Prop->AddInstanceComponent(Back);Back->SetupAttachment(Sign);
    Back->SetText(FText::FromString(Text));Back->SetWorldSize(Size);Back->SetHorizontalAlignment(EHTA_Center);Back->SetTextRenderColor(Color);
    Back->SetRelativeRotation(FRotator(0,180,0));Back->SetRelativeLocation(FVector(-.1f,0,0));Back->SetCollisionEnabled(ECollisionEnabled::NoCollision);Back->SetCullDistance(10000);Back->RegisterComponent();return Prop;
}
AActor* Npc(UWorld* World,FVector Point,const FString& Name)
{
    UClass* Class=LoadClass<ACharacter>(nullptr,TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C"));
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    ACharacter* Person=Class?World->SpawnActor<ACharacter>(Class,Point,FRotator(0,-90,0),Params):nullptr;
    if(Person){Person->Tags.Add(TEXT("CityDistrictNPC"));Person->GetCharacterMovement()->DisableMovement();
        TInlineComponentArray<UPrimitiveComponent*> Geometry(Person);for(auto* Part:Geometry)Part->SetCullDistance(2500);}
    return Person;
}
bool WalkTo(ACityLiveRPPlayerController* Player,FVector Point)
{
    ACharacter* Walker=Player?Cast<ACharacter>(Player->GetPawn()):nullptr;
    if(!Walker || Player->GetSeatedSedan())return false;
    Walker->GetCharacterMovement()->StopMovementImmediately();return Walker->TeleportTo(Point,FRotator(0,90,0));
}
}
int32 UCityDistrictGameplayComponent::MaxFloor() const
{for(const auto& Entry:Entries())if(CurrentBuilding==FName(Entry.Id))return Entry.Floors;return 1;}
int32 UCityDistrictGameplayComponent::ApartmentPrice() const
{for(const auto& Entry:Entries())if(CurrentBuilding==FName(Entry.Id))return Entry.Price;return 0;}
void UCityDistrictGameplayComponent::InitializeDistrict()
{
    if(bInitialized)return;bInitialized=true;PrimaryComponentTick.bCanEverTick=true;SetComponentTickEnabled(true);
    for(const auto& Entry:Entries())DistrictActors.Add(Label(GetWorld(),Entry.Position+FVector(0,0,75),FString(Entry.Id)+TEXT("\nВХОД"),FColor::Cyan,28));
    DistrictActors.Add(Label(GetWorld(),PoliceDoor+FVector(0,0,100),TEXT("ДЕЖУРНАЯ ЧАСТЬ"),FColor::Cyan));
    const FLinearColor Wall(.7f,.73f,.7f),Glass(.15f,.35f,.5f);
    DistrictActors.Add(Box(GetWorld(),FVector(14000,-10500,400),FVector(1800,1200,800),Wall));
    for(int Row=0;Row<2;++Row)for(int Window=0;Window<6;++Window)
        DistrictActors.Add(Box(GetWorld(),FVector(13300+Window*280,-9890,250+Row*350),FVector(150,15,180),Glass));
    DistrictActors.Add(Label(GetWorld(),FVector(14000,-9860,720),TEXT("БОЛЬНИЦА  +"),FColor::Red,85));
    DistrictActors.Add(Label(GetWorld(),HospitalDoor+FVector(0,0,90),TEXT("ВХОД В БОЛЬНИЦУ"),FColor::Cyan));
    DistrictActors.Add(Npc(GetWorld(),CourierDesk,TEXT("Курьер")));
    DistrictActors.Add(Label(GetWorld(),CourierDesk+FVector(0,0,120),TEXT("РАБОТА КУРЬЕРА"),FColor::Yellow));
}
void UCityDistrictGameplayComponent::BuildInterior(bool Apartment)
{
    for(AActor* Prop:InteriorActors)if(IsValid(Prop))Prop->Destroy();InteriorActors.Reset();
    const FLinearColor Wall(.65f,.68f,.7f),Floor(.32f,.3f,.27f);
    auto Add=[&](FVector P,FVector S,FLinearColor C){InteriorActors.Add(Box(GetWorld(),Lobby+P,S,C));};
    Add(FVector(0,0,-15),FVector(1400,1000,30),Floor);
    Add(FVector(0,500,140),FVector(1400,25,280),Wall);Add(FVector(0,-500,140),FVector(1400,25,280),Wall);
    Add(FVector(-700,0,140),FVector(25,1000,280),Wall);Add(FVector(700,0,140),FVector(25,1000,280),Wall);
    Add(FVector(0,0,290),FVector(1400,1000,25),Wall);
    InteriorActors.Add(Label(GetWorld(),Lobby+FVector(0,-450,120),TEXT("ВЫХОД"),FColor::Green));
    if(InteriorKind==TEXT("Residence") && !Apartment)
    {
        for(int Door=1;Door<=4;++Door)
        {
            const FVector P(-450+(Door-1)*300,460,110);Add(P,FVector(100,15,220),FLinearColor(.25f,.16f,.08f));
            InteriorActors.Add(Label(GetWorld(),Lobby+P+FVector(0,-15,100),FString::Printf(TEXT("КВ. %d"),(CurrentFloor-1)*4+Door),FColor::White,24));
        }
        Add(FVector(-600,-100,110),FVector(120,15,220),FLinearColor(.35f,.35f,.4f));
        InteriorActors.Add(Label(GetWorld(),Lobby+FVector(-600,-125,200),TEXT("ЛИФТ"),FColor::Yellow));
        InteriorActors.Add(Label(GetWorld(),Lobby+FVector(0,450,250),CurrentBuilding.ToString()+FString::Printf(TEXT(" · ЭТАЖ %d"),CurrentFloor)));
    }
    else if(Apartment)
    {
        Add(FVector(250,250,30),FVector(220,150,60),FLinearColor(.2f,.3f,.3f));
        Add(FVector(-250,200,40),FVector(130,80,80),Floor);
        InteriorActors.Add(Label(GetWorld(),Lobby+FVector(0,400,220),TEXT("ВАША КВАРТИРА"),FColor::Cyan));
    }
    else
    {
        Add(FVector(100,200,45),FVector(350,100,90),Floor);
        InteriorActors.Add(Npc(GetWorld(),Lobby+FVector(100,320,97),InteriorKind==TEXT("Police")?TEXT("Дежурный"):TEXT("Регистратор")));
        InteriorActors.Add(Label(GetWorld(),Lobby+FVector(100,320,220),InteriorKind==TEXT("Police")?TEXT("ДЕЖУРНЫЙ"):TEXT("РЕГИСТРАТОР"),FColor::Cyan));
    }
    AActor* Lamp=GetWorld()->SpawnActor<AActor>();auto* Light=NewObject<UPointLightComponent>(Lamp);
    Lamp->SetRootComponent(Light);Lamp->AddInstanceComponent(Light);Light->SetWorldLocation(Lobby+FVector(0,0,240));
    Light->SetIntensity(5000);Light->SetAttenuationRadius(1800);Light->SetCastShadows(false);Light->RegisterComponent();InteriorActors.Add(Lamp);
}
bool UCityDistrictGameplayComponent::EnterBuilding(FName Id,FText& Error)
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player||!Player->bEnteredCity||!HasInteraction())return false;
    ReturnPoint=Player->GetPawn()->GetActorLocation();CurrentBuilding=Id;CurrentFloor=1;bInApartment=false;
    Context=Id==TEXT("Police")?TEXT("Police"):Id==TEXT("Hospital")?TEXT("Hospital"):TEXT("Residence");
    InteriorKind=Context;BuildInterior();if(!WalkTo(Player,Lobby+FVector(0,-250,97))){Error=FText::FromString(TEXT("Выйдите из машины перед входом."));return false;}
    bInside=true;return true;
}
bool UCityDistrictGameplayComponent::ChangeFloor(int32 Floor,FText& Error)
{
    if(!bInside||InteriorKind!=TEXT("Residence")||Floor<1||Floor>MaxFloor())return false;
    CurrentFloor=Floor;BuildInterior();return WalkTo(Cast<ACityLiveRPPlayerController>(GetOwner()),Lobby+FVector(-450,-100,97));
}
bool UCityDistrictGameplayComponent::LeaveBuilding(FText& Error)
{
    if(!bInside)return false;
    if(bInApartment){bInApartment=false;Context=TEXT("Residence");BuildInterior();return WalkTo(Cast<ACityLiveRPPlayerController>(GetOwner()),Lobby+FVector(0,-250,97));}
    if(!WalkTo(Cast<ACityLiveRPPlayerController>(GetOwner()),ReturnPoint))return false;
    bInside=false;Context=TEXT("Chat");
    for(AActor* Prop:InteriorActors)if(IsValid(Prop))Prop->Destroy();InteriorActors.Reset();return true;
}
bool UCityDistrictGameplayComponent::EnterApartment(int32 Door,FText& Error)
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player||!Player->ActiveProfile||!bInside)return false;
    const FString Id=CurrentBuilding.ToString()+FString::Printf(TEXT(":%d:%d"),CurrentFloor,Door);
    if(!Player->ActiveProfile->Apartments.Contains(Id)){Error=FText::FromString(TEXT("Сначала купите эту квартиру."));return false;}
    bInApartment=true;BuildInterior(true);return WalkTo(Player,Lobby+FVector(0,-250,97));
}
void UCityDistrictGameplayComponent::RefreshInteraction()
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());Nearby.Empty();
    if(!Player||!Player->bEnteredCity||!Player->GetPawn()||Player->GetSeatedSedan())return;
    const FVector Here=Player->GetPawn()->GetActorLocation();
    auto Near=[&](FVector Point,float Radius){return FVector::DistSquared(Here,Point)<FMath::Square(Radius);};
    if(bInside)
    {
        if(Near(Lobby+FVector(0,-400,97),200))Nearby=TEXT("Exit");
        else if(InteriorKind==TEXT("Residence")&&!bInApartment && (Near(Lobby+FVector(-550,-100,97),240)||Here.Y>Lobby.Y+200))Nearby=TEXT("Housing");
        else if(!bInApartment&&InteriorKind!=TEXT("Residence")&&Near(Lobby+FVector(100,320,97),250))Nearby=TEXT("Npc");
        return;
    }
    for(const auto& Entry:Entries())if(Near(Entry.Position,250)){Nearby=Entry.Id;return;}
    if(Near(PoliceDoor,300))Nearby=TEXT("Police");else if(Near(HospitalDoor,300))Nearby=TEXT("Hospital");else if(Near(CourierDesk,220))Nearby=TEXT("Courier");
}
bool UCityDistrictGameplayComponent::HasInteraction() const {return !Nearby.IsEmpty();}
void UCityDistrictGameplayComponent::Interact()
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player||Player->IsCityMenuOpen())return;
    RefreshInteraction();if(Nearby.IsEmpty())return;FText Error;
    if(Nearby==TEXT("Exit")){LeaveBuilding(Error);return;}
    if(!bInside && Nearby!=TEXT("Courier")){EnterBuilding(FName(*Nearby),Error);return;}
    Context=Nearby==TEXT("Courier")?TEXT("Courier"):InteriorKind;Player->OpenCityMenu(false,true);
}
void UCityDistrictGameplayComponent::OpenChat()
{auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(Player&&Player->bEnteredCity&&!Player->IsCityMenuOpen()){Context=TEXT("Chat");Player->OpenCityMenu(false,true);}}
void UCityDistrictGameplayComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,Type,Function);if(!bInitialized)return;ScanTime+=Dt;if(ScanTime<.2f)return;ScanTime=0;
    RefreshInteraction();FText Error;
    if(bDeliveryActive)
    {
        if(!IsValid(CourierMarker))CourierMarker=Label(GetWorld(),DeliveryTarget+FVector(0,0,120),TEXT("▼ ДОСТАВКА"),FColor::Red,90);
        if(CompleteDelivery(Error)){if(IsValid(CourierMarker))CourierMarker->Destroy();CourierMarker=nullptr;ChatHistory=(ChatHistory+TEXT("\nДоставка выполнена: +5 000 ₽")).Right(1200);}
    }
    else if(IsValid(CourierMarker)){CourierMarker->Destroy();CourierMarker=nullptr;}
}
void UCityDistrictGameplayComponent::SetGps(FName Destination)
{
    if(Destination==TEXT("Police")){GpsTarget=PoliceDoor;GpsLabel=TEXT("Полиция");}
    else if(Destination==TEXT("Hospital")){GpsTarget=HospitalDoor;GpsLabel=TEXT("Больница");}
    else if(Destination==TEXT("Courier")){GpsTarget=CourierDesk;GpsLabel=TEXT("Работа курьера");}
    else{for(const auto& Entry:Entries())if(Destination==FName(Entry.Id)){GpsTarget=Entry.Position;GpsLabel=Entry.Id;}}
    if(IsValid(NavigationMarker))NavigationMarker->Destroy();NavigationMarker=Label(GetWorld(),GpsTarget+FVector(0,0,180),TEXT("▼ GPS"),FColor::Cyan,90);
}
FString UCityDistrictGameplayComponent::NavigationText() const
{
    const auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player||!Player->GetPawn()||GpsLabel.IsEmpty())return FString();
    if(bInside)return TEXT("GPS: сначала выйдите из здания");
    return FString::Printf(TEXT("GPS: %s · %.0f м"),*GpsLabel,FVector::Dist2D(Player->GetPawn()->GetActorLocation(),GpsTarget)/100);
}
bool UCityDistrictGameplayComponent::JoinOrganization(FName Organization,int32 Correct,FText& Error)
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player||!Player->ActiveProfile||!bInside||Nearby!=TEXT("Npc"))return false;
    if(Correct<3){Error=FText::FromString(TEXT("Экзамен не пройден. Можно попробовать ещё раз."));return false;}
    const FString Previous=Player->ActiveProfile->Organization;Player->ActiveProfile->Organization=Organization.ToString();
    if(!UGameplayStatics::SaveGameToSlot(Player->ActiveProfile,UCityPlayerProfile::Slot(Player->ActiveProfile->FirstName,Player->ActiveProfile->Surname),0))
    {Player->ActiveProfile->Organization=Previous;return false;}Error=FText::FromString(TEXT("Вы приняты на начальную должность."));return true;
}
FText UCityDistrictGameplayComponent::ExecuteChat(FString Text)
{
    Text.TrimStartAndEndInline();auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player || !Player->ActiveProfile || !Player->bEnteredCity)return FText::GetEmpty();
    if(Text==TEXT("/gps")){Context=TEXT("Gps");return FText::FromString(TEXT("Выберите раздел GPS."));}
    if(Text==TEXT("/help"))return FText::FromString(TEXT("/gps /me /do /try /b /stats /time /job /quitjob"));
    if(Text==TEXT("/stats"))return FText::FromString(Player->GetProfileName()+FString::Printf(TEXT(" · %d ₽ · доставок %d"),Player->GetBalance(),Player->ActiveProfile->CompletedDeliveries));
    if(Text==TEXT("/time"))return FText::FromString(FDateTime::Now().ToString(TEXT("%H:%M")));
    if(Text==TEXT("/job")){Context=TEXT("Courier");return FText::FromString(TEXT("Подойдите к NPC курьера для трудоустройства."));}
    if(Text==TEXT("/quitjob")){FText Error;if(!SetCourierEmployment(false,Error))return Error;return FText::FromString(TEXT("Смена курьера завершена."));}
    if(Text.StartsWith(TEXT("/")) && !Text.StartsWith(TEXT("/me "))&&!Text.StartsWith(TEXT("/do "))&&!Text.StartsWith(TEXT("/try "))&&!Text.StartsWith(TEXT("/b ")))
        return FText::FromString(TEXT("Команда недоступна. /help — список команд этой игры."));
    if(Text.Len()>180)Text=Text.Left(180);if(Text.StartsWith(TEXT("/try ")))Text+=FMath::RandBool()?TEXT(" — успешно"):TEXT(" — неудачно");
    ChatHistory=(ChatHistory+TEXT("\n")+Player->GetProfileName()+TEXT(": ")+Text).Right(1200);return FText::FromString(ChatHistory);
}

bool UCityDistrictGameplayComponent::SpawnWorkScooter(FText& Error)
{
    auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());
    if(!Player || !Player->ActiveProfile || !Player->ActiveProfile->bCourierEmployed || Nearby!=TEXT("Courier") || Player->GetSeatedSedan())
    {Error=FText::FromString(TEXT("Подойдите к NPC курьера после трудоустройства."));return false;}
    for(int32 Direction=0;Direction<12;++Direction)
    {
        const float Yaw=Direction*30.f;FVector Point;
        if(!ACitySedan::FindFlatSpawn(GetWorld(),CourierDesk+FRotator(0,Yaw,0).Vector()*650,Yaw,Player->GetPawn(),Point))continue;
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        if(auto* Scooter=GetWorld()->SpawnActor<ACityCourierScooter>(Point,FRotator(0,Yaw,0),Params))
        {if(IsValid(Player->TestSedan))Player->TestSedan->Destroy();Player->TestSedan=Scooter;Error=FText::FromString(TEXT("Скутер выдан. Подойдите к нему и нажмите кнопку посадки."));return true;}
    }
    Error=FText::FromString(TEXT("Нет свободного места для скутера."));return false;
}
