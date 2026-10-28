// Copyright Epic Games, Inc. All Rights Reserved.


#include "CityLiveRPPlayerController.h"
#include "SCityMobileControls.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CityLiveRP.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ACityLiveRPPlayerController::BeginPlay()
{
	Super::BeginPlay();

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
}

void ACityLiveRPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

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

void ACityLiveRPPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
