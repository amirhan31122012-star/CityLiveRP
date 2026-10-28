#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UObject/SoftObjectPtr.h"
#include "CitySedan.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class ACityLiveRPCharacter;
class APlayerController;
class USceneComponent;
class USkeletalMesh;
class UPoseableMeshComponent;

/** Local arcade prototype. No external model or vehicle plugin required. */
UCLASS()
class ACitySedan : public APawn
{
    GENERATED_BODY()
public:
    ACitySedan();
    static bool FindFlatSpawn(UWorld* World, const FVector& Candidate, float Yaw, AActor* Ignore, FVector& OutLocation);
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    bool CanEnter(const APlayerController* Player, bool bPassengerSeat = false) const;
    bool TryEnter(APlayerController* Player, bool bPassengerSeat = false);
    bool IsPassenger(const APlayerController* Player) const;
    bool TryExit(APlayerController* Player);
    void SetSteering(float Value) { Steering = FMath::Clamp(Value, -1.f, 1.f); }
    void SetThrottle(bool bHeld) { bThrottle = bHeld; }
    void SetBrake(bool bHeld) { bBrake = bHeld; }
    void ResetInput();
    bool IsEntering(const APlayerController* Player) const;
    void CancelEntry();

    UPROPERTY(EditDefaultsOnly, Category="CityLive|Vehicle Model")
    TSoftObjectPtr<USkeletalMesh> SedanMeshAsset;
    UPROPERTY(EditDefaultsOnly, Category="CityLive|Vehicle Model")
    TSoftObjectPtr<USkeletalMesh> FourDoorMeshAsset;

private:
    UPROPERTY() TObjectPtr<UBoxComponent> CollisionBox;
    UPROPERTY() TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY() TObjectPtr<UCameraComponent> VehicleCamera;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PrototypeParts;
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> ImportedSedan;
    TMap<FName, FTransform> RestVehicleBones;
    bool bDoorRigReady = true; // The primitive prototype has four moving panels.
    float WheelRollDegrees = 0.f;
    void UpdateImportedWheels(float DeltaTime);
    void TickVehiclePhysics(float DeltaTime, bool Gas, bool Brake);
    void SetEntryPhysicsPaused(bool bPaused);
    TArray<FVector> SuspensionAnchors;
    TArray<float> WheelSuspensionOffsets;
    float DriveLogTimer = 0.f;
    float RestTimer = 0.f;
    UPROPERTY() TObjectPtr<ACityLiveRPCharacter> Driver;
    UPROPERTY() TArray<TObjectPtr<ACityLiveRPCharacter>> Passengers;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> DoorPivots;
    UPROPERTY() TObjectPtr<ACityLiveRPCharacter> EnteringCharacter;
    TWeakObjectPtr<APlayerController> EnteringPlayer;
    FVector EntryStart = FVector::ZeroVector;
    FVector EntryApproach = FVector::ZeroVector;
    FRotator EntryStartRotation = FRotator::ZeroRotator;
    float EntryTime = 0.f;
    float EntryCameraBlend = 0.f;
    bool bEnteringPassenger = false;
    int32 EntrySeat = -1;
    bool bExiting = false;
    TArray<FVector> EntryRoute;
    float RouteLength = 0.f;
    float RouteDuration = 0.f;
    void UpdateEntry(float DeltaTime);
    void CompleteEntry();
    void CompleteExit();
    bool BuildDoorRoute(ACityLiveRPCharacter* Occupant);
    void UpdateSeatedPose(ACityLiveRPCharacter* Occupant, int32 Seat);
    int32 FindPassengerSeat(const APlayerController* Player) const;
    int32 GetPassengerSeat(const APlayerController* Player) const;
    FVector HandleLocation() const;
    float Steering = 0.f;
    float SmoothedSteering = 0.f;
    float Speed = 0.f;
    bool bThrottle = false;
    bool bBrake = false;
};
