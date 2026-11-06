// ========================
// WFCGridGenerator.cpp (WFCRuntime 模块)
// 关卡内 WFC 可视化生成器实现
// ========================
#include "WFCGridGenerator.h"
#include "WFCSolver.h"
#include "WFCTileData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HexData.h"

DEFINE_LOG_CATEGORY_STATIC(LogWFCGenerator, Log, All);

AWFCGridGenerator::AWFCGridGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 创建根组件
	GridRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GridRoot"));
	RootComponent = GridRoot;

	// 加载引擎内置 Plane Mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		PlaneMesh = PlaneMeshFinder.Object;
	}
}

// ========================
// Generate
// ========================
void AWFCGridGenerator::Generate()
{
	if (!TileSet)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("Generate failed: TileSet is not set"));
		return;
	}

	if (TileSet->Tiles.Num() == 0)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("Generate failed: TileSet has no tiles"));
		return;
	}

	// 创建求解器
	UWFCSolver* Solver = NewObject<UWFCSolver>(this);
	if (!Solver->Initialize(TileSet, GridWidth, GridHeight, Seed))
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("Generate failed: solver initialization failed"));
		return;
	}

	// 执行求解
	if (!Solver->Solve(MaxRetries))
	{
		UE_LOG(LogWFCGenerator, Warning, TEXT("WFC Generation failed after %d retries"), MaxRetries);
		return;
	}

	// 获取结果并构建可视化
	TArray<int32> Result = Solver->GetResult();
	BuildVisualGrid(Result);

	UE_LOG(LogWFCGenerator, Log, TEXT("WFC Generation succeeded: %dx%d grid"), GridWidth, GridHeight);
}
// ========================
// DrawGridLines — 使用 ProceduralMeshComponent 生成网格线（Shipping 可见）
// 每条线用两个三角形组成的细长 Quad 表示
// ========================
void AWFCGridGenerator::DrawGridLines()
{
	// 清除旧的网格线
	if (GridLineMesh)
	{
		GridLineMesh->DestroyComponent();
		GridLineMesh = nullptr;
	}

	if (!bShowGridLines)
	{
		return;
	}

	// 创建 ProceduralMeshComponent
	GridLineMesh = NewObject<UProceduralMeshComponent>(this, TEXT("GridLineMesh"));
	GridLineMesh->SetupAttachment(GridRoot);
	GridLineMesh->RegisterComponent();
	GridLineMesh->SetCastShadow(false);
	GridLineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GridLineMesh->bUseComplexAsSimpleCollision = false;

	// Plane mesh 中心在格子中心, 网格线偏移 HalfTile 对齐到 Tile 边界
	const float HalfTile = TileSize * 0.5f;
	const float HalfWidth = GridLineWidth * 0.5f;
	const float GridTotalX = GridWidth * TileSize;
	const float GridTotalY = GridHeight * TileSize;
	// 网格线略微上抬，避免和 Plane 面 Z-fighting
	const float ZOffset = 0.5f;

	const FColor VertColor = GridLineColor.ToFColor(/*bSRGB=*/true);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FColor> Colors;

	// 辅助 Lambda：添加一个 Quad（两个三角形）
	// P0--P1
	// |  / |
	// P2--P3
	auto AddQuad = [&](const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3)
	{
		int32 BaseIdx = Vertices.Num();
		Vertices.Add(P0);
		Vertices.Add(P1);
		Vertices.Add(P2);
		Vertices.Add(P3);

		Colors.Add(VertColor);
		Colors.Add(VertColor);
		Colors.Add(VertColor);
		Colors.Add(VertColor);

		// 三角形 1: P0-P2-P1
		Triangles.Add(BaseIdx + 0);
		Triangles.Add(BaseIdx + 2);
		Triangles.Add(BaseIdx + 1);
		// 三角形 2: P1-P2-P3
		Triangles.Add(BaseIdx + 1);
		Triangles.Add(BaseIdx + 2);
		Triangles.Add(BaseIdx + 3);
	};

	// 竖线 (沿 Y 方向, X 恒定) —— GridWidth + 1 条
	for (int32 X = 0; X <= GridWidth; ++X)
	{
		float LineX = X * TileSize - HalfTile;
		float YStart = -HalfTile;
		float YEnd = GridTotalY - HalfTile;

		AddQuad(
			FVector(LineX - HalfWidth, YStart, ZOffset),   // P0 左上
			FVector(LineX + HalfWidth, YStart, ZOffset),   // P1 右上
			FVector(LineX - HalfWidth, YEnd,   ZOffset),   // P2 左下
			FVector(LineX + HalfWidth, YEnd,   ZOffset)    // P3 右下
		);
	}

	// 横线 (沿 X 方向, Y 恒定) —— GridHeight + 1 条
	for (int32 Y = 0; Y <= GridHeight; ++Y)
	{
		float LineY = Y * TileSize - HalfTile;
		float XStart = -HalfTile;
		float XEnd = GridTotalX - HalfTile;

		AddQuad(
			FVector(XStart, LineY - HalfWidth, ZOffset),   // P0 左上
			FVector(XEnd,   LineY - HalfWidth, ZOffset),   // P1 右上
			FVector(XStart, LineY + HalfWidth, ZOffset),   // P2 左下
			FVector(XEnd,   LineY + HalfWidth, ZOffset)    // P3 右下
		);
	}

	// 法线和 UV（简单填充）
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	Normals.Init(FVector::UpVector, Vertices.Num());
	UVs.Init(FVector2D::ZeroVector, Vertices.Num());

	// 创建 Mesh Section 0
	GridLineMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

	// 创建 Unlit 纯色材质
	UMaterialInterface* UnlitMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (UnlitMat)
	{
		UMaterialInstanceDynamic* LineMID = UMaterialInstanceDynamic::Create(UnlitMat, this);
		if (LineMID)
		{
			LineMID->SetVectorParameterValue(FName("Color"), GridLineColor);
			GridLineMesh->SetMaterial(0, LineMID);
		}
	}

	UE_LOG(LogWFCGenerator, Log, TEXT("Grid lines drawn: %d vertical + %d horizontal (%d vertices)"),
		GridWidth + 1, GridHeight + 1, Vertices.Num());
}
// ========================
// DrawHexGridLines — 生成六边形点阵
// ========================
void AWFCGridGenerator::DrawHexGridLines()
{
	ClearGrid();

	GridLineMesh = NewObject<UProceduralMeshComponent>(this, TEXT("HexGridMesh"));
	GridLineMesh->SetupAttachment(GridRoot);
	GridLineMesh->RegisterComponent();
	GridLineMesh->SetCastShadow(false);
	GridLineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GridLineMesh->bUseComplexAsSimpleCollision = false;

	const float HexSize = TileSize * 0.5f;
	const float HalfWidth = GridLineWidth * 0.5f;
	const float ZOffset = 0.5f;
	const FColor VertColor = GridLineColor.ToFColor(/*bSRGB=*/true);
	const TArray<FVector> HexVertices = FHexCoord::BuildSpiralPoints(HexGridRadius, HexSize);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TSet<FString> AddedEdges;

	auto MakeCorner = [&](const FVector& Center, int32 CornerIndex)
	{
		const float AngleDeg = 60.0f * CornerIndex;
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);
		return FVector(
			Center.X + HexSize * FMath::Cos(AngleRad),
			Center.Y + HexSize * FMath::Sin(AngleRad),
			Center.Z + ZOffset);
	};

	auto MakePointKey = [](const FVector& Point)
	{
		const int32 X = FMath::RoundToInt(Point.X * 100.0f);
		const int32 Y = FMath::RoundToInt(Point.Y * 100.0f);
		return FString::Printf(TEXT("%d_%d"), X, Y);
	};

	auto AddEdgeQuad = [&](const FVector& Start, const FVector& End)
	{
		const FString StartKey = MakePointKey(Start);
		const FString EndKey = MakePointKey(End);
		const FString EdgeKey = (StartKey < EndKey)
			? FString::Printf(TEXT("%s|%s"), *StartKey, *EndKey)
			: FString::Printf(TEXT("%s|%s"), *EndKey, *StartKey);

		if (AddedEdges.Contains(EdgeKey))
		{
			return;
		}
		AddedEdges.Add(EdgeKey);

		FVector EdgeDir = End - Start;
		EdgeDir.Z = 0.0f;
		EdgeDir = EdgeDir.GetSafeNormal();
		const FVector SideOffset(-EdgeDir.Y * HalfWidth, EdgeDir.X * HalfWidth, 0.0f);

		const int32 BaseIndex = Vertices.Num();
		Vertices.Add(Start - SideOffset);
		Vertices.Add(Start + SideOffset);
		Vertices.Add(End - SideOffset);
		Vertices.Add(End + SideOffset);

		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);

		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));

		Colors.Add(VertColor);
		Colors.Add(VertColor);
		Colors.Add(VertColor);
		Colors.Add(VertColor);

		Triangles.Add(BaseIndex + 0);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 3);
	};

	for (int32 RingRadius = 0; RingRadius <= HexGridRadius; ++RingRadius)
	{
		const TArray<FVector> RingCenters = FHexCoord::GrabRing(HexVertices, RingRadius);
		for (const FVector& Center : RingCenters)
		{
			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector Start = MakeCorner(Center, CornerIndex);
				const FVector End = MakeCorner(Center, (CornerIndex + 1) % 6);
				AddEdgeQuad(Start, End);
			}
		}
	}

	GridLineMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

	UMaterialInterface* MaterialToUse = BaseMaterial ? BaseMaterial.Get() : GetOrCreateDefaultMaterial();
	if (MaterialToUse)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MaterialToUse, this);
		if (MID)
		{
			MID->SetVectorParameterValue(FName("Color"), GridLineColor);
			GridLineMesh->SetMaterial(0, MID);
		}
	}

	UE_LOG(LogWFCGenerator, Log, TEXT("Hex grid lines drawn: %d unique edges, %d vertices"),
		AddedEdges.Num(), Vertices.Num());
}
// ========================
// BuildVisualGrid
// ========================
void AWFCGridGenerator::BuildVisualGrid(const TArray<int32>& Result)
{
	// 先清除旧的
	ClearGrid();

	if (!PlaneMesh)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("BuildVisualGrid: Plane mesh not found"));
		return;
	}

	UMaterialInterface* MaterialToUse = BaseMaterial ? BaseMaterial.Get() : GetOrCreateDefaultMaterial();
	if (!MaterialToUse)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("BuildVisualGrid: No material available"));
		return;
	}

	// Plane mesh 默认是 100x100 单位，朝上（Z+）
	// 缩放因子：TileSize / 100.0
	const float PlaneScale = TileSize / 100.0f;

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			int32 Index = Y * GridWidth + X;
			int32 TileIdx = Result[Index];

			// 创建 StaticMeshComponent
			FName CompName = *FString::Printf(TEXT("TileMesh_%d_%d"), X, Y);
			UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this, CompName);
			MeshComp->SetStaticMesh(PlaneMesh);
			MeshComp->SetupAttachment(GridRoot);
			MeshComp->RegisterComponent();

			// 设置位置和缩放
			// X 轴 = 列方向，Y 轴 = 行方向，Plane 面朝 Z+
			FVector LocalPos(X * TileSize, Y * TileSize, 0.0f);
			MeshComp->SetRelativeLocation(LocalPos);
			MeshComp->SetRelativeScale3D(FVector(PlaneScale, PlaneScale, 1.0f));

			// 禁用碰撞
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->SetCastShadow(false);

			// 创建动态材质实例
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(MaterialToUse, this);

			if (MID && TileSet && TileSet->Tiles.IsValidIndex(TileIdx))
			{
				UWFCTileData* TileData = TileSet->Tiles[TileIdx];
				if (TileData && TileData->TileTexture)
				{
					MID->SetTextureParameterValue(FName("BaseTexture"), TileData->TileTexture);
				}
			}

			MeshComp->SetMaterial(0, MID);

			TileMeshComponents.Add(MeshComp);
			TileMaterials.Add(MID);
		}
	}
}

