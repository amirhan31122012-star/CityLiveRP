#include "SCityMobileControls.h"
#include "CityLiveRPCharacter.h"
#include "CityLiveRPPlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "HAL/PlatformTime.h"
#include "Camera/PlayerCameraManager.h"
#include "Misc/CoreDelegates.h"

void SCityMobileControls::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    ForceVolatile(true);
    FCoreDelegates::ApplicationWillDeactivateDelegate.AddSP(this, &SCityMobileControls::ResetControls);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddSP(this, &SCityMobileControls::ResetControls);
}

ACityLiveRPCharacter* SCityMobileControls::GetCharacter() const
{
    return Controller.IsValid() ? Cast<ACityLiveRPCharacter>(Controller->GetPawn()) : nullptr;
}

void SCityMobileControls::Layout(const FGeometry& Geometry, FVector2D& Left, FVector2D& Right, float& Radius)
{
    const FVector2D Size = Geometry.GetLocalSize();
    Radius = FMath::Min(Size.X, Size.Y) * 0.12f;
    Left = FVector2D(Size.X * 0.18f, Size.Y * 0.76f);
    Right = FVector2D(Size.X * 0.82f, Size.Y * 0.76f);
}

void SCityMobileControls::UpdateStick(const FGeometry& Geometry, const FPointerEvent& Event)
{
    FVector2D Left, Right;
    float Radius;
    Layout(Geometry, Left, Right, Radius);
    Stick = (Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()) - Left) / FMath::Max(Radius, 1.f);
    Stick = Stick.GetClampedToMaxSize(1.f);
    if (Stick.Size() < 0.12f) Stick = FVector2D::ZeroVector;
}

FReply SCityMobileControls::OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event)
{
    ACityLiveRPCharacter* Character = GetCharacter();
    if (!Character) return FReply::Unhandled();
    if (ControlledCharacter.Get() != Character)
    {
        ResetControls();
        ControlledCharacter = Character;
    }
    FVector2D Left, Right;
    float Radius;
    Layout(Geometry, Left, Right, Radius);
    const FVector2D Position = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    const double Now = FPlatformTime::Seconds();
    if (StickPointer == INDEX_NONE && FVector2D::Distance(Position, Left) <= Radius * 1.3f)
    {
        StickPointer = Event.GetPointerIndex();
        // Lock the movement reference for this gesture: the trailing camera must
        // not rotate the movement vector again each frame (which causes circling).
        MovementYaw = Controller->PlayerCameraManager->GetCameraRotation().Yaw;
        if (Now - LastStickTap <= DoubleTapSeconds)
        {
            Character->ToggleMobileCrouch();
            LastStickTap = -100.0;
        }
        else LastStickTap = Now;
        UpdateStick(Geometry, Event);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    if (RunPointer == INDEX_NONE && FVector2D::Distance(Position, Right) <= Radius)
    {
        RunPointer = Event.GetPointerIndex();
        bRunHeld = true;
        Character->SetMobileSprint(true);
        if (Now - LastRunTap <= DoubleTapSeconds)
        {
            Character->DoJumpStart();
            LastRunTap = -100.0;
        }
        else LastRunTap = Now;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    return FReply::Unhandled();
}

FReply SCityMobileControls::OnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetPointerIndex() == StickPointer)
    {
        UpdateStick(Geometry, Event);
        return FReply::Handled();
    }
    return Event.GetPointerIndex() == RunPointer ? FReply::Handled() : FReply::Unhandled();
}

FReply SCityMobileControls::OnTouchEnded(const FGeometry&, const FPointerEvent& Event)
{
    if (Event.GetPointerIndex() == StickPointer)
    {
        StickPointer = INDEX_NONE;
        Stick = FVector2D::ZeroVector;
        return FReply::Handled().ReleaseMouseCapture();
    }
    if (Event.GetPointerIndex() == RunPointer)
    {
        RunPointer = INDEX_NONE;
        bRunHeld = false;
        if (ACityLiveRPCharacter* Character = GetCharacter())
        {
            Character->SetMobileSprint(false);
            Character->DoJumpEnd();
        }
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

void SCityMobileControls::ResetControls()
{
    if (ACityLiveRPCharacter* Character = ControlledCharacter.Get())
    {
        Character->SetMobileSprint(false);
        Character->DoJumpEnd();
    }
    Stick = FVector2D::ZeroVector;
    StickPointer = RunPointer = INDEX_NONE;
    bRunHeld = false;
    LastStickTap = LastRunTap = -100.0;
}

void SCityMobileControls::OnFocusLost(const FFocusEvent& Event)
{
    ResetControls();
    SLeafWidget::OnFocusLost(Event);
}

void SCityMobileControls::Tick(const FGeometry& Geometry, double Time, float DeltaTime)
{
    SLeafWidget::Tick(Geometry, Time, DeltaTime);
    ACityLiveRPCharacter* Character = GetCharacter();
    if (ControlledCharacter.Get() != Character)
    {
        ResetControls();
        ControlledCharacter = Character;
    }
    if (Character && !Stick.IsNearlyZero())
    {
        const FRotationMatrix Basis(FRotator(0, MovementYaw, 0));
        Character->AddMovementInput(Basis.GetUnitAxis(EAxis::X), -Stick.Y);
        Character->AddMovementInput(Basis.GetUnitAxis(EAxis::Y), Stick.X);
    }
}

int32 SCityMobileControls::OnPaint(const FPaintArgs&, const FGeometry& Geometry,
    const FSlateRect&, FSlateWindowElementList& Out, int32 Layer,
    const FWidgetStyle&, bool) const
{
    FVector2D Left, Right;
    float Radius;
    Layout(Geometry, Left, Right, Radius);
    auto Circle = [&](FVector2D Center, float R, FLinearColor Color)
    {
        TArray<FVector2D> Points;
        for (int32 i = 0; i <= 48; ++i)
        {
            const float Angle = 2.f * PI * i / 48.f;
            Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R);
        }
        FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Color, true, 3.f);
    };
    Circle(Left, Radius, FLinearColor(1, 1, 1, 0.6f));
    Circle(Left + Stick * Radius * 0.65f, Radius * 0.32f, FLinearColor::White);
    Circle(Right, Radius * 0.85f, bRunHeld ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor::White);
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(12, FMath::RoundToInt(Radius * 0.28f)));
    FSlateDrawElement::MakeText(Out, Layer + 1,
        Geometry.ToPaintGeometry(FVector2D(Radius * 1.4f, Radius * 0.5f), FSlateLayoutTransform(Right - FVector2D(Radius * 0.36f, Radius * 0.2f))),
        FText::FromString(TEXT("БЕГ")), Font, ESlateDrawEffect::None, FLinearColor::White);
    return Layer + 1;
}
