#include "CityParkingMarker.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

ACityParkingMarker::ACityParkingMarker()
{
    PrimaryActorTick.bCanEverTick=false;
    USceneComponent* Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    // Two faces remain legible when approaching from either side of the parking.
    for (int32 Side=0; Side<2; ++Side)
    {
        UTextRenderComponent* Sign=CreateDefaultSubobject<UTextRenderComponent>(
            Side==0 ? TEXT("ParkingFront") : TEXT("ParkingBack"));
        Sign->SetupAttachment(Root);
        Sign->SetText(FText::FromString(TEXT("P")));
        Sign->SetTextRenderColor(FColor(80,205,255));
        Sign->SetWorldSize(85.f);
        Sign->SetHorizontalAlignment(EHTA_Center);
        Sign->SetVerticalAlignment(EVRTA_TextCenter);
        Sign->SetRelativeRotation(FRotator(0,Side*180.f,0));
        Sign->SetRelativeLocation(FVector(Side==0 ? .5f : -.5f,0,0));
        Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Sign->SetCastShadow(false);
    }
}
