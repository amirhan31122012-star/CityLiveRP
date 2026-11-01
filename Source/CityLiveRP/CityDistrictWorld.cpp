#include "CityDistrictGameplayComponent.h"
#include "CityLiveRPPlayerController.h"
#include "CityPlayerProfile.h"
#include "CityLiveRPCharacter.h"
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
#include <initializer_list>
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
    Geometry->SetCollisionProfileName(TEXT("BlockAll"));
    if(UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CityLiveUI/M_DistrictSurface.M_DistrictSurface")))
    {auto* Material=UMaterialInstanceDynamic::Create(Base,Prop);Material->SetVectorParameterValue(TEXT("BaseTint"),Color);Geometry->SetMaterial(0,Material);}
    Geometry->RegisterComponent();return Prop;
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
    ACityLiveRPCharacter* Person=Class?World->SpawnActorDeferred<ACityLiveRPCharacter>(Class,FTransform(FRotator(0,-90,0),Point),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn):nullptr;
    if(Person)
    {
        Person->AppearanceRole=Name==TEXT("Дежурный")?FName(TEXT("Police")):Name==TEXT("Регистратор")?FName(TEXT("Medic")):FName(TEXT("Courier"));
        UGameplayStatics::FinishSpawningActor(Person,FTransform(FRotator(0,-90,0),Point));
    }
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
    const bool Police=InteriorKind==TEXT("Police"),Hospital=InteriorKind==TEXT("Hospital");
    const FLinearColor Wall=Police?FLinearColor(.69f,.64f,.51f):Hospital?FLinearColor(.72f,.84f,.81f):FLinearColor(.64f,.61f,.55f);
    const FLinearColor Floor=Police?FLinearColor(.18f,.2f,.23f):FLinearColor(.32f,.4f,.4f);
    const FLinearColor Trim=Police?FLinearColor(.025f,.095f,.2f):FLinearColor(.035f,.38f,.4f);
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
        // Contrasting lower wall panels and skirting, ceiling coves and floor joints.
        Add(FVector(0,482,60),FVector(1380,18,120),Trim);
        Add(FVector(0,-482,60),FVector(1380,18,120),Trim);
        Add(FVector(-682,0,60),FVector(18,960,120),Trim);Add(FVector(682,0,60),FVector(18,960,120),Trim);
        for(int32 Line=-6;Line<=6;++Line)Add(FVector(Line*100,0,1),FVector(1,980,1),FLinearColor(.16f,.2f,.22f));
        for(int32 Line=-4;Line<=4;++Line)Add(FVector(0,Line*100,1),FVector(1380,1,1),FLinearColor(.16f,.2f,.22f));
        for(int32 Beam=-2;Beam<=2;++Beam)
        {Add(FVector(Beam*260,0,272),FVector(130,700,12),FLinearColor(.84f,.86f,.8f));Add(FVector(Beam*260,0,281),FVector(150,750,10),Trim);}
        const FLinearColor Wood=Police?FLinearColor(.25f,.105f,.045f):FLinearColor(.75f,.79f,.74f);
        Add(FVector(100,210,45),FVector(420,115,90),Wood);
        Add(FVector(100,210,94),FVector(450,135,10),Trim);
        Add(FVector(100,143,43),FVector(300,5,45),Trim);
        Add(FVector(110,220,118),FVector(65,10,43),FLinearColor(.018f,.025f,.035f));
        Add(FVector(110,210,100),FVector(45,30,5),FLinearColor(.08f,.09f,.1f));
        Add(FVector(-30,205,104),FVector(35,25,7),FLinearColor(.85f,.86f,.8f));
        for(int32 Seat=0;Seat<3;++Seat)
        {
            const float X=-450+Seat*120.f;
            Add(FVector(X,-100,42),FVector(90,80,12),Trim);
            Add(FVector(X,-135,82),FVector(90,10,70),Trim);
            for(float Side:{-30.f,30.f})Add(FVector(X+Side,-100,20),FVector(5,60,40),FLinearColor(.19f,.21f,.23f));
        }
        Add(FVector(550,250,100),FVector(180,60,200),Wood);
        for(int32 Shelf=0;Shelf<4;++Shelf)
        {Add(FVector(550,215,45+Shelf*45),FVector(170,5,7),Trim);for(int32 Folder=0;Folder<7;++Folder)Add(FVector(480+Folder*22,212,60+Shelf*45),FVector(13,25,25),FLinearColor(.2f,.3f+Folder*.05f,.33f));}
        Add(FVector(-475,480,170),FVector(220,10,100),Trim);
        InteriorActors.Add(Label(GetWorld(),Lobby+FVector(-475,465,190),Police?TEXT("ДЕЖУРНАЯ ЧАСТЬ\nПРИЁМ ГРАЖДАН"):TEXT("РЕГИСТРАТУРА\nЗАПИСЬ НА ПРИЁМ"),FColor::White,22));
        if(Police)
        {
            Add(FVector(0,475,193),FVector(210,8,65),Trim);
            InteriorActors.Add(Label(GetWorld(),Lobby+FVector(0,462,210),TEXT("ПОЛИЦИЯ"),FColor(230,195,105),34));
            for(int32 Bar=0;Bar<3;++Bar)Add(FVector(-300,478,220-Bar*18),FVector(95,6,18),Bar==0?FLinearColor(.9f,.9f,.85f):Bar==1?FLinearColor(.04f,.18f,.65f):FLinearColor(.65f,.025f,.025f));
        }
        else
        {
            Add(FVector(0,477,200),FVector(22,8,95),FLinearColor(.72f,.035f,.05f));Add(FVector(0,477,200),FVector(80,8,22),FLinearColor(.72f,.035f,.05f));
            Add(FVector(500,-280,40),FVector(160,80,16),FLinearColor(.16f,.52f,.52f));
            for(float Leg:{450.f,550.f})Add(FVector(Leg,-280,18),FVector(6,65,36),Trim);
            Add(FVector(-610,280,58),FVector(55,55,116),FLinearColor(.8f,.82f,.8f));Add(FVector(-610,280,135),FVector(38,38,38),FLinearColor(.1f,.4f,.6f));
        }
        InteriorActors.Add(Npc(GetWorld(),Lobby+FVector(100,320,97),Police?TEXT("Дежурный"):TEXT("Регистратор")));
        InteriorActors.Add(Label(GetWorld(),Lobby+FVector(100,320,220),Police?TEXT("ДЕЖУРНЫЙ"):TEXT("РЕГИСТРАТОР"),FColor::Cyan));
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
    {Player->ActiveProfile->Organization=Previous;return false;}Player->RefreshAppearance();Error=FText::FromString(TEXT("Вы приняты на начальную должность."));return true;
}
FText UCityDistrictGameplayComponent::ExecuteChat(FString Text)
{
    Text.TrimStartAndEndInline();auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(!Player || !Player->ActiveProfile || !Player->bEnteredCity)return FText::GetEmpty();
    if(Text==TEXT("/gps")){Context=TEXT("Gps");return FText::FromString(TEXT("Выберите раздел GPS."));}
    if(Text==TEXT("/credits"))return FText::FromString(TEXT("Основа персонажей: Casual Beige Knit Look — restore50, CC BY 4.0. Изменения, одежда и риг: CityLive RP. sketchfab.com/3d-models/casual-beige-knit-look-e92cefa2b26e47dc953cfdd16dbc076b · creativecommons.org/licenses/by/4.0/"));
    if(Text==TEXT("/help"))return FText::FromString(TEXT("/gps /me /do /try /b /stats /time /job /quitjob /credits"));
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

void UCityDistrictGameplayComponent::OpenCurrentMenu()
{auto* Player=Cast<ACityLiveRPPlayerController>(GetOwner());if(Player&&Player->IsProfileActive()&&!Player->IsCityMenuOpen())Player->OpenCityMenu(false,true);}
