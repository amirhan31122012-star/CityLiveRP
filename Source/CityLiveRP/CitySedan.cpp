#include "CitySedan.h"
#include <initializer_list>
#include "CitySedanDynamics.h"
#include "CityEntryMotion.h"
#include "CityVehicleLayout.h"
#include "CitySuspension.h"
#include "CityOrbitCamera.h"
#include "CityCrouchMeshComponent.h"
#include "Components/SceneComponent.h"
#include "CityLiveRPCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    Passengers.SetNum(3);
    for(int32 Index=0;Index<4;++Index)
    {
        const int32 Seat=Index-1;
        const float Side=CityVehicleLayout::Side(Seat),X=CityVehicleLayout::X(Seat);
        FCityVehicleSeat Definition;
        Definition.Side=Side;Definition.DoorIndex=CityVehicleLayout::Door(Seat);
        Definition.Pelvis=FVector(X,Side*40.f,-12.f);
        Definition.SeatEdge=FVector(X,Side*76.f,-12.f);
        Definition.Approach=FVector(X,Side*135.f,0);
        Definition.LeftFootFloor=FVector(X+68,Side*40-11,-43);
        Definition.RightFootFloor=FVector(X+68,Side*40+11,-43);
        Definition.HandleOffset=FVector(-90,Side*5,38);
        Definition.DoorPullOffset=FVector(-75,-Side*5,35);
        Definition.LeftHand=FVector(X+64,Side*40-12,24);
        Definition.RightHand=FVector(X+64,Side*40+12,24);
        if(Seat>=0)
        {
            Definition.LeftHand=FVector(X+25,Side*40-12,3);
            Definition.RightHand=FVector(X+25,Side*40+12,3);
        }
        Seating.Add(Definition);
        USceneComponent* Anchor=CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("SeatPelvis%d"),Index));
        SeatAnchors.Add(Anchor);
    }
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("VehicleCollision"));
    SetRootComponent(CollisionBox);
    for(int32 I=0;I<SeatAnchors.Num();++I) {SeatAnchors[I]->SetupAttachment(CollisionBox);SeatAnchors[I]->SetRelativeLocation(Seating[I].Pelvis);}
    CollisionBox->SetBoxExtent(FVector(210.f, 85.f, 28.f));
    CollisionBox->SetCollisionProfileName(TEXT("PhysicsActor"));
    for (int32 Wheel = 0; Wheel < 4; ++Wheel)
        SuspensionAnchors.Add(FVector(Wheel < 2 ? 135.f : -135.f, Wheel % 2 ? 95.f : -95.f, 0));
    WheelSuspensionOffsets.Init(0.f, 4);
    SedanMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/VehicleVarietyVol2/Skeletons/SK_Sedan_01a.SK_Sedan_01a")));
    FourDoorMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/CityLiveVehicles/Sedan/SK_CitySedan_4Doors.SK_CitySedan_4Doors")));
    ImportedSedan = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("ImportedSedan"));
    ImportedSedan->SetupAttachment(CollisionBox);
    ImportedSedan->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ImportedSedan->SetVisibility(false);
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
        PrototypeParts.Add(Piece);
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
        PrototypeParts.Add(Door);
        UStaticMeshComponent* Handle = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("DoorHandle%d"), Side));
        Handle->SetupAttachment(Hinge);
        Handle->SetStaticMesh(Cube.Object);
        Handle->SetRelativeLocation(FVector(-90.f, Side % 2 ? 5.f : -5.f, 22.f));
        Handle->SetRelativeScale3D(FVector(.18f, .07f, .04f));
        Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PrototypeParts.Add(Handle);
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
        PrototypeParts.Add(Wheel);
    }
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("VehicleCameraArm"));
    CameraArm->SetupAttachment(CollisionBox);
    CameraArm->TargetArmLength = 650.f;
    CameraArm->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
    CameraArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
    CameraArm->bUsePawnControlRotation = false;
    // Keep the horizon steady while the chassis pitches/rolls on wheel contacts.
    CameraArm->bInheritPitch = false;
    CameraArm->bInheritRoll = false;
    CameraArm->bEnableCameraLag = true;
    CameraArm->CameraLagSpeed = 14.f;
    CameraArm->CameraLagMaxDistance = 65.f;
    CameraArm->bEnableCameraRotationLag = true;
    CameraArm->CameraRotationLagSpeed = 10.f;
    CameraArm->bUseCameraLagSubstepping = true;
    CameraArm->CameraLagMaxTimeStep = 1.f / 120.f;
    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

