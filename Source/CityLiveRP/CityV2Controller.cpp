#include "CityLiveRPPlayerController.h"
#include "CityLiveRPCharacter.h"
#include "CityPlayerProfile.h"
#include "CityParkingMarker.h"
#include "CitySedan.h"
#include "SCityMobileControls.h"
#include "SCityV2Menu.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "TimerManager.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include <initializer_list>

int32 ACityLiveRPPlayerController::GetBalance() const { return ActiveProfile ? ActiveProfile->Balance : 0; }
FString ACityLiveRPPlayerController::GetProfileName() const
{
    return ActiveProfile ? ActiveProfile->FirstName+TEXT("_")+ActiveProfile->Surname : FString();
}
FString ACityLiveRPPlayerController::CurrentMapId() const
{
    FString Id=GetWorld()->GetOutermost()->GetName();
    if(!GetWorld()->StreamingLevelsPrefix.IsEmpty()) Id.ReplaceInline(*GetWorld()->StreamingLevelsPrefix,TEXT(""));
    return Id;
}
void ACityLiveRPPlayerController::InitializeV2Session()
{
    if (UCityLoginMemory* Memory=Cast<UCityLoginMemory>(UGameplayStatics::LoadGameFromSlot(TEXT("CityV2_LastLogin"),0)))
    { RememberedFirst=Memory->FirstName; RememberedLast=Memory->Surname; }
    if (UGameViewportClient* Viewport=GetWorld()->GetGameViewport())
    {
        CityHud=SNew(SOverlay)
            .Visibility_Lambda([this](){return bEnteredCity && !IsCityMenuOpen() ? EVisibility::HitTestInvisible : EVisibility::Hidden;})
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24)
            [SNew(SBorder).Padding(14).BorderBackgroundColor(FLinearColor(.02f,.035f,.06f,.8f))
                [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",20))
                    .Text_Lambda([this](){return FText::FromString(GetProfileName()+TEXT("\n")+
                        FText::AsNumber(GetBalance()).ToString()+TEXT(" ₽"));})]]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0,0,0,22)
            [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",26))
                .Visibility_Lambda([this](){return IsSedanDriver() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;})
                .Text_Lambda([this](){
                    const ACitySedan* Car=GetSeatedSedan();
                    const int32 Speed=Car ? FMath::RoundToInt(Car->GetSpeedKmh()) : 0;
                    return FText::FromString(FString::Printf(TEXT("%03d км/ч"),Speed));})];
        Viewport->AddViewportWidgetContent(CityHud.ToSharedRef(),30);
    }
    CreateParkingMarkers();
    GetWorldTimerManager().SetTimer(ParkingTimer,this,&ACityLiveRPPlayerController::CheckParking,.2f,true);
    GetWorldTimerManager().SetTimer(SaveTimer,this,&ACityLiveRPPlayerController::SaveProfile,5.f,true);
    BackgroundSaveHandle=FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&ACityLiveRPPlayerController::SaveProfile);
    DeactivateSaveHandle=FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&ACityLiveRPPlayerController::SaveProfile);
    OpenCityMenu(false);
}
void ACityLiveRPPlayerController::OpenCityMenu(bool bGarage)
{
    if (CityMenu.IsValid()) return;
    UGameViewportClient* Viewport=GetWorld()->GetGameViewport();
    if (!Viewport) return;
    if (CityMobileControls.IsValid())
    { CityMobileControls->ResetControls(); CityMobileControls->SetVisibility(EVisibility::Hidden); }
    if (ACityLiveRPCharacter* PlayerCharacter=Cast<ACityLiveRPCharacter>(GetPawn()))
    { PlayerCharacter->ResetMobileSteering(); PlayerCharacter->GetCharacterMovement()->StopMovementImmediately(); }
    SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
    SAssignNew(CityMenu,SCityV2Menu).Controller(this).Garage(bGarage);
    Viewport->AddViewportWidgetContent(CityMenu.ToSharedRef(),100);
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(CityMenu);
    Mode.SetHideCursorDuringCapture(false);
    SetInputMode(Mode);
    bShowMouseCursor=!ShouldUseTouchControls();
}
void ACityLiveRPPlayerController::CloseCityMenu()
{
    if (!CityMenu.IsValid() || !bEnteredCity) return;
    GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(CityMenu.ToSharedRef());
    CityMenu.Reset();
    FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
    SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
    SetInputMode(FInputModeGameOnly()); bShowMouseCursor=false;
    if (CityMobileControls.IsValid()) CityMobileControls->SetVisibility(EVisibility::Visible);
}
bool ACityLiveRPPlayerController::SelectIdentity(FString First,FString Last,bool& bNew,FText& Error)
{
    First.TrimStartAndEndInline(); Last.TrimStartAndEndInline();
    const FTCHARToUTF8 A(*First),B(*Last);
    if (!CityProfileRules::ValidName(std::string_view(A.Get(),A.Length())) ||
        !CityProfileRules::ValidName(std::string_view(B.Get(),B.Length())))
    { Error=FText::FromString(TEXT("Имя и фамилия: по 2–20 английских букв, без пробелов и цифр."));return false; }
    PendingSlot=UCityPlayerProfile::Slot(First,Last);
    bCreatingProfile=!UGameplayStatics::DoesSaveGameExist(PendingSlot,0);
    PendingProfile=bCreatingProfile ? NewObject<UCityPlayerProfile>(this) :
        Cast<UCityPlayerProfile>(UGameplayStatics::LoadGameFromSlot(PendingSlot,0));
    if (!PendingProfile || PendingProfile->FormatVersion!=1)
    { Error=FText::FromString(TEXT("Не удалось прочитать профиль. Сохранение не изменено."));return false; }
    if (bCreatingProfile) { PendingProfile->FirstName=First;PendingProfile->Surname=Last; }
    bNew=bCreatingProfile;
    return true;
}
bool ACityLiveRPPlayerController::Authenticate(const FString& Password,FText& Error)
{
    if (!PendingProfile || Password.Len()<6 || Password.Len()>64)
    { Error=FText::FromString(TEXT("Пароль должен содержать от 6 до 64 символов."));return false; }
    if (bCreatingProfile)
    {
        // Do not overwrite a profile if one appeared since the identity step.
        if (UGameplayStatics::DoesSaveGameExist(PendingSlot,0))
        { Error=FText::FromString(TEXT("Этот профиль уже создан. Вернитесь к вводу имени."));return false; }
        PendingProfile->SetPassword(Password);
        if (!UGameplayStatics::SaveGameToSlot(PendingProfile,PendingSlot,0))
        { Error=FText::FromString(TEXT("Не удалось сохранить профиль. Проверьте свободное место."));return false; }
    }
    else if (!PendingProfile->CheckPassword(Password))
    { Error=FText::FromString(TEXT("Неверный пароль."));return false; }
    bCreatingProfile=false;
    ActiveProfile=PendingProfile;
    RememberedFirst=ActiveProfile->FirstName; RememberedLast=ActiveProfile->Surname;
    UCityLoginMemory* Memory=NewObject<UCityLoginMemory>(this);
    Memory->FirstName=RememberedFirst;Memory->Surname=RememberedLast;
    UGameplayStatics::SaveGameToSlot(Memory,TEXT("CityV2_LastLogin"),0);
    return true;
}
bool ACityLiveRPPlayerController::HasSavedPosition() const
{
    return ActiveProfile && ActiveProfile->bHasLastPosition && ActiveProfile->LastMap==CurrentMapId() &&
        !ActiveProfile->LastPosition.ContainsNaN() && FMath::IsFinite(ActiveProfile->LastYaw);
}
bool ACityLiveRPPlayerController::SafeWalkingPosition(const FVector& Near,FVector& Out,bool bSearch) const
{
    if (Near.ContainsNaN()) return false;
    const ACharacter* PlayerCharacter=Cast<ACharacter>(GetPawn());
    const float Half=PlayerCharacter ? PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
    const float Radius=PlayerCharacter ? PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 42.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CitySafeResume),false,GetPawn());
    for (int32 Index=0;Index<(bSearch ? 17 : 1);++Index)
    {
        FVector Candidate=Near;
        if (Index>0) Candidate+=FRotator(0,(Index-1)*45.f,0).Vector()*(Index<=8 ? 150.f : 350.f);
        FHitResult Hit;
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Candidate+FVector(0,0,100),Candidate-FVector(0,0,350),ECC_Visibility,Params) ||
            Hit.ImpactNormal.Z<.8f) continue;
        Candidate.Z=Hit.ImpactPoint.Z+Half+4.f;
        if (GetWorld()->OverlapBlockingTestByChannel(Candidate,FQuat::Identity,ECC_Pawn,
            FCollisionShape::MakeCapsule(Radius,Half),Params)) continue;
        Out=Candidate;return true;
    }
    return false;
}
bool ACityLiveRPPlayerController::EnterCity(bool bLastPosition,FText& Error)
{
    ACityLiveRPCharacter* PlayerCharacter=Cast<ACityLiveRPCharacter>(GetPawn());
    if (!ActiveProfile || !PlayerCharacter || !bHasInitialSpawn)
    { Error=FText::FromString(TEXT("Город ещё запускается. Нажмите ещё раз через секунду."));return false; }
    FVector Target=InitialSpawn.GetLocation();
    float Yaw=InitialSpawn.Rotator().Yaw;
    if(bLastPosition)
    {
        if(!HasSavedPosition() || !SafeWalkingPosition(ActiveProfile->LastPosition,Target,true))
        { Error=FText::FromString(TEXT("Место выхода недоступно. Выберите начальное место спавна."));return false; }
        Yaw=ActiveProfile->LastYaw;
    }
    if (!PlayerCharacter->TeleportTo(Target,FRotator(0,Yaw,0)))
    { Error=FText::FromString(TEXT("Место занято. Выберите другое место спавна."));return false; }
    PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
    SetControlRotation(FRotator(0,Yaw,0)); SetViewTarget(PlayerCharacter);
    bEnteredCity=true;
    CloseCityMenu(); SaveProfile();
    return true;
}
void ACityLiveRPPlayerController::SaveProfile()
{
    if(!bEnteredCity || !ActiveProfile || !GetPawn() || IsVehicleEntryActive()) return;
    if (const ACharacter* PlayerCharacter=Cast<ACharacter>(GetPawn()))
        if (PlayerCharacter->GetCharacterMovement()->IsFalling()) return;
    FVector Safe;
    if (SafeWalkingPosition(GetPawn()->GetActorLocation(),Safe,false))
    {
        ActiveProfile->LastPosition=Safe; ActiveProfile->LastYaw=GetPawn()->GetActorRotation().Yaw;
        ActiveProfile->LastMap=CurrentMapId();ActiveProfile->bHasLastPosition=true;
    }
    if(!UGameplayStatics::SaveGameToSlot(ActiveProfile,UCityPlayerProfile::Slot(ActiveProfile->FirstName,ActiveProfile->Surname),0))
        UE_LOG(LogTemp,Warning,TEXT("CityV2: profile save failed"));
}
void ACityLiveRPPlayerController::CreateParkingMarkers()
{
    // Police parking is 38 m from the building centre. Derive the FBX axis from
    // actual road collisions rather than assuming Blender/Unreal handedness.
    FBox Police(ForceInit);
    for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
    {
        const UStaticMeshComponent* Mesh=It->GetStaticMeshComponent();
        if(Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName().Contains(TEXT("PD_P_")))
        { FVector Origin,Extent;It->GetActorBounds(false,Origin,Extent);Police+=FBox(Origin-Extent,Origin+Extent); }
    }
    if(!Police.IsValid) {UE_LOG(LogTemp,Warning,TEXT("CityV2: district police landmark missing; parking not placed"));return;}
    const FVector Center=Police.GetCenter();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityParkingPlacement),false);
    for(int32 Direction=0;Direction<4;++Direction)
    {
        const FVector Outward=FRotator(0,Direction*90.f,0).Vector();
        const FVector Side=FRotator(0,Direction*90.f+90.f,0).Vector();
        for(float Offset : {-1600.f,0.f,1600.f})
        {
            FVector Point=Center+Outward*3800.f+Side*Offset;
            FHitResult Hit;
            if(!GetWorld()->LineTraceSingleByChannel(Hit,FVector(Point.X,Point.Y,5000),FVector(Point.X,Point.Y,-500),ECC_Visibility,Params))continue;
            const AStaticMeshActor* Road=Cast<AStaticMeshActor>(Hit.GetActor());
            if(!Road || !Road->GetStaticMeshComponent()->GetStaticMesh() ||
                !Road->GetStaticMeshComponent()->GetStaticMesh()->GetName().Contains(TEXT("PD_R_")) || Hit.ImpactNormal.Z<.99f)continue;
            FVector Flat;
            if(!ACitySedan::FindFlatSpawn(GetWorld(),Hit.ImpactPoint,Side.Rotation().Yaw,GetPawn(),Flat))continue;
            Point.Z=Hit.ImpactPoint.Z+110.f;
            if(ACityParkingMarker* Marker=GetWorld()->SpawnActor<ACityParkingMarker>(Point,Outward.Rotation()))
                ParkingMarkers.Add(Marker);
        }
        if(!ParkingMarkers.IsEmpty()) break;
    }
    UE_LOG(LogTemp,Display,TEXT("CityV2: %d parking markers placed"),ParkingMarkers.Num());
}
void ACityLiveRPPlayerController::CheckParking()
{
    if(!bEnteredCity || !GetPawn())return;
    if(SuppressedParking.IsValid() && FVector::DistSquared2D(GetPawn()->GetActorLocation(),SuppressedParking->GetActorLocation())>FMath::Square(260.f))
        SuppressedParking.Reset();
    if(IsCityMenuOpen() || !Cast<ACityLiveRPCharacter>(GetPawn()) || GetSeatedSedan() || IsVehicleEntryActive())return;
    for(ACityParkingMarker* Marker:ParkingMarkers)
        if(IsValid(Marker) && Marker!=SuppressedParking.Get() &&
            FVector::DistSquared2D(GetPawn()->GetActorLocation(),Marker->GetActorLocation())<FMath::Square(150.f) &&
            FMath::Abs(GetPawn()->GetActorLocation().Z-Marker->GetActorLocation().Z)<160.f)
        {
            CurrentParking=Marker;SuppressedParking=Marker;OpenCityMenu(true);return;
        }
}
bool ACityLiveRPPlayerController::SpawnParkingCar(FText& Error)
{
    if(!bEnteredCity || !CurrentParking.IsValid() || !Cast<ACityLiveRPCharacter>(GetPawn()) ||
        FVector::DistSquared2D(GetPawn()->GetActorLocation(),CurrentParking->GetActorLocation())>FMath::Square(200.f))
    {Error=FText::FromString(TEXT("Подойдите к парковочной метке."));return false;}
    ACitySedan* NewCar=nullptr;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    for(float Distance:{650.f,1000.f,1500.f})
    {
        for(int32 Direction=0;Direction<12;++Direction)
        {
            const float Yaw=Direction*30.f;
            const FVector Candidate=CurrentParking->GetActorLocation()+FRotator(0,Yaw,0).Vector()*Distance;
            FVector Flat;
            if(!ACitySedan::FindFlatSpawn(GetWorld(),Candidate,Yaw,GetPawn(),Flat))continue;
            NewCar=GetWorld()->SpawnActor<ACitySedan>(ACitySedan::StaticClass(),Flat,FRotator(0,Yaw,0),Params);
            if(NewCar)break;
        }
        if(NewCar)break;
    }
    if(!NewCar) {Error=FText::FromString(TEXT("Рядом нет свободного места для автомобиля. Попробуйте другую метку."));return false;}
    if(!NewCar->TryEnter(this,false,true))
    {NewCar->Destroy();Error=FText::FromString(TEXT("Не удалось начать посадку. Проверьте свободный проход и монтажи."));return false;}
    if(IsValid(TestSedan)) TestSedan->Destroy();
    TestSedan=NewCar;
    CloseCityMenu();SaveProfile();return true;
}
