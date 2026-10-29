#include "CityCharacterSkinComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include <initializer_list>

UCityCharacterSkinComponent::UCityCharacterSkinComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool UCityCharacterSkinComponent::InitializeSkin(USkeletalMeshComponent* Source)
{
    USkeletalMesh* Skin = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/CityLiveCharacters/Beige/SK_CityBeige_v2.SK_CityBeige_v2"));
    if (!Source || !Source->GetSkeletalMeshAsset() || !Skin || !Skin->GetSkeleton()) return false;
    const FReferenceSkeleton& SourceRef = Source->GetSkeletalMeshAsset()->GetRefSkeleton();
    const FReferenceSkeleton& TargetRef = Skin->GetRefSkeleton();
    for (const TCHAR* Required : {TEXT("pelvis"), TEXT("head"), TEXT("hand_l"), TEXT("hand_r"), TEXT("foot_l"), TEXT("foot_r")})
        if (TargetRef.FindBoneIndex(FName(Required)) == INDEX_NONE || SourceRef.FindBoneIndex(FName(Required)) == INDEX_NONE) return false;
    AnimationSource = Source;
    SetSkinnedAssetAndUpdate(Skin);
    auto BuildReference = [](const FReferenceSkeleton& Ref, TArray<FTransform>& Result)
    {
        Result = Ref.GetRefBonePose();
        for (int32 Bone = 0; Bone < Result.Num(); ++Bone)
        {
            const int32 Parent = Ref.GetParentIndex(Bone);
            if (Parent != INDEX_NONE) Result[Bone] = Result[Bone] * Result[Parent];
        }
    };
    BuildReference(SourceRef, SourceReference);
    BuildReference(TargetRef, SkinReference);
    SourceIndices.Reset();
    for (int32 Bone = 0; Bone < TargetRef.GetNum(); ++Bone)
        SourceIndices.Add(SourceRef.FindBoneIndex(TargetRef.GetBoneName(Bone)));
    Source->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    // Hide only the mannequin, keeping its animation and existing gameplay IK.
    Source->SetVisibility(false, false);
    SetVisibility(true);
    SetComponentTickEnabled(true);
    AddTickPrerequisiteComponent(Source);
    return true;
}

void UCityCharacterSkinComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    USkeletalMeshComponent* Source = AnimationSource.Get();
    const USkeletalMesh* ActiveSkin = Cast<USkeletalMesh>(GetSkinnedAsset());
    if (!Source || !ActiveSkin) return;
    const TArray<FTransform>& SourcePose = Source->GetComponentSpaceTransforms();
    const FReferenceSkeleton& Ref = ActiveSkin->GetRefSkeleton();
    TArray<FTransform> Result = SkinReference;
    for (int32 Bone = 0; Bone < Result.Num(); ++Bone)
    {
        const int32 Index = SourceIndices[Bone];
        const int32 Parent = Ref.GetParentIndex(Bone);
        if (!SourcePose.IsValidIndex(Index)) continue;
        const FQuat Delta = SourcePose[Index].GetRotation() * SourceReference[Index].GetRotation().Inverse();
        Result[Bone].SetRotation((Delta * SkinReference[Bone].GetRotation()).GetNormalized());
        if (Parent == INDEX_NONE)
            Result[Bone].SetLocation(SkinReference[Bone].GetLocation() + SourcePose[Index].GetLocation() - SourceReference[Index].GetLocation());
        else
        {
            const int32 SourceParent = SourceIndices[Parent];
            if (!SourcePose.IsValidIndex(SourceParent)) continue;
            const FQuat ParentDelta = SourcePose[SourceParent].GetRotation() * SourceReference[SourceParent].GetRotation().Inverse();
            const FVector SourceRestOffset = SourceReference[Index].GetLocation() - SourceReference[SourceParent].GetLocation();
            const FVector AnimatedOffset = SourcePose[Index].GetLocation() - SourcePose[SourceParent].GetLocation();
            const FVector Translation = AnimatedOffset - ParentDelta.RotateVector(SourceRestOffset);
            Result[Bone].SetLocation(Result[Parent].GetLocation() + ParentDelta.RotateVector(
                SkinReference[Bone].GetLocation() - SkinReference[Parent].GetLocation()) + Translation);
        }
        SetBoneTransformByName(Ref.GetBoneName(Bone), Result[Bone], EBoneSpaces::ComponentSpace);
    }
    RefreshBoneTransforms();
}
