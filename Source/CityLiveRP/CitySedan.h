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

/** Local arcade prototype. No external model or vehicle plugin required. */
UCLASS()
class ACitySedan : public APawn
{
    GENERATED_BODY()
public:
    ACitySedan();
    virtual void Tick(float DeltaTime) override;
    bool CanEnter(const APlayerController* Player) const;
    bool TryEnter(APlayerController* Player);
    bool TryExit(APlayerController* Player);
    void SetSteering(float Value) { Steering = FMath::Clamp(Value, -1.f, 1.f); }
    void SetThrottle(bool bHeld) { bThrottle = bHeld; }
    void SetBrake(bool bHeld) { bBrake = bHeld; }
    void ResetInput();

private:
    UPROPERTY() TObjectPtr<UBoxComponent> CollisionBox;
    UPROPERTY() TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY() TObjectPtr<UCameraComponent> VehicleCamera;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
    UPROPERTY() TObjectPtr<ACityLiveRPCharacter> Driver;
    float Steering = 0.f;
    float SmoothedSteering = 0.f;
    float Speed = 0.f;
    float FallSpeed = 0.f;
    bool bThrottle = false;
    bool bBrake = false;
};