// ========================
// ClearGrid
// ========================
void AWFCGridGenerator::ClearGrid()
{
	for (UStaticMeshComponent* Comp : TileMeshComponents)
	{
		if (Comp)
		{
			Comp->DestroyComponent();
		}
	}
	TileMeshComponents.Empty();
	TileMaterials.Empty();

	for (UTextRenderComponent* Label : CandidateCountLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	CandidateCountLabels.Empty();

	if (GridLineMesh)
	{
		GridLineMesh->DestroyComponent();
		GridLineMesh = nullptr;
	}

	if (bInteractiveMode)
	{
		InteractiveSolver = nullptr;
		bInteractiveMode = false;
		SetActorTickEnabled(false);
	}
}

// ========================
// Regenerate
// ========================
void AWFCGridGenerator::Regenerate()
{
	ClearGrid();
	Generate();
}

// ========================
// 交互模式
// ========================

void AWFCGridGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (TileSet && !bInteractiveMode) Generate();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && bInteractiveMode)
	{
		PC->bShowMouseCursor = true;
	}
}

void AWFCGridGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInteractiveMode || !InteractiveSolver)
	{
		return;
	}

	// 检测鼠标左键点击
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		int32 ClickX, ClickY;
		if (GetGridCoordsFromMouse(ClickX, ClickY))
		{
			InteractiveCollapseAt(ClickX, ClickY);
		}
	}
}

