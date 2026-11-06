#include "GrassInteractionDemo.h"
#include "GrassSpringSimulation.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "RenderingThread.h"
#include "UObject/ConstructorHelpers.h"

AGrassInteractionDemo::AGrassInteractionDemo()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Grass = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Grass"));
    Grass->SetupAttachment(RootComponent);
    Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Grass->SetCastShadow(false);
    Grass->BoundsScale = 2.f; // Include WPO displacement in the culling bounds.

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Brush = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteractorSphere"));
    Brush->SetupAttachment(RootComponent);
    Brush->SetStaticMesh(Sphere.Object);
    Brush->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PositionPreview = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplacementPreview"));
    VelocityPreview = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VelocityPreview"));
    for (UStaticMeshComponent* Preview : {PositionPreview.Get(), VelocityPreview.Get()})
    {
        Preview->SetupAttachment(RootComponent);
        Preview->SetStaticMesh(Plane.Object);
        Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Preview->SetCastShadow(false);
        Preview->SetRelativeScale3D(FVector(3.2f));
    }
    PositionPreview->SetRelativeLocation(FVector(-220, -850, 5));
    VelocityPreview->SetRelativeLocation(FVector(220, -850, 5));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("DemoCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeLocation(FVector(1150, -1550, 1350));
    Camera->SetRelativeRotation((FVector(0, -150, 0) - Camera->GetRelativeLocation()).Rotation());
}

void AGrassInteractionDemo::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    BuildGrass();
}

void AGrassInteractionDemo::BuildGrass()
{
    TArray<FVector> Vertices, Normals;
    TArray<int32> Indices;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    FRandomStream Random(42);
    const int32 Count = FMath::Clamp(GrassPerSide, 8, 100);
    const float Size = FMath::Max(FieldSize, 200.f);
    for (int32 Y = 0; Y < Count; ++Y)
    for (int32 X = 0; X < Count; ++X)
    {
        const FVector2D UV((X + Random.FRandRange(0.15f, 0.85f)) / Count,
                           (Y + Random.FRandRange(0.15f, 0.85f)) / Count);
        const FVector Root((UV.X - 0.5) * Size, (UV.Y - 0.5) * Size, 1.f);
        const float Height = Random.FRandRange(38.f, 65.f);
        const float Shade = Random.FRand();
        const float Angle = Random.FRandRange(0.f, PI);
        // Two crossed tapered ribbons, each with 4 vertical segments.
        for (int32 Blade = 0; Blade < 2; ++Blade)
        {
            const FVector Right(FMath::Cos(Angle + Blade * HALF_PI), FMath::Sin(Angle + Blade * HALF_PI), 0);
            const int32 Base = Vertices.Num();
            for (int32 Row = 0; Row <= 4; ++Row)
            {
                const float T = Row / 4.f;
                const float Width = FMath::Max(0.15f, 3.5f * (1.f - T));
                for (int32 Side = 0; Side < 2; ++Side)
                {
                    Vertices.Add(Root + Right * (Side ? Width : -Width) + FVector(0, 0, Height * T));
                    Normals.Add(FVector::CrossProduct(Right, FVector::UpVector));
                    UVs.Add(UV); // Every vertex of this tuft samples the same root pixel.
                    Colors.Add(FLinearColor(T, Shade, 0, 1)); // R = height mask; G = color variation.
                }
            }
            for (int32 Row = 0; Row < 4; ++Row)
            {
                const int32 I = Base + Row * 2;
                Indices.Append({I, I + 2, I + 1, I + 1, I + 2, I + 3});
            }
        }
    }
    Grass->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
    if (GrassMaterial) Grass->SetMaterial(0, GrassMaterial);
}

void AGrassInteractionDemo::BeginPlay()
{
    Super::BeginPlay();
    // This teaching demo uses a horizontal field with unit actor scale.
    SetActorRotation(FRotator::ZeroRotator);
    SetActorScale3D(FVector::OneVector);
    FieldSize = FMath::Clamp(FieldSize, 200.f, 4000.f);
    Resolution = FMath::Clamp(Resolution, 16, 512);
    BuildGrass();
    StateA = UKismetRenderingLibrary::CreateRenderTarget2D(this, Resolution, Resolution, RTF_RGBA16f, FLinearColor::Transparent, false, true);
    StateB = UKismetRenderingLibrary::CreateRenderTarget2D(this, Resolution, Resolution, RTF_RGBA16f, FLinearColor::Transparent, false, true);
    for (UTextureRenderTarget2D* RT : {StateA.Get(), StateB.Get()})
    {
        RT->AddressX = TA_Clamp;
        RT->AddressY = TA_Clamp;
        RT->Filter = TF_Bilinear;
        RT->UpdateResource(); // Recreate the sampler after changing filtering/addressing.
    }
    GrassMID = Grass->CreateDynamicMaterialInstance(0, GrassMaterial);
    if (DebugMaterial)
    {
        PositionMID = PositionPreview->CreateDynamicMaterialInstance(0, DebugMaterial);
        VelocityMID = VelocityPreview->CreateDynamicMaterialInstance(0, DebugMaterial);
        PositionMID->SetScalarParameterValue(TEXT("ShowVelocity"), 0.f);
        VelocityMID->SetScalarParameterValue(TEXT("ShowVelocity"), 1.f);
    }
    ResetSimulation();
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (bUseDemoCamera)
        {
            // Fixed exposure keeps the signed-state preview readable against a black background.
            auto& PP = Camera->PostProcessSettings;
            PP.bOverride_AutoExposureMethod = true;
            PP.AutoExposureMethod = AEM_Manual;
            PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
            PP.AutoExposureApplyPhysicalCameraExposure = false;
            PP.bOverride_AutoExposureBias = true;
            PP.AutoExposureBias = 0.f;
            PC->SetViewTarget(this);
            if (APawn* Pawn = PC->GetPawn(); Pawn && Pawn->IsA<ADefaultPawn>())
                Pawn->SetActorHiddenInGame(true);
        }
        PC->bShowMouseCursor = true;
    }
}

