// Copyright Epic Games, Inc. All Rights Reserved.


#include "CityLiveRPPlayerController.h"
#include <initializer_list>
#include "CityLiveRPCharacter.h"
#include "CitySedan.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "CollisionQueryParams.h"
#include "SCityMobileControls.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CityLiveRP.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "CityPlayerProfile.h"
#include "SCityV2Menu.h"
#include "Misc/CoreDelegates.h"
#include "TimerManager.h"

void ACityLiveRPPlayerController::BeginPlay()
{
	Super::BeginPlay();

    UE_LOG(LogCityLiveRP, Display, TEXT("CityLive RP controls v4: smooth continuous steering, runner icon and pawn-follow camera. Pawn: %s"),
        *GetNameSafe(GetPawn()));

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
        // Replace both the default two-stick interface and template mobile HUD.
        ActivateTouchInterface(nullptr);
        if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
        {
            SAssignNew(CityMobileControls, SCityMobileControls).Controller(this);
            Viewport->AddViewportWidgetForPlayer(GetLocalPlayer(), CityMobileControls.ToSharedRef(), 20);
        }

	}
    if (IsLocalPlayerController()) InitializeV2Session();
}

void ACityLiveRPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ACityLiveRPPlayerController::InteractWithSedan);
    InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ACityLiveRPPlayerController::UsePassengerDoor);

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ACityLiveRPPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ACityLiveRPPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (IsLocalController() && Cast<ACityLiveRPCharacter>(InPawn))
    {
        SetViewTarget(InPawn);
        if (!bHasInitialSpawn)
        {
            InitialSpawn = InPawn->GetActorTransform();
            bHasInitialSpawn = true;
        }
    }
}

bool ACityLiveRPPlayerController::CanInteractWithSedan() const
{
    return !IsCityMenuOpen() && bEnteredCity && (GetSeatedSedan() || (IsValid(TestSedan) && TestSedan->CanEnter(this)));
}

ACitySedan* ACityLiveRPPlayerController::GetSeatedSedan() const
{
    if (ACitySedan* Vehicle = Cast<ACitySedan>(GetPawn())) return Vehicle;
    if (GetPawn())
        if (ACitySedan* Vehicle = Cast<ACitySedan>(GetPawn()->GetAttachParentActor()))
            if (Vehicle->IsPassenger(this)) return Vehicle;
    return nullptr;
}

bool ACityLiveRPPlayerController::IsSedanDriver() const
{
    return Cast<ACitySedan>(GetPawn()) != nullptr;
}

bool ACityLiveRPPlayerController::IsVehicleEntryActive() const
{
    return IsValid(TestSedan) && TestSedan->IsEntering(this);
}

bool ACityLiveRPPlayerController::CanUsePassengerDoor() const
{
    return !IsCityMenuOpen() && bEnteredCity && (GetSeatedSedan() || (IsValid(TestSedan) && TestSedan->CanEnter(this, true)));
}

void ACityLiveRPPlayerController::UsePassengerDoor()
{
    if (IsCityMenuOpen() || !bEnteredCity || IsVehicleEntryActive()) return;
    if (GetSeatedSedan()) { InteractWithSedan(); return; }
    if (CityMobileControls.IsValid()) CityMobileControls->ResetControls();
    if (IsValid(TestSedan)) TestSedan->TryEnter(this, true);
}

void ACityLiveRPPlayerController::InteractWithSedan()
{
    if (IsCityMenuOpen() || !bEnteredCity || IsVehicleEntryActive()) return;
    if (CityMobileControls.IsValid()) CityMobileControls->ResetControls();
    if (ACitySedan* Vehicle = GetSeatedSedan())
    {
        if (!Vehicle->TryExit(this)) ClientMessage(TEXT("Остановите машину. Для выхода нужно свободное место рядом."));
    }
    else if (IsValid(TestSedan)) TestSedan->TryEnter(this);
}

void ACityLiveRPPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();
    if (IsLocalController() && Cast<ACityLiveRPCharacter>(GetPawn()))
    {
        SetViewTarget(GetPawn());
    }
}

void ACityLiveRPPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SaveProfile();
    GetWorldTimerManager().ClearTimer(ParkingTimer);
    GetWorldTimerManager().ClearTimer(SaveTimer);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundSaveHandle);
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateSaveHandle);
    if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
    {
        if (CityMenu.IsValid()) Viewport->RemoveViewportWidgetContent(CityMenu.ToSharedRef());
        if (CityHud.IsValid()) Viewport->RemoveViewportWidgetContent(CityHud.ToSharedRef());
    }
    CityMenu.Reset();
    CityHud.Reset();
    if (CityMobileControls.IsValid())
    {
        CityMobileControls->ResetControls();
        if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
        {
            Viewport->RemoveViewportWidgetForPlayer(GetLocalPlayer(), CityMobileControls.ToSharedRef());
        }
        CityMobileControls.Reset();
    }
    Super::EndPlay(EndPlayReason);
}
