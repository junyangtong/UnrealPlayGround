#include "StainedGlassProjector.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AStainedGlassProjector::AStainedGlassProjector()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ShadowCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("StainedGlassShadowCapture"));
	ShadowCapture->SetupAttachment(RootComponent);
	ShadowCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	ShadowCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	ShadowCapture->bCaptureEveryFrame = true;
	ShadowCapture->bCaptureOnMovement = true;
}

void AStainedGlassProjector::BeginPlay()
{
	Super::BeginPlay();

	SunDir = FVector::ZeroVector;

	ProjMesh = NewObject<UProceduralMeshComponent>(this, TEXT("StainedGlassProjectionMesh"));
	ProjMesh->SetupAttachment(RootComponent);
	ProjMesh->RegisterComponent();
	ProjMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjMesh->SetCastShadow(false);

	VolumeMesh = NewObject<UProceduralMeshComponent>(this, TEXT("StainedGlassVolumeMesh"));
	VolumeMesh->SetupAttachment(RootComponent);
	VolumeMesh->RegisterComponent();
	VolumeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VolumeMesh->SetCastShadow(false);
	VolumeMesh->SetTranslucentSortPriority(1);
	VolumeMesh->SetVisibility(VolumeMaterial != nullptr);

	if (ProjMaterial)
	{
		DynamicProjMaterial = UMaterialInstanceDynamic::Create(ProjMaterial, this);
		ProjMesh->SetMaterial(0, DynamicProjMaterial);
	}

	if (VolumeMaterial)
	{
		DynamicVolumeMaterial = UMaterialInstanceDynamic::Create(VolumeMaterial, this);
		VolumeMesh->SetMaterial(0, DynamicVolumeMaterial);
		VolumeMesh->SetVisibility(true);
	}

	EnsureShadowRenderTarget();
	UpdateSunDirection();
	UpdateProjection(true);
	CacheProjectionState(GetWindowWorldTransform(), GetActorTransform(), GetEffectiveWindowSize());
}

void AStainedGlassProjector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSunDirection();

	const FTransform WindowTransform = GetWindowWorldTransform();
	const FTransform ActorTransform = GetActorTransform();
	const FVector2D EffectiveSize = GetEffectiveWindowSize();
	const bool bRebuildMesh = ShouldRebuildProjectionMesh(WindowTransform, ActorTransform, EffectiveSize);
	UpdateProjection(bRebuildMesh);
	if (bRebuildMesh)
	{
		CacheProjectionState(WindowTransform, ActorTransform, EffectiveSize);
	}
}

UStaticMeshComponent* AStainedGlassProjector::ResolveGlassMeshComponent() const
{
	if (GlassMeshComponent)
	{
		return GlassMeshComponent.Get();
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent && MeshComponent->GetStaticMesh())
		{
			return MeshComponent;
		}
	}

	return nullptr;
}

USceneComponent* AStainedGlassProjector::ResolveWindowFrameComponent() const
{
	switch (WindowFrameSource)
	{
	case EStainedGlassWindowFrameSource::GlassMeshComponent:
		return ResolveGlassMeshComponent();
	case EStainedGlassWindowFrameSource::CustomComponent:
		return WindowFrameComponent.Get();
	case EStainedGlassWindowFrameSource::Actor:
	default:
		return GetRootComponent();
	}
}

FTransform AStainedGlassProjector::GetWindowWorldTransform() const
{
	if (const USceneComponent* FrameComponent = ResolveWindowFrameComponent())
	{
		return FrameComponent->GetComponentTransform();
	}

	return GetActorTransform();
}

FVector2D AStainedGlassProjector::GetScaledBaseWindowSize(const FVector& Scale) const
{
	const FVector AbsScale(FMath::Abs(Scale.X), FMath::Abs(Scale.Y), FMath::Abs(Scale.Z));

	switch (MeshNormalAxis)
	{
	case EAxis::X:
		return FVector2D(BaseWindowSize.X * AbsScale.Y, BaseWindowSize.Y * AbsScale.Z);
	case EAxis::Y:
		return FVector2D(BaseWindowSize.X * AbsScale.X, BaseWindowSize.Y * AbsScale.Z);
	case EAxis::Z:
		return FVector2D(BaseWindowSize.X * AbsScale.X, BaseWindowSize.Y * AbsScale.Y);
	default:
		return FVector2D(BaseWindowSize.X * AbsScale.Y, BaseWindowSize.Y * AbsScale.Z);
	}
}