void AWFCGridGenerator::InitInteractiveMode()
{
	if (!TileSet)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("InitInteractiveMode: TileSet is not set"));
		return;
	}

	if (TileSet->Tiles.Num() == 0)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("InitInteractiveMode: TileSet has no tiles"));
		return;
	}

	// 清除已有内容
	ClearGrid();
	bInteractiveMode = false;

	// 创建求解器
	InteractiveSolver = NewObject<UWFCSolver>(this);
	if (!InteractiveSolver->InitializeInteractive(TileSet, GridWidth, GridHeight, Seed))
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("InitInteractiveMode: solver init failed"));
		InteractiveSolver = nullptr;
		return;
	}

	bInteractiveMode = true;

	// 记录初始最大候选数
	MaxCandidateCount = 0;
	for (int32 i = 0; i < TileSet->Tiles.Num(); ++i)
	{
		if (TileSet->Tiles[i] != nullptr)
		{
			++MaxCandidateCount;
		}
	}

	if (!PlaneMesh)
	{
		UE_LOG(LogWFCGenerator, Error, TEXT("InitInteractiveMode: Plane mesh not found"));
		return;
	}

	UMaterialInterface* MaterialToUse = BaseMaterial ? BaseMaterial.Get() : GetOrCreateDefaultMaterial();
	const float PlaneScale = TileSize / 100.0f;

	// 为每个格子创建 Mesh 和标签
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			// 创建 Plane Mesh
			FName CompName = *FString::Printf(TEXT("TileMesh_%d_%d"), X, Y);
			UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this, CompName);
			MeshComp->SetStaticMesh(PlaneMesh);
			MeshComp->SetupAttachment(GridRoot);
			MeshComp->RegisterComponent();

			FVector LocalPos(X * TileSize, Y * TileSize, 0.0f);
			MeshComp->SetRelativeLocation(LocalPos);
			MeshComp->SetRelativeScale3D(FVector(PlaneScale, PlaneScale, 1.0f));
			MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
			MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			MeshComp->SetCastShadow(false);

			// 创建动态材质 - 初始为未确定颜色
			UMaterialInstanceDynamic* MID = nullptr;
			if (MaterialToUse)
			{
				MID = UMaterialInstanceDynamic::Create(MaterialToUse, this);
			}
			MeshComp->SetMaterial(0, MID);

			TileMeshComponents.Add(MeshComp);
			TileMaterials.Add(MID);

			// 创建候选数标签
			FName LabelName = *FString::Printf(TEXT("Label_%d_%d"), X, Y);
			UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this, LabelName);
			Label->SetupAttachment(GridRoot);
			Label->RegisterComponent();
			Label->SetRelativeLocation(LocalPos + FVector(0.0f, 0.0f, 1.0f));
			Label->SetRelativeRotation(FRotator(90.0f, 0.0f, 180.0f));
			Label->SetHorizontalAlignment(EHTA_Center);
			Label->SetVerticalAlignment(EVRTA_TextCenter);
			Label->SetWorldSize(TileSize * 0.3f);
			Label->SetTextRenderColor(FColor::White);
			CandidateCountLabels.Add(Label);
		}
	}

	// 刷新全部可视化
	RefreshInteractiveVisuals();

	// 绘制网格线
	DrawGridLines();

	// 启用 Tick 用于鼠标检测
	SetActorTickEnabled(true);

	UE_LOG(LogWFCGenerator, Log, TEXT("Interactive mode initialized: %dx%d, %d candidates per cell"),
		GridWidth, GridHeight, MaxCandidateCount);
}

