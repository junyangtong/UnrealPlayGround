#include "WFCTownBuilder.h"
#include "WFCTownMeshGenerator.h"
#include "WFCMarchingSquares.h"
#include "ProceduralMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AWFCTownBuilder::AWFCTownBuilder()
{
	PrimaryActorTick.bCanEverTick = true;
	TownMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TownMesh"));
	SetRootComponent(TownMesh);
	TownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("TownCamera"));
	Camera->SetupAttachment(RootComponent);
	Camera->FieldOfView = 45.f;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	SurfaceMaterial = Material.Object;
}

void AWFCTownBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitializeTown();
}

void AWFCTownBuilder::InitializeTown()
{
	Radius = FMath::Clamp(Radius, 1, 12);
	Spacing = FMath::IsFinite(Spacing) ? FMath::Clamp(Spacing, 10.f, 10000.f) : 160.f;
	FloorHeight = FMath::IsFinite(FloorHeight) ? FMath::Clamp(FloorHeight, 10.f, 1000.f) : 100.f;
	MaxFloors = FMath::Clamp(MaxFloors, 1, 20);
	RelaxIterations = FMath::Clamp(RelaxIterations, 0, 50);
	// 拓扑版本变化会改变单元编号，清空旧高度，避免建筑被映射到错误单元。
	constexpr uint32 TopologyVersion = 2;
	const uint32 Signature = HashCombine(TopologyVersion, HashCombine(HashCombine(GetTypeHash(Radius), GetTypeHash(Seed)),
		HashCombine(GetTypeHash(Spacing), GetTypeHash(RelaxIterations))));
	Grid.Generate(Radius, Spacing, Seed, RelaxIterations);
	if (Signature != GridSignature || Heights.Num() != Grid.Quads.Num())
	{
		Heights.Init(0, Grid.Quads.Num());
		UndoStack.Reset();
		RedoStack.Reset();
	}
	Volume.Initialize(Grid, Signature != GridSignature);
	UndoStack.Reset();
	RedoStack.Reset();
	GridSignature = Signature;
	for (int32& Height : Heights) Height = FMath::Clamp(Height, 0, MaxFloors);
	HoverCell = INDEX_NONE;
	HoverVertex = INDEX_NONE;
	Distance = Spacing * (Radius + 1) * 4.5f;
	Rebuild();
	UpdateCamera();
}

void AWFCTownBuilder::BeginPlay()
{
	Super::BeginPlay();
	// Native topology is intentionally transient, so reconstruct after loading a cooked map.
	InitializeTown();
	if (!bEnableDemoControls) return;
	Controller = GetWorld()->GetFirstPlayerController();
	if (Controller.IsValid() && Controller->IsLocalController())
	{
		PreviousView = Controller->GetViewTarget();
		bPreviousCursor = Controller->bShowMouseCursor;
		Controller->bShowMouseCursor = true;
		Controller->SetViewTarget(this);
	}
}

void AWFCTownBuilder::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Controller.IsValid() && Controller->GetViewTarget() == this)
	{
		Controller->bShowMouseCursor = bPreviousCursor;
		if (PreviousView.IsValid()) Controller->SetViewTarget(PreviousView.Get());
	}
	Super::EndPlay(Reason);
}

void AWFCTownBuilder::ResetTown()
{
	Modify();
	Volume.Initialize(Grid, true);
	Heights.Init(0, Grid.Quads.Num());
	UndoStack.Reset();
	RedoStack.Reset();
	Rebuild();
}

bool AWFCTownBuilder::EditCell(int32 Cell, int32 Delta)
{
	if (bUseMarchingSquares || !Heights.IsValidIndex(Cell) || Delta == 0) return false;
	const int32 Before = Heights[Cell];
	const int32 After = FMath::Clamp(Before + FMath::Clamp(Delta, -1, 1), 0, MaxFloors);
	if (Before == After) return false;
	Modify();
	UndoStack.Add({Cell, Before, After});
	if (UndoStack.Num() > 256) UndoStack.RemoveAt(0);
	RedoStack.Reset();
	Heights[Cell] = After;
	Rebuild();
	return true;
}

void AWFCTownBuilder::Undo()
{
	if (UndoStack.IsEmpty()) return;
	const FEdit Edit = UndoStack.Pop();
	if (Edit.bVertexEdit) Volume.SetOccupied(Edit.Cell, Edit.Before != 0);
	else Heights[Edit.Cell] = Edit.Before;
	RedoStack.Add(Edit);
	Rebuild();
}

void AWFCTownBuilder::Redo()
{
	if (RedoStack.IsEmpty()) return;
	const FEdit Edit = RedoStack.Pop();
	if (Edit.bVertexEdit) Volume.SetOccupied(Edit.Cell, Edit.After != 0);
	else Heights[Edit.Cell] = Edit.After;
	UndoStack.Add(Edit);
	Rebuild();
}

void AWFCTownBuilder::UpdateCamera()
{
	const FRotator Rotation(-Pitch, Yaw, 0);
	Camera->SetRelativeLocation(-Rotation.Vector() * Distance + FVector(0, 0, FloorHeight));
	Camera->SetRelativeRotation(Rotation);
}

void AWFCTownBuilder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bEnableDemoControls || !Controller.IsValid() || Controller->GetViewTarget() != this) return;
	HandleCameraInput();
	HandleBuildInput();
	ShowControlsHint();
}

