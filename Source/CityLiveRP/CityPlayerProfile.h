#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CityProfileRules.h"
#include "CityPlayerProfile.generated.h"

UCLASS()
class CITYLIVERP_API UCityPlayerProfile : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 FormatVersion = 1;
    UPROPERTY(SaveGame) FString FirstName;
    UPROPERTY(SaveGame) FString Surname;
    UPROPERTY(SaveGame) FString PasswordSalt;
    UPROPERTY(SaveGame) TArray<uint8> PasswordHash;
    UPROPERTY(SaveGame) TArray<FString> Apartments;
    UPROPERTY(SaveGame) bool bCourierEmployed=false;
    UPROPERTY(SaveGame) int32 CompletedDeliveries=0;
    UPROPERTY(SaveGame) FString Organization;
    UPROPERTY(SaveGame) int32 PoliceQuizScore=0;
    UPROPERTY(SaveGame) int32 HospitalQuizScore=0;
    UPROPERTY(SaveGame) int32 Balance = CityProfileRules::StartingBalance;
    UPROPERTY(SaveGame) bool bHasLastPosition = false;
    UPROPERTY(SaveGame) FVector LastPosition = FVector::ZeroVector;
    UPROPERTY(SaveGame) float LastYaw = 0.f;
    UPROPERTY(SaveGame) FString LastMap;
    static FString Slot(const FString& First, const FString& Last);
    void SetPassword(const FString& Password);
    bool CheckPassword(const FString& Password) const;
};

UCLASS()
class CITYLIVERP_API UCityLoginMemory : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FString FirstName;
    UPROPERTY(SaveGame) FString Surname;
};
