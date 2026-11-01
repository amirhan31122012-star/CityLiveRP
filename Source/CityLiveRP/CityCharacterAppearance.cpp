#include "CityLiveRPCharacter.h"
#include "CityCharacterSkinComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"

bool ACityLiveRPCharacter::SetAppearanceRole(FName Role)
{
    if(Role!=TEXT("Civilian")&&Role!=TEXT("Police")&&Role!=TEXT("Medic")&&Role!=TEXT("Courier"))return false;
    if(!CharacterSkin->InitializeSkin(GetMesh(),bUseRoleWardrobe?Role:FName(TEXT("Legacy"))))return false;
    AppearanceRole=Role;
    if(Role==TEXT("Courier"))
    {
        USkeletalMesh* Pack=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/CityLiveCharacters/Roles/SK_CityCourierPack.SK_CityCourierPack"));
        CourierBag->SetSkinnedAssetAndUpdate(Pack);
        CourierBag->SetLeaderPoseComponent(CharacterSkin,true,false);
        CourierBag->PrimaryComponentTick.TickGroup=TG_PostPhysics;
        CourierBag->AddTickPrerequisiteComponent(CharacterSkin);
    }
    UpdateCourierBag(false);return true;
}
void ACityLiveRPCharacter::UpdateCourierBag(bool bInVehiclePose)
{
    AActor* Vehicle=GetAttachParentActor();
    if(!Vehicle)if(APlayerController* Player=Cast<APlayerController>(GetController()))Vehicle=Player->GetViewTarget();
    const bool Stowed=bInVehiclePose && (!Vehicle || !Vehicle->ActorHasTag(TEXT("CityCourierScooter")));
    CourierBag->SetVisibility(bUseRoleWardrobe && AppearanceRole==TEXT("Courier") && !Stowed && CourierBag->GetSkeletalMeshAsset()!=nullptr);
}
