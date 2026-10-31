#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "Containers/Ticker.h"
#include "Styling/SlateBrush.h"
#include "CityLoadingGameInstance.generated.h"

struct FStreamableHandle;
class SWidget;
class UTexture2D;

UCLASS()
class CITYLIVERP_API ACityBootGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACityBootGameMode();
};

/** Real asynchronous asset progress between the native splash and city startup. */
UCLASS(Config=Game)
class CITYLIVERP_API UCityLoadingGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void OnStart() override;
    virtual void Shutdown() override;
    UPROPERTY(Config) FString CityMapPath;
private:
    UPROPERTY() TObjectPtr<UTexture2D> LoadingImage;
    FSlateBrush LoadingBrush;
    TSharedPtr<SWidget> LoadingWidget;
    TSharedPtr<FStreamableHandle> LoadingHandle;
    FTSTicker::FDelegateHandle TickHandle;
    FDelegateHandle MapLoadedHandle;
    float Progress = 0.f;
    bool bTravelRequested = false;
    bool bReadyToTravel = false;
    bool bFailed = false;
    bool TickLoading(float DeltaTime);
    void CityLoaded(UWorld* LoadedWorld);
    void RemoveLoadingScreen();
};
