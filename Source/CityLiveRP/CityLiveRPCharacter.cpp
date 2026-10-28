
// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityLiveRPCharacter.h"
#include "MobileSteering.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraTypes.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "CityLiveRP.h"

ACityLiveRPCharacter::ACityLiveRPCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Поворот персонажа
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 75.0f, 0.0f);

    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = MobileWalkSpeed;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 160.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

    // Камера следует за горизонтальным поворотом персонажа
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetAbsolute(false, false, false);
    CameraBoom->SetRelativeLocation(FVector::ZeroVector);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->SocketOffset = FVector::ZeroVector;
    CameraBoom->TargetOffset = FVector::ZeroVector;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = true;
    CameraBoom->bInheritRoll = false;

    CameraBoom->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
    // The character already turns smoothly. Lagging the camera separately
    // lets it swing beside/in front of the character during sharp turns.
    CameraBoom->bEnableCameraLag = false;
    CameraBoom->bEnableCameraRotationLag = false;
    CameraBoom->bDoCollisionTest = true;

    // Камера
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ACityLiveRPCharacter::BeginPlay()
{
    Super::BeginPlay();
    // Apply at runtime too: existing Blueprint instances may store template defaults.
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->MaxWalkSpeed = MobileWalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 160.f;
    CameraBoom->SetAbsolute(false, false, false);
    CameraBoom->SetRelativeLocation(FVector::ZeroVector);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->SocketOffset = FVector::ZeroVector;
    CameraBoom->TargetOffset = FVector::ZeroVector;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = true;
    CameraBoom->bInheritRoll = false;
    CameraBoom->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
    // The character already turns smoothly. Lagging the camera separately
    // lets it swing beside/in front of the character during sharp turns.
    CameraBoom->bEnableCameraLag = false;
    CameraBoom->bEnableCameraRotationLag = false;
    CameraBoom->bDoCollisionTest = true;
    FollowCamera->SetAbsolute(false, false, false);
    FollowCamera->SetRelativeTransform(FTransform::Identity);
    FollowCamera->bUsePawnControlRotation = false;
    CameraBoom->AddTickPrerequisiteComponent(GetCharacterMovement());
    SetActorTickEnabled(true);
    GetCharacterMovement()->AddTickPrerequisiteActor(this);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetMesh()->SetOwnerNoSee(false);
    GetMesh()->SetOnlyOwnerSee(false);
    GetMesh()->SetVisibility(true);
}

void ACityLiveRPCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    // Calculate the final player view from the pawn, not from a possibly
    // overridden Blueprint camera, control rotation, or spring-arm transform.
    FollowCamera->GetCameraView(DeltaTime, OutResult);
    const FVector Focus = GetActorLocation() + FVector(0.f, 0.f, 35.f);
    const FVector Forward = FRotator(0.f, GetActorRotation().Yaw, 0.f).Vector();
    const FVector Desired = Focus - Forward * 420.f + FVector(0.f, 0.f, 140.f);
    FVector CameraLocation = Desired;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityFollowCamera), false, this);
    if (GetWorld()->SweepSingleByChannel(Hit, Focus, Desired, FQuat::Identity,
        ECC_Camera, FCollisionShape::MakeSphere(12.f), Params))
    {
        CameraLocation = Hit.Location;
    }
    OutResult.Location = CameraLocation;
    OutResult.Rotation = (Focus - CameraLocation).Rotation();
    OutResult.FOV = 80.f;
}

void ACityLiveRPCharacter::SetMobileSteering(float Steering, float Forward)
{
    bMobileSteeringActive = true;
    MobileSteering = FMath::Clamp(Steering, -1.f, 1.f);
    MobileForward = FMath::Clamp(Forward, -1.f, 1.f);
    // Horizontal input is angular velocity, not a destination heading.
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void ACityLiveRPCharacter::ResetMobileSteering()
{
    MobileSteering = MobileForward = MobileYawRate = 0.f;
}

void ACityLiveRPCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bMobileSteeringActive || !IsLocallyControlled()) return;
    const float Angle = CityLiveMobile::AdvanceTurn(MobileYawRate, MobileSteering,
        DeltaTime, MobileTurnSpeed, MobileTurnResponse);
    AddActorWorldRotation(FRotator(0.f, Angle, 0.f));
    // Forward/back input follows the new heading; holding left/right keeps turning.
    AddMovementInput(GetActorForwardVector(), MobileForward);
}

void ACityLiveRPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACityLiveRPCharacter::Move);
        EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACityLiveRPCharacter::Look);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACityLiveRPCharacter::Look);
    }
    else
    {
        UE_LOG(LogCityLiveRP, Error,
            TEXT("'%s' Failed to find an Enhanced Input component!"),
            *GetNameSafe(this));
    }
}

void ACityLiveRPCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void ACityLiveRPCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACityLiveRPCharacter::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
        const FRotator Rotation = GetController()->GetControlRotation();
        const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

        const FVector ForwardDirection =
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        const FVector RightDirection =
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, Forward);
        AddMovementInput(RightDirection, Right);
    }
}

void ACityLiveRPCharacter::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void ACityLiveRPCharacter::DoJumpStart()
{
    Jump();
}

void ACityLiveRPCharacter::DoJumpEnd()
{
    StopJumping();
}

void ACityLiveRPCharacter::SetMobileSprint(bool bEnabled)
{
    if (bEnabled) UnCrouch();
    GetCharacterMovement()->MaxWalkSpeed = bEnabled ? MobileSprintSpeed : MobileWalkSpeed;
}

void ACityLiveRPCharacter::ToggleMobileCrouch()
{
    if (bIsCrouched || GetCharacterMovement()->bWantsToCrouch)
    {
        UnCrouch();
    }
    else
    {
        SetMobileSprint(false);
        Crouch();
    }
}