void AWFCTownBuilder::HandleCameraInput()
{
	if (Controller->IsInputKeyDown(EKeys::MiddleMouseButton))
	{
		float X = 0, Y = 0;
		Controller->GetInputMouseDelta(X, Y);
		Yaw += X * 0.3f;
		Pitch = FMath::Clamp(Pitch + Y * 0.3f, 20.f, 85.f);
	}
	if (Controller->WasInputKeyJustPressed(EKeys::MouseScrollUp)) Distance *= 0.9f;
	if (Controller->WasInputKeyJustPressed(EKeys::MouseScrollDown)) Distance *= 1.1f;
	Distance = FMath::Clamp(Distance, Spacing * 2.f, Spacing * (Radius + 1) * 12.f);
	UpdateCamera();
}

void AWFCTownBuilder::HandleBuildInput()
{
	if (bUseMarchingSquares)
	{
		const int32 NewHover = PickVertex();
		if (HoverVertex != NewHover) { HoverVertex = NewHover; Rebuild(); }
		if (Controller->WasInputKeyJustPressed(EKeys::LeftMouseButton)) SetVertexOccupied(HoverVertex, true);
		if (Controller->WasInputKeyJustPressed(EKeys::RightMouseButton)) SetVertexOccupied(HoverVertex, false);
	}
	else
	{
		const int32 NewHover = PickCell();
		if (HoverCell != NewHover) { HoverCell = NewHover; Rebuild(); }
		if (Controller->WasInputKeyJustPressed(EKeys::LeftMouseButton)) EditCell(HoverCell, 1);
		if (Controller->WasInputKeyJustPressed(EKeys::RightMouseButton)) EditCell(HoverCell, -1);
	}
	if (Controller->WasInputKeyJustPressed(EKeys::Z)) Undo();
	if (Controller->WasInputKeyJustPressed(EKeys::Y)) Redo();
}

void AWFCTownBuilder::ShowControlsHint() const
{
	if (GEngine && bUseMarchingSquares)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::White,
			FString::Printf(TEXT("MARCHING SQUARES | LMB occupy / RMB clear VERTEX | MMB orbit | Wheel zoom | Z undo / Y redo | Vertex %d"), HoverVertex));
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.f, FColor::White,
			FString::Printf(TEXT("TOWN PROTOTYPE | LMB build | RMB remove | MMB orbit | Wheel zoom | Z undo / Y redo | Cell %d : %d/%d"),
			HoverCell, GetCellHeight(HoverCell), MaxFloors));
	}
}

void AWFCTownBuilder::Rebuild()
{
	const FWFCTownMeshData Mesh = bUseMarchingSquares
		? FWFCMarchingSquares::BuildMesh(Grid, Volume, SquareModules, FloorHeight, HoverVertex)
		: FWFCTownMeshGenerator::BuildExtrudedMesh(Grid, Heights, FloorHeight, HoverCell);
	TownMesh->ClearAllMeshSections();
	TownMesh->CreateMeshSection_LinearColor(0, Mesh.Positions, Mesh.Indices, Mesh.Normals,
		Mesh.UVs, Mesh.Colors, TArray<FProcMeshTangent>(), false);
	TownMesh->SetMaterial(0, SurfaceMaterial);
}

int32 AWFCTownBuilder::PickCell() const
{
	FVector Origin, Direction;
	if (!Controller.IsValid() || !Controller->DeprojectMousePositionToWorld(Origin, Direction)) return INDEX_NONE;
	// 射线与网格必须处于相同的 Actor 局部坐标空间。
	Origin = GetActorTransform().InverseTransformPosition(Origin);
	Direction = GetActorTransform().InverseTransformVector(Direction).GetSafeNormal();
	return FWFCTownMeshGenerator::RaycastCell(Grid, Heights, FloorHeight, Origin, Direction);
}

bool AWFCTownBuilder::SetVertexOccupied(int32 Vertex, bool bOccupied)
{
	if (!bUseMarchingSquares || !Volume.Occupied.IsValidIndex(Vertex) || Volume.IsOccupied(Vertex) == bOccupied) return false;
	Modify();
	const int32 Before = Volume.IsOccupied(Vertex) ? 1 : 0;
	Volume.SetOccupied(Vertex, bOccupied);
	UndoStack.Add({Vertex, Before, bOccupied ? 1 : 0, true});
	if (UndoStack.Num() > 256) UndoStack.RemoveAt(0);
	RedoStack.Reset();
	Rebuild();
	return true;
}

int32 AWFCTownBuilder::GetSquareMask(int32 Cell) const
{
	return Grid.Quads.IsValidIndex(Cell) ? Volume.GetSquareMask(Grid.Quads[Cell]) : INDEX_NONE;
}

int32 AWFCTownBuilder::PickVertex() const
{
	if (!Controller.IsValid()) return INDEX_NONE;
	float MouseX, MouseY;
	if (!Controller->GetMousePosition(MouseX, MouseY)) return INDEX_NONE;
	double BestDistanceSquared = 18. * 18.; // 屏幕像素阈值，缩放时仍容易选中。
	int32 BestVertex = INDEX_NONE;
	for (int32 Vertex = 0; Vertex < Grid.Vertices.Num(); ++Vertex)
	{
		FVector2D Screen;
		const FVector Marker = GetActorTransform().TransformPosition(Grid.Vertices[Vertex] + FVector(0,0,FloorHeight+8));
		if (!Controller->ProjectWorldLocationToScreen(Marker, Screen)) continue;
		const double DistanceSquared = FVector2D::DistSquared(Screen, FVector2D(MouseX, MouseY));
		if (DistanceSquared < BestDistanceSquared) { BestDistanceSquared = DistanceSquared; BestVertex = Vertex; }
	}
	return BestVertex;
}
