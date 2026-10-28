#include "CitySedan.h"
#include <initializer_list>
#include "CitySedanDynamics.h"
#include "CityEntryMotion.h"
#include "CityVehicleLayout.h"
#include "CityCrouchMeshComponent.h"
#include "Components/SceneComponent.h"
#include "CityLiveRPCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "InputCoreTypes.h"

ACitySedan::ACitySedan()
{
    PrimaryActorTick.bCanEverTick = true;
    Passengers.SetNum(3);
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("VehicleCollision"));
    SetRootComponent(CollisionBox);
    CollisionBox->SetBoxExtent(FVector(215.f, 92.f, 60.f));
    CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto Part = [&](const TCHAR* Name, FVector Position, FVector Scale)
    {
        UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Piece->SetupAttachment(CollisionBox);
        Piece->SetStaticMesh(Cube.Object);
        Piece->SetRelativeLocation(Position);
        Piece->SetRelativeScale3D(Scale);
        Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    };
    // Leave real space for occupants instead of intersecting them with a solid block.
    Part(TEXT("Body"), FVector(0, 0, -35), FVector(4.2f, 1.8f, 0.15f));
    Part(TEXT("LeftSill"), FVector(0, -86, -15), FVector(4.2f, .12f, .3f));
    Part(TEXT("RightSill"), FVector(0, 86, -15), FVector(4.2f, .12f, .3f));
    Part(TEXT("DriverSeat"), FVector(-25, -40, -15), FVector(.65f, .6f, .15f));
    Part(TEXT("PassengerSeat"), FVector(-25, 40, -15), FVector(.65f, .6f, .15f));
    Part(TEXT("RearLeftSeat"), FVector(-95, -40, -15), FVector(.6f, .6f, .15f));
    Part(TEXT("RearRightSeat"), FVector(-95, 40, -15), FVector(.6f, .6f, .15f));
    Part(TEXT("Cabin"), FVector(-25.f, 0.f, 90.f), FVector(2.1f, 1.55f, 0.08f));
    Part(TEXT("FrontWindow"), FVector(77.f, 0.f, 51.f), FVector(.06f, 1.5f, .6f));
    Part(TEXT("RearWindow"), FVector(-127.f, 0.f, 51.f), FVector(.06f, 1.5f, .6f));
    Part(TEXT("Hood"), FVector(145.f, 0.f, 20.f), FVector(1.1f, 1.7f, 0.25f));
    for (int32 Side = 0; Side < 4; ++Side)
    {
        USceneComponent* Hinge = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("DoorHinge%d"), Side));
        Hinge->SetupAttachment(CollisionBox);
        Hinge->SetRelativeLocation(FVector(Side < 2 ? 65.f : -45.f, Side % 2 ? 93.f : -93.f, 20.f));
        UStaticMeshComponent* Door = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DoorPanel%d"), Side));
        Door->SetupAttachment(Hinge);
        Door->SetStaticMesh(Cube.Object);
        Door->SetRelativeLocation(FVector(-55.f, 0.f, 0.f));
        Door->SetRelativeScale3D(FVector(1.1f, .05f, .7f));
        Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UStaticMeshComponent* Handle = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DoorHandle%d"), Side));
        Handle->SetupAttachment(Hinge);
        Handle->SetStaticMesh(Cube.Object);
        Handle->SetRelativeLocation(FVector(-90.f, Side % 2 ? 5.f : -5.f, 22.f));
        Handle->SetRelativeScale3D(FVector(.18f, .07f, .04f));
        Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DoorPivots.Add(Hinge);
    }
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UStaticMeshComponent* Wheel = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wheel%d"), Index));
        Wheel->SetupAttachment(CollisionBox);
        Wheel->SetStaticMesh(Cylinder.Object);
        Wheel->SetRelativeLocation(FVector(Index < 2 ? 135.f : -135.f, Index % 2 ? 95.f : -95.f, -27.f));
        Wheel->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
        Wheel->SetRelativeScale3D(FVector(0.64f, 0.64f, 0.22f));
        Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Wheels.Add(Wheel);
    }
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("VehicleCameraArm"));
    CameraArm->SetupAttachment(CollisionBox);
    CameraArm->TargetArmLength = 650.f;
    CameraArm->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    CameraArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
    CameraArm->bUsePawnControlRotation = false;
    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