FVector2D AStainedGlassProjector::GetEffectiveWindowSize() const
{
	const FVector Scale = GetWindowWorldTransform().GetScale3D();
	return GetScaledBaseWindowSize(Scale);
}

void AStainedGlassProjector::GetWindowAxes(FVector& OutNormal, FVector& OutRight, FVector& OutUp) const
{
	const FTransform WindowTransform = GetWindowWorldTransform();
	const FQuat Rotation = WindowTransform.GetRotation();

	const FVector CompForward = Rotation.GetForwardVector();
	const FVector CompRight = Rotation.GetRightVector();
	const FVector CompUp = Rotation.GetUpVector();

	switch (MeshNormalAxis)
	{
	case EAxis::X:
		OutNormal = CompForward;
		OutRight = CompRight;
		OutUp = CompUp;
		break;
	case EAxis::Y:
		OutNormal = CompRight;
		OutRight = CompForward;
		OutUp = CompUp;
		break;
	case EAxis::Z:
		OutNormal = CompUp;
		OutRight = CompForward;
		OutUp = CompRight;
		break;
	default:
		OutNormal = CompForward;
		OutRight = CompRight;
		OutUp = CompUp;
		break;
	}
}

void AStainedGlassProjector::UpdateSunDirection()
{
	if (SunLight)
	{
		SunDir = SunLight->GetActorForwardVector();
	}
}

bool AStainedGlassProjector::ShouldRebuildProjectionMesh(
	const FTransform& WindowTransform,
	const FTransform& ActorTransform,
	const FVector2D& EffectiveSize) const
{
	constexpr float TransformTolerance = 0.01f;
	constexpr float VectorTolerance = 0.001f;

	return !bHasProjectionCache
		|| !LastWindowTransform.Equals(WindowTransform, TransformTolerance)
		|| !LastActorTransform.Equals(ActorTransform, TransformTolerance)
		|| !LastEffectiveWindowSize.Equals(EffectiveSize, TransformTolerance)
		|| !LastSunDir.Equals(SunDir, VectorTolerance)
		|| !FMath::IsNearlyEqual(LastProjectionDistance, ProjectionDistance, VectorTolerance)
		|| LastMeshNormalAxis != MeshNormalAxis.GetValue()
		|| bLastEnableOcclusionCapture != bEnableOcclusionCapture;
}

void AStainedGlassProjector::CacheProjectionState(
	const FTransform& WindowTransform,
	const FTransform& ActorTransform,
	const FVector2D& EffectiveSize)
{
	LastWindowTransform = WindowTransform;
	LastActorTransform = ActorTransform;
	LastEffectiveWindowSize = EffectiveSize;
	LastSunDir = SunDir;
	LastProjectionDistance = ProjectionDistance;
	LastMeshNormalAxis = MeshNormalAxis.GetValue();
	bLastEnableOcclusionCapture = bEnableOcclusionCapture;
	bHasProjectionCache = true;
}

