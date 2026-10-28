#include "SCityMobileControls.h"
#include <initializer_list>
#include "CityLiveRPCharacter.h"
#include "CityLiveRPPlayerController.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"
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
    RunHighlight = 0.f;
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
    RunHighlight = FMath::FInterpTo(RunHighlight, bRunHeld ? 1.f : 0.f, DeltaTime, 14.f);
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
    // A low-opacity green fill gives feedback without obscuring the game.
    const FSlateRoundedBoxBrush Background(FLinearColor::White, Radius * 0.85f);
    FSlateDrawElement::MakeBox(Out, Layer,
        Geometry.ToPaintGeometry(FVector2D(Radius * 1.7f, Radius * 1.7f),
            FSlateLayoutTransform(Right - FVector2D(Radius * 0.85f, Radius * 0.85f))),
        &Background, ESlateDrawEffect::None,
        FMath::Lerp(FLinearColor(0.08f, 0.08f, 0.08f, 0.18f),
            FLinearColor(0.25f, 0.85f, 0.4f, 0.18f), RunHighlight));
    Circle(Right, Radius * 0.85f,
        FMath::Lerp(FLinearColor(1.f, 1.f, 1.f, 0.65f),
            FLinearColor(0.55f, 0.9f, 0.65f, 0.7f), RunHighlight));

    // Draw a resolution-independent running figure instead of a text label.
    const FLinearColor IconColor(0.95f, 0.98f, 0.95f, 0.95f);
    auto Limb = [&](std::initializer_list<FVector2D> Joints)
    {
        TArray<FVector2D> Points;
        for (const FVector2D& Joint : Joints) Points.Add(Right + Joint * Radius);
        FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(),
            Points, ESlateDrawEffect::None, IconColor, true, Radius * 0.09f);
    };
    const FVector2D Head = Right + FVector2D(0.16f, -0.48f) * Radius;
    const float HeadRadius = Radius * 0.115f;
    const FSlateRoundedBoxBrush HeadBrush(FLinearColor::White, HeadRadius);
    FSlateDrawElement::MakeBox(Out, Layer + 1,
        Geometry.ToPaintGeometry(FVector2D(HeadRadius * 2.f, HeadRadius * 2.f),
            FSlateLayoutTransform(Head - FVector2D(HeadRadius, HeadRadius))),
        &HeadBrush, ESlateDrawEffect::None, IconColor);
    Limb({FVector2D(0.09f, -0.29f), FVector2D(-0.12f, 0.14f)});
    Limb({FVector2D(0.04f, -0.19f), FVector2D(0.30f, -0.04f), FVector2D(0.46f, -0.20f)});
    Limb({FVector2D(0.02f, -0.17f), FVector2D(-0.28f, -0.25f), FVector2D(-0.45f, -0.02f)});
    Limb({FVector2D(-0.12f, 0.14f), FVector2D(0.16f, 0.30f), FVector2D(0.23f, 0.56f)});
    Limb({FVector2D(-0.12f, 0.14f), FVector2D(-0.35f, 0.39f), FVector2D(-0.55f, 0.35f)});
    return Layer + 1;
}