bool ACitySedan::FindFlatSpawn(UWorld* World, const FVector& Candidate, float Yaw, AActor* Ignore, FVector& OutLocation)
{
    if (!World) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityFlatParking), false, Ignore);
    const FQuat Rotation = FRotator(0, Yaw, 0).Quaternion();
    float Minimum = TNumericLimits<float>::Max(), Maximum = TNumericLimits<float>::Lowest();
    for (float X : {-220.f, 0.f, 220.f})
    for (float Y : {-115.f, 0.f, 115.f})
    {
        const FVector Point = Candidate + Rotation.RotateVector(FVector(X, Y, 0));
        FHitResult Hit;
        if (!World->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 250), Point - FVector(0, 0, 600), ECC_Visibility, Params)) return false;
        if (Hit.ImpactNormal.Z < .995f) return false;
        Minimum = FMath::Min(Minimum, static_cast<float>(Hit.ImpactPoint.Z));
        Maximum = FMath::Max(Maximum, static_cast<float>(Hit.ImpactPoint.Z));
    }
    if (Maximum - Minimum > 4.f) return false;
    OutLocation = FVector(Candidate.X, Candidate.Y, (Minimum + Maximum) * .5f + 65.f);
    return true;
}

void ACitySedan::ResetInput()
{
    Steering = SmoothedSteering = 0.f;
    bThrottle = bBrake = false;
}

bool ACitySedan::CanEnter(const APlayerController* Player, bool bPassengerSeat) const
{
    return !EnteringCharacter && (bPassengerSeat ? FindPassengerSeat(Player) >= 0 : !Driver) && FMath::Abs(Speed) <= 30.f &&
        Player && Cast<ACityLiveRPCharacter>(Player->GetPawn()) && !IsPassenger(Player) &&
        FVector::DistSquared(GetActorLocation(), Player->GetPawn()->GetActorLocation()) <= FMath::Square(400.f);
}

bool ACitySedan::IsPassenger(const APlayerController* Player) const
{
    return GetPassengerSeat(Player) >= 0;
}

int32 ACitySedan::GetPassengerSeat(const APlayerController* Player) const
{
    if (Player && Player->GetPawn())
        for (int32 Seat = 0; Seat < Passengers.Num(); ++Seat)
            if (Passengers[Seat] && Player->GetPawn() == Passengers[Seat].Get()) return Seat;
    return -2;
}

int32 ACitySedan::FindPassengerSeat(const APlayerController* Player) const
{
    if (!Player || !Player->GetPawn()) return -2;
    const FVector Local = GetActorTransform().InverseTransformPosition(Player->GetPawn()->GetActorLocation());
    return CityVehicleLayout::SelectPassenger(Local.X, Local.Y, {{!!Passengers[0], !!Passengers[1], !!Passengers[2]}});
}

bool ACitySedan::TryEnter(APlayerController* Player, bool bPassengerSeat)
{
    if (!CanEnter(Player, bPassengerSeat) || !HasAuthority()) return false;
    ACityLiveRPCharacter* Occupant = Cast<ACityLiveRPCharacter>(Player->GetPawn());
    EntrySeat = bPassengerSeat ? FindPassengerSeat(Player) : -1;
    if (!BuildDoorRoute(Occupant))
    {
        Player->ClientMessage(TEXT("Не удалось подойти к двери: обойдите препятствие или подойдите ближе к нужной стороне."));
        return false;
    }
    EnteringCharacter = Occupant;
    EnteringPlayer = Player;
    bEnteringPassenger = bPassengerSeat;
    EntryStart = Occupant->GetActorLocation();
    EntryStartRotation = Occupant->GetActorRotation();
    bExiting = false;
    EntryTime = -RouteDuration;
    Occupant->ResetMobileSteering();
    Occupant->SetMobileSprint(false);
    Occupant->GetCharacterMovement()->StopMovementImmediately();
    Occupant->GetCharacterMovement()->DisableMovement();
    Occupant->GetCharacterMovement()->SetComponentTickEnabled(false);
    Occupant->SetActorEnableCollision(false);
    Occupant->SetActorTickEnabled(false);
    Occupant->GetMesh()->AddTickPrerequisiteActor(this);
    ResetInput();
    Speed = 0.f;
    Player->SetViewTargetWithBlend(this, .4f);
    return true;
}