static FMatrix CalcWindowProjectionMatrix(
	const FVector& WindowCenter,
	const FVector& WindowNormal,
	const FVector& WindowRight,
	const FVector& WindowUp,
	const FVector2D& InWindowSize,
	float InProjectionDistance,
	const FVector& LightDirection)
{
	const FMatrix WorldToWindow = FMatrix(
		FPlane(WindowRight.X, WindowUp.X, WindowNormal.X, 0.0f),
		FPlane(WindowRight.Y, WindowUp.Y, WindowNormal.Y, 0.0f),
		FPlane(WindowRight.Z, WindowUp.Z, WindowNormal.Z, 0.0f),
		FPlane(
			-FVector::DotProduct(WindowRight, WindowCenter),
			-FVector::DotProduct(WindowUp, WindowCenter),
			-FVector::DotProduct(WindowNormal, WindowCenter),
			1.0f));

	const FVector LocalLightDir = FVector(
		FVector::DotProduct(LightDirection, WindowRight),
		FVector::DotProduct(LightDirection, WindowUp),
		FVector::DotProduct(LightDirection, WindowNormal));

	FMatrix ObliqueProj = FMatrix::Identity;
	if (FMath::Abs(LocalLightDir.Z) > KINDA_SMALL_NUMBER)
	{
		const float RatioX = LocalLightDir.X / LocalLightDir.Z;
		const float RatioY = LocalLightDir.Y / LocalLightDir.Z;

		ObliqueProj = FMatrix(
			FPlane(1.0f, 0.0f, 0.0f, 0.0f),
			FPlane(0.0f, 1.0f, 0.0f, 0.0f),
			FPlane(-RatioX, -RatioY, 0.0f, 0.0f),
			FPlane(0.0f, 0.0f, 0.0f, 1.0f));
	}

	const FMatrix ScaleToNDC = FMatrix(
		FPlane(1.0f / InWindowSize.X, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, 1.0f / InWindowSize.Y, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 1.0f / InProjectionDistance, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f));

	return WorldToWindow * ObliqueProj * ScaleToNDC;
}

static void BuildProjectionFrustumMesh(
	const FVector& Center,
	const FVector& Normal,
	const FVector& Right,
	const FVector& Up,
	const FVector2D& InWindowSize,
	float InProjectionDistance,
	const FVector& LightDirection,
	TArray<FVector>& OutVertices,
	TArray<int32>& OutIndices)
{
	const float HalfW = InWindowSize.X * 0.5f;
	const float HalfH = InWindowSize.Y * 0.5f;

	const FVector Corners[4] =
	{
		Center - Right * HalfW - Up * HalfH,
		Center + Right * HalfW - Up * HalfH,
		Center + Right * HalfW + Up * HalfH,
		Center - Right * HalfW + Up * HalfH,
	};

	const FVector Extrude = LightDirection * InProjectionDistance;

	OutVertices.SetNum(8);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		OutVertices[Index] = Corners[Index] - Normal * 5.0f;
		OutVertices[Index + 4] = Corners[Index] + Extrude;
	}

	static const int32 BoxIndices[36] =
	{
		0, 2, 1, 0, 3, 2,
		4, 5, 6, 4, 6, 7,
		0, 1, 5, 0, 5, 4,
		1, 2, 6, 1, 6, 5,
		2, 3, 7, 2, 7, 6,
		3, 0, 4, 3, 4, 7,
	};

	OutIndices.Append(BoxIndices, UE_ARRAY_COUNT(BoxIndices));
}

static void BuildLightSpaceBasis(const FVector& LightDirection, FVector& OutRight, FVector& OutUp)
{
	const FVector LightDir = LightDirection.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	const FVector UpSeed = FMath::Abs(FVector::DotProduct(LightDir, FVector::UpVector)) < 0.95f
		? FVector::UpVector
		: FVector::RightVector;

	OutRight = FVector::CrossProduct(UpSeed, LightDir).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::RightVector);
	OutUp = FVector::CrossProduct(LightDir, OutRight).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
}

static float CalcShadowOrthoWidth(
	const FVector& Center,
	const FVector& WindowRight,
	const FVector& WindowUp,
	const FVector& LightRight,
	const FVector& LightUp,
	const FVector2D& InWindowSize,
	float Padding)
{
	const float HalfW = InWindowSize.X * 0.5f;
	const float HalfH = InWindowSize.Y * 0.5f;
	const FVector Corners[4] =
	{
		Center - WindowRight * HalfW - WindowUp * HalfH,
		Center + WindowRight * HalfW - WindowUp * HalfH,
		Center + WindowRight * HalfW + WindowUp * HalfH,
		Center - WindowRight * HalfW + WindowUp * HalfH,
	};

	float MaxExtent = 1.0f;
	for (const FVector& Corner : Corners)
	{
		const FVector Relative = Corner - Center;
		MaxExtent = FMath::Max(MaxExtent, FMath::Abs(FVector::DotProduct(Relative, LightRight)));
		MaxExtent = FMath::Max(MaxExtent, FMath::Abs(FVector::DotProduct(Relative, LightUp)));
	}

	return MaxExtent * 2.0f * FMath::Max(Padding, 1.0f);
}

