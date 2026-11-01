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
private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    TSharedPtr<SVerticalBox> Fields;
    TSharedPtr<SEditableTextBox> First, Last, Password, Confirmation;
    FText Title, Message;
    FButtonStyle ActionStyle;
    FEditableTextBoxStyle InputStyle;
    FSlateBrush BackgroundBrush;
    TStrongObjectPtr<UTexture2D> BackgroundImage;
    bool bLoginLayout=false;
    bool bNewProfile=false;
    bool bLastSpawn=false;
    bool bCarSelected=false;
    void DistrictPage();
    void ChatOverlay();
    void SendChat();
    void QuizPage(FName Organization,int32 Question=0,int32 Score=0);
    void IdentityPage();
    void PasswordPage();
    void SpawnPage();
    void GaragePage();
    void Button(const FText& Text, TFunction<FReply()> Action);
};
