#pragma once
#include "CoreMinimal.h"
#include "CityVehicleSeat.generated.h"
class USceneComponent;

/** Coordinates in vehicle-root centimetres, never character scale factors. */
USTRUCT(BlueprintType)
struct FCityVehicleSeat
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Pelvis=FVector(-25,-40,-12);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector SeatEdge=FVector(-25,-76,-12);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Approach=FVector(-25,-135,0);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftFootFloor=FVector(43,-51,-43);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector RightFootFloor=FVector(43,-29,-43);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector LeftHand=FVector(39,-52,24);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector RightHand=FVector(39,-28,24);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector HandleOffset=FVector(-90,-5,38);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector DoorPullOffset=FVector(-75,5,35);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Side=-1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 DoorIndex=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float OpenAngle=60;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float RoofInnerZ=78;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float SillTopZ=-24;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float DrivingLeanDegrees=15;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName SeatSocket=NAME_None;
};

USTRUCT(BlueprintType)
struct FCityVehicleBodyPose
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bActive=false;
    UPROPERTY() TObjectPtr<USceneComponent> MovementFrame=nullptr;
    UPROPERTY() FTransform SampledFrame;
    UPROPERTY(BlueprintReadOnly) float LegWeight=0;
    UPROPERTY(BlueprintReadOnly) float HandWeight=0;
    UPROPERTY(BlueprintReadOnly) float LeftHandWeight=-1;
    UPROPERTY(BlueprintReadOnly) float RightHandWeight=-1;
    UPROPERTY(BlueprintReadOnly) float TorsoCorrection=0;
    UPROPERTY(BlueprintReadOnly) FVector Pelvis=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector LeftFoot=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector RightFoot=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector LeftHand=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector RightHand=FVector::ZeroVector;
};