void AStainedGlassProjector::EnsureShadowRenderTarget()
{
	if (!bEnableOcclusionCapture)
	{
		return;
	}

	const int32 ClampedResolution = FMath::Clamp(ShadowMapResolution, 128, 4096);
	const bool bNeedsNewTarget =
		!ShadowDepthTarget ||
		ShadowDepthTarget->SizeX != ClampedResolution ||
		ShadowDepthTarget->SizeY != ClampedResolution;

	if (bNeedsNewTarget)
	{
		ShadowDepthTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("StainedGlassShadowDepthTarget"));
		ShadowDepthTarget->RenderTargetFormat = RTF_R32f;
		ShadowDepthTarget->ClearColor = FLinearColor::White;
		ShadowDepthTarget->bAutoGenerateMips = false;
		ShadowDepthTarget->InitAutoFormat(ClampedResolution, ClampedResolution);
		ShadowDepthTarget->UpdateResourceImmediate(true);
	}

	if (ShadowCapture)
	{
		ShadowCapture->TextureTarget = ShadowDepthTarget;
	}
}

void AStainedGlassProjector::UpdateShadowCapture(
	const FVector& WinCenter,
	const FVector& WinRight,
	const FVector& WinUp,
	const FVector2D& EffectiveSize)
{
	if (!ShadowCapture)
	{
		return;
	}

	ShadowCapture->SetComponentTickEnabled(bEnableOcclusionCapture);
	ShadowCapture->bCaptureEveryFrame = bEnableOcclusionCapture;
	ShadowCapture->bCaptureOnMovement = bEnableOcclusionCapture;

	if (!bEnableOcclusionCapture || SunDir.IsNearlyZero())
	{
		return;
	}

	EnsureShadowRenderTarget();
	if (!ShadowDepthTarget)
	{
		return;
	}

	FVector LightRight;
	FVector LightUp;
	BuildLightSpaceBasis(SunDir, LightRight, LightUp);

	const FVector LightDir = SunDir.GetSafeNormal();
	const FVector CaptureOrigin = WinCenter - LightDir * ShadowStartOffset;
	const float OrthoWidth = CalcShadowOrthoWidth(
		WinCenter, WinRight, WinUp, LightRight, LightUp, EffectiveSize, ShadowCapturePadding);

	ShadowCapture->SetWorldLocation(CaptureOrigin);
	ShadowCapture->SetWorldRotation(FRotationMatrix::MakeFromXY(LightDir, LightRight).Rotator());
	ShadowCapture->OrthoWidth = OrthoWidth;
	ShadowCapture->MaxViewDistanceOverride = ProjectionDistance + ShadowStartOffset + ShadowDepthBias * 2.0f;

	ShadowCapture->HiddenActors.Reset();
	ShadowCapture->HiddenActors.Add(this);
	if (GlassMeshComponent && GlassMeshComponent->GetOwner())
	{
		ShadowCapture->HiddenActors.AddUnique(GlassMeshComponent->GetOwner());
	}

}

void AStainedGlassProjector::SetProjectorCustomPrimitiveDataFloat(int32 Index, float Value)
{
	if (ProjMesh)
	{
		ProjMesh->SetCustomPrimitiveDataFloat(Index, Value);
	}

	if (VolumeMesh)
	{
		VolumeMesh->SetCustomPrimitiveDataFloat(Index, Value);
	}
}

