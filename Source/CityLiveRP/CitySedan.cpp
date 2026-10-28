#include "CitySedan.h"
#include <initializer_list>
#include "CitySedanDynamics.h"
#include "CityEntryMotion.h"
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
    Part(TEXT("Body"), FVector::ZeroVector, FVector(4.2f, 1.8f, 0.55f));
    Part(TEXT("Cabin"), FVector(-25.f, 0.f, 82.f), FVector(2.1f, 1.55f, 0.08f));
    Part(TEXT("FrontWindow"), FVector(77.f, 0.f, 51.f), FVector(.06f, 1.5f, .6f));
    Part(TEXT("RearWindow"), FVector(-127.f, 0.f, 51.f), FVector(.06f, 1.5f, .6f));
    Part(TEXT("Hood"), FVector(145.f, 0.f, 20.f), FVector(1.1f, 1.7f, 0.25f));
    for (int32 Side = 0; Side < 2; ++Side)
    {
        USceneComponent* Hinge = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("DoorHinge%d"), Side));
        Hinge->SetupAttachment(CollisionBox);
        Hinge->SetRelativeLocation(FVector(65.f, Side ? 93.f : -93.f, 20.f));
        UStaticMeshComponent* Door = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DoorPanel%d"), Side));
        Door->SetupAttachment(Hinge);
        Door->SetStaticMesh(Cube.Object);
        Door->SetRelativeLocation(FVector(-55.f, 0.f, 0.f));
        Door->SetRelativeScale3D(FVector(1.1f, .05f, .7f));
        Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UStaticMeshComponent* Handle = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DoorHandle%d"), Side));
        Handle->SetupAttachment(Hinge);
        Handle->SetStaticMesh(Cube.Object);
        Handle->SetRelativeLocation(FVector(-90.f, Side ? 5.f : -5.f, 22.f));
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

void ACitySedan::ResetInput()
{
    Steering = SmoothedSteering = 0.f;
    bThrottle = bBrake = false;
}

bool ACitySedan::CanEnter(const APlayerController* Player, bool bPassengerSeat) const
{
    return !EnteringCharacter && !(bPassengerSeat ? Passenger : Driver) && FMath::Abs(Speed) <= 30.f &&
        Player && Cast<ACityLiveRPCharacter>(Player->GetPawn()) && Player->GetPawn() != Passenger.Get() &&
        FVector::DistSquared(GetActorLocation(), Player->GetPawn()->GetActorLocation()) <= FMath::Square(400.f);
}

bool ACitySedan::IsPassenger(const APlayerController* Player) const
{
    return Player && Passenger && Player->GetPawn() == Passenger;
}

bool ACitySedan::TryEnter(APlayerController* Player, bool bPassengerSeat)
{
    if (!CanEnter(Player, bPassengerSeat) || !HasAuthority()) return false;
    ACityLiveRPCharacter* Occupant = Cast<ACityLiveRPCharacter>(Player->GetPawn());
    const FVector SidePosition = GetActorTransform().TransformPosition(FVector(-25.f, bPassengerSeat ? 165.f : -165.f, 0.f));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityEntryPosition), false, this);
    Params.AddIgnoredActor(Occupant);
    FHitResult Floor;
    if (!GetWorld()->LineTraceSingleByChannel(Floor, SidePosition + FVector(0, 0, 160),
        SidePosition - FVector(0, 0, 250), ECC_Visibility, Params)) return false;
    const float HalfHeight = Occupant->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    EntryApproach = FVector(SidePosition.X, SidePosition.Y, Floor.ImpactPoint.Z + HalfHeight + 2.f);
    if (GetWorld()->SweepSingleByChannel(Floor, Occupant->GetActorLocation(), EntryApproach, FQuat::Identity,
        ECC_Pawn, FCollisionShape::MakeCapsule(Occupant->GetCapsuleComponent()->GetScaledCapsuleRadius(), HalfHeight), Params)) return false;
    EnteringCharacter = Occupant;
    EnteringPlayer = Player;
    bEnteringPassenger = bPassengerSeat;
    EntryStart = Occupant->GetActorLocation();
    EntryStartRotation = Occupant->GetActorRotation();
    EntryTime = 0.f;
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

bool ACitySedan::IsEntering(const APlayerController* Player) const
{
    return EnteringCharacter && EnteringPlayer.Get() == Player;
}

FVector ACitySedan::HandleLocation() const
{
    const int32 Side = bEnteringPassenger ? 1 : 0;
    return DoorPivots[Side]->GetComponentTransform().TransformPosition(FVector(-90.f, bEnteringPassenger ? 5.f : -5.f, 22.f));
}

void ACitySedan::CancelEntry()
{
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
            EntryMesh->SetVehicleEntryPose(0, 0, FVector::ZeroVector, false);
        if (EnteringPlayer.IsValid()) EnteringPlayer->SetViewTarget(EnteringCharacter);
    }
    for (USceneComponent* Hinge : DoorPivots)
        if (IsValid(Hinge)) Hinge->SetRelativeRotation(FRotator::ZeroRotator);
    EnteringCharacter = nullptr;
    EnteringPlayer.Reset();
}