bool AWFCGridGenerator::InteractiveCollapseAt(int32 X, int32 Y)
{
	if (!bInteractiveMode || !InteractiveSolver)
	{
		UE_LOG(LogWFCGenerator, Warning, TEXT("InteractiveCollapseAt: not in interactive mode"));
		return false;
	}

	if (InteractiveSolver->IsCellCollapsed(X, Y))
	{
		UE_LOG(LogWFCGenerator, Log, TEXT("Cell (%d,%d) already collapsed, ignoring click"), X, Y);
		return false;
	}

	bool bSuccess = InteractiveSolver->CollapseAt(X, Y);

	// 刷新全图可视化（因为传播可能影响很多格子）
	RefreshInteractiveVisuals();

	if (InteractiveSolver->IsFullyCollapsed())
	{
		UE_LOG(LogWFCGenerator, Log, TEXT("Interactive WFC complete! All cells determined."));
		// 完成后禁用 Tick
		SetActorTickEnabled(false);
	}

	return bSuccess;
}

void AWFCGridGenerator::ResetInteractiveMode()
{
	ClearGrid();

	// 清除标签
	for (UTextRenderComponent* Label : CandidateCountLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	CandidateCountLabels.Empty();

	InteractiveSolver = nullptr;
	bInteractiveMode = false;
	SetActorTickEnabled(false);

	UE_LOG(LogWFCGenerator, Log, TEXT("Interactive mode reset"));
}

bool AWFCGridGenerator::IsInteractiveComplete() const
{
	if (!bInteractiveMode || !InteractiveSolver)
	{
		return false;
	}
	return InteractiveSolver->IsFullyCollapsed();
}

bool AWFCGridGenerator::GetGridCoordsFromMouse(int32& OutX, int32& OutY) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}

	// 从鼠标位置做射线检测
	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return false;
	}

	FHitResult HitResult;
	FVector TraceEnd = WorldLocation + WorldDirection * 50000.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	if (!GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	// 检查是否击中了我们的 Mesh
	UStaticMeshComponent* HitComp = Cast<UStaticMeshComponent>(HitResult.GetComponent());
	if (!HitComp || !TileMeshComponents.Contains(HitComp))
	{
		return false;
	}

	// 从组件列表中找到索引
	int32 Index = TileMeshComponents.IndexOfByKey(HitComp);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	OutX = Index % GridWidth;
	OutY = Index / GridWidth;
	return true;
}