void ACitySedan::BeginPlay()
{
    Super::BeginPlay();
    CollisionBox->SetMassOverrideInKg(NAME_None, 1200.f);
    CollisionBox->SetCenterOfMass(FVector(0, 0, -12.f));
    CollisionBox->SetLinearDamping(.035f);
    CollisionBox->SetAngularDamping(2.f);
    CollisionBox->SetUseCCD(true);
    CollisionBox->SetSimulatePhysics(true);
    USkeletalMesh* SedanAsset = FourDoorMeshAsset.LoadSynchronous();
    if (SedanAsset && !SedanAsset->GetSkeleton())
    {
        UE_LOG(LogTemp, Error, TEXT("CityLive: four-door mesh has no saved Skeleton; rerun ImportSedanDoors.cmd. Using original model."));
        SedanAsset = nullptr;
    }
    if (!SedanAsset) SedanAsset = SedanMeshAsset.LoadSynchronous();
    if (!SedanAsset || !SedanAsset->GetSkeleton())
    {
        UE_LOG(LogTemp, Warning, TEXT("CityLive sedan asset missing at %s; keeping prototype model."), *SedanMeshAsset.ToString());
        return;
    }
    ImportedSedan->SetSkinnedAssetAndUpdate(SedanAsset);
    const FBoxSphereBounds AssetBounds = SedanAsset->GetBounds();
    // Native centimetre dimensions; center XY and place tires at the root's bottom.
    ImportedSedan->SetRelativeLocation(FVector(-AssetBounds.Origin.X, -AssetBounds.Origin.Y,
        -60.f - (AssetBounds.Origin.Z - AssetBounds.BoxExtent.Z)));
    ImportedSedan->SetVisibility(true);
    for (UStaticMeshComponent* Part : PrototypeParts) Part->SetVisibility(false);
    for (const FName Bone : {FName(TEXT("Wheel_Front_Left")), FName(TEXT("Wheel_Front_Right")),
        FName(TEXT("Wheel_Rear_Left")), FName(TEXT("Wheel_Rear_Right")), FName(TEXT("SteeringWheel")),
        FName(TEXT("Door_Front_Left")), FName(TEXT("Door_Front_Right")),
        FName(TEXT("Door_Rear_Left")), FName(TEXT("Door_Rear_Right"))})
    {
        if (ImportedSedan->GetBoneIndex(Bone) != INDEX_NONE)
        {
            RestVehicleBones.Add(Bone, ImportedSedan->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace));
            const int32 Wheel = Bone == FName(TEXT("Wheel_Front_Left")) ? 0 : Bone == FName(TEXT("Wheel_Front_Right")) ? 1 :
                Bone == FName(TEXT("Wheel_Rear_Left")) ? 2 : Bone == FName(TEXT("Wheel_Rear_Right")) ? 3 : INDEX_NONE;
            if (Wheel != INDEX_NONE)
            {
                const FVector WheelPosition = ImportedSedan->GetRelativeTransform().TransformPosition(RestVehicleBones[Bone].GetLocation());
                SuspensionAnchors[Wheel] = FVector(WheelPosition.X, WheelPosition.Y, 0);
            }
        }
    }
    // Equal spring preloads balance only when their support centroid is above
    // the centre of mass. Imported axle locations need not be symmetric about
    // the mesh bounds; keeping the box COM at zero caused a parked pitch bias.
    const float CenterX = CitySuspension::SupportCenter(SuspensionAnchors[0].X, SuspensionAnchors[1].X,
        SuspensionAnchors[2].X, SuspensionAnchors[3].X);
    const float CenterY = CitySuspension::SupportCenter(SuspensionAnchors[0].Y, SuspensionAnchors[1].Y,
        SuspensionAnchors[2].Y, SuspensionAnchors[3].Y);
    CollisionBox->SetCenterOfMass(FVector(CenterX, CenterY, -12.f));
    bDoorRigReady = RestVehicleBones.Contains(FName(TEXT("Door_Front_Left"))) && RestVehicleBones.Contains(FName(TEXT("Door_Front_Right"))) &&
        RestVehicleBones.Contains(FName(TEXT("Door_Rear_Left"))) && RestVehicleBones.Contains(FName(TEXT("Door_Rear_Right")));
    const FName DoorBones[] = {FName(TEXT("Door_Front_Left")), FName(TEXT("Door_Front_Right")),
        FName(TEXT("Door_Rear_Left")), FName(TEXT("Door_Rear_Right"))};
    for (int32 Door = 0; Door < 4; ++Door)
        if (const FTransform* Hinge = RestVehicleBones.Find(DoorBones[Door]))
            DoorPivots[Door]->SetRelativeLocation(ImportedSedan->GetRelativeTransform().TransformPosition(Hinge->GetLocation()));
    for(int32 I=0;I<Seating.Num()&&I<SeatAnchors.Num();++I)
        if(!Seating[I].SeatSocket.IsNone() && ImportedSedan->DoesSocketExist(Seating[I].SeatSocket))
        {
            SeatAnchors[I]->AttachToComponent(ImportedSedan,FAttachmentTransformRules::KeepRelativeTransform,Seating[I].SeatSocket);
            SeatAnchors[I]->SetRelativeLocation(Seating[I].Pelvis);
        }
    UE_LOG(LogTemp, Display, TEXT("CityLive sedan: imported model active (%s). Four-door rig: %s."), *SedanAsset->GetPathName(), bDoorRigReady ? TEXT("present") : TEXT("missing"));
    if (!bDoorRigReady)
        UE_LOG(LogTemp, Warning, TEXT("CityLive: original sedan has fixed doors. Install SK_CitySedan_4Doors_v2.fbx using ImportSedanDoors.cmd; updating code alone does not install the model."));
}

