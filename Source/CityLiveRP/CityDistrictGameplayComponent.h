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
    void InitializeDistrict();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    bool HasInteraction() const;
    void Interact();
    void OpenChat();
    FString Context=TEXT("Chat");
    FString ChatHistory;
    FName CurrentBuilding;
    int32 CurrentFloor=1;
    int32 MaxFloor() const;
    int32 ApartmentPrice() const;
    bool EnterBuilding(FName Id,FText& Error);
    bool ChangeFloor(int32 Floor,FText& Error);
    bool LeaveBuilding(FText& Error);
    bool EnterApartment(int32 Door,FText& Error);
    bool SpawnWorkScooter(FText& Error);
    bool JoinOrganization(FName Organization,int32 Correct,FText& Error);
    FText ExecuteChat(FString Text);
    void SetGps(FName Destination);
    FString NavigationText() const;
private:
    UPROPERTY() TArray<TObjectPtr<AActor>> InteriorActors;
    UPROPERTY() TArray<TObjectPtr<AActor>> DistrictActors;
    UPROPERTY() TObjectPtr<AActor> NavigationMarker;
    UPROPERTY() TObjectPtr<AActor> CourierMarker;
    bool bInitialized=false;
    bool bInside=false;
    bool bInApartment=false;
    float ScanTime=0;
    FVector ReturnPoint=FVector::ZeroVector;
    FVector GpsTarget=FVector::ZeroVector;
    FString GpsLabel;
    FString Nearby;
    FString InteriorKind;
public:
    bool InteriorReturnPoint(FVector& Point) const {if(!bInside)return false;Point=ReturnPoint;return true;}
private:
    void BuildInterior(bool Apartment=false);
    void RefreshInteraction();
public:
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
