#include "CityCrouchMeshComponent.h"
#include "ProceduralCrouchMath.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UCityCrouchMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    const ACharacter* Character = Cast<ACharacter>(GetOwner());
    const float GroundCrouch = Character && Character->bIsCrouched &&
        !Character->GetCharacterMovement()->IsFalling() ? 1.f : 0.f;
    const float Target = FMath::Max(GroundCrouch, EntrySit);
    const float Blend = 1.f - FMath::Exp(-BlendSpeed * DeltaTime);
    CrouchAlpha = FMath::Lerp(CrouchAlpha, Target, Blend);
    Super::TickComponent(DeltaTime, TickType, TickFunction);
}

void UCityCrouchMeshComponent::FinalizeBoneTransform()
{
    // Do not deform an already-published pose a second time.
    if (bNeedToFlipSpaceBaseBuffers && !IsSimulatingPhysics())
    {
        if (CrouchAlpha > 0.001f) ApplyCrouch();
        if (EntryReach > 0.001f) ApplyEntryHand();
    }
    Super::FinalizeBoneTransform();
}

void UCityCrouchMeshComponent::SetVehicleEntryPose(float Sit, float Reach, FVector Handle, bool bRightHand)
{
    EntrySit = FMath::Clamp(Sit, 0.f, 1.f);
    EntryReach = FMath::Clamp(Reach, 0.f, 1.f);
    EntryHandle = Handle;
    bEntryRightHand = bRightHand;
}