void ACitySedan::UpdateImportedWheels(float DeltaTime)
{
    if (RestVehicleBones.IsEmpty()) return;
    WheelRollDegrees = FMath::Fmod(WheelRollDegrees - FMath::RadiansToDegrees(Speed * DeltaTime / 32.f), 360.f);
    for (const auto& Bone : RestVehicleBones)
    {
        FTransform BonePose = Bone.Value;
        FQuat Rotation;
        const int32 Door = Bone.Key == FName(TEXT("Door_Front_Left")) ? 0 : Bone.Key == FName(TEXT("Door_Front_Right")) ? 1 :
            Bone.Key == FName(TEXT("Door_Rear_Left")) ? 2 : Bone.Key == FName(TEXT("Door_Rear_Right")) ? 3 : INDEX_NONE;
        if (Door != INDEX_NONE)
            Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(DoorPivots[Door]->GetRelativeRotation().Yaw));
        else if (Bone.Key == FName(TEXT("SteeringWheel")))
            Rotation = FQuat(Bone.Value.GetRotation().GetAxisX(), FMath::DegreesToRadians(-SmoothedSteering * 150.f));
        else
        {
            const bool bFront = Bone.Key == FName(TEXT("Wheel_Front_Left")) || Bone.Key == FName(TEXT("Wheel_Front_Right"));
            Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(bFront ? SmoothedSteering * 34.f : 0.f)) *
                FQuat(FVector::RightVector, FMath::DegreesToRadians(WheelRollDegrees));
        }
        BonePose.SetRotation((Rotation * Bone.Value.GetRotation()).GetNormalized());
        const int32 Wheel = Bone.Key == FName(TEXT("Wheel_Front_Left")) ? 0 : Bone.Key == FName(TEXT("Wheel_Front_Right")) ? 1 :
            Bone.Key == FName(TEXT("Wheel_Rear_Left")) ? 2 : Bone.Key == FName(TEXT("Wheel_Rear_Right")) ? 3 : INDEX_NONE;
        if (Wheel != INDEX_NONE) BonePose.AddToTranslation(FVector(0, 0, WheelSuspensionOffsets[Wheel]));
        ImportedSedan->SetBoneTransformByName(Bone.Key, BonePose, EBoneSpaces::ComponentSpace);
    }
}

bool ACitySedan::FindFlatSpawn(UWorld* World, const FVector& Candidate, float Yaw, AActor* Ignore, FVector& OutLocation)
{
    if (!World) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityFlatParking), false, Ignore);
    const FQuat Rotation = FRotator(0, Yaw, 0).Quaternion();
    float Minimum = TNumericLimits<float>::Max(), Maximum = TNumericLimits<float>::Lowest();
    for (float X : {-260.f, 0.f, 260.f})
    for (float Y : {-135.f, 0.f, 135.f})
    {
        const FVector Point = Candidate + Rotation.RotateVector(FVector(X, Y, 0));
        FHitResult Hit;
        if (!World->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 250), Point - FVector(0, 0, 600), ECC_Visibility, Params)) return false;
        if (Hit.ImpactNormal.Z < .995f) return false;
        Minimum = FMath::Min(Minimum, static_cast<float>(Hit.ImpactPoint.Z));
        Maximum = FMath::Max(Maximum, static_cast<float>(Hit.ImpactPoint.Z));
    }
    if (Maximum - Minimum > 4.f) return false;
    OutLocation = FVector(Candidate.X, Candidate.Y, (Minimum + Maximum) * .5f +
        CitySuspension::RestLength + CitySuspension::WheelRadius);
    return true;
}

void ACitySedan::ResetInput()
{
    Steering = SmoothedSteering = 0.f;
    Pedals.Reset();
    bDriftHeld = false;
}

void ACitySedan::OrbitCamera(float YawDelta, float PitchDelta)
{
    if (EnteringCharacter) return;
    const CityOrbitCamera::State View = CityOrbitCamera::Advance(CameraYawOffset, CameraPitch, YawDelta, PitchDelta);
    CameraYawOffset = View.Yaw;
    CameraPitch = View.Pitch;
    CameraArm->SetRelativeRotation(FRotator(CameraPitch, CameraYawOffset, 0.f));
}

bool ACitySedan::CanEnter(const APlayerController* Player, bool bPassengerSeat) const
{
    return !EnteringCharacter && (bPassengerSeat ? FindPassengerSeat(Player) >= 0 : !Driver) && FMath::Abs(Speed) <= 30.f &&
        CollisionBox->GetPhysicsLinearVelocity().SizeSquared() <= FMath::Square(45.f) &&
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
    int32 Best=-2;double BestDistance=TNumericLimits<double>::Max();
    for(int32 Seat=0;Seat<Passengers.Num();++Seat)if(!Passengers[Seat])
    {
        const FCityVehicleSeat& Definition=SeatSettings(Seat);
        const double Distance=FVector::DistSquared2D(Local,Definition.Approach)+(Local.Y*Definition.Side>=0 ? 0. : 1.e9);
        if(Distance<BestDistance){Best=Seat;BestDistance=Distance;}
    }
    return Best;
}

bool ACitySedan::TryEnter(APlayerController* Player, bool bPassengerSeat, bool bFromParking)
{
    if (!HasAuthority() || !Player || EnteringCharacter) return false;
    const bool bParkingApproach=bFromParking && !bPassengerSeat && !Driver &&
        Cast<ACityLiveRPCharacter>(Player->GetPawn()) &&
        FVector::DistSquared(GetActorLocation(),Player->GetPawn()->GetActorLocation())<FMath::Square(2000.f);
    if (!bParkingApproach && !CanEnter(Player,bPassengerSeat)) return false;
    ACityLiveRPCharacter* Occupant = Cast<ACityLiveRPCharacter>(Player->GetPawn());
    EntrySeat = bPassengerSeat ? FindPassengerSeat(Player) : -1;
    if(Occupant->bIsCrouched || Occupant->GetCharacterMovement()->IsFalling())
    {Player->ClientMessage(TEXT("Для посадки встаньте на землю в полный рост."));return false;}
    if (!Occupant->HasVehicleAnimations())
    {Player->ClientMessage(TEXT("Подготовьте монтажи через InstallVehicleEntryAnimations.cmd."));return false;}
    if (!Occupant->GetActorScale3D().Equals(FVector::OneVector,.0001f) || !ValidateEntrySpace(Occupant))
    {Player->ClientMessage(TEXT("Недостаточно места для двери или персонажа. Проверьте точки сиденья и салон."));return false;}
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
    bReversingEntry = bMontageStarted = false;
    LastBodyPose=FCityVehicleBodyPose();
    EntryTime = -RouteDuration;
    Player->SetIgnoreMoveInput(true);Player->SetIgnoreLookInput(true);bEntryIgnoredInput=true;
    Occupant->ResetMobileSteering();
    Occupant->ConsumeMovementInputVector();Occupant->StopJumping();
    Occupant->SetMobileSprint(false);
    Occupant->GetCharacterMovement()->StopMovementImmediately();
    Occupant->GetCharacterMovement()->DisableMovement();
    Occupant->GetCharacterMovement()->SetComponentTickEnabled(false);
    Occupant->SetActorEnableCollision(false);
    Occupant->SetActorTickEnabled(false);
    Occupant->GetMesh()->AddTickPrerequisiteActor(this);
    ResetInput();
    Speed = 0.f;
    SetEntryPhysicsPaused(true);
    Player->SetViewTargetWithBlend(this, .4f);
    return true;
}

