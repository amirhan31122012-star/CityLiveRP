#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
class ACityLiveRPPlayerController;
class SEditableTextBox;
class SVerticalBox;
class SCityMap;

class SCityV2Menu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCityV2Menu) {}
        SLATE_ARGUMENT(ACityLiveRPPlayerController*, Controller)
        SLATE_ARGUMENT(bool, Garage)
        SLATE_ARGUMENT(bool, District)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    void FocusChatInput();
    virtual bool SupportsKeyboardFocus() const override{return true;}
    virtual FReply OnKeyChar(const FGeometry&,const FCharacterEvent&) override;
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override;
private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    TSharedPtr<SVerticalBox> Fields;
    TSharedPtr<SVerticalBox> Actions,KeyboardHost;
    TSharedPtr<SCityMap> DistrictMap;
    TSharedPtr<SEditableTextBox> First, Last, Password, Confirmation;
    FText Title, Message;
    FButtonStyle ActionStyle,SecondaryStyle,RowStyle,KeyStyle;
    FEditableTextBoxStyle InputStyle;
    FScrollBarStyle ScrollStyle;
    FSlateBrush BackgroundBrush;
    TStrongObjectPtr<UTexture2D> BackgroundImage;
    FSlateBrush AccountBrush,SpawnBrushes[5];
    TStrongObjectPtr<UTexture2D> AccountImage,SpawnImages[5];
    bool bLoginLayout=false;
    bool bNewProfile=false;
    FName SelectedSpawn=TEXT("Start");
    bool bCarSelected=false;
    bool bPasswordVisible=false,bChatLayout=false,bRussian=false,bShift=false,bSymbols=false;
    FString LoginName,SelectedGps,ChatDraft;
    int32 HistoryIndex=0,ChatChannel=0;
    FVector MapSelection=FVector::ZeroVector;
    FString MapLabel;
    bool bMapSelection=false;
    bool bLegendOpen=false;
    FSlateBrush FrameBrush;
    void BuildFrame(int32 Mode);
    void MapPage();
    void GpsPage();
    void KeyboardPage();
    void TypeChat(const FString& Value);
    void BackspaceChat();
    void DistrictPage();
    void ChatOverlay();
    void SendChat();
    void QuizPage(FName Organization,int32 Question=0,int32 Score=0);
    void IdentityPage();
    void PasswordPage();
    void SpawnPage();
    void GaragePage();
    void Button(const FText& Text,TFunction<FReply()> Action,bool bPrimary=false);
};
