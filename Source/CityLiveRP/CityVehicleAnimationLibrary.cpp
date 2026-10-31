#include "CityVehicleAnimationLibrary.h"
#include "Engine/SkeletalMesh.h"

TArray<FCityReferenceBone> UCityVehicleAnimationLibrary::GetReferencePose(USkeletalMesh* Mesh)
{
    TArray<FCityReferenceBone> Result;
    if(!Mesh)return Result;
    const FReferenceSkeleton& Ref=Mesh->GetRefSkeleton();
    const TArray<FTransform>& Local=Ref.GetRefBonePose();
    Result.Reserve(Local.Num());
    for(int32 Bone=0;Bone<Local.Num();++Bone)
    {
        const int32 Parent=Ref.GetParentIndex(Bone);
        FCityReferenceBone Item;
        Item.BoneName=Ref.GetBoneName(Bone);
        Item.ParentName=Parent==INDEX_NONE ? NAME_None : Ref.GetBoneName(Parent);
        Item.ComponentTransform=Parent==INDEX_NONE ? Local[Bone] : Local[Bone]*Result[Parent].ComponentTransform;
        Result.Add(Item);
    }
    return Result;
}
