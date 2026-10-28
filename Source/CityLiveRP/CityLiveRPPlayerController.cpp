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
}

void ACityLiveRPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ACityLiveRPPlayerController::InteractWithSedan);

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
        if (HasAuthority() && !TestSedan)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
            FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(CitySedanSpawn), false, InPawn);
            for (const FVector& Offset : {InPawn->GetActorForwardVector() * 650.f,
                InPawn->GetActorRightVector() * 650.f, -InPawn->GetActorRightVector() * 650.f,
                -InPawn->GetActorForwardVector() * 650.f})
            {
                const FVector Candidate = InPawn->GetActorLocation() + Offset;
                FHitResult FloorHit;
                if (!GetWorld()->LineTraceSingleByChannel(FloorHit, Candidate + FVector(0, 0, 200),
                    Candidate - FVector(0, 0, 500), ECC_Visibility, FloorParams)) continue;
                TestSedan = GetWorld()->SpawnActor<ACitySedan>(ACitySedan::StaticClass(),
                    FloorHit.ImpactPoint + FVector(0, 0, 65), InPawn->GetActorRotation(), Params);
                if (TestSedan) break;
            }
            if (!TestSedan) UE_LOG(LogCityLiveRP, Warning, TEXT("No free floor location for test sedan near PlayerStart."));
        }
    }
}

bool ACityLiveRPPlayerController::CanInteractWithSedan() const
{
    return Cast<ACitySedan>(GetPawn()) || (IsValid(TestSedan) && TestSedan->CanEnter(this));
}

void ACityLiveRPPlayerController::InteractWithSedan()
{
    if (CityMobileControls.IsValid()) CityMobileControls->ResetControls();
    if (ACitySedan* Vehicle = Cast<ACitySedan>(GetPawn()))
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
