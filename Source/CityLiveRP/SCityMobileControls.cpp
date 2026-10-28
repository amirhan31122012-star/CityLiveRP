#include "SCityMobileControls.h"
#include <initializer_list>
#include "CityLiveRPCharacter.h"
#include "CitySedan.h"
#include "CityLiveRPPlayerController.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "HAL/PlatformTime.h"
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
    return Controller.IsValid() && !Controller->GetSeatedSedan() ? Cast<ACityLiveRPCharacter>(Controller->GetPawn()) : nullptr;
}

ACitySedan* SCityMobileControls::GetVehicle() const
{
    return Controller.IsValid() ? Controller->GetSeatedSedan() : nullptr;
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
    if (ACitySedan* Vehicle = GetVehicle()) Vehicle->SetSteering(Stick.X);
    if (ACityLiveRPCharacter* Character = GetCharacter())
    {
        const float Steering = FMath::Abs(Stick.X) < 0.12f ? 0.f : Stick.X;
        const float Forward = FMath::Abs(Stick.Y) < 0.12f ? 0.f : -Stick.Y;
        Character->SetMobileSteering(Steering, Forward);
    }
}

FReply SCityMobileControls::OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event)
{
    ACityLiveRPCharacter* Character = GetCharacter();
    ACitySedan* Vehicle = GetVehicle();
    if (!Character && !Vehicle) return FReply::Unhandled();
    if (ControlledCharacter.Get() != Character || ControlledVehicle.Get() != Vehicle)
    {
        ResetControls();
        ControlledCharacter = Character;
        ControlledVehicle = Vehicle;
    }
    FVector2D Left, Right;
    float Radius;
    Layout(Geometry, Left, Right, Radius);
    const FVector2D Position = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    const FVector2D Interaction(Right.X, Right.Y - Radius * 2.4f);
    if (Controller.IsValid() && Controller->CanUsePassengerDoor() &&
        FVector2D::Distance(Position, Interaction) <= Radius)
    {
        Controller->UsePassengerDoor();
        return FReply::Handled();
    }
    if (Controller.IsValid() && !Vehicle && Controller->CanInteractWithSedan() &&
        FVector2D::Distance(Position, Interaction - FVector2D(Radius * 2.2f, 0)) <= Radius)
    {
        Controller->InteractWithSedan();
        return FReply::Handled();
    }
    if (Vehicle && !Controller->IsSedanDriver()) return FReply::Unhandled();
    if (Vehicle && BrakePointer == INDEX_NONE &&
        FVector2D::Distance(Position, Right - FVector2D(Radius * 2.2f, 0)) <= Radius)
    {
        BrakePointer = Event.GetPointerIndex();
        Vehicle->SetBrake(true);
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    const double Now = FPlatformTime::Seconds();
    if (StickPointer == INDEX_NONE && FVector2D::Distance(Position, Left) <= Radius * 1.3f)
    {
        StickPointer = Event.GetPointerIndex();
        if (Character && Now - LastStickTap <= DoubleTapSeconds)
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
        if (Vehicle) Vehicle->SetThrottle(true);
        if (Character) Character->SetMobileSprint(true);
        if (Character && Now - LastRunTap <= DoubleTapSeconds)
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
    return Event.GetPointerIndex() == RunPointer || Event.GetPointerIndex() == BrakePointer ? FReply::Handled() : FReply::Unhandled();
}

FReply SCityMobileControls::OnTouchEnded(const FGeometry&, const FPointerEvent& Event)
{
    if (Event.GetPointerIndex() == BrakePointer)
    {
        BrakePointer = INDEX_NONE;
        if (ACitySedan* Vehicle = GetVehicle()) Vehicle->SetBrake(false);
        return FReply::Handled().ReleaseMouseCapture();
    }
    if (Event.GetPointerIndex() == StickPointer)
    {
        StickPointer = INDEX_NONE;
        Stick = FVector2D::ZeroVector;
        if (ACitySedan* Vehicle = GetVehicle()) Vehicle->SetSteering(0.f);
        if (ACityLiveRPCharacter* Character = GetCharacter()) Character->SetMobileSteering(0.f, 0.f);
        return FReply::Handled().ReleaseMouseCapture();
    }
    if (Event.GetPointerIndex() == RunPointer)
    {
        RunPointer = INDEX_NONE;
        bRunHeld = false;
        if (ACitySedan* Vehicle = GetVehicle()) Vehicle->SetThrottle(false);
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
    if (Controller.IsValid() && Controller->IsSedanDriver())
        if (ACitySedan* Vehicle = ControlledVehicle.Get()) Vehicle->ResetInput();
    if (ACityLiveRPCharacter* Character = ControlledCharacter.Get())
    {
        Character->SetMobileSprint(false);
        Character->DoJumpEnd();
        Character->ResetMobileSteering();
    }
    Stick = FVector2D::ZeroVector;
    StickPointer = RunPointer = BrakePointer = INDEX_NONE;
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
    ACitySedan* Vehicle = GetVehicle();
    if (ControlledCharacter.Get() != Character || ControlledVehicle.Get() != Vehicle)
    {
        ResetControls();
        ControlledCharacter = Character;
        ControlledVehicle = Vehicle;
    }
    RunHighlight = FMath::FInterpTo(RunHighlight, bRunHeld ? 1.f : 0.f, DeltaTime, 14.f);
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
    auto Stroke = [&](FVector2D Center, std::initializer_list<FVector2D> Shape)
    {
        TArray<FVector2D> Points;
        for (const FVector2D& Point : Shape) Points.Add(Center + Point * Radius);
        FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, FLinearColor::White, true, FMath::Max(2.f, Radius * 0.045f));
    };
    const FVector2D DoorCenter(Right.X, Right.Y - Radius * 2.4f);
    if (Controller.IsValid() && Controller->CanUsePassengerDoor())
    {
        Circle(DoorCenter, Radius * 0.85f, FLinearColor::White);
        Stroke(DoorCenter, {{-.45f, .5f}, {-.45f, -.5f}, {.4f, -.32f}, {.4f, .5f}, {-.45f, .5f}});
        Stroke(DoorCenter, {{-.32f, -.34f}, {.26f, -.22f}, {.26f, .02f}, {-.32f, .02f}, {-.32f, -.34f}});
        Stroke(DoorCenter, {{.12f, .2f}, {.27f, .2f}});
    }
    if (Controller.IsValid() && !GetVehicle() && Controller->CanInteractWithSedan())
    {
        const FVector2D CarCenter = DoorCenter - FVector2D(Radius * 2.2f, 0);
        Circle(CarCenter, Radius * 0.85f, FLinearColor::White);
        Stroke(CarCenter, {{-.58f, .42f}, {-.58f, -.04f}, {-.35f, -.5f}, {.35f, -.5f}, {.58f, -.04f}, {.58f, .42f}, {-.58f, .42f}});
        Stroke(CarCenter, {{-.48f, -.04f}, {.48f, -.04f}});
        Stroke(CarCenter, {{-.4f, .42f}, {-.4f, .58f}});
        Stroke(CarCenter, {{.4f, .42f}, {.4f, .58f}});
        Stroke(CarCenter, {{-.43f, .18f}, {-.23f, .18f}});
        Stroke(CarCenter, {{.23f, .18f}, {.43f, .18f}});
    }
    // A passenger sees only the exit door and cannot operate the pedals or steering.
    if (GetVehicle() && Controller.IsValid() && !Controller->IsSedanDriver()) return Layer + 1;
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

    if (GetVehicle())
    {
        const FVector2D Brake = Right - FVector2D(Radius * 2.2f, 0);
        Circle(Brake, Radius * 0.85f, BrakePointer != INDEX_NONE ? FLinearColor(.55f, .9f, .65f, .7f) : FLinearColor::White);
        // Accelerator: tall narrow pedal, brake: short wide pedal, each with rubber grooves.
        Stroke(Right, {{-.23f, -.6f}, {.32f, -.6f}, {.22f, .6f}, {-.33f, .6f}, {-.23f, -.6f}});
        for (int32 Groove = 0; Groove < 5; ++Groove)
        {
            const float Y = -.4f + Groove * .2f;
            const float X = -.02f - Groove * .016f;
            Stroke(Right, {{X - .14f, Y}, {X + .18f, Y}});
        }
        Stroke(Brake, {{-.58f, -.28f}, {.58f, -.28f}, {.58f, .3f}, {-.58f, .3f}, {-.58f, -.28f}});
        Stroke(Brake, {{0.f, .3f}, {0.f, .6f}, {.3f, .6f}});
        for (int32 Groove = 0; Groove < 4; ++Groove)
        {
            const float X = -.36f + Groove * .24f;
            Stroke(Brake, {{X, -.16f}, {X, .17f}});
        }
        return Layer + 1;
    }

    // Draw a resolution-independent running figure instead of a text label.
    const FLinearColor IconColor(0.95f, 0.98f, 0.95f, 0.95f);
    auto Limb = [&](std::initializer_list<FVector2D> Joints)
    {
        TArray<FVector2D> Points;
        for (const FVector2D& Joint : Joints) Points.Add(Right + Joint * Radius);
        FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(),
            Points, ESlateDrawEffect::None, IconColor, true, Radius * 0.115f);
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
