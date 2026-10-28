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

    UPROPERTY(EditAnywhere, Category="CityLive|Crouch", meta=(ClampMin="0", ClampMax="40"))
    float PelvisDrop = 30.f;

    UPROPERTY(EditAnywhere, Category="CityLive|Crouch", meta=(ClampMin="1", ClampMax="20"))
    float BlendSpeed = 10.f;

private:
    float CrouchAlpha = 0.f;
    void ApplyCrouch();
};
