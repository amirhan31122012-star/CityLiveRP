#include "CitySedan.h"
#include <initializer_list>
#include "CitySedanDynamics.h"
#include "CityLiveRPCharacter.h"
#include "Camera/CameraComponent.h"
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
    Part(TEXT("Cabin"), FVector(-25.f, 0.f, 52.f), FVector(2.1f, 1.55f, 0.65f));
    Part(TEXT("Hood"), FVector(145.f, 0.f, 20.f), FVector(1.1f, 1.7f, 0.25f));
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

bool ACitySedan::CanEnter(const APlayerController* Player) const
{
    return !Driver && Player && Cast<ACityLiveRPCharacter>(Player->GetPawn()) &&
        FVector::DistSquared(GetActorLocation(), Player->GetPawn()->GetActorLocation()) <= FMath::Square(400.f);
}

bool ACitySedan::TryEnter(APlayerController* Player)
{
    if (!CanEnter(Player) || !HasAuthority()) return false;
    Driver = Cast<ACityLiveRPCharacter>(Player->GetPawn());
    Driver->ResetMobileSteering();
    Driver->SetMobileSprint(false);
    Driver->GetCharacterMovement()->StopMovementImmediately();
    Driver->SetActorEnableCollision(false);
    Driver->SetActorHiddenInGame(true);
    Driver->SetActorTickEnabled(false);
    Driver->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    ResetInput();
    Player->Possess(this);
    Player->SetViewTarget(this);
    return true;
}

bool ACitySedan::TryExit(APlayerController* Player)
{
    if (!Driver || !Player || Player->GetPawn() != this || FMath::Abs(Speed) > 30.f) return false;
    const float Radius = Driver->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = Driver->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CitySedanExit), false, this);
    Params.AddIgnoredActor(Driver);
    for (const FVector& Offset : {GetActorRightVector() * 190.f, -GetActorRightVector() * 190.f, -GetActorForwardVector() * 320.f})
    {
        FVector ExitLocation = GetActorLocation() + Offset;
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, ExitLocation + FVector(0, 0, 150),
            ExitLocation - FVector(0, 0, 300), ECC_Visibility, Params)) continue;
        ExitLocation.Z = Floor.ImpactPoint.Z + HalfHeight + 3.f;
        if (GetWorld()->OverlapBlockingTestByChannel(ExitLocation, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)) continue;
        ACityLiveRPCharacter* ExitingDriver = Driver;
        Driver = nullptr;
        ResetInput();
        ExitingDriver->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        ExitingDriver->SetActorLocationAndRotation(ExitLocation, GetActorRotation());
        ExitingDriver->SetActorHiddenInGame(false);
        ExitingDriver->SetActorEnableCollision(true);
        ExitingDriver->SetActorTickEnabled(true);
        Player->Possess(ExitingDriver);
        Player->SetViewTarget(ExitingDriver);
        return true;
    }
    return false;
}

void ACitySedan::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
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
