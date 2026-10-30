#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "CityFloatingStick.h"

class ACityLiveRPPlayerController;
class ACityLiveRPCharacter;
class ACitySedan;

/** One movement stick and one hold-to-run / double-tap-to-jump button. */
class SCityMobileControls : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCityMobileControls) {}
        SLATE_ARGUMENT(ACityLiveRPPlayerController*, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    void ResetControls();
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(320, 180); }
    virtual void Tick(const FGeometry&, double, float) override;
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply OnTouchStarted(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnTouchMoved(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnTouchEnded(const FGeometry&, const FPointerEvent&) override;
    virtual void OnFocusLost(const FFocusEvent&) override;
    virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override;

private:
    TWeakObjectPtr<ACityLiveRPPlayerController> Controller;
    TWeakObjectPtr<ACityLiveRPCharacter> ControlledCharacter;
    TWeakObjectPtr<ACitySedan> ControlledVehicle;
    CityLiveTouch::FloatingStick StickAnchor;
    FVector2D Stick = FVector2D::ZeroVector;
    int32 StickPointer = INDEX_NONE;
    int32 RunPointer = INDEX_NONE;
    int32 BrakePointer = INDEX_NONE;
    int32 LookPointer = INDEX_NONE;
    FVector2D LastLookPosition = FVector2D::ZeroVector;
    double LastStickTap = -100.0;
    double LastRunTap = -100.0;
    bool bRunHeld = false;
    float RunHighlight = 0.f;
    static constexpr double DoubleTapSeconds = 0.3;

    ACityLiveRPCharacter* GetCharacter() const;
    ACitySedan* GetVehicle() const;
    static void Layout(const FGeometry&, FVector2D& Left, FVector2D& Right, float& Radius);
    void UpdateStick(const FGeometry&, const FPointerEvent&);
    bool ReleasePointer(int32 PointerIndex);
};
