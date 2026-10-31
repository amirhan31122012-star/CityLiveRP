// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CityLiveRPPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class SCityMobileControls;
class ACitySedan;
class ACityParkingMarker;
class UCityPlayerProfile;
class SCityV2Menu;
class SWidget;
class UCityDistrictGameplayComponent;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ACityLiveRPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    ACityLiveRPPlayerController();
    friend class UCityDistrictGameplayComponent;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="CityLive|District") TObjectPtr<UCityDistrictGameplayComponent> DistrictGameplay;
    bool IsCityMenuOpen() const { return CityMenu.IsValid(); }
    bool IsProfileActive() const { return ActiveProfile != nullptr; }
    int32 GetBalance() const;
    FString GetProfileName() const;
    FString RememberedFirst, RememberedLast;
    bool SelectIdentity(FString First, FString Last, bool& bNew, FText& Error);
    bool Authenticate(const FString& Password, FText& Error);
    bool HasSavedPosition() const;
    bool EnterCity(bool bLastPosition, FText& Error);
    bool SpawnParkingCar(FText& Error);
    void CloseCityMenu();
    void SaveProfile();
    bool CanInteractWithSedan() const;
    void InteractWithSedan();
    bool CanUsePassengerDoor() const;
    void UsePassengerDoor();
    ACitySedan* GetSeatedSedan() const;
    bool IsSedanDriver() const;
    bool IsVehicleEntryActive() const;
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	TSharedPtr<SCityMobileControls> CityMobileControls;
    UPROPERTY() TObjectPtr<ACitySedan> TestSedan;
    UPROPERTY() TObjectPtr<UCityPlayerProfile> PendingProfile;
    UPROPERTY() TObjectPtr<UCityPlayerProfile> ActiveProfile;
    UPROPERTY() TArray<TObjectPtr<ACityParkingMarker>> ParkingMarkers;
    TWeakObjectPtr<ACityParkingMarker> CurrentParking;
    TWeakObjectPtr<ACityParkingMarker> SuppressedParking;
    TSharedPtr<SCityV2Menu> CityMenu;
    TSharedPtr<SWidget> CityHud;
    FString PendingSlot;
    bool bCreatingProfile = false;
    bool bEnteredCity = false;
    FTransform InitialSpawn;
    bool bHasInitialSpawn = false;
    FTimerHandle ParkingTimer, SaveTimer;
    FDelegateHandle BackgroundSaveHandle, DeactivateSaveHandle;
    void InitializeV2Session();
    void OpenCityMenu(bool bGarage);
    void CreateParkingMarkers();
    void CheckParking();
    FString CurrentMapId() const;
    bool SafeWalkingPosition(const FVector& Near, FVector& Out, bool bSearch) const;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnRep_Pawn() override;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