bool ACitySedan::BuildDoorRoute(ACityLiveRPCharacter* Occupant)
{
    const FVector LocalStart = GetActorTransform().InverseTransformPosition(Occupant->GetActorLocation());
    const float HalfHeight = Occupant->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Radius = Occupant->GetCapsuleComponent()->GetScaledCapsuleRadius();
    FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(CityDoorFloor), false, this);
    FloorParams.AddIgnoredActor(Occupant);
    FCollisionQueryParams WalkParams(SCENE_QUERY_STAT(CityDoorWalk), false, Occupant);
    TArray<TArray<FVector>> Candidates;
    Candidates.Add({SeatSettings(EntrySeat).Approach});
    for (float End : {-300.f, 300.f})
        Candidates.Add({FVector(End, LocalStart.Y, 0), FVector(End, SeatSettings(EntrySeat).Approach.Y, 0), SeatSettings(EntrySeat).Approach});
    for (const TArray<FVector>& Candidate : Candidates)
    {
        TArray<FVector> Route;
        FVector Previous = Occupant->GetActorLocation();
        Route.Add(Previous);
        bool bFree = true;
        for (const FVector& LocalPoint : Candidate)
        {
            FVector Point = GetActorTransform().TransformPosition(LocalPoint);
            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 180), Point - FVector(0, 0, 300), ECC_Visibility, FloorParams)) { bFree = false; break; }
            Point.Z = Hit.ImpactPoint.Z + HalfHeight + 1.f;
            if (GetWorld()->SweepSingleByChannel(Hit, Previous, Point, FQuat::Identity,
                ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight-2.f), WalkParams)) { bFree = false; break; }
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
    const FCityVehicleSeat& Seat=SeatSettings(EntrySeat);
    return DoorPivots[Seat.DoorIndex]->GetComponentTransform().TransformPosition(Seat.HandleOffset);
}

void ACitySedan::CancelEntry()
{
    if(IsValid(EnteringCharacter) && EnteringPlayer.IsValid())
    {
        // Roll the same continuous animation back to a safe endpoint. No warp.
        bReversingEntry=true;
        return;
    }
    EnteringCharacter=nullptr;EnteringPlayer.Reset();
    SetEntryPhysicsPaused(false);
}

void ACitySedan::CompleteEntry()
{
    ACityLiveRPCharacter* Occupant=EnteringCharacter;
    APlayerController* Player=EnteringPlayer.Get();
    if(!IsValid(Occupant)||!Player){CancelEntry();return;}
    const bool bPassengerSeat=bEnteringPassenger;
    if(bPassengerSeat)Passengers[EntrySeat]=Occupant;else Driver=Occupant;
    EnteringCharacter=nullptr;EnteringPlayer.Reset();
    // The final animation sample has already reached the anchor. Attachment
    // changes only the parent, preserving position, rotation and world scale.
    Occupant->AttachToComponent(SeatAnchors[EntrySeat+1],FAttachmentTransformRules::KeepWorldTransform);
    Occupant->GetCharacterMovement()->Velocity=FVector::ZeroVector;
    Occupant->PlayVehicleAnimation(TEXT("DriverIdle"));
    Occupant->SetVehicleAnimationTime(0.f);
    UpdateSeatedPose(Occupant,EntrySeat);
    if(bEntryIgnoredInput){Player->SetIgnoreMoveInput(false);Player->SetIgnoreLookInput(false);bEntryIgnoredInput=false;}
    if(!bPassengerSeat){ResetInput();Player->Possess(this);}
    Player->SetViewTarget(this);
    SetEntryPhysicsPaused(false);
}

bool ACitySedan::TryExit(APlayerController* Player)
{
    if (EnteringCharacter || !Player || FMath::Abs(Speed) > 30.f || CollisionBox->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(45.f)) return false;
    const bool bPassengerExit = IsPassenger(Player);
    EntrySeat = bPassengerExit ? GetPassengerSeat(Player) : -1;
    ACityLiveRPCharacter* Occupant = bPassengerExit ? Passengers[EntrySeat].Get() : Driver.Get();
    if (!Occupant || (!bPassengerExit && Player->GetPawn() != this) || !Occupant->HasVehicleAnimations() || !ValidateEntrySpace(Occupant)) return false;
    const float Radius = Occupant->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = Occupant->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CitySedanExit), false, this);
    Params.AddIgnoredActor(Occupant);
    for (float DoorOffsetX : {-25.f, -65.f, 15.f})
    {
        FVector ExitLocation = GetActorTransform().TransformPosition(SeatSettings(EntrySeat).Approach + FVector(DoorOffsetX + 25.f,0,0));
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, ExitLocation + FVector(0, 0, 150),
            ExitLocation - FVector(0, 0, 300), ECC_Visibility, Params)) continue;
        ExitLocation.Z = Floor.ImpactPoint.Z + HalfHeight + 1.f;
        if (GetWorld()->OverlapBlockingTestByChannel(ExitLocation, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)) continue;
        EnteringCharacter = Occupant;
        EnteringPlayer = Player;
        bEnteringPassenger = bPassengerExit;
        bExiting = true;
        Player->SetIgnoreMoveInput(true);Player->SetIgnoreLookInput(true);bEntryIgnoredInput=true;
        bReversingEntry = false;
        LastBodyPose=FCityVehicleBodyPose();
        bMontageStarted = Occupant->PlayVehicleAnimation(TEXT("Exit"),SeatSettings(EntrySeat).Side>0);
        EntryTime = 0.f;
        EntryApproach = ExitLocation;
        EntryStart = Occupant->GetActorLocation();
        EntryStartRotation = GetActorRotation();
        Occupant->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Occupant->GetMesh()->AddTickPrerequisiteActor(this);
        ResetInput();
        Speed = 0.f;
        SetEntryPhysicsPaused(true);
        Player->SetViewTargetWithBlend(this, .3f);
        return true;
    }
    return false;
}