bool ACitySedan::BuildDoorRoute(ACityLiveRPCharacter* Occupant)
{
    const float Side = CityVehicleLayout::Side(EntrySeat);
    const float DoorX = CityVehicleLayout::X(EntrySeat);
    const FVector LocalStart = GetActorTransform().InverseTransformPosition(Occupant->GetActorLocation());
    const float HalfHeight = Occupant->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Radius = Occupant->GetCapsuleComponent()->GetScaledCapsuleRadius();
    FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(CityDoorFloor), false, this);
    FloorParams.AddIgnoredActor(Occupant);
    FCollisionQueryParams WalkParams(SCENE_QUERY_STAT(CityDoorWalk), false, Occupant);
    TArray<TArray<FVector>> Candidates;
    Candidates.Add({FVector(DoorX, Side * 150.f, 0)});
    for (float End : {-300.f, 300.f})
        Candidates.Add({FVector(End, LocalStart.Y, 0), FVector(End, Side * 150.f, 0), FVector(DoorX, Side * 150.f, 0)});
    for (const TArray<FVector>& Candidate : Candidates)
    {
        TArray<FVector> Route;
        FVector Previous = Occupant->GetActorLocation() + FVector(0, 0, 3);
        Route.Add(Previous);
        bool bFree = true;
        for (const FVector& LocalPoint : Candidate)
        {
            FVector Point = GetActorTransform().TransformPosition(LocalPoint);
            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 180), Point - FVector(0, 0, 300), ECC_Visibility, FloorParams)) { bFree = false; break; }
            Point.Z = Hit.ImpactPoint.Z + HalfHeight + 4.f;
            if (GetWorld()->SweepSingleByChannel(Hit, Previous, Point, FQuat::Identity,
                ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), WalkParams)) { bFree = false; break; }
            Route.Add(Point);
            Previous = Point;
        }
        if (!bFree) continue;
        EntryRoute = MoveTemp(Route);
        EntryApproach = EntryRoute.Last();
        RouteLength = 0.f;
        for (int32 I = 1; I < EntryRoute.Num(); ++I) RouteLength += FVector::Distance(EntryRoute[I-1], EntryRoute[I]);
        RouteDuration = FMath::Max(.2f, RouteLength / 180.f);
        return true;
    }
    return false;
}

bool ACitySedan::IsEntering(const APlayerController* Player) const
{
    return EnteringCharacter && EnteringPlayer.Get() == Player;
}

FVector ACitySedan::HandleLocation() const
{
    const int32 DoorIndex = CityVehicleLayout::Door(EntrySeat);
    return DoorPivots[DoorIndex]->GetComponentTransform().TransformPosition(FVector(-90.f, CityVehicleLayout::Side(EntrySeat) * 5.f, 22.f));
}