void AStainedGlassProjector::ApplyProjectorMaterialParameters(
	UMaterialInstanceDynamic* Material,
	const FVector& CaptureOrigin,
	const FVector& LightRight,
	const FVector& LightUp,
	const FVector& LightDir,
	float OrthoWidth,
	float InvOrthoWidth,
	float OcclusionEnabled)
{
	if (!Material)
	{
		return;
	}

	if (GlassTexture)
	{
		Material->SetTextureParameterValue(TEXT("GlassTexture"), GlassTexture);
	}
	if (ShadowDepthTarget)
	{
		Material->SetTextureParameterValue(TEXT("SG_ShadowDepthTexture"), ShadowDepthTarget);
	}

	Material->SetVectorParameterValue(
		TEXT("AtlasScaleOffset"),
		FLinearColor(AtlasScaleOffset.X, AtlasScaleOffset.Y, AtlasScaleOffset.Z, AtlasScaleOffset.W));
	Material->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	Material->SetScalarParameterValue(TEXT("SG_ProjectionDistance"), ProjectionDistance);
	Material->SetVectorParameterValue(TEXT("SG_ShadowOrigin"), FLinearColor(CaptureOrigin.X, CaptureOrigin.Y, CaptureOrigin.Z, 0.0f));
	Material->SetVectorParameterValue(TEXT("SG_LightRight"), FLinearColor(LightRight.X, LightRight.Y, LightRight.Z, 0.0f));
	Material->SetVectorParameterValue(TEXT("SG_LightUp"), FLinearColor(LightUp.X, LightUp.Y, LightUp.Z, 0.0f));
	Material->SetVectorParameterValue(TEXT("SG_SunDirection"), FLinearColor(LightDir.X, LightDir.Y, LightDir.Z, 0.0f));
	Material->SetScalarParameterValue(TEXT("SG_ShadowOrthoWidth"), OrthoWidth);
	Material->SetScalarParameterValue(TEXT("SG_ShadowInvOrthoWidth"), InvOrthoWidth);
	Material->SetScalarParameterValue(TEXT("SG_ShadowDepthBias"), ShadowDepthBias);
	Material->SetScalarParameterValue(TEXT("SG_ShadowStrength"), ShadowStrength);
	Material->SetScalarParameterValue(TEXT("SG_OcclusionEnabled"), OcclusionEnabled);
}

void AStainedGlassProjector::UpdateMaterialParameters(
	const FVector& WinCenter,
	const FVector& WinNormal,
	const FVector& WinRight,
	const FVector& WinUp,
	const FVector2D& EffectiveSize)
{
	if (!ProjMesh && !VolumeMesh)
	{
		return;
	}

	if (ProjMaterial && ProjMesh && !DynamicProjMaterial)
	{
		DynamicProjMaterial = UMaterialInstanceDynamic::Create(ProjMaterial, this);
		ProjMesh->SetMaterial(0, DynamicProjMaterial);
	}

	if (VolumeMaterial && VolumeMesh && !DynamicVolumeMaterial)
	{
		DynamicVolumeMaterial = UMaterialInstanceDynamic::Create(VolumeMaterial, this);
		VolumeMesh->SetMaterial(0, DynamicVolumeMaterial);
		VolumeMesh->SetVisibility(true);
	}
	else if (VolumeMesh && !VolumeMaterial)
	{
		VolumeMesh->SetVisibility(false);
	}

	FVector LightRight;
	FVector LightUp;
	BuildLightSpaceBasis(SunDir, LightRight, LightUp);

	const FVector LightDir = SunDir.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	const FVector CaptureOrigin = WinCenter - LightDir * ShadowStartOffset;
	const float OrthoWidth = CalcShadowOrthoWidth(
		WinCenter, WinRight, WinUp, LightRight, LightUp, EffectiveSize, ShadowCapturePadding);
	const float InvOrthoWidth = OrthoWidth > KINDA_SMALL_NUMBER ? 1.0f / OrthoWidth : 0.0f;
	const float OcclusionEnabled = (bEnableOcclusionCapture && ShadowDepthTarget) ? 1.0f : 0.0f;
	FVector BeamWindowNormal = WinNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	if (FVector::DotProduct(BeamWindowNormal, LightDir) < 0.0f)
	{
		BeamWindowNormal *= -1.0f;
	}
	const float WindowPlaneOffset = FVector::DotProduct(WinCenter, BeamWindowNormal);

	SetProjectorCustomPrimitiveDataFloat(16, CaptureOrigin.X);
	SetProjectorCustomPrimitiveDataFloat(17, CaptureOrigin.Y);
	SetProjectorCustomPrimitiveDataFloat(18, CaptureOrigin.Z);
	SetProjectorCustomPrimitiveDataFloat(19, InvOrthoWidth);
	SetProjectorCustomPrimitiveDataFloat(20, LightRight.X);
	SetProjectorCustomPrimitiveDataFloat(21, LightRight.Y);
	SetProjectorCustomPrimitiveDataFloat(22, LightRight.Z);
	SetProjectorCustomPrimitiveDataFloat(23, ShadowDepthBias);
	SetProjectorCustomPrimitiveDataFloat(24, LightUp.X);
	SetProjectorCustomPrimitiveDataFloat(25, LightUp.Y);
	SetProjectorCustomPrimitiveDataFloat(26, LightUp.Z);
	SetProjectorCustomPrimitiveDataFloat(27, ShadowStrength);
	SetProjectorCustomPrimitiveDataFloat(28, LightDir.X);
	SetProjectorCustomPrimitiveDataFloat(29, LightDir.Y);
	SetProjectorCustomPrimitiveDataFloat(30, LightDir.Z);
	SetProjectorCustomPrimitiveDataFloat(31, OcclusionEnabled);
	SetProjectorCustomPrimitiveDataFloat(32, BeamWindowNormal.X);
	SetProjectorCustomPrimitiveDataFloat(33, BeamWindowNormal.Y);
	SetProjectorCustomPrimitiveDataFloat(34, BeamWindowNormal.Z);
	SetProjectorCustomPrimitiveDataFloat(35, WindowPlaneOffset);

	ApplyProjectorMaterialParameters(
		DynamicProjMaterial,
		CaptureOrigin,
		LightRight,
		LightUp,
		LightDir,
		OrthoWidth,
		InvOrthoWidth,
		OcclusionEnabled);

	ApplyProjectorMaterialParameters(
		DynamicVolumeMaterial,
		CaptureOrigin,
		LightRight,
		LightUp,
		LightDir,
		OrthoWidth,
		InvOrthoWidth,
		OcclusionEnabled);
}

