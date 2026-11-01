#include "CityCourierScooter.h"
#include "CityLiveRPCharacter.h"
#include "CityEntryMotion.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

ACityCourierScooter::ACityCourierScooter()
{
    bCourierScooter=true;Tags.Add(TEXT("CityCourierScooter"));
    auto& Seat=Seating[0];Seat.Pelvis=FVector(-20,0,30);Seat.SeatEdge=Seat.Pelvis;
    Seat.Approach=FVector(-20,-100,0);Seat.LeftFootFloor=FVector(15,-24,-43);Seat.RightFootFloor=FVector(15,24,-43);
    Seat.LeftHand=FVector(45,-30,55);Seat.RightHand=FVector(45,30,55);Seat.RoofInnerZ=250;Seat.OpenAngle=0;
}
// A dedicated mount/dismount: no invisible car door or instant seating on the scooter.
void ACitySedan::UpdateScooterEntry(float DeltaTime)
{
    if(!EnteringCharacter || !EnteringPlayer.IsValid()){CancelEntry();return;}
    const float Previous=EntryTime;EntryTime=CityLiveEntry::AdvanceTime(EntryTime,DeltaTime,bReversingEntry);
    const float Alpha=CityLiveEntry::Blend(EntryTime,0,2.8f);
    const float Sit=bExiting?1-Alpha:Alpha;
    const auto& Seat=SeatSettings(-1);
    const FQuat Facing=FQuat::Slerp(ApproachRotation.Quaternion(),GetActorQuat(),Sit);
    const FVector Standing=EntryApproach+Facing.RotateVector(EnteringCharacter->ReferencePelvisOffset());
    const FVector Pelvis=FMath::Lerp(Standing,SeatPelvisWorld(-1),Sit);
    PendingBodyPose=FCityVehicleBodyPose();PendingBodyPose.bActive=true;PendingBodyPose.Pelvis=Pelvis;
    PendingBodyPose.LegWeight=Sit;PendingBodyPose.HandWeight=Sit;
    PendingBodyPose.LeftFoot=GetActorTransform().TransformPosition(Seat.LeftFootFloor)+GetActorUpVector()*EnteringCharacter->BodyAnkleHeight();
    PendingBodyPose.RightFoot=GetActorTransform().TransformPosition(Seat.RightFootFloor)+GetActorUpVector()*EnteringCharacter->BodyAnkleHeight();
    PendingBodyPose.LeftHand=GetActorTransform().TransformPosition(Seat.LeftHand);PendingBodyPose.RightHand=GetActorTransform().TransformPosition(Seat.RightHand);
    if(!bMontageStarted){bMontageStarted=EnteringCharacter->PlayVehicleAnimation(bExiting ? TEXT("ScooterExit") : TEXT("ScooterMount"));}
    EnteringCharacter->SetVehicleAnimationTime(EntryTime);
    if(!MoveEntryActor(Pelvis-Facing.RotateVector(EnteringCharacter->ReferencePelvisOffset()),Facing.Rotator()))
    {EntryTime=Previous;bReversingEntry=true;return;}
    EnteringCharacter->SetVehicleBodyPose(PendingBodyPose);
    if(bReversingEntry&&EntryTime<=0)
    {
        if(bExiting)CompleteEntry();else{auto* Person=EnteringCharacter.Get();auto* Player=EnteringPlayer.Get();EnteringCharacter=nullptr;EnteringPlayer.Reset();RestoreWalking(Person,Player);SetEntryPhysicsPaused(false);}return;
    }
    if(EntryTime>=2.8f){if(bExiting)CompleteExit();else CompleteEntry();}
}
