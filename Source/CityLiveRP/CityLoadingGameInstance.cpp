#include "CityLoadingGameInstance.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

ACityBootGameMode::ACityBootGameMode()
{
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
}

void UCityLoadingGameInstance::OnStart()
{
    Super::OnStart();
    if (!GetWorld() || !GetWorld()->GetMapName().Contains(TEXT("CityLive_Boot"))) return;
    UGameViewportClient* Viewport = GetGameViewportClient();
    if (!Viewport) return;
    LoadingImage = LoadObject<UTexture2D>(nullptr, TEXT("/Game/CityLiveUI/T_LoadingCity.T_LoadingCity"));
    LoadingBrush.SetResourceObject(LoadingImage);
    LoadingBrush.ImageSize = FVector2D(1280,720);
    LoadingWidget = SNew(SOverlay)
        + SOverlay::Slot()[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)
            [SNew(SImage).Image(&LoadingBrush)]]
        + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom)
        [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(.012f,.02f,.035f,.96f))
            .Padding(FMargin(55,18,55,28))
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                [SNew(STextBlock).Justification(ETextJustify::Center)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold",22))
                    .Text_Lambda([this]() {
                        if (bFailed) return FText::FromString(TEXT("Не удалось загрузить карту. Перезапустите игру."));
                        if (bReadyToTravel || bTravelRequested) return FText::FromString(TEXT("100% — запуск города…"));
                        return FText::FromString(FString::Printf(TEXT("Загрузка карты — %d%%"),
                            FMath::FloorToInt(Progress*100.f)));
                    })]
                + SVerticalBox::Slot().AutoHeight()
                [SNew(SBox).HeightOverride(14)
                    [SNew(SProgressBar).Percent_Lambda([this]() { return TOptional<float>(Progress); })
                        .FillColorAndOpacity(FLinearColor(.95f,.61f,.12f))]]]];
    Viewport->AddViewportWidgetContent(LoadingWidget.ToSharedRef(),10000);
    MapLoadedHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,&UCityLoadingGameInstance::CityLoaded);
    if (!CityMapPath.StartsWith(TEXT("/Game/CityLiveMaps/ModernCity/Run_")) ||
        !CityMapPath.EndsWith(TEXT("/CityLive_ModernCity"))) { bFailed = true; return; }
    const FString WorldPath = CityMapPath + TEXT(".CityLive_ModernCity");
    TArray<FSoftObjectPath> Assets { FSoftObjectPath(WorldPath),
        FSoftObjectPath(TEXT("/Game/CityLiveCharacters/Beige/SK_CityBeige_v3.SK_CityBeige_v3")),
        FSoftObjectPath(TEXT("/Game/CityLiveCharacters/Beige/M_CityBeige_v3.M_CityBeige_v3")),
        FSoftObjectPath(TEXT("/Game/CityLiveVehicles/Sedan/SK_CitySedan_4Doors.SK_CitySedan_4Doors")) };
    LoadingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Assets);
    if (!LoadingHandle.IsValid()) { bFailed = true; return; }
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UCityLoadingGameInstance::TickLoading));
}

bool UCityLoadingGameInstance::TickLoading(float /*DeltaTime*/)
{
    if (bFailed || bTravelRequested || !LoadingHandle.IsValid()) return true;
    if (bReadyToTravel)
    {
        bTravelRequested = true;
        UGameplayStatics::OpenLevel(this,FName(*CityMapPath));
        return true;
    }
    Progress = FMath::Max(Progress,FMath::Clamp(LoadingHandle->GetProgress(),0.f,1.f));
    if (LoadingHandle->HasLoadCompleted())
    {
        const FString WorldPath = CityMapPath + TEXT(".CityLive_ModernCity");
        if (!FindObject<UWorld>(nullptr,*WorldPath)) { bFailed = true; return true; }
        // Give Slate a frame at real 100% before requesting world travel.
        // Keep the overlay until the destination world has actually started.
        Progress = 1.f;
        bReadyToTravel = true;
    }
    return true;
}

void UCityLoadingGameInstance::CityLoaded(UWorld* LoadedWorld)
{
    if (LoadedWorld && LoadedWorld->GetGameInstance()==this &&
        !LoadedWorld->GetMapName().Contains(TEXT("CityLive_Boot"))) RemoveLoadingScreen();
}

void UCityLoadingGameInstance::RemoveLoadingScreen()
{
    if (LoadingWidget.IsValid() && GetGameViewportClient())
        GetGameViewportClient()->RemoveViewportWidgetContent(LoadingWidget.ToSharedRef());
    LoadingWidget.Reset();
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapLoadedHandle);
    LoadingHandle.Reset();
}

void UCityLoadingGameInstance::Shutdown()
{
    RemoveLoadingScreen();
    Super::Shutdown();
}