void AWFCGridGenerator::RefreshInteractiveVisuals()
{
	if (!InteractiveSolver)
	{
		return;
	}

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			RefreshCellVisual(X, Y);
		}
	}
}

void AWFCGridGenerator::RefreshCellVisual(int32 X, int32 Y)
{
	if (!InteractiveSolver || !TileSet)
	{
		return;
	}

	int32 Index = Y * GridWidth + X;
	if (!TileMeshComponents.IsValidIndex(Index) || !TileMaterials.IsValidIndex(Index))
	{
		return;
	}

	UMaterialInstanceDynamic* MID = TileMaterials[Index];
	UTextRenderComponent* Label = CandidateCountLabels.IsValidIndex(Index) ? CandidateCountLabels[Index] : nullptr;

	bool bCollapsed = InteractiveSolver->IsCellCollapsed(X, Y);
	int32 CandidateCount = InteractiveSolver->GetCellCandidateCount(X, Y);

	if (bCollapsed)
	{
		// 已坍缩 → 显示实际 Tile 纹理
		int32 TileIdx = InteractiveSolver->GetCellCollapsedTile(X, Y);
		if (MID && TileSet->Tiles.IsValidIndex(TileIdx))
		{
			UWFCTileData* TileData = TileSet->Tiles[TileIdx];
			if (TileData && TileData->TileTexture)
			{
				MID->SetTextureParameterValue(FName("BaseTexture"), TileData->TileTexture);
			}
			// 恢复不透明
			MID->SetVectorParameterValue(FName("Color"), FLinearColor::White);
		}

		// 隐藏标签
		if (Label)
		{
			Label->SetVisibility(false);
		}
	}
	else if (CandidateCount == 0)
	{
		// 矛盾 → 显示红色
		if (MID)
		{
			MID->SetVectorParameterValue(FName("Color"), ContradictionColor);
		}
		if (Label)
		{
			Label->SetText(FText::FromString(TEXT("X")));
			Label->SetTextRenderColor(FColor::Red);
			Label->SetVisibility(true);
		}
	}
	else
	{
		// 未确定 → 显示候选数和渐变色
		FLinearColor CellColor = GetCellColor(CandidateCount, MaxCandidateCount);
		if (MID)
		{
			MID->SetVectorParameterValue(FName("Color"), CellColor);
			// 清除纹理（未确定时不显示）
			MID->SetTextureParameterValue(FName("BaseTexture"), nullptr);
		}
		if (Label)
		{
			Label->SetText(FText::FromString(FString::Printf(TEXT("%d"), CandidateCount)));
			Label->SetTextRenderColor(FColor::White);
			Label->SetVisibility(true);
		}
	}
}

