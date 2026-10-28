#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "CityCrouchMeshComponent.generated.h"

/** Applies a procedural crouch to the evaluated animation before publishing bones. */
UCLASS()
class UCityCrouchMeshComponent : public USkeletalMeshComponent
{
    GENERATED_BODY()

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void FinalizeBoneTransform() override;
    float GetCrouchAlpha() const { return CrouchAlpha; }
    void SetVehicleEntryPose(float Sit, float Reach, FVector Handle, bool bRightHand);
    void SetVehicleSeatLegs(float Alpha, FVector LeftFoot, FVector RightFoot);

    UPROPERTY(EditAnywhere, Category="CityLive|Crouch", meta=(ClampMin="0", ClampMax="60"))
    float PelvisDrop = 52.f;

    UPROPERTY(EditAnywhere, Category="CityLive|Crouch", meta=(ClampMin="1", ClampMax="20"))
    float BlendSpeed = 10.f;

private:
    float CrouchAlpha = 0.f;
    float EntrySit = 0.f;
    float EntryReach = 0.f;
    FVector EntryHandle = FVector::ZeroVector;
    bool bEntryRightHand = false;
    float SeatLegAlpha = 0.f;
    FVector SeatFootLeft = FVector::ZeroVector;
    FVector SeatFootRight = FVector::ZeroVector;
    void ApplyCrouch();
    void ApplyEntryHand();
    void ApplySeatLegs();
};