void ACitySedan::CancelEntry()
{
    if (bExiting && IsValid(EnteringCharacter) && EnteringPlayer.IsValid())
    {
        CompleteExit();
        for (USceneComponent* Hinge : DoorPivots)
            if (IsValid(Hinge)) Hinge->SetRelativeRotation(FRotator::ZeroRotator);
        return;
    }
    if (IsValid(EnteringCharacter))
    {
        EnteringCharacter->SetActorLocationAndRotation(EntryStart, EntryStartRotation);
        EnteringCharacter->SetActorHiddenInGame(false);
        EnteringCharacter->SetActorEnableCollision(true);
        EnteringCharacter->SetActorTickEnabled(true);
        EnteringCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        EnteringCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
        EnteringCharacter->GetCharacterMovement()->SetComponentTickEnabled(true);
        EnteringCharacter->GetMesh()->RemoveTickPrerequisiteActor(this);
        if (UCityCrouchMeshComponent* EntryMesh = Cast<UCityCrouchMeshComponent>(EnteringCharacter->GetMesh()))
        {
            EntryMesh->SetVehicleEntryPose(0, 0, FVector::ZeroVector, false);
            EntryMesh->SetVehicleSeatLegs(0, FVector::ZeroVector, FVector::ZeroVector);
            EntryMesh->SetVehicleFit(0.f);
        }
        if (EnteringPlayer.IsValid()) EnteringPlayer->SetViewTarget(EnteringCharacter);
    }
    for (USceneComponent* Hinge : DoorPivots)
        if (IsValid(Hinge)) Hinge->SetRelativeRotation(FRotator::ZeroRotator);
    EnteringCharacter = nullptr;
    EnteringPlayer.Reset();
    bExiting = false;
}

void ACitySedan::CompleteEntry()
{
    ACityLiveRPCharacter* Occupant = EnteringCharacter;
    APlayerController* Player = EnteringPlayer.Get();
    if (!IsValid(Occupant) || !Player) { CancelEntry(); return; }
    const bool bPassengerSeat = bEnteringPassenger;
    EnteringCharacter = nullptr;
    EnteringPlayer.Reset();
    if (bPassengerSeat) Passengers[EntrySeat] = Occupant;
    else Driver = Occupant;
    Occupant->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    if (UCityCrouchMeshComponent* EntryMesh = Cast<UCityCrouchMeshComponent>(Occupant->GetMesh()))
        EntryMesh->SetVehicleEntryPose(0, 0, FVector::ZeroVector, false);
    Occupant->SetActorHiddenInGame(false);
    Occupant->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Occupant->SetActorRelativeLocation(FVector(CityVehicleLayout::X(EntrySeat), CityVehicleLayout::Side(EntrySeat) * 40.f, 10.f));
    UpdateSeatedPose(Occupant, EntrySeat);
    if (!bPassengerSeat)
    {
        ResetInput();
        Player->Possess(this);
    }
    Player->SetViewTarget(this);
}

bool ACitySedan::TryExit(APlayerController* Player)
{
    if (EnteringCharacter || !Player || FMath::Abs(Speed) > 30.f) return false;
    const bool bPassengerExit = IsPassenger(Player);
    EntrySeat = bPassengerExit ? GetPassengerSeat(Player) : -1;
    ACityLiveRPCharacter* Occupant = bPassengerExit ? Passengers[EntrySeat].Get() : Driver.Get();
    if (!Occupant || (!bPassengerExit && Player->GetPawn() != this)) return false;
    const float Radius = Occupant->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = Occupant->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CitySedanExit), false, this);
    Params.AddIgnoredActor(Occupant);
    for (float DoorOffsetX : {-25.f, -65.f, 15.f})
    {
        FVector ExitLocation = GetActorTransform().TransformPosition(FVector(CityVehicleLayout::X(EntrySeat) + DoorOffsetX + 25.f,
            CityVehicleLayout::Side(EntrySeat) * 150.f, 0.f));
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, ExitLocation + FVector(0, 0, 150),
            ExitLocation - FVector(0, 0, 300), ECC_Visibility, Params)) continue;
        ExitLocation.Z = Floor.ImpactPoint.Z + HalfHeight + 3.f;
        if (GetWorld()->OverlapBlockingTestByChannel(ExitLocation, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)) continue;
        EnteringCharacter = Occupant;
        EnteringPlayer = Player;
        bEnteringPassenger = bPassengerExit;
        bExiting = true;
        EntryTime = 0.f;
        EntryApproach = ExitLocation;
        EntryStart = Occupant->GetActorLocation();
        EntryStartRotation = GetActorRotation();
        Occupant->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Occupant->GetMesh()->AddTickPrerequisiteActor(this);
        ResetInput();
        Speed = 0.f;
        Player->SetViewTargetWithBlend(this, .3f);
        return true;
    }
    return false;
}

