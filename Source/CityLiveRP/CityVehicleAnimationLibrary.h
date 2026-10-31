#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CityVehicleAnimationLibrary.generated.h"
class USkeletalMesh;

USTRUCT(BlueprintType)
struct FCityReferenceBone
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName BoneName;
    UPROPERTY(BlueprintReadOnly) FName ParentName;
    UPROPERTY(BlueprintReadOnly) FTransform ComponentTransform;
};

/** Native reference-pose access for Blueprint and the animation preparation script.
 * No transient actors, editor-only modules or live animation state are required. */
UCLASS()
class CITYLIVERP_API UCityVehicleAnimationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="CityLive|Vehicle")
    static TArray<FCityReferenceBone> GetReferencePose(USkeletalMesh* Mesh);
};
