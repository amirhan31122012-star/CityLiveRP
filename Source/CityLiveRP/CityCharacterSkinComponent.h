#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "CityCharacterSkinComponent.generated.h"

class USkeletalMeshComponent;

/** Retargets the evaluated gameplay mesh, including procedural entry/crouch. */
UCLASS()
class UCityCharacterSkinComponent : public UPoseableMeshComponent
{
    GENERATED_BODY()
public:
    UCityCharacterSkinComponent();
    bool InitializeSkin(USkeletalMeshComponent* Source);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    TWeakObjectPtr<USkeletalMeshComponent> AnimationSource;
    TArray<FTransform> SourceReference;
    TArray<FTransform> SkinReference;
    TArray<int32> SourceIndices;
};