void UCityCrouchMeshComponent::ApplyEntryHand()
{
    const USkeletalMesh* Asset = GetSkeletalMeshAsset();
    if (!Asset) return;
    const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
    TArray<FTransform>& Pose = GetEditableComponentSpaceTransforms();
    if (Pose.Num() > Ref.GetNum()) return;
    const int32 Shoulder = Ref.FindBoneIndex(bEntryRightHand ? TEXT("upperarm_r") : TEXT("upperarm_l"));
    const int32 Elbow = Ref.FindBoneIndex(bEntryRightHand ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
    const int32 HandBone = Ref.FindBoneIndex(bEntryRightHand ? TEXT("hand_r") : TEXT("hand_l"));
    if (!Pose.IsValidIndex(Shoulder) || !Pose.IsValidIndex(Elbow) || !Pose.IsValidIndex(HandBone)) return;
    const FVector Origin = Pose[Shoulder].GetLocation();
    const FVector OriginalElbow = Pose[Elbow].GetLocation();
    const FVector OriginalHand = Pose[HandBone].GetLocation();
    const FVector Target = FMath::Lerp(OriginalHand, GetComponentTransform().InverseTransformPosition(EntryHandle), EntryReach);
    const FVector Direction = (Target - Origin).GetSafeNormal();
    FVector Pole = (OriginalElbow - Origin) - Direction * FVector::DotProduct(OriginalElbow - Origin, Direction);
    if (Pole.IsNearlyZero()) Pole = FVector::CrossProduct(Direction, FVector::UpVector);
    double Along, Bend;
    if (Pole.IsNearlyZero() || !CityLiveCrouch::KneeOffset(FVector::Distance(Origin, OriginalElbow),
        FVector::Distance(OriginalElbow, OriginalHand), FVector::Distance(Origin, Target), Along, Bend)) return;
    const FVector DesiredElbow = Origin + Direction * Along + Pole.GetSafeNormal() * Bend;
    auto RotateBranch = [&](int32 Root, const FQuat& Rotation)
    {
        const FVector Pivot = Pose[Root].GetLocation();
        for (int32 Bone = Root; Bone < Pose.Num(); ++Bone)
        {
            int32 Parent = Bone;
            while (Parent != INDEX_NONE && Parent != Root) Parent = Ref.GetParentIndex(Parent);
            if (Parent != Root) continue;
            Pose[Bone].SetLocation(Pivot + Rotation.RotateVector(Pose[Bone].GetLocation() - Pivot));
            Pose[Bone].SetRotation((Rotation * Pose[Bone].GetRotation()).GetNormalized());
        }
    };
    RotateBranch(Shoulder, FQuat::FindBetweenNormals((OriginalElbow - Origin).GetSafeNormal(), (DesiredElbow - Origin).GetSafeNormal()));
    const FVector NewElbow = Pose[Elbow].GetLocation();
    RotateBranch(Elbow, FQuat::FindBetweenNormals((Pose[HandBone].GetLocation() - NewElbow).GetSafeNormal(), (Target - NewElbow).GetSafeNormal()));
}

void UCityCrouchMeshComponent::ApplyCrouch()
{
    const USkeletalMesh* Asset = GetSkeletalMeshAsset();
    if (!Asset || !GetOwner()) return;
    const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
    TArray<FTransform>& Pose = GetEditableComponentSpaceTransforms();
    if (Pose.Num() > Ref.GetNum()) return;
    const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
    const int32 Spine = Ref.FindBoneIndex(TEXT("spine_01"));
    const int32 Thighs[] = {Ref.FindBoneIndex(TEXT("thigh_l")), Ref.FindBoneIndex(TEXT("thigh_r"))};
    const int32 Calves[] = {Ref.FindBoneIndex(TEXT("calf_l")), Ref.FindBoneIndex(TEXT("calf_r"))};
    const int32 Feet[] = {Ref.FindBoneIndex(TEXT("foot_l")), Ref.FindBoneIndex(TEXT("foot_r"))};
    if (!Pose.IsValidIndex(Pelvis) || !Pose.IsValidIndex(Spine)) return;
    for (int32 Side = 0; Side < 2; ++Side)
        if (!Pose.IsValidIndex(Thighs[Side]) || !Pose.IsValidIndex(Calves[Side]) ||
            !Pose.IsValidIndex(Feet[Side])) return;

    auto IsInBranch = [&Ref](int32 Bone, int32 Root)
    {
        while (Bone != INDEX_NONE)
        {
            if (Bone == Root) return true;
            Bone = Ref.GetParentIndex(Bone);
        }
        return false;
    };
    auto RotateBranch = [&](int32 Root, const FQuat& Delta)
    {
        const FVector Pivot = Pose[Root].GetLocation();
        for (int32 Bone = Root; Bone < Pose.Num(); ++Bone)
        {
            if (!IsInBranch(Bone, Root)) continue;
            Pose[Bone].SetLocation(Pivot + Delta.RotateVector(Pose[Bone].GetLocation() - Pivot));
            Pose[Bone].SetRotation((Delta * Pose[Bone].GetRotation()).GetNormalized());
        }
    };
    const FTransform OriginalFeet[] = {Pose[Feet[0]], Pose[Feet[1]]};
    const FVector Down = GetComponentTransform().InverseTransformVector(FVector(0.f, 0.f, -PelvisDrop * CrouchAlpha));
    const FVector Forward = GetComponentTransform().InverseTransformVectorNoScale(GetOwner()->GetActorForwardVector()).GetSafeNormal();
    const FVector Up = GetComponentTransform().InverseTransformVectorNoScale(FVector::UpVector).GetSafeNormal();
    for (int32 Bone = Pelvis; Bone < Pose.Num(); ++Bone)
        if (IsInBranch(Bone, Pelvis)) Pose[Bone].AddToTranslation(Down);

    // Lean the upper body forward, keeping the original arm animation.
    const FVector LeanAxis = FVector::CrossProduct(Up, Forward).GetSafeNormal();
    RotateBranch(Spine, FQuat(LeanAxis, FMath::DegreesToRadians(20.f * CrouchAlpha)));
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const int32 HipBone = Thighs[Side], KneeBone = Calves[Side], FootBone = Feet[Side];
        const FVector Hip = Pose[HipBone].GetLocation();
        const FVector Knee = Pose[KneeBone].GetLocation();
        const FVector Ankle = Pose[FootBone].GetLocation();
        const FVector Target = OriginalFeet[Side].GetLocation();
        const FVector Direction = (Target - Hip).GetSafeNormal();
        const FVector ForwardBend = (Forward - Direction * FVector::DotProduct(Forward, Direction)).GetSafeNormal();
        const FVector OriginalBend = ((Knee - Hip) - Direction * FVector::DotProduct(Knee - Hip, Direction)).GetSafeNormal();
        const FVector BendDirection = FMath::Lerp(OriginalBend.IsNearlyZero() ? ForwardBend : OriginalBend,
            ForwardBend, CrouchAlpha).GetSafeNormal();
        double Along, Bend;
        if (BendDirection.IsNearlyZero() || !CityLiveCrouch::KneeOffset(
            FVector::Distance(Hip, Knee), FVector::Distance(Knee, Ankle),
            FVector::Distance(Hip, Target), Along, Bend)) continue;
        const FVector DesiredKnee = Hip + Direction * Along + BendDirection * Bend;
        RotateBranch(HipBone, FQuat::FindBetweenNormals((Knee - Hip).GetSafeNormal(), (DesiredKnee - Hip).GetSafeNormal()));
        const FVector NewKnee = Pose[KneeBone].GetLocation();
        RotateBranch(KneeBone, FQuat::FindBetweenNormals(
            (Pose[FootBone].GetLocation() - NewKnee).GetSafeNormal(), (Target - NewKnee).GetSafeNormal()));
        RotateBranch(FootBone, OriginalFeet[Side].GetRotation() * Pose[FootBone].GetRotation().Inverse());
    }
}