void ACitySedan::CompleteExit()
{
    ACityLiveRPCharacter* Occupant = EnteringCharacter;
    APlayerController* Player = EnteringPlayer.Get();
    if (!IsValid(Occupant) || !Player) { CancelEntry(); return; }
    const bool bWasPassenger = bEnteringPassenger;
    if (bWasPassenger) Passengers[EntrySeat] = nullptr;
    else Driver = nullptr;
    EnteringCharacter = nullptr;
    EnteringPlayer.Reset();
    bExiting = false;
    Occupant->SetActorLocation(EntryApproach);
    Occupant->GetMesh()->RemoveTickPrerequisiteActor(this);
    if (UCityCrouchMeshComponent* EntryMesh = Cast<UCityCrouchMeshComponent>(Occupant->GetMesh()))
    {
        EntryMesh->SetVehicleEntryPose(0, 0, FVector::ZeroVector, false);
        EntryMesh->SetVehicleSeatLegs(0, FVector::ZeroVector, FVector::ZeroVector);
        EntryMesh->SetVehicleFit(0.f);
    }
    Occupant->SetActorEnableCollision(true);
    Occupant->SetActorTickEnabled(true);
    Occupant->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    Occupant->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Occupant->GetCharacterMovement()->SetComponentTickEnabled(true);
    if (!bWasPassenger) Player->Possess(Occupant);
    Player->SetViewTargetWithBlend(Occupant, .4f);
}

void ACitySedan::UpdateSeatedPose(ACityLiveRPCharacter* Occupant, int32 Seat)
{
    if (!IsValid(Occupant) || Occupant == EnteringCharacter.Get()) return;
    if (UCityCrouchMeshComponent* SeatMesh = Cast<UCityCrouchMeshComponent>(Occupant->GetMesh()))
    {
        const float SeatY = CityVehicleLayout::Side(Seat) * 40.f;
        SeatMesh->SetVehicleFit(1.f);
        SeatMesh->SetVehicleSeatLegs(1.f,
            GetActorTransform().TransformPosition(FVector(CityVehicleLayout::X(Seat) + 50.f, SeatY - 11.f, -25)),
            GetActorTransform().TransformPosition(FVector(CityVehicleLayout::X(Seat) + 50.f, SeatY + 11.f, -25)));
    }
}