void AStainedGlassProjector::UpdateProjection(bool bRebuildMesh)
{
	if (!ProjMesh && !VolumeMesh)
	{
		return;
	}

	FVector WinNormal;
	FVector WinRight;
	FVector WinUp;
	GetWindowAxes(WinNormal, WinRight, WinUp);

	const FVector WinCenter = GetWindowWorldTransform().GetLocation();
	const FVector2D EffectiveSize = GetEffectiveWindowSize();
	UpdateShadowCapture(WinCenter, WinRight, WinUp, EffectiveSize);

	const FMatrix ProjMatrix = CalcWindowProjectionMatrix(
		WinCenter, WinNormal, WinRight, WinUp,
		EffectiveSize, ProjectionDistance, SunDir);

	for (int32 Row = 0; Row < 4; ++Row)
	{
		for (int32 Col = 0; Col < 4; ++Col)
		{
			SetProjectorCustomPrimitiveDataFloat(Row * 4 + Col, ProjMatrix.M[Row][Col]);
		}
	}

	if (bRebuildMesh)
	{
		TArray<FVector> Verts;
		TArray<int32> Indices;
		BuildProjectionFrustumMesh(
			WinCenter, WinNormal, WinRight, WinUp,
			EffectiveSize, ProjectionDistance, SunDir,
			Verts, Indices);

		const FTransform WorldToActor = GetActorTransform().Inverse();
		for (FVector& Vertex : Verts)
		{
			Vertex = WorldToActor.TransformPosition(Vertex);
		}

		const int32 NumVerts = Verts.Num();
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FProcMeshTangent> Tangents;
		TArray<FColor> Colors;

		Normals.Init(FVector::UpVector, NumVerts);
		UVs.Init(FVector2D::ZeroVector, NumVerts);
		Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), NumVerts);
		Colors.Init(FColor::White, NumVerts);

		if (ProjMesh)
		{
			ProjMesh->CreateMeshSection(0, Verts, Indices, Normals, UVs, Colors, Tangents, false);
		}

		if (VolumeMesh)
		{
			VolumeMesh->CreateMeshSection(0, Verts, Indices, Normals, UVs, Colors, Tangents, false);
		}
	}

	UpdateMaterialParameters(WinCenter, WinNormal, WinRight, WinUp, EffectiveSize);
}