void ACitySedan::CompleteEntry()
{
    ACityLiveRPCharacter* Occupant = EnteringCharacter;
    APlayerController* Player = EnteringPlayer.Get();
    if (!IsValid(Occupant) || !Player) { CancelEntry(); return; }
    const bool bPassengerSeat = bEnteringPassenger;
    EnteringCharacter = nullptr;
    EnteringPlayer.Reset();
    if (bPassengerSeat) Passenger = Occupant;
    else Driver = Occupant;
    Occupant->GetMesh()->RemoveTickPrerequisiteActor(this);
    Occupant->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    if (UCityCrouchMeshComponent* EntryMesh = Cast<UCityCrouchMeshComponent>(Occupant->GetMesh()))
        EntryMesh->SetVehicleEntryPose(0, 0, FVector::ZeroVector, false);
    Occupant->SetActorHiddenInGame(true);
    Occupant->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Occupant->SetActorRelativeLocation(FVector(-25.f, bPassengerSeat ? 40.f : -40.f, 30.f));
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
    ACityLiveRPCharacter* Occupant = bPassengerExit ? Passenger.Get() : Driver.Get();
    if (!Occupant || (!bPassengerExit && Player->GetPawn() != this)) return false;
    const float Radius = Occupant->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = Occupant->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CitySedanExit), false, this);
    Params.AddIgnoredActor(Occupant);
    for (const FVector& Offset : {GetActorRightVector() * 190.f, -GetActorRightVector() * 190.f, -GetActorForwardVector() * 320.f})
    {
        FVector ExitLocation = GetActorLocation() + Offset;
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, ExitLocation + FVector(0, 0, 150),
            ExitLocation - FVector(0, 0, 300), ECC_Visibility, Params)) continue;
        ExitLocation.Z = Floor.ImpactPoint.Z + HalfHeight + 3.f;
        if (GetWorld()->OverlapBlockingTestByChannel(ExitLocation, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)) continue;
        ACityLiveRPCharacter* ExitingDriver = Occupant;
        if (bPassengerExit) Passenger = nullptr;
        else { Driver = nullptr; ResetInput(); }
        ExitingDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        ExitingDriver->SetActorLocationAndRotation(ExitLocation, GetActorRotation());
        ExitingDriver->SetActorHiddenInGame(false);
        ExitingDriver->SetActorEnableCollision(true);
        ExitingDriver->SetActorTickEnabled(true);
        ExitingDriver->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        ExitingDriver->GetCharacterMovement()->SetComponentTickEnabled(true);
        if (!bPassengerExit) Player->Possess(ExitingDriver);
        Player->SetViewTarget(ExitingDriver);
        return true;
    }
    return false;
}

void ACitySedan::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    EntryCameraBlend = FMath::FInterpTo(EntryCameraBlend, EnteringCharacter ? 1.f : 0.f, DeltaTime, 5.f);
    if (EnteringCharacter)
    {
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
    if (Passenger) Params.AddIgnoredActor(Passenger);
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
}

void ACitySedan::UpdateEntry(float DeltaTime)
{
    if (!IsValid(EnteringCharacter) || !EnteringPlayer.IsValid()) { CancelEntry(); return; }
    EntryTime += DeltaTime;
    const float Approach = CityLiveEntry::Blend(EntryTime, 0.f, .9f);
    const float Sit = CityLiveEntry::Sit(EntryTime);
    const FVector Seat = GetActorTransform().TransformPosition(FVector(-25.f, bEnteringPassenger ? 40.f : -40.f, 30.f));
    FVector Position = FMath::Lerp(EntryStart, EntryApproach, Approach);
    Position = FMath::Lerp(Position, Seat, Sit);
    const FRotator FacingDoor = (HandleLocation() - EntryApproach).Rotation();
    FRotator DesiredRotation = FRotator(0, FacingDoor.Yaw, 0);
    if (Sit > 0.f) DesiredRotation = FQuat::Slerp(DesiredRotation.Quaternion(), GetActorQuat(), Sit).Rotator();
    const FVector Previous = EnteringCharacter->GetActorLocation();
    EnteringCharacter->SetActorLocationAndRotation(Position,
        FQuat::Slerp(EntryStartRotation.Quaternion(), DesiredRotation.Quaternion(), Approach));
    EnteringCharacter->GetCharacterMovement()->Velocity = DeltaTime > 0.f && Sit <= 0.f ? (Position - Previous) / DeltaTime : FVector::ZeroVector;
    DoorPivots[bEnteringPassenger ? 1 : 0]->SetRelativeRotation(
        FRotator(0, (bEnteringPassenger ? -70.f : 70.f) * CityLiveEntry::Door(EntryTime), 0));
    if (UCityCrouchMeshComponent* EntryMesh = Cast<UCityCrouchMeshComponent>(EnteringCharacter->GetMesh()))
        EntryMesh->SetVehicleEntryPose(Sit, CityLiveEntry::Hand(EntryTime), HandleLocation(), !bEnteringPassenger);
    if (EntryTime >= CityLiveEntry::Duration) CompleteEntry();
}

void ACitySedan::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    VehicleCamera->GetCameraView(DeltaTime, OutResult);
    if (EntryCameraBlend <= .001f) return;
    const FVector Focus = GetActorLocation() + FVector(0, 0, 40);
    FVector SideView = GetActorLocation() + GetActorRightVector() * (bEnteringPassenger ? 430.f : -430.f)
        - GetActorForwardVector() * 180.f + FVector(0, 0, 180);
    FCollisionQueryParams ViewParams(SCENE_QUERY_STAT(CityEntryCamera), false, this);
    if (EnteringCharacter) ViewParams.AddIgnoredActor(EnteringCharacter);
    if (Driver) ViewParams.AddIgnoredActor(Driver);
    if (Passenger) ViewParams.AddIgnoredActor(Passenger);
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