void ACitySedan::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    EntryCameraBlend = FMath::FInterpTo(EntryCameraBlend, EnteringCharacter ? 1.f : 0.f, DeltaTime, 5.f);
    if (EnteringCharacter)
    {
        UpdateSeatedPose(Driver, -1);
        for (int32 Seat = 0; Seat < Passengers.Num(); ++Seat) UpdateSeatedPose(Passengers[Seat], Seat);
        UpdateEntry(DeltaTime);
        return;
    }
    const APlayerController* DrivingPlayer = Cast<APlayerController>(GetController());
    const bool KeyboardGas = DrivingPlayer && DrivingPlayer->IsInputKeyDown(EKeys::W);
    const bool KeyboardBrake = DrivingPlayer && DrivingPlayer->IsInputKeyDown(EKeys::S);
    float KeyboardSteering = 0.f;
    if (DrivingPlayer)
    {
        KeyboardSteering += DrivingPlayer->IsInputKeyDown(EKeys::D) ? 1.f : 0.f;
        KeyboardSteering -= DrivingPlayer->IsInputKeyDown(EKeys::A) ? 1.f : 0.f;
    }
    Speed = CityLiveDriving::AdvanceSpeed(Speed, Driver != nullptr, bThrottle || KeyboardGas, bBrake || KeyboardBrake, DeltaTime);
    SmoothedSteering = FMath::FInterpTo(SmoothedSteering, FMath::Clamp(Steering + KeyboardSteering, -1.f, 1.f), DeltaTime, 5.f);
    if (Driver && FMath::Abs(Speed) > 5.f)
        AddActorWorldRotation(FRotator(0, SmoothedSteering * 55.f * FMath::Clamp(Speed / 350.f, -1.f, 1.f) * DeltaTime, 0));
    FHitResult MovementHit;
    AddActorWorldOffset(GetActorForwardVector() * Speed * DeltaTime, true, &MovementHit);
    if (MovementHit.IsValidBlockingHit()) Speed = 0.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CitySedanFloor), false, this);
    if (Driver) Params.AddIgnoredActor(Driver);
    for (ACityLiveRPCharacter* Occupant : Passengers) if (Occupant) Params.AddIgnoredActor(Occupant);
    FHitResult Floor;
    const FVector Position = GetActorLocation();
    if (GetWorld()->LineTraceSingleByChannel(Floor, Position, Position - FVector(0, 0, 100), ECC_Visibility, Params))
    {
        FallSpeed = 0.f;
        const float DesiredZ = Floor.ImpactPoint.Z + 61.f;
        AddActorWorldOffset(FVector(0, 0, DesiredZ - Position.Z), true);
    }
    else
    {
        FallSpeed -= 980.f * DeltaTime;
        AddActorWorldOffset(FVector(0, 0, FallSpeed * DeltaTime), true);
    }
    for (UStaticMeshComponent* Wheel : Wheels)
        Wheel->AddLocalRotation(FRotator(0, -FMath::RadiansToDegrees(Speed * DeltaTime / 32.f), 0));
    UpdateSeatedPose(Driver, -1);
    for (int32 Seat = 0; Seat < Passengers.Num(); ++Seat) UpdateSeatedPose(Passengers[Seat], Seat);
}

