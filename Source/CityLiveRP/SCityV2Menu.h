#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class ACityLiveRPPlayerController;
class SEditableTextBox;
class SVerticalBox;

class SCityV2Menu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCityV2Menu) {}
        SLATE_ARGUMENT(ACityLiveRPPlayerController*, Controller)
        SLATE_ARGUMENT(bool, Garage)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    TSharedPtr<SVerticalBox> Fields;
    TSharedPtr<SEditableTextBox> First, Last, Password, Confirmation;
    FText Title, Message;
    bool bNewProfile=false;
    bool bLastSpawn=false;
    bool bCarSelected=false;
    void IdentityPage();
    void PasswordPage();
    void SpawnPage();
    void GaragePage();
    void Button(const FText& Text, TFunction<FReply()> Action);
};
