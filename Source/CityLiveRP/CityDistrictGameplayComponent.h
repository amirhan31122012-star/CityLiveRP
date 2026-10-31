#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CityDistrictGameplayComponent.generated.h"

/** Persistent local district transactions; UI and scene actors use these operations. */
UCLASS(ClassGroup=(CityLive),meta=(BlueprintSpawnableComponent))
class CITYLIVERP_API UCityDistrictGameplayComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCityDistrictGameplayComponent();
    UFUNCTION(BlueprintCallable,Category="CityLive|District") bool BuyApartment(FName Building,int32 Floor,int32 Door,FText& Error);
    UFUNCTION(BlueprintCallable,Category="CityLive|District") bool SetCourierEmployment(bool Employed,FText& Error);
    UFUNCTION(BlueprintCallable,Category="CityLive|District") bool BeginDelivery(FText& Error);
    UFUNCTION(BlueprintCallable,Category="CityLive|District") bool CompleteDelivery(FText& Error);
    UFUNCTION(BlueprintPure,Category="CityLive|District") bool HasDelivery() const {return bDeliveryActive;}
    UFUNCTION(BlueprintPure,Category="CityLive|District") FVector DeliveryPosition() const {return DeliveryTarget;}
private:
    bool bDeliveryActive=false;
    FVector DeliveryTarget=FVector::ZeroVector;
    FName DeliveryBuilding;
    int32 NextDestination=0;
};