void ACitySedan::RestoreWalking(ACityLiveRPCharacter* Occupant,APlayerController* Player)
{
    Occupant->GetMesh()->RemoveTickPrerequisiteActor(this);
    Occupant->ClearVehicleAnimation();
    Occupant->ConsumeMovementInputVector();Occupant->StopJumping();Occupant->ResetMobileSteering();
    if(Player && bEntryIgnoredInput){Player->SetIgnoreMoveInput(false);Player->SetIgnoreLookInput(false);bEntryIgnoredInput=false;}
    Occupant->SetActorEnableCollision(true);
    Occupant->SetActorTickEnabled(true);
    Occupant->GetCharacterMovement()->Velocity=FVector::ZeroVector;
    Occupant->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Occupant->GetCharacterMovement()->SetComponentTickEnabled(true);
    if(Player && Player->GetPawn()!=Occupant)Player->Possess(Occupant);
    if(Player)Player->SetViewTargetWithBlend(Occupant,.3f);
}
void ACitySedan::CompleteExit()
{
    ACityLiveRPCharacter* Occupant=EnteringCharacter;
    APlayerController* Player=EnteringPlayer.Get();
    if(!IsValid(Occupant)||!Player){CancelEntry();return;}
    if(bEnteringPassenger)Passengers[EntrySeat]=nullptr;else Driver=nullptr;
    EnteringCharacter=nullptr;EnteringPlayer.Reset();bExiting=false;
    // Actor is already standing outside; do not SetActorLocation at completion.
    RestoreWalking(Occupant,Player);
    SetEntryPhysicsPaused(false);
}
void ACitySedan::UpdateSeatedPose(ACityLiveRPCharacter* Occupant,int32 Seat)
{
    if(!IsValid(Occupant)||Occupant==EnteringCharacter.Get())return;
    const FCityVehicleSeat& Definition=SeatSettings(Seat);
    FCityVehicleBodyPose Pose;Pose.bActive=true;Pose.LegWeight=Pose.HandWeight=1.f;
    Pose.MovementFrame=GetRootComponent();Pose.SampledFrame=GetRootComponent()->GetComponentTransform();
    Pose.Pelvis=SeatPelvisWorld(Seat);
    Pose.TorsoCorrection=Definition.DrivingLeanDegrees-15.f;
    Pose.LeftFoot=GetActorTransform().TransformPosition(Definition.LeftFootFloor)+GetActorUpVector()*Occupant->BodyAnkleHeight();
    Pose.RightFoot=GetActorTransform().TransformPosition(Definition.RightFootFloor)+GetActorUpVector()*Occupant->BodyAnkleHeight();
    Pose.LeftHand=GetActorTransform().TransformPosition(Definition.LeftHand);
    Pose.RightHand=GetActorTransform().TransformPosition(Definition.RightHand);
    Occupant->SetVehicleBodyPose(Pose);
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
        UpdateImportedWheels(0.f);
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
    SmoothedSteering = FMath::FInterpTo(SmoothedSteering, FMath::Clamp(Steering + KeyboardSteering, -1.f, 1.f), DeltaTime, 5.f);
    const bool bDrift = Driver && (bDriftHeld || (DrivingPlayer && DrivingPlayer->IsInputKeyDown(EKeys::SpaceBar)));
    const float DriftTarget = bDrift ? CityLiveDriving::DriftStrength(Speed) : 0.f;
    DriftAmount = FMath::FInterpTo(DriftAmount, DriftTarget, DeltaTime, 5.f);
    TickVehiclePhysics(DeltaTime, Pedals.Throttle || KeyboardGas, Pedals.Brake || KeyboardBrake);
    for (UStaticMeshComponent* Wheel : Wheels)
        Wheel->AddLocalRotation(FRotator(0, -FMath::RadiansToDegrees(Speed * DeltaTime / 32.f), 0));
    UpdateImportedWheels(DeltaTime);
    UpdateSeatedPose(Driver, -1);
    for (int32 Seat = 0; Seat < Passengers.Num(); ++Seat) UpdateSeatedPose(Passengers[Seat], Seat);
}

void ACitySedan::SetEntryPhysicsPaused(bool bPaused)
{
    RestTimer = 0.f;
    if (bPaused)
    {
        CollisionBox->SetPhysicsLinearVelocity(FVector::ZeroVector);
        CollisionBox->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    }
    CollisionBox->SetSimulatePhysics(!bPaused);
}

