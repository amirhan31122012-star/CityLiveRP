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
    return Controller.IsValid() && !Controller->GetSeatedSedan() && !Controller->IsVehicleEntryActive()
        ? Cast<ACityLiveRPCharacter>(Controller->GetPawn()) : nullptr;
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
    if (Controller.IsValid() && Controller->IsVehicleEntryActive()) return Layer;
    FVector2D Left, Right;
    float Radius;
    Layout(Geometry, Left, Right, Radius);
    auto Circle = [&](FVector2D Center, float R, FLinearColor Color)
    {
        const FSlateRoundedBoxBrush Face(FLinearColor::White, R);
        const FVector2D Diameter(R * 2.f, R * 2.f);
        FSlateDrawElement::MakeBox(Out, Layer,
            Geometry.ToPaintGeometry(Diameter, FSlateLayoutTransform(Center - FVector2D(R, R) + FVector2D(0, 3))),
            &Face, ESlateDrawEffect::None, FLinearColor(0, 0, 0, .22f));
        const float Activation = FMath::Clamp((Color.G - Color.R) * 2.5f, 0.f, 1.f);
        FSlateDrawElement::MakeBox(Out, Layer,
            Geometry.ToPaintGeometry(Diameter, FSlateLayoutTransform(Center - FVector2D(R, R))),
            &Face, ESlateDrawEffect::None,
            FMath::Lerp(FLinearColor(.035f, .05f, .07f, .45f), FLinearColor(.1f, .4f, .22f, .38f), Activation));
        TArray<FVector2D> Points;
        for (int32 i = 0; i <= 96; ++i)
        {
            const float Angle = 2.f * PI * i / 96.f;
            Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R);
        }
        FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Color * FLinearColor(1, 1, 1, .65f), true, 1.5f);
    };
    auto Stroke = [&](FVector2D Center, std::initializer_list<FVector2D> Shape, float Thickness = .04f)
    {
        // Trim each corner and sample a quadratic curve instead of sharp joins.
        const TArray<FVector2D> Vertices(Shape.begin(), static_cast<int32>(Shape.size()));
        TArray<FVector2D> Points;
        const bool bClosed = Vertices.Num() > 2 && Vertices[0].Equals(Vertices.Last(), .001);
        const int32 Count = bClosed ? Vertices.Num() - 1 : Vertices.Num();
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const FVector2D Point = Vertices[Index];
            if (!bClosed && (Index == 0 || Index == Count - 1)) { Points.Add(Center + Point * Radius); continue; }
            const FVector2D Before = Vertices[(Index + Count - 1) % Count];
            const FVector2D After = Vertices[(Index + 1) % Count];
            const double Cut = FMath::Min(.08, FMath::Min((Before - Point).Size(), (After - Point).Size()) * .3);
            const FVector2D In = Point + (Before - Point).GetSafeNormal() * Cut;
            const FVector2D Next = Point + (After - Point).GetSafeNormal() * Cut;
            for (int32 Step = 0; Step <= 8; ++Step)
            {
                const double T = Step / 8.0;
                Points.Add(Center + (In * ((1-T)*(1-T)) + Point * (2*T*(1-T)) + Next * (T*T)) * Radius);
            }
        }
        if (bClosed && !Points.IsEmpty())
        {
            // TArray rejects Add with a reference to one of its own elements.
            // Copy first: growing the array must not invalidate the source.
            const FVector2D ClosingPoint = Points[0];
            Points.Add(ClosingPoint);
        }
        const float Width = FMath::Max(2.f, Radius * Thickness);
        FSlateDrawElement::MakeLines(Out, Layer + 1, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, FLinearColor(.94f, .97f, 1.f, .95f), true, Width);
        if (!bClosed)
        {
            const FSlateRoundedBoxBrush Cap(FLinearColor::White, Width * .5f);
            for (const FVector2D& End : {Points[0], Points.Last()})
                FSlateDrawElement::MakeBox(Out, Layer + 1,
                    Geometry.ToPaintGeometry(FVector2D(Width, Width), FSlateLayoutTransform(End - FVector2D(Width*.5f, Width*.5f))),
                    &Cap, ESlateDrawEffect::None, FLinearColor(.94f, .97f, 1.f, .95f));
        }
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
        // Sedan front: curved hood, sloped windshield, mirrors, lamps, grille and tires.
        Stroke(CarCenter, {{-.61f,.3f},{-.6f,-.02f},{-.46f,-.18f},{-.33f,-.51f},{-.22f,-.57f},
            {.22f,-.57f},{.33f,-.51f},{.46f,-.18f},{.6f,-.02f},{.61f,.3f},{.5f,.4f},{-.5f,.4f},{-.61f,.3f}});
        Stroke(CarCenter, {{-.4f,-.17f},{-.28f,-.44f},{.28f,-.44f},{.4f,-.17f},{-.4f,-.17f}});
        Stroke(CarCenter, {{-.48f,-.1f},{-.65f,-.14f},{-.67f,-.02f},{-.57f,.02f}});
        Stroke(CarCenter, {{.48f,-.1f},{.65f,-.14f},{.67f,-.02f},{.57f,.02f}});
        Stroke(CarCenter, {{-.48f,.1f},{-.25f,.13f},{-.29f,.22f},{-.49f,.2f},{-.48f,.1f}});
        Stroke(CarCenter, {{.48f,.1f},{.25f,.13f},{.29f,.22f},{.49f,.2f},{.48f,.1f}});
        Stroke(CarCenter, {{-.18f,.17f},{.18f,.17f},{.14f,.28f},{-.14f,.28f},{-.18f,.17f}});
        Stroke(CarCenter, {{-.52f,.4f},{-.52f,.53f},{-.35f,.53f},{-.35f,.4f}});
        Stroke(CarCenter, {{.52f,.4f},{.52f,.53f},{.35f,.53f},{.35f,.4f}});
        Stroke(CarCenter, {{-.48f,.32f},{.48f,.32f}}, .025f);
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
        Stroke(Right, Joints, .09f);
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
