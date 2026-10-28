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
    const float Target = Character && Character->bIsCrouched &&
        !Character->GetCharacterMovement()->IsFalling() ? 1.f : 0.f;
    const float Blend = 1.f - FMath::Exp(-BlendSpeed * DeltaTime);
    CrouchAlpha = FMath::Lerp(CrouchAlpha, Target, Blend);
    Super::TickComponent(DeltaTime, TickType, TickFunction);
}

void UCityCrouchMeshComponent::FinalizeBoneTransform()
{
    // Do not deform an already-published pose a second time.
    if (bNeedToFlipSpaceBaseBuffers && CrouchAlpha > 0.001f && !IsSimulatingPhysics())
    {
        ApplyCrouch();
    }
    Super::FinalizeBoneTransform();
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
