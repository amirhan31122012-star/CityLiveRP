#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityParkingMarker.generated.h"

/** Torso-height parking sign. Controller checks proximity only five times a second. */
UCLASS()
class CITYLIVERP_API ACityParkingMarker : public AActor
{
    GENERATED_BODY()
public:
    ACityParkingMarker();
};
