#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CitySedan.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class ACityLiveRPCharacter;
class APlayerController;
class USceneComponent;

/** Local arcade prototype. No external model or vehicle plugin required. */
UCLASS()
class ACitySedan : public APawn
{
    GENERATED_BODY()
public:
    ACitySedan();
    virtual void Tick(float DeltaTime) override;
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

private:
    UPROPERTY() TObjectPtr<UBoxComponent> CollisionBox;
    UPROPERTY() TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY() TObjectPtr<UCameraComponent> VehicleCamera;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
    UPROPERTY() TObjectPtr<ACityLiveRPCharacter> Driver;
    UPROPERTY() TObjectPtr<ACityLiveRPCharacter> Passenger;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> DoorPivots;
    UPROPERTY() TObjectPtr<ACityLiveRPCharacter> EnteringCharacter;
    TWeakObjectPtr<APlayerController> EnteringPlayer;
    FVector EntryStart = FVector::ZeroVector;
    FVector EntryApproach = FVector::ZeroVector;
    FRotator EntryStartRotation = FRotator::ZeroRotator;
    float EntryTime = 0.f;
    float EntryCameraBlend = 0.f;
    bool bEnteringPassenger = false;
    void UpdateEntry(float DeltaTime);
    void CompleteEntry();
    FVector HandleLocation() const;
    float Steering = 0.f;
    float SmoothedSteering = 0.f;
    float Speed = 0.f;
    float FallSpeed = 0.f;
    bool bThrottle = false;
    bool bBrake = false;
};
