#include "CityCharacterSkinComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "ProceduralCrouchMath.h"
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
        TEXT("/Game/CityLiveCharacters/Beige/SK_CityBeige_v3.SK_CityBeige_v3"));
    if (!Source || !Source->GetSkeletalMeshAsset() || !Skin || !Skin->GetSkeleton()) return false;
    const FReferenceSkeleton& SourceRef = Source->GetSkeletalMeshAsset()->GetRefSkeleton();
    const FReferenceSkeleton& TargetRef = Skin->GetRefSkeleton();
    for (const TCHAR* Required : {TEXT("pelvis"), TEXT("head"), TEXT("hand_l"), TEXT("hand_r"), TEXT("foot_l"), TEXT("foot_r")})
        if (TargetRef.FindBoneIndex(FName(Required)) == INDEX_NONE || SourceRef.FindBoneIndex(FName(Required)) == INDEX_NONE) return false;
    AnimationSource = Source;
    SetSkinnedAssetAndUpdate(Skin);
    const FBoxSphereBounds SourceBounds = Source->GetSkeletalMeshAsset()->GetBounds();
    const FBoxSphereBounds SkinBounds = Skin->GetBounds();
    SetRelativeLocation(FVector(0, 0, (SourceBounds.Origin.Z - SourceBounds.BoxExtent.Z) -
        (SkinBounds.Origin.Z - SkinBounds.BoxExtent.Z)));
    if (UMaterialInterface* SkinMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/CityLiveCharacters/Beige/M_CityBeige_v3.M_CityBeige_v3")))
        for (int32 Slot = 0; Slot < Skin->GetMaterials().Num(); ++Slot) SetMaterial(Slot, SkinMaterial);
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
    AimChildren.Init(INDEX_NONE, TargetRef.GetNum());
    for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
    {
        for (const auto& Pair : {TPair<FString, FString>(TEXT("thigh_"), TEXT("calf_")),
            TPair<FString, FString>(TEXT("calf_"), TEXT("foot_")),
            TPair<FString, FString>(TEXT("upperarm_"), TEXT("lowerarm_")),
            TPair<FString, FString>(TEXT("lowerarm_"), TEXT("hand_"))})
        {
            const int32 Bone = TargetRef.FindBoneIndex(FName(*(Pair.Key + Side)));
            if (Bone != INDEX_NONE) AimChildren[Bone] = TargetRef.FindBoneIndex(FName(*(Pair.Value + Side)));
        }
    }
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
        const int32 Child = AimChildren[Bone];
        if (SkinReference.IsValidIndex(Child) && SourcePose.IsValidIndex(SourceIndices[Child]))
        {
            const FVector RestDirection = (SkinReference[Child].GetLocation() - SkinReference[Bone].GetLocation()).GetSafeNormal();
            const FVector AnimatedDirection = (SourcePose[SourceIndices[Child]].GetLocation() - SourcePose[Index].GetLocation()).GetSafeNormal();
            if (!RestDirection.IsNearlyZero() && !AnimatedDirection.IsNearlyZero())
                Result[Bone].SetRotation((FQuat::FindBetweenNormals(RestDirection, AnimatedDirection) * SkinReference[Bone].GetRotation()).GetNormalized());
        }
        if (Parent == INDEX_NONE)
            Result[Bone].SetLocation(SkinReference[Bone].GetLocation() + SourcePose[Index].GetLocation() - SourceReference[Index].GetLocation());
        else
        {
            const int32 SourceParent = SourceIndices[Parent];
            if (!SourcePose.IsValidIndex(SourceParent)) continue;
            const FQuat SourceParentDelta = SourcePose[SourceParent].GetRotation() * SourceReference[SourceParent].GetRotation().Inverse();
            const FQuat ParentDelta = Result[Parent].GetRotation() * SkinReference[Parent].GetRotation().Inverse();
            const FVector SourceRestOffset = SourceReference[Index].GetLocation() - SourceReference[SourceParent].GetLocation();
            const FVector AnimatedOffset = SourcePose[Index].GetLocation() - SourcePose[SourceParent].GetLocation();
            const FVector Translation = AnimatedOffset - SourceParentDelta.RotateVector(SourceRestOffset);
            Result[Bone].SetLocation(Result[Parent].GetLocation() + ParentDelta.RotateVector(
                SkinReference[Bone].GetLocation() - SkinReference[Parent].GetLocation()) + Translation);
        }
    }
    // Match the animated ankle targets while preserving this model's leg lengths.
    // Reference-pose deltas alone retained the scan's backwards/closed stance.
    for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
    {
        const int32 Hip = Ref.FindBoneIndex(FName(*(FString(TEXT("thigh_")) + Side)));
        const int32 Knee = Ref.FindBoneIndex(FName(*(FString(TEXT("calf_")) + Side)));
        const int32 Foot = Ref.FindBoneIndex(FName(*(FString(TEXT("foot_")) + Side)));
        if (!Result.IsValidIndex(Hip) || !Result.IsValidIndex(Knee) || !Result.IsValidIndex(Foot)) continue;
        const int32 SourceHip = SourceIndices[Hip], SourceKnee = SourceIndices[Knee], SourceFoot = SourceIndices[Foot];
        if (!SourcePose.IsValidIndex(SourceHip) || !SourcePose.IsValidIndex(SourceKnee) || !SourcePose.IsValidIndex(SourceFoot)) continue;
        const FVector Origin = Result[Hip].GetLocation();
        FVector Goal = SourcePose[SourceFoot].GetLocation();
        Goal.Z += SkinReference[Foot].GetLocation().Z - SourceReference[SourceFoot].GetLocation().Z;
        const FVector Direction = (Goal - Origin).GetSafeNormal();
        if (Direction.IsNearlyZero()) continue;
        const double UpperLength = FVector::Distance(SkinReference[Hip].GetLocation(), SkinReference[Knee].GetLocation());
        const double LowerLength = FVector::Distance(SkinReference[Knee].GetLocation(), SkinReference[Foot].GetLocation());
        const double Reach = FMath::Clamp(FVector::Distance(Origin, Goal), FMath::Abs(UpperLength - LowerLength) + .01, UpperLength + LowerLength - .01);
        Goal = Origin + Direction * Reach;
        FVector Pole = SourcePose[SourceKnee].GetLocation() - SourcePose[SourceHip].GetLocation();
        const FVector SourceDirection = (SourcePose[SourceFoot].GetLocation() - SourcePose[SourceHip].GetLocation()).GetSafeNormal();
        Pole -= SourceDirection * FVector::DotProduct(Pole, SourceDirection);
        if (Pole.SizeSquared() < 1.f)
        {
            const FVector Forward = Source->GetOwner() ? Source->GetComponentTransform().InverseTransformVectorNoScale(
                Source->GetOwner()->GetActorForwardVector()) : FVector(0, 1, 0);
            Pole = Forward;
        }
        Pole -= Direction * FVector::DotProduct(Pole, Direction);
        if (Pole.IsNearlyZero()) continue;
        double Along, Bend;
        if (!CityLiveCrouch::KneeOffset(UpperLength, LowerLength, Reach, Along, Bend)) continue;
        const FVector KneeGoal = Origin + Direction * Along + Pole.GetSafeNormal() * Bend;
        Result[Hip].SetRotation((FQuat::FindBetweenNormals(
            (SkinReference[Knee].GetLocation() - SkinReference[Hip].GetLocation()).GetSafeNormal(),
            (KneeGoal - Origin).GetSafeNormal()) * SkinReference[Hip].GetRotation()).GetNormalized());
        Result[Knee].SetLocation(KneeGoal);
        Result[Knee].SetRotation((FQuat::FindBetweenNormals(
            (SkinReference[Foot].GetLocation() - SkinReference[Knee].GetLocation()).GetSafeNormal(),
            (Goal - KneeGoal).GetSafeNormal()) * SkinReference[Knee].GetRotation()).GetNormalized());
        Result[Foot].SetLocation(Goal);
    }
    for (int32 Bone = 0; Bone < Result.Num(); ++Bone)
        SetBoneTransformByName(Ref.GetBoneName(Bone), Result[Bone], EBoneSpaces::ComponentSpace);
    RefreshBoneTransforms();
}