void ACitySedan::TickVehiclePhysics(float DeltaTime, bool Gas, bool Brake)
{
    if (DeltaTime <= 0.f || !CollisionBox->IsSimulatingPhysics()) return;
    if (!CollisionBox->IsAnyRigidBodyAwake())
    {
        if (!Gas && !Brake) { Speed = 0.f; return; }
        CollisionBox->WakeAllRigidBodies();
    }
    const float Mass = CollisionBox->GetMass();
    const FVector Up = GetActorUpVector();
    const FVector Velocity = CollisionBox->GetPhysicsLinearVelocity();
    Speed = FVector::DotProduct(Velocity, GetActorForwardVector());
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityWheelSuspension), false, this);
    if (Driver) Params.AddIgnoredActor(Driver);
    for (ACityLiveRPCharacter* Occupant : Passengers) if (Occupant) Params.AddIgnoredActor(Occupant);
    int32 Contacts = 0;
    FVector GroundNormal = FVector::ZeroVector;
    TArray<FVector> ForcePoints;
    TArray<FVector> SpringForces;
    for (int32 Wheel = 0; Wheel < SuspensionAnchors.Num(); ++Wheel)
    {
        const FVector Anchor = GetActorTransform().TransformPosition(SuspensionAnchors[Wheel]);
        FHitResult Hit;
        const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Anchor,
            Anchor - Up * (CitySuspension::RestLength + CitySuspension::Travel), FQuat::Identity,
            ECC_Visibility, FCollisionShape::MakeSphere(CitySuspension::WheelRadius), Params);
        if (!bHit || Hit.ImpactNormal.Z < .65f)
        {
            WheelSuspensionOffsets[Wheel] = FMath::FInterpTo(WheelSuspensionOffsets[Wheel], -CitySuspension::Travel, DeltaTime, 8.f);
            continue;
        }
        const float Compression = CitySuspension::RestLength - Hit.Distance;
        WheelSuspensionOffsets[Wheel] = FMath::FInterpTo(WheelSuspensionOffsets[Wheel],
            FMath::Clamp(Compression, -CitySuspension::Travel, CitySuspension::Travel), DeltaTime, 12.f);
        const float VerticalSpeed = FVector::DotProduct(CollisionBox->GetPhysicsLinearVelocityAtPoint(Anchor), Up);
        const float LiftForce = CitySuspension::Force(Compression, VerticalSpeed, Mass, DeltaTime);
        ForcePoints.Add(Anchor);
        SpringForces.Add(Up * LiftForce);
        // A hit is still ground contact when the damper momentarily unloads it.
        // Treating zero spring force as airborne intermittently cut motor input.
        ++Contacts;
        GroundNormal += Hit.ImpactNormal * FMath::Max(LiftForce, Mass * 980.f * .025f);
    }
    DriveLogTimer += DeltaTime;
    if (Driver && (Gas || Brake) && DriveLogTimer >= 2.f)
    {
        UE_LOG(LogTemp, Display, TEXT("CityLiveDrive: speed=%.1f km/h contacts=%d/4 mass=%.0f kg gas=%d brake=%d fps=%.1f vz=%.1f angular=%.2f"),
            Speed * .036f, Contacts, Mass, Gas ? 1 : 0, Brake ? 1 : 0, 1.f / DeltaTime,
            Velocity.Z, CollisionBox->GetPhysicsAngularVelocityInRadians().Size());
        DriveLogTimer = 0.f;
    }
    if (Contacts == 0) return;
    GroundNormal.Normalize();
    const bool bSettled = !Gas && !Brake && Contacts == 4 && GroundNormal.Z > .985f &&
        Velocity.SizeSquared() < 9.f && CollisionBox->GetPhysicsAngularVelocityInRadians().SizeSquared() < .0025f;
    RestTimer = bSettled ? RestTimer + DeltaTime : 0.f;
    if (RestTimer > .75f)
    {
        CollisionBox->SetPhysicsLinearVelocity(FVector::ZeroVector);
        CollisionBox->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
        CollisionBox->PutAllRigidBodiesToSleep();
        Speed = 0.f;
        return;
    }
    for (int32 ForceIndex = 0; ForceIndex < SpringForces.Num(); ++ForceIndex)
        CollisionBox->AddForceAtLocation(SpringForces[ForceIndex], ForcePoints[ForceIndex]);
    const FVector Forward = FVector::VectorPlaneProject(GetActorForwardVector(), GroundNormal).GetSafeNormal();
    const FVector Right = FVector::CrossProduct(GroundNormal, Forward).GetSafeNormal();
    const float Driven = CityLiveDriving::AdvanceSpeed(Speed, Driver != nullptr, Gas, Brake, DeltaTime);
    const float Desired = CityLiveDriving::HandbrakeSpeed(Driven, DriftAmount, DeltaTime);
    const float Acceleration = FMath::Clamp((Desired - Speed) / DeltaTime,
        -CityLiveDriving::BrakeAcceleration, CityLiveDriving::BrakeAcceleration);
    const float Grip = Contacts / 4.f;
    // A driven axle can transmit engine force with two grounded wheels. Previously
    // multiplying all motor force by contacts/4 made such contact halve acceleration.
    const float DriveGrip = FMath::Min(1.f, Contacts * .5f);
    CollisionBox->AddForce((Forward * Acceleration * DriveGrip -
        Right * FVector::DotProduct(Velocity, Right) * CityLiveDriving::LateralGrip(DriftAmount) * Grip) * Mass);
    const FVector AngularVelocity = CollisionBox->GetPhysicsAngularVelocityInRadians();
    const float TargetYaw = Driver ? CityLiveDriving::YawRateRadians(SmoothedSteering, Speed) * (1.f + .18f * DriftAmount) : 0.f;
    const FVector YawTorque = GroundNormal * (TargetYaw - FVector::DotProduct(AngularVelocity, GroundNormal)) * FMath::Lerp(4.f, 2.8f, DriftAmount);
    // Suspension supplies most of the body roll; light damping avoids endless rocking.
    const FVector TiltDamping = FVector::VectorPlaneProject(AngularVelocity, GroundNormal) * -1.5f;
    CollisionBox->AddTorqueInRadians((YawTorque + TiltDamping) * Grip, NAME_None, true);
}

