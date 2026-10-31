#pragma once
#include "CoreMinimal.h"
#include "CityVehicleSeat.h"
#include "Components/SceneComponent.h"
#include "ReferenceSkeleton.h"
#include "ProceduralCrouchMath.h"
namespace CityBoneIK
{
inline bool InBranch(const FReferenceSkeleton& Ref,int32 Bone,int32 Root)
{
    while(Bone!=INDEX_NONE) {if(Bone==Root)return true;Bone=Ref.GetParentIndex(Bone);}return false;
}
inline void Rotate(TArray<FTransform>& Pose,const FReferenceSkeleton& Ref,int32 Root,const FQuat& Delta)
{
    const FVector Pivot=Pose[Root].GetLocation();
    for(int32 Bone=Root;Bone<Pose.Num();++Bone) if(InBranch(Ref,Bone,Root))
    {
        Pose[Bone].SetLocation(Pivot+Delta.RotateVector(Pose[Bone].GetLocation()-Pivot));
        Pose[Bone].SetRotation((Delta*Pose[Bone].GetRotation()).GetNormalized());
    }
}
inline void Pelvis(TArray<FTransform>& Pose,const FReferenceSkeleton& Ref,const FVector& Target)
{
    const int32 Root=Ref.FindBoneIndex(TEXT("pelvis"));if(!Pose.IsValidIndex(Root))return;
    const FVector Delta=Target-Pose[Root].GetLocation();
    for(int32 Bone=Root;Bone<Pose.Num();++Bone)if(InBranch(Ref,Bone,Root))Pose[Bone].AddToTranslation(Delta);
}
// Analytic two-bone IK: clamp reach, rotate branches, never stretch bones or scale.
inline void Limb(TArray<FTransform>& Pose,const FReferenceSkeleton& Ref,FName A,FName B,FName C,
    FVector Target,const FVector& Pole,float Alpha)
{
    const int32 Root=Ref.FindBoneIndex(A),Joint=Ref.FindBoneIndex(B),End=Ref.FindBoneIndex(C);
    if(!Pose.IsValidIndex(Root)||!Pose.IsValidIndex(Joint)||!Pose.IsValidIndex(End))return;
    Target=FMath::Lerp(Pose[End].GetLocation(),Target,FMath::Clamp(Alpha,0.f,1.f));
    const FVector Start=Pose[Root].GetLocation();
    const double Upper=(Pose[Joint].GetLocation()-Start).Size(),Lower=(Pose[End].GetLocation()-Pose[Joint].GetLocation()).Size();
    FVector Direction=(Target-Start).GetSafeNormal();if(Direction.IsNearlyZero()||Upper+Lower<.1)return;
    const double Reach=FMath::Clamp((Target-Start).Size(),FMath::Abs(Upper-Lower)+.01,Upper+Lower-.01);
    Target=Start+Direction*Reach;
    FVector Bend=Pole-Direction*FVector::DotProduct(Pole,Direction);
    if(Bend.IsNearlyZero())Bend=FVector::CrossProduct(Direction,FVector::RightVector);
    if(Bend.IsNearlyZero())return;
    double Along,Height;if(!CityLiveCrouch::KneeOffset(Upper,Lower,Reach,Along,Height))return;
    const FVector Desired=Start+Direction*Along+Bend.GetSafeNormal()*Height;
    const FQuat OriginalEnd=Pose[End].GetRotation();
    Rotate(Pose,Ref,Root,FQuat::FindBetweenNormals((Pose[Joint].GetLocation()-Start).GetSafeNormal(),(Desired-Start).GetSafeNormal()));
    const FVector Pivot=Pose[Joint].GetLocation();
    Rotate(Pose,Ref,Joint,FQuat::FindBetweenNormals((Pose[End].GetLocation()-Pivot).GetSafeNormal(),(Target-Pivot).GetSafeNormal()));
    Rotate(Pose,Ref,End,OriginalEnd*Pose[End].GetRotation().Inverse());
}
inline void Body(TArray<FTransform>& Pose,const FReferenceSkeleton& Ref,const FTransform& Component,
    const FTransform& Actor,const FCityVehicleBodyPose& Raw)
{
    FCityVehicleBodyPose Goals=Raw;
    if(IsValid(Goals.MovementFrame.Get()))
    {
        // Refresh sampled world targets after vehicle physics/attachment updates.
        // Otherwise the post-physics visible skin would chase last frame's seat.
        const FTransform Current=Goals.MovementFrame->GetComponentTransform();
        auto Update=[&](FVector& Point){Point=Current.TransformPosition(Goals.SampledFrame.InverseTransformPosition(Point));};
        Update(Goals.Pelvis);Update(Goals.LeftFoot);Update(Goals.RightFoot);Update(Goals.LeftHand);Update(Goals.RightHand);
    }
    if(!Goals.bActive || Pose.Num()>Ref.GetNum())return;
    Pelvis(Pose,Ref,Component.InverseTransformPosition(Goals.Pelvis));
    const FVector Forward=Component.InverseTransformVectorNoScale(Actor.GetUnitAxis(EAxis::X));
    const FVector Down=Component.InverseTransformVectorNoScale(-FVector::UpVector);
    const FVector Side=Component.InverseTransformVectorNoScale(Actor.GetUnitAxis(EAxis::Y));
    Limb(Pose,Ref,TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l"),Component.InverseTransformPosition(Goals.LeftFoot),Forward,Goals.LegWeight);
    Limb(Pose,Ref,TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r"),Component.InverseTransformPosition(Goals.RightFoot),Forward,Goals.LegWeight);
    Limb(Pose,Ref,TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),Component.InverseTransformPosition(Goals.LeftHand),Down-Side*.4f,Goals.LeftHandWeight>=0 ? Goals.LeftHandWeight : Goals.HandWeight);
    Limb(Pose,Ref,TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r"),Component.InverseTransformPosition(Goals.RightHand),Down+Side*.4f,Goals.RightHandWeight>=0 ? Goals.RightHandWeight : Goals.HandWeight);
}
}
