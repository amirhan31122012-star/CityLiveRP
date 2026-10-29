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
    const float Target = GroundCrouch * (1.f - SeatLegAlpha);
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
        if (EntryTorsoDip > .001f) ApplyEntryTorso();
        if (SeatLegAlpha > .001f) ApplySeatLegs();
        if (EntryReach > 0.001f) ApplyEntryHand();
    }
    Super::FinalizeBoneTransform();
}

void UCityCrouchMeshComponent::SetVehicleSeatLegs(float Alpha, FVector LeftFoot, FVector RightFoot)
{
    SeatLegAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
    SeatFootLeft = LeftFoot;
    SeatFootRight = RightFoot;
}

void UCityCrouchMeshComponent::SetVehicleFit(float Alpha)
{
    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
    if (!bVehicleFitActive && Alpha <= 0.f) return;
    if (!bVehicleFitActive)
    {
        OriginalVehicleMeshTransform = GetRelativeTransform();
        bVehicleFitActive = true;
    }
    if (Alpha <= 0.f)
    {
        SetRelativeTransform(OriginalVehicleMeshTransform);
        bVehicleFitActive = false;
        return;
    }
    const float Scale = FMath::Lerp(1.f, .78f, Alpha);
    SetRelativeScale3D(OriginalVehicleMeshTransform.GetScale3D() * Scale);
    // Preserve pelvis height while giving head and shoulders clearance below the roof.
    SetRelativeLocation(OriginalVehicleMeshTransform.GetLocation() +
        FVector(0, 0, (1.f - Scale) * 90.f * OriginalVehicleMeshTransform.GetScale3D().Z));
}

float UCityCrouchMeshComponent::GetAnkleWorldHeight() const
{
    const USkeletalMesh* Asset = GetSkeletalMeshAsset();
    if (!Asset) return 8.f;
    const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
    int32 Foot = Ref.FindBoneIndex(TEXT("foot_l"));
    if (Foot == INDEX_NONE) return 8.f;
    FTransform Reference = Ref.GetRefBonePose()[Foot];
    for (Foot = Ref.GetParentIndex(Foot); Foot != INDEX_NONE; Foot = Ref.GetParentIndex(Foot))
        Reference = Reference * Ref.GetRefBonePose()[Foot];
    const FBoxSphereBounds BodyAssetBounds = Asset->GetBounds();
    const double FloorZ = BodyAssetBounds.Origin.Z - BodyAssetBounds.BoxExtent.Z;
    return static_cast<float>(FMath::Max(1.0, Reference.GetLocation().Z - FloorZ) * FMath::Abs(GetComponentTransform().GetScale3D().Z));
}

void UCityCrouchMeshComponent::ApplySeatLegs()
{
    const USkeletalMesh* Asset = GetSkeletalMeshAsset();
    if (!Asset || !GetOwner()) return;
    const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
    TArray<FTransform>& Pose = GetEditableComponentSpaceTransforms();
    if (Pose.Num() > Ref.GetNum()) return;
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
    const FVector Forward = GetComponentTransform().InverseTransformVectorNoScale(GetOwner()->GetActorForwardVector());
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const int32 Hip = Ref.FindBoneIndex(Side ? TEXT("thigh_r") : TEXT("thigh_l"));
        const int32 Knee = Ref.FindBoneIndex(Side ? TEXT("calf_r") : TEXT("calf_l"));
        const int32 Foot = Ref.FindBoneIndex(Side ? TEXT("foot_r") : TEXT("foot_l"));
        if (!Pose.IsValidIndex(Hip) || !Pose.IsValidIndex(Knee) || !Pose.IsValidIndex(Foot)) continue;
        const FVector Start = Pose[Hip].GetLocation();
        const FVector Target = FMath::Lerp(Pose[Foot].GetLocation(),
            GetComponentTransform().InverseTransformPosition(Side ? SeatFootRight : SeatFootLeft), SeatLegAlpha);
        const FVector Direction = (Target - Start).GetSafeNormal();
        const FVector BendDirection = (Forward - Direction * FVector::DotProduct(Forward, Direction)).GetSafeNormal();
        double Along, Bend;
        if (BendDirection.IsNearlyZero() || !CityLiveCrouch::KneeOffset(FVector::Distance(Start, Pose[Knee].GetLocation()),
            FVector::Distance(Pose[Knee].GetLocation(), Pose[Foot].GetLocation()), FVector::Distance(Start, Target), Along, Bend)) continue;
        const FVector DesiredKnee = Start + Direction * Along + BendDirection * Bend;
        const FQuat FootRotation = Pose[Foot].GetRotation();
        RotateBranch(Hip, FQuat::FindBetweenNormals((Pose[Knee].GetLocation() - Start).GetSafeNormal(), (DesiredKnee - Start).GetSafeNormal()));
        RotateBranch(Knee, FQuat::FindBetweenNormals((Pose[Foot].GetLocation() - Pose[Knee].GetLocation()).GetSafeNormal(),
            (Target - Pose[Knee].GetLocation()).GetSafeNormal()));
        RotateBranch(Foot, FootRotation * Pose[Foot].GetRotation().Inverse());
    }
}

void UCityCrouchMeshComponent::SetVehicleEntryPose(float Sit, float Reach, FVector Handle, bool bRightHand)
{
    EntryTorsoDip = FMath::Clamp(Sit, 0.f, 1.f);
    EntryReach = FMath::Clamp(Reach, 0.f, 1.f);
    EntryHandle = Handle;
    bEntryRightHand = bRightHand;
}

void UCityCrouchMeshComponent::ApplyEntryTorso()
{
    const USkeletalMesh* Asset = GetSkeletalMeshAsset();
    if (!Asset || !GetOwner()) return;
    const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
    TArray<FTransform>& Pose = GetEditableComponentSpaceTransforms();
    const int32 Spine = Ref.FindBoneIndex(TEXT("spine_01"));
    if (!Pose.IsValidIndex(Spine) || Pose.Num() > Ref.GetNum()) return;
    const FVector Pivot = Pose[Spine].GetLocation();
    const FVector Axis = GetComponentTransform().InverseTransformVectorNoScale(GetOwner()->GetActorRightVector()).GetSafeNormal();
    const FQuat Rotation(Axis, FMath::DegreesToRadians(22.f * EntryTorsoDip));
    for (int32 Bone = Spine; Bone < Pose.Num(); ++Bone)
    {
        int32 Ancestor = Bone;
        while (Ancestor != INDEX_NONE && Ancestor != Spine) Ancestor = Ref.GetParentIndex(Ancestor);
        if (Ancestor != Spine) continue;
        Pose[Bone].SetLocation(Pivot + Rotation.RotateVector(Pose[Bone].GetLocation() - Pivot));
        Pose[Bone].SetRotation((Rotation * Pose[Bone].GetRotation()).GetNormalized());
    }
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