FLinearColor AWFCGridGenerator::GetCellColor(int32 CandidateCount, int32 MaxCandidates) const
{
	if (MaxCandidates <= 1)
	{
		return UncollapsedColorLow;
	}

	// 归一化：1 → 0.0 (即将确定)，MaxCandidates → 1.0 (完全不确定)
	float Alpha = static_cast<float>(CandidateCount - 1) / static_cast<float>(MaxCandidates - 1);
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	return FMath::Lerp(UncollapsedColorLow, UncollapsedColorHigh, Alpha);
}

// ========================
// 默认材质
// ========================
UMaterialInterface* AWFCGridGenerator::GetOrCreateDefaultMaterial()
{
	if (DefaultMaterial)
	{
		return DefaultMaterial;
	}

	// 尝试加载引擎内置的 Unlit 纹理材质
	// UE5 的 /Engine/EngineMaterials/DefaultMaterial 不支持纹理参数
	// 我们创建一个简单的内置材质引用
	DefaultMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineMaterials/DefaultTexturedMaterial.DefaultTexturedMaterial"));

	if (!DefaultMaterial)
	{
		// 回退：使用最基础的默认材质
		DefaultMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
		UE_LOG(LogWFCGenerator, Warning,
			TEXT("Could not load DefaultTexturedMaterial, using DefaultMaterial. "
				 "Texture parameter 'BaseTexture' may not work. Consider assigning a custom BaseMaterial."));
	}

	return DefaultMaterial;
}