void ACitySedan::UpdateEntry(float DeltaTime)
{
    if (!IsValid(EnteringCharacter) || !EnteringPlayer.IsValid()) { CancelEntry(); return; }
    EntryTime += DeltaTime;
    const float Phase = bExiting ? CityLiveEntry::Duration - EntryTime : EntryTime;
    const float Sit = CityLiveEntry::Sit(Phase);
    const float Side = CityVehicleLayout::Side(EntrySeat);
    const float SeatY = Side * 40.f;
    const float SeatX = CityVehicleLayout::X(EntrySeat);
    const FVector Seat = GetActorTransform().TransformPosition(FVector(SeatX, SeatY, 10.f));
    FVector Position = FMath::Lerp(EntryApproach, Seat, Sit);
    if (!bExiting && EntryTime < 0.f)
    {
        float Distance = FMath::Clamp((EntryTime + RouteDuration) / RouteDuration, 0.f, 1.f) * RouteLength;
        for (int32 I = 1; I < EntryRoute.Num(); ++I)
        {
            const float Segment = FVector::Distance(EntryRoute[I-1], EntryRoute[I]);
            if (Distance <= Segment || I == EntryRoute.Num()-1)
            {
                Position = FMath::Lerp(EntryRoute[I-1], EntryRoute[I], Segment > .001f ? FMath::Clamp(Distance / Segment, 0.f, 1.f) : 1.f);
                break;
            }
            Distance -= Segment;
        }
    }
    const FRotator FacingDoor = (HandleLocation() - EntryApproach).Rotation();
    FRotator DesiredRotation = FRotator(0, FacingDoor.Yaw, 0);
    if (Sit > 0.f) DesiredRotation = FQuat::Slerp(DesiredRotation.Quaternion(), GetActorQuat(), Sit).Rotator();
    const FVector Previous = EnteringCharacter->GetActorLocation();
    const FVector WalkDirection = Position - Previous;
    if (Phase < 0.f && WalkDirection.SizeSquared2D() > .1f) DesiredRotation = FRotator(0, WalkDirection.Rotation().Yaw, 0);
    EnteringCharacter->SetActorLocationAndRotation(Position,
        FMath::RInterpTo(EnteringCharacter->GetActorRotation(), DesiredRotation, DeltaTime, 7.f));
    EnteringCharacter->GetCharacterMovement()->Velocity = DeltaTime > 0.f && Sit <= 0.f ? (Position - Previous) / DeltaTime : FVector::ZeroVector;
    DoorPivots[CityVehicleLayout::Door(EntrySeat)]->SetRelativeRotation(
        FRotator(0, -Side * 70.f * CityLiveEntry::Door(Phase), 0));
    if (UCityCrouchMeshComponent* EntryMesh = Cast<UCityCrouchMeshComponent>(EnteringCharacter->GetMesh()))
    {
        EntryMesh->SetVehicleEntryPose(0, CityLiveEntry::Hand(Phase), HandleLocation(), Side < 0.f);
        // Reach cabin size before the head crosses the roof edge; restore after clearing it.
        EntryMesh->SetVehicleFit(FMath::Clamp(Sit * 1.35f, 0.f, 1.f));
        FVector Outside = GetActorTransform().TransformPosition(FVector(SeatX, Side * 112.f, 0));
        Outside.Z = EntryApproach.Z - EnteringCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.f;
        const FVector OutsideLeft = Outside - GetActorForwardVector() * 12.f;
        const FVector OutsideRight = Outside + GetActorForwardVector() * 12.f;
        const FVector InsideLeft = GetActorTransform().TransformPosition(FVector(SeatX + 50.f, SeatY-11.f, -25));
        const FVector InsideRight = GetActorTransform().TransformPosition(FVector(SeatX + 50.f, SeatY+11.f, -25));
        EntryMesh->SetVehicleSeatLegs(Sit,
            FMath::Lerp(OutsideLeft, InsideLeft, CityLiveEntry::Leg(Phase, Side > 0.f)),
            FMath::Lerp(OutsideRight, InsideRight, CityLiveEntry::Leg(Phase, Side < 0.f)));
    }
    if (EntryTime >= CityLiveEntry::Duration)
    {
        if (bExiting) CompleteExit();
        else CompleteEntry();
    }
}

void ACitySedan::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    VehicleCamera->GetCameraView(DeltaTime, OutResult);
    if (EntryCameraBlend <= .001f) return;
    const FVector Focus = GetActorLocation() + FVector(0, 0, 40);
    FVector SideView = GetActorLocation() + GetActorRightVector() * (CityVehicleLayout::Side(EntrySeat) * 430.f)
        - GetActorForwardVector() * 180.f + FVector(0, 0, 180);
    FCollisionQueryParams ViewParams(SCENE_QUERY_STAT(CityEntryCamera), false, this);
    if (EnteringCharacter) ViewParams.AddIgnoredActor(EnteringCharacter);
    if (Driver) ViewParams.AddIgnoredActor(Driver);
    for (ACityLiveRPCharacter* Occupant : Passengers) if (Occupant) ViewParams.AddIgnoredActor(Occupant);
    FHitResult ViewHit;
    if (GetWorld()->SweepSingleByChannel(ViewHit, Focus, SideView, FQuat::Identity,
        ECC_Camera, FCollisionShape::MakeSphere(12.f), ViewParams)) SideView = ViewHit.Location;
    OutResult.Location = FMath::Lerp(OutResult.Location, SideView, EntryCameraBlend);
    OutResult.Rotation = FQuat::Slerp(OutResult.Rotation.Quaternion(), (Focus - SideView).Rotation().Quaternion(), EntryCameraBlend).Rotator();
}

void ACitySedan::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelEntry();
    Super::EndPlay(EndPlayReason);
}