void ACitySedan::UpdateEntry(float DeltaTime)
{
    if(!IsValid(EnteringCharacter)||!EnteringPlayer.IsValid()){CancelEntry();return;}
    const float PreviousTime=EntryTime;
    EntryTime=CityLiveEntry::AdvanceTime(EntryTime,DeltaTime,bReversingEntry);
    const FCityVehicleSeat& Definition=SeatSettings(EntrySeat);
    const float Side=Definition.Side;
    const CityLiveEntry::Pose P=bExiting ? CityLiveEntry::Exit(EntryTime) : CityLiveEntry::Enter(EntryTime);
    FVector Position=EntryApproach;
    const int32 DoorIndex=Definition.DoorIndex;
    const FRotator PreviousDoorRotation=DoorPivots[DoorIndex]->GetRelativeRotation();
    const float NextAngle=-Side*Definition.OpenAngle*P.Door;
    if(!DoorMotionClear(PreviousDoorRotation.Yaw,NextAngle,EnteringCharacter))
    {EntryTime=PreviousTime;bReversingEntry=true;return;}
    DoorPivots[DoorIndex]->SetRelativeRotation(FRotator(0,NextAngle,0));
    FTransform ClosedHinge=DoorPivots[DoorIndex]->GetComponentTransform();ClosedHinge.SetRotation(GetActorQuat());
    const FVector ClosedHandle=ClosedHinge.TransformPosition(Definition.HandleOffset);
    const float DoorYaw=(ClosedHandle-EntryApproach).Rotation().Yaw;
    FRotator DesiredRotation(0,DoorYaw,0);
    if(!bExiting && EntryTime<0.f)
    {
        float Distance=FMath::Clamp((EntryTime+RouteDuration)/RouteDuration,0.f,1.f)*RouteLength;
        for(int32 I=1;I<EntryRoute.Num();++I)
        {
            const float Segment=FVector::Distance(EntryRoute[I-1],EntryRoute[I]);
            if(Distance<=Segment||I==EntryRoute.Num()-1)
            {Position=FMath::Lerp(EntryRoute[I-1],EntryRoute[I],Segment>.001f ? FMath::Clamp(Distance/Segment,0.f,1.f) : 1.f);break;}
            Distance-=Segment;
        }
        const FVector Delta=Position-EnteringCharacter->GetActorLocation();
        DesiredRotation=FMath::RInterpTo(EnteringCharacter->GetActorRotation(),Delta.SizeSquared2D()>.1f ?
            FRotator(0,Delta.Rotation().Yaw,0) : DesiredRotation,FMath::Min(DeltaTime,1.f/15.f),7.f);
        ApproachRotation=DesiredRotation;
    }
    else
    {
        if(!bMontageStarted)
        {
            bMontageStarted=EnteringCharacter->PlayVehicleAnimation(TEXT("Enter"),Side>0);
            if(!bMontageStarted){bReversingEntry=true;EntryTime=0;}
        }
        EnteringCharacter->SetVehicleAnimationTime(FMath::Clamp(EntryTime,0.f,CityLiveEntry::Duration));
        const FQuat FacingDoor=FRotator(0,DoorYaw,0).Quaternion();
        const FQuat Outward=FRotator(0,GetActorRotation().Yaw+Side*90.f,0).Quaternion();
        const FQuat Forward=FRotator(0,GetActorRotation().Yaw,0).Quaternion();
        if(bExiting)
        {
            const FQuat Seated=FQuat::Slerp(Outward,Forward,P.BodyTurn);
            DesiredRotation=FQuat::Slerp(Seated,FacingDoor,CityLiveEntry::Blend(EntryTime,4.85f,5.15f)).Rotator();
        }
        else
        {
            const FQuat Aligned=FQuat::Slerp(ApproachRotation.Quaternion(),FacingDoor,CityLiveEntry::Blend(EntryTime,0.f,.5f));
            const FQuat Sideways=FQuat::Slerp(Aligned,Outward,CityLiveEntry::Blend(EntryTime,1.7f,2.05f));
            DesiredRotation=FQuat::Slerp(Sideways,Forward,P.BodyTurn).Rotator();
        }
        const FVector Edge=GetActorTransform().TransformPosition(Definition.SeatEdge);
        const FVector Seated=FMath::Lerp(Edge,SeatPelvisWorld(EntrySeat),P.SeatShift);
        FVector StandingRoot=EntryApproach;
        if(bExiting)
        {
            FVector OutsideRoot=GetActorTransform().TransformPosition(FVector(Definition.Approach.X,Side*130.f,0));
            OutsideRoot.Z=EntryApproach.Z;
            StandingRoot=FMath::Lerp(OutsideRoot,EntryApproach,CityLiveEntry::Blend(EntryTime,4.5f,5.1f));
        }
        const bool bExteriorHandle=bExiting ? EntryTime>4.8f : EntryTime<2.f;
        const float Follow=bExiting ? CityLiveEntry::Blend(EntryTime,4.5f,5.15f)*(1-CityLiveEntry::Blend(EntryTime,6.4f,6.8f)) :
            1-CityLiveEntry::Blend(EntryTime,1.7f,2.05f);
        StandingRoot+=(HandleLocation()-ClosedHandle)*(Follow*.5f);
        const FVector StandingPelvis=StandingRoot+DesiredRotation.RotateVector(EnteringCharacter->ReferencePelvisOffset());
        const FVector Pelvis=FMath::Lerp(StandingPelvis,Seated,P.Sit);
        Position=Pelvis-DesiredRotation.RotateVector(EnteringCharacter->ReferencePelvisOffset());
        FCityVehicleBodyPose Goals;Goals.bActive=true;Goals.Pelvis=Pelvis;
        Goals.LegWeight=CityLiveEntry::Blend(P.Sit,.05f,.25f);
        Goals.TorsoCorrection=(Definition.DrivingLeanDegrees-15.f)*P.Sit;
        FVector Outside=GetActorTransform().TransformPosition(FVector(Definition.Approach.X,Side*130,0));
        const float HalfHeight=EnteringCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        const float Ankle=EnteringCharacter->BodyAnkleHeight();
        auto GroundFoot=[&](FVector Point)
        {
            Point.Z=EntryApproach.Z-HalfHeight;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(CityEntryFoot),false,this);Params.AddIgnoredActor(EnteringCharacter);
            FHitResult Hit;
            if(GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector(0,0,70),Point-FVector(0,0,100),ECC_Visibility,Params))
                Point=Hit.ImpactPoint+Hit.ImpactNormal*Ankle;
            else Point.Z+=Ankle;
            return Point;
        };
        const FVector OutsideLeft=GroundFoot(Outside-GetActorForwardVector()*(Side*12));
        const FVector OutsideRight=GroundFoot(Outside+GetActorForwardVector()*(Side*12));
        auto Transfer=[&](FVector Start,FVector End,float Progress)
        {
            FVector Result=FMath::Lerp(Start,End,CityLiveEntry::FootTravel(Progress));
            const float Height=FMath::Max(static_cast<float>(FMath::Max(Start.Z,End.Z)),Definition.SillTopZ+static_cast<float>(GetActorLocation().Z)+Ankle+5.f);
            Result.Z=FMath::Lerp(static_cast<float>(Result.Z),Height+6,CityLiveEntry::FootLift(Progress));return Result;
        };
        const bool bLeftFirst=bExiting ? Side<0 : Side>0;
        Goals.LeftFoot=Transfer(OutsideLeft,GetActorTransform().TransformPosition(Definition.LeftFootFloor)+GetActorUpVector()*Ankle,bLeftFirst ? P.FirstLeg : P.SecondLeg);
        Goals.RightFoot=Transfer(OutsideRight,GetActorTransform().TransformPosition(Definition.RightFootFloor)+GetActorUpVector()*Ankle,bLeftFirst ? P.SecondLeg : P.FirstLeg);
        Goals.HandWeight=P.Wheel;
        const bool bLeftHandle=Side<0;
        Goals.LeftHandWeight=bLeftHandle ? FMath::Max(P.Hand,P.Wheel) : P.Wheel;
        Goals.RightHandWeight=!bLeftHandle ? FMath::Max(P.Hand,P.Wheel) : P.Wheel;
        Goals.LeftHand=GetActorTransform().TransformPosition(Definition.LeftHand);
        Goals.RightHand=GetActorTransform().TransformPosition(Definition.RightHand);
        // Use the door-side hand for the handle. The other hand stays free until driving.
        const FVector Handle=bExteriorHandle ? HandleLocation() :
            DoorPivots[DoorIndex]->GetComponentTransform().TransformPosition(Definition.DoorPullOffset);
        const float HandleBlend=P.Hand/FMath::Max(.0001f,P.Hand+P.Wheel);
        if(bLeftHandle)Goals.LeftHand=FMath::Lerp(Goals.LeftHand,Handle,HandleBlend);
        else Goals.RightHand=FMath::Lerp(Goals.RightHand,Handle,HandleBlend);
        PendingBodyPose=Goals;
    }
    if(!MoveEntryActor(Position,DesiredRotation))
    {
        EntryTime=PreviousTime;
        if(bMontageStarted)EnteringCharacter->SetVehicleAnimationTime(FMath::Max(0.f,PreviousTime));
        DoorPivots[DoorIndex]->SetRelativeRotation(PreviousDoorRotation);
        if(!bReversingEntry){bReversingEntry=true;EnteringPlayer->ClientMessage(TEXT("Проход перекрыт. Посадка/выход отменяется плавно."));}
        return;
    }
    if(EntryTime>=0.f){EnteringCharacter->SetVehicleBodyPose(PendingBodyPose);LastBodyPose=PendingBodyPose;}
    if(bReversingEntry && EntryTime<=0.f)
    {
        if(bExiting)CompleteEntry();
        else
        {
            ACityLiveRPCharacter* Occupant=EnteringCharacter;APlayerController* Player=EnteringPlayer.Get();
            EnteringCharacter=nullptr;EnteringPlayer.Reset();RestoreWalking(Occupant,Player);SetEntryPhysicsPaused(false);
        }
        return;
    }
    if(EntryTime>=CityLiveEntry::Duration){if(bExiting)CompleteExit();else CompleteEntry();}
}

void ACitySedan::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    VehicleCamera->GetCameraView(DeltaTime, OutResult);
    if (EntryCameraBlend <= .001f) return;
    const FVector Focus = GetActorLocation() + FVector(0, 0, 40);
    FVector SideView = GetActorLocation() + GetActorRightVector() * (SeatSettings(EntrySeat).Side * 430.f)
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
    if(IsValid(EnteringCharacter))RestoreWalking(EnteringCharacter,EnteringPlayer.Get());
    EnteringCharacter=nullptr;EnteringPlayer.Reset();
    Super::EndPlay(EndPlayReason);
}
