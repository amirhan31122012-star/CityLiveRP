#include "CityLiveRPCharacter.h"
#include "CityCharacterSkinComponent.h"
#include "CityCrouchMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include <initializer_list>

namespace
{
FVector ReferencePelvis(const USkinnedMeshComponent* Component,FName Name=TEXT("pelvis"))
{
    const USkeletalMesh* Asset=Cast<USkeletalMesh>(Component->GetSkinnedAsset());
    if(!Asset)return FVector::ZeroVector;
    const FReferenceSkeleton& Ref=Asset->GetRefSkeleton();
    int32 Bone=Ref.FindBoneIndex(Name);if(Bone==INDEX_NONE)return FVector::ZeroVector;
    FTransform Result=Ref.GetRefBonePose()[Bone];
    for(Bone=Ref.GetParentIndex(Bone);Bone!=INDEX_NONE;Bone=Ref.GetParentIndex(Bone))Result=Result*Ref.GetRefBonePose()[Bone];
    return Result.GetLocation();
}
UAnimSequence* LoadClip(FName Name)
{
    const FString Path=TEXT("/Game/CityLiveCharacters/Vehicle/AS_City")+Name.ToString();
    return LoadObject<UAnimSequence>(nullptr,*(Path+TEXT(".AS_City")+Name.ToString()));
}
}
bool ACityLiveRPCharacter::HasVehicleAnimations() const
{
    for(const FName Name:{FName(TEXT("Enter")),FName(TEXT("Exit")),FName(TEXT("EnterRight")),FName(TEXT("ExitRight")),FName(TEXT("DriverIdle"))})
    {
        UAnimSequence* Clip=LoadClip(Name);
        if(!Clip || !GetMesh()->GetSkeletalMeshAsset() || Clip->GetSkeleton()!=GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())return false;
    }
    return GetMesh()->GetAnimInstance()!=nullptr;
}
bool ACityLiveRPCharacter::PlayVehicleAnimation(FName Clip,bool bRightDoor)
{
    UAnimInstance* Instance=GetMesh()->GetAnimInstance();
    const FName Name=bRightDoor && Clip!=FName(TEXT("DriverIdle")) ? FName(*(Clip.ToString()+TEXT("Right"))) : Clip;
    UAnimSequence* Sequence=LoadClip(Name);
    if(!Instance||!Sequence)return false;
    VehicleMontage=Instance->PlaySlotAnimationAsDynamicMontage(Sequence,TEXT("DefaultSlot"),.15f,.15f,1.f,1);
    if(!VehicleMontage)return false;
    // A single authoritative timeline drives pose sampling, actor travel, IK and doors.
    // Root motion is intentionally disabled in these clips; actor displacement is swept.
    Instance->Montage_SetPlayRate(VehicleMontage,0.f);
    return true;
}
void ACityLiveRPCharacter::SetVehicleAnimationTime(float Seconds)
{
    if(VehicleMontage && GetMesh()->GetAnimInstance())GetMesh()->GetAnimInstance()->Montage_SetPosition(
        VehicleMontage,FMath::Clamp(Seconds,0.f,FMath::Max(0.f,VehicleMontage->GetPlayLength()-.01f)));
}
FVector ACityLiveRPCharacter::ReferencePelvisOffset() const
{
    const USkinnedMeshComponent* Body=CharacterSkin && CharacterSkin->GetSkinnedAsset() ?
        static_cast<const USkinnedMeshComponent*>(CharacterSkin.Get()) : GetMesh();
    return GetActorTransform().InverseTransformPosition(Body->GetComponentTransform().TransformPosition(ReferencePelvis(Body)));
}
float ACityLiveRPCharacter::BodyHeightAbovePelvis() const
{
    const USkinnedMeshComponent* Body=CharacterSkin && CharacterSkin->GetSkinnedAsset() ?
        static_cast<const USkinnedMeshComponent*>(CharacterSkin.Get()) : GetMesh();
    const USkeletalMesh* Asset=Cast<USkeletalMesh>(Body->GetSkinnedAsset());
    return Asset ? static_cast<float>((Asset->GetBounds().Origin.Z+Asset->GetBounds().BoxExtent.Z-ReferencePelvis(Body).Z)*
        FMath::Abs(Body->GetComponentTransform().GetScale3D().Z)) : 90.f;
}
float ACityLiveRPCharacter::BodyAnkleHeight() const
{
    const USkinnedMeshComponent* Body=CharacterSkin && CharacterSkin->GetSkinnedAsset() ?
        static_cast<const USkinnedMeshComponent*>(CharacterSkin.Get()) : GetMesh();
    const USkeletalMesh* Asset=Cast<USkeletalMesh>(Body->GetSkinnedAsset());
    return Asset ? FMath::Max(1.f,static_cast<float>((ReferencePelvis(Body,TEXT("foot_l")).Z-
        (Asset->GetBounds().Origin.Z-Asset->GetBounds().BoxExtent.Z))*FMath::Abs(Body->GetComponentTransform().GetScale3D().Z))) : 8.f;
}
void ACityLiveRPCharacter::SetVehicleBodyPose(const FCityVehicleBodyPose& Pose)
{
    if(CharacterSkin)CharacterSkin->SetVehicleBodyPose(Pose);
    if(UCityCrouchMeshComponent* Mesh=Cast<UCityCrouchMeshComponent>(GetMesh()))
    {
        FCityVehicleBodyPose Source=Pose;
        const FVector Visible=ReferencePelvisOffset();
        const FVector Hidden=GetActorTransform().InverseTransformPosition(GetMesh()->GetComponentTransform().TransformPosition(ReferencePelvis(GetMesh())));
        Source.Pelvis-=GetActorTransform().TransformVectorNoScale(Visible-Hidden);
        Mesh->SetVehicleEntryPose(0,0,FVector::ZeroVector,false);
        Mesh->SetVehicleSeatLegs(0,FVector::ZeroVector,FVector::ZeroVector);
        Mesh->SetVehicleBodyPose(Source);
    }
}
void ACityLiveRPCharacter::ClearVehicleAnimation()
{
    SetVehicleBodyPose(FCityVehicleBodyPose());
    if(GetMesh()->GetAnimInstance() && VehicleMontage)GetMesh()->GetAnimInstance()->Montage_Stop(.2f,VehicleMontage);
    VehicleMontage=nullptr;
}