void AGrassInteractionDemo::ResetSimulation()
{
    if (!StateA || !StateB) return;
    UKismetRenderingLibrary::ClearRenderTarget2D(this, StateA, FLinearColor::Transparent);
    UKismetRenderingLibrary::ClearRenderTarget2D(this, StateB, FLinearColor::Transparent);
    CurrentState = StateA;
    Accumulator = 0.f;
    SimulationTime = 0.f;
    BindCurrentState();
}

void AGrassInteractionDemo::BindCurrentState()
{
    for (UMaterialInstanceDynamic* MID : {GrassMID.Get(), PositionMID.Get(), VelocityMID.Get()})
        if (MID) MID->SetTextureParameterValue(TEXT("StateTexture"), CurrentState);
}

void AGrassInteractionDemo::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!CurrentState) return;
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && PC->WasInputKeyJustPressed(EKeys::SpaceBar)) bAutoMove = !bAutoMove;
    if (PC && PC->WasInputKeyJustPressed(EKeys::R)) ResetSimulation();

    // A fixed 120 Hz step makes the spring consistent at 30/60/120 fps.
    // Cap catch-up to 8 steps so a breakpoint/hitch cannot explode the simulation.
    constexpr float Dt = 1.f / 120.f;
    Accumulator = FMath::Min(Accumulator + DeltaSeconds, 8.f * Dt);
    while (Accumulator >= Dt)
    {
        SimulationTime += Dt;
        FVector LocalBrush(FMath::Sin(SimulationTime * 0.7f) * FieldSize * 0.32f,
                           FMath::Sin(SimulationTime * 1.1f) * FieldSize * 0.23f, 0);
        bool bApplyForce = bAutoMove;
        if (Interactor)
        {
            LocalBrush = GetActorTransform().InverseTransformPosition(Interactor->GetActorLocation());
            bApplyForce = FMath::Abs(LocalBrush.Z) < 150.f;
        }
        else if (PC && PC->IsInputKeyDown(EKeys::LeftMouseButton))
        {
            FVector Origin, Direction;
            if (PC->DeprojectMousePositionToWorld(Origin, Direction) && FMath::Abs(Direction.Z) > KINDA_SMALL_NUMBER)
            {
                const float T = (GetActorLocation().Z - Origin.Z) / Direction.Z;
                if (T > 0.f)
                {
                    LocalBrush = Origin + T * Direction - GetActorLocation();
                    bApplyForce = true;
                }
            }
        }
        Brush->SetRelativeLocation(FVector(LocalBrush.X, LocalBrush.Y, 38));
        Brush->SetRelativeScale3D(FVector(FMath::Clamp(BrushRadius, 10.f, 300.f) / 100.f));
        Brush->SetVisibility(bApplyForce);

        UTextureRenderTarget2D* WriteRT = CurrentState == StateA ? StateB.Get() : StateA.Get();
        FGrassSpringStep Step{Dt, FMath::Clamp(Stiffness, 1.f, 100.f), FMath::Clamp(Damping, 0.1f, 20.f),
            bApplyForce ? FMath::Clamp(ForceStrength, 0.f, 3000.f) : 0.f, FieldSize,
            FVector2f(LocalBrush.X, LocalBrush.Y), FMath::Clamp(BrushRadius, 10.f, 300.f)};
        DispatchGrassSpring(CurrentState, WriteRT, Step); // Read A -> write B. Next step: read B -> write A.
        CurrentState = WriteRT;
        Accumulator -= Dt;
    }
    BindCurrentState(); // Grass and debug planes sample the last completed state in render command order.
}

void AGrassInteractionDemo::EndPlay(const EEndPlayReason::Type Reason)
{
    // Ensure queued passes finish before transient RT resources can be released.
    FlushRenderingCommands();
    Super::EndPlay(Reason);
}
