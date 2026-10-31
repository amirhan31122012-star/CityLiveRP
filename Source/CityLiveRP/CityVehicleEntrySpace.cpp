#include "CitySedan.h"
#include "CityLiveRPCharacter.h"
#include "CityEntryMotion.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <initializer_list>

void ACitySedan::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    for(int32 I=0;I<SeatAnchors.Num()&&I<Seating.Num();++I)
        SeatAnchors[I]->SetRelativeLocation(Seating[I].Pelvis);
}
const FCityVehicleSeat& ACitySedan::SeatSettings(int32 Seat) const
{
    static const FCityVehicleSeat Default;
    return Seating.IsValidIndex(Seat+1) ? Seating[Seat+1] : Default;
}
FVector ACitySedan::SeatPelvisWorld(int32 Seat) const
{
    const FCityVehicleSeat& Definition=SeatSettings(Seat);
    return SeatAnchors.IsValidIndex(Seat+1) ? SeatAnchors[Seat+1]->GetComponentLocation() :
        GetActorTransform().TransformPosition(Definition.Pelvis);
}
bool ACitySedan::ValidateEntrySpace(ACityLiveRPCharacter* Occupant) const
{
    if(!Occupant || !GetActorScale3D().Equals(FVector::OneVector,.0001f))return false;
    const FCityVehicleSeat& Definition=SeatSettings(EntrySeat);
    if(!Seating.IsValidIndex(EntrySeat+1)||!DoorPivots.IsValidIndex(Definition.DoorIndex)||!SeatAnchors.IsValidIndex(EntrySeat+1) ||
        !FMath::IsNearlyEqual(FMath::Abs(Definition.Side),1.f,.001f) || Definition.OpenAngle<45 || Definition.OpenAngle>90)return false;
    const FName Doors[]={TEXT("Door_Front_Left"),TEXT("Door_Front_Right"),TEXT("Door_Rear_Left"),TEXT("Door_Rear_Right")};
    if(Definition.DoorIndex<0 || Definition.DoorIndex>=4 ||
        (ImportedSedan->IsVisible() && !RestVehicleBones.Contains(Doors[Definition.DoorIndex])))return false;
    const float Height=Occupant->BodyHeightAbovePelvis();
    const float SeatZ=static_cast<float>(GetActorTransform().InverseTransformPosition(SeatPelvisWorld(EntrySeat)).Z);
    if(SeatZ+Height*FMath::Cos(FMath::DegreesToRadians(Definition.DrivingLeanDegrees))>Definition.RoofInnerZ-1.f)return false;
    return DoorMotionClear(0.f,-Definition.Side*Definition.OpenAngle,Occupant);
}
bool ACitySedan::DoorMotionClear(float From,float To,ACityLiveRPCharacter* Occupant) const
{
    if(FMath::IsNearlyEqual(From,To,.001f))return true;
    const FCityVehicleSeat& Definition=SeatSettings(EntrySeat);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityDoorClearance),false,this);Params.AddIgnoredActor(Occupant);
    FTransform Hinge=DoorPivots[Definition.DoorIndex]->GetComponentTransform();Hinge.SetRotation(GetActorQuat());
    for(float HeightZ:{-35.f,5.f,45.f,65.f})
    for(float Length:{-110.f,-55.f})
    {
        const FVector Local(Length,0,HeightZ);
        FVector Previous=Hinge.TransformPosition(FRotator(0,From,0).RotateVector(Local));
        const int32 Steps=FMath::Max(1,FMath::CeilToInt(FMath::Abs(To-From)/4.f));
        for(int32 Step=1;Step<=Steps;++Step)
        {
            const float Angle=FMath::Lerp(From,To,static_cast<float>(Step)/Steps);
            const FVector Next=Hinge.TransformPosition(FRotator(0,Angle,0).RotateVector(Local));
            FHitResult Hit;
            if(GetWorld()->SweepSingleByChannel(Hit,Previous,Next,FQuat::Identity,ECC_Visibility,
                FCollisionShape::MakeSphere(3.f),Params))return false;
            Previous=Next;
        }
    }
    return true;
}
bool ACitySedan::MoveEntryActor(const FVector& Position,const FRotator& Rotation)
{
    if(!EnteringCharacter)return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityEntryBodySweep),false,this);
    Params.AddIgnoredActor(EnteringCharacter);
    const FVector Previous=EnteringCharacter->GetActorLocation();
    FHitResult Hit;
    const auto BodyState=bExiting ? CityLiveEntry::Exit(EntryTime) : CityLiveEntry::Enter(EntryTime);
    if(EntryTime<0.f || BodyState.Sit<.05f)
    {
        if(GetWorld()->SweepSingleByChannel(Hit,Previous,Position,FQuat::Identity,ECC_Pawn,
            FCollisionShape::MakeCapsule(EnteringCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius(),
                EnteringCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-2.f),Params))return false;
    }
    else
    {
        const FCityVehicleSeat& Definition=SeatSettings(EntrySeat);
        const auto P=bExiting ? CityLiveEntry::Exit(EntryTime) : CityLiveEntry::Enter(EntryTime);
        const float Lean=Definition.DrivingLeanDegrees*P.Sit+(42.f-Definition.DrivingLeanDegrees)*P.Lean;
        const float Height=EnteringCharacter->BodyHeightAbovePelvis();
        const FVector Pelvis=PendingBodyPose.Pelvis;
        const FVector Top=Pelvis+Rotation.Vector()*(Height*FMath::Sin(FMath::DegreesToRadians(Lean)))+
            FVector::UpVector*(Height*FMath::Cos(FMath::DegreesToRadians(Lean)));
        const FVector LocalTop=GetActorTransform().InverseTransformPosition(Top);
        // Keep the full-sized head under the roof whenever it crosses the aperture.
        if(FMath::Abs(LocalTop.Y)<73.f && LocalTop.X>-140.f && LocalTop.X<75.f && LocalTop.Z>Definition.RoofInnerZ)return false;
        for(float Fraction:{0.f,.5f,.85f})
        {
            const FVector Next=FMath::Lerp(Pelvis,Top,Fraction);
            const FVector Last=Next+(Previous-Position);
            if(GetWorld()->SweepSingleByChannel(Hit,Last,Next,FQuat::Identity,ECC_Pawn,
                FCollisionShape::MakeSphere(Fraction==0.f ? 16.f : 12.f),Params))return false;
        }
    }
    if(EntryTime>=0 && LastBodyPose.bActive && LastBodyPose.LegWeight>.5f && PendingBodyPose.LegWeight>.5f)
    {
        const float Radius=FMath::Clamp(EnteringCharacter->BodyAnkleHeight()*.45f,2.f,5.f);
        for(int32 Side=0;Side<2;++Side)
        {
            const FVector Last=Side ? LastBodyPose.RightFoot : LastBodyPose.LeftFoot;
            const FVector Next=Side ? PendingBodyPose.RightFoot : PendingBodyPose.LeftFoot;
            if(GetWorld()->SweepSingleByChannel(Hit,Last,Next,FQuat::Identity,ECC_Pawn,
                FCollisionShape::MakeSphere(Radius),Params))return false;
        }
    }
    // Continuous per-frame coordinated movement, never TeleportPhysics or a snap.
    const FVector Delta=Position-Previous;
    EnteringCharacter->SetActorLocationAndRotation(Position,Rotation,false,nullptr,ETeleportType::None);
    EnteringCharacter->GetCharacterMovement()->Velocity=EntryTime<0 && GetWorld()->GetDeltaSeconds()>0 ?
        Delta/GetWorld()->GetDeltaSeconds() : FVector::ZeroVector;
    return true;
}
