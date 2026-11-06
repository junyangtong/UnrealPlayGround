// 基于透明材质的彩窗投射器 性能：O(n)
// 管理窗户数据（位置、朝向、纹理、投影矩阵）
// 构建投影截锥体网格

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "StainedGlassProjector.generated.h"

UENUM(BlueprintType)
enum class EStainedGlassWindowFrameSource : uint8
{
	Actor,
	GlassMeshComponent,
	CustomComponent,
};

UCLASS()
class STAINEDGLASS_API AStainedGlassProjector : public AActor
{
	GENERATED_BODY()

public:
	AStainedGlassProjector();

	// ──────── Window 配置 ────────

	// 玻璃 Mesh 组件（从这个组件读取实际朝向）
	// 在 BP 里把你的玻璃 StaticMesh 拖到这里
	UPROPERTY(EditAnywhere, Category = "Window")
	TObjectPtr<UStaticMeshComponent> GlassMeshComponent = nullptr;

	// Projection frame is separate from the displayed glass mesh import orientation.
	UPROPERTY(EditAnywhere, Category = "Window")
	EStainedGlassWindowFrameSource WindowFrameSource = EStainedGlassWindowFrameSource::Actor;

	UPROPERTY(EditAnywhere, Category = "Window", meta = (EditCondition = "WindowFrameSource == EStainedGlassWindowFrameSource::CustomComponent"))
	TObjectPtr<USceneComponent> WindowFrameComponent = nullptr;

	// 玻璃基础尺寸（单位：cm）。实际尺寸会乘以 GlassMeshComponent 的 Scale
	// 如果不设置 GlassMeshComponent，则使用 Actor Scale
	UPROPERTY(EditAnywhere, Category = "Window")
	FVector2D BaseWindowSize = FVector2D(100.f, 100.f);

	// 玻璃 Mesh 的本地空间中，哪个轴是法线方向
	// 大多数平面 Mesh 的法线是 +Z 或 +X，根据你的 Mesh 设置
	UPROPERTY(EditAnywhere, Category = "Window")
	TEnumAsByte<EAxis::Type> MeshNormalAxis = EAxis::X;

	UPROPERTY(EditAnywhere, Category = "Window")
	UTexture2D* GlassTexture = nullptr;

	UPROPERTY(EditAnywhere, Category = "Window")
	FVector4 AtlasScaleOffset = FVector4(1, 1, 0, 0);

	// ──────── Projection 配置 ────────

	UPROPERTY(EditAnywhere, Category = "Projection")
	float ProjectionDistance = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Projection")
	float Intensity = 1.0f;

	// 关联的方向光（拖进来就行）
	UPROPERTY(EditAnywhere, Category = "Projection")
	AActor* SunLight = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projection")
	UMaterialInterface* ProjMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Volume")
	UMaterialInterface* VolumeMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Occlusion")
	bool bEnableOcclusionCapture = true;

	UPROPERTY(EditAnywhere, Category = "Occlusion", meta = (ClampMin = "128", ClampMax = "4096"))
	int32 ShadowMapResolution = 1024;

	UPROPERTY(EditAnywhere, Category = "Occlusion", meta = (ClampMin = "1.0"))
	float ShadowCapturePadding = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Occlusion", meta = (ClampMin = "0.0"))
	float ShadowDepthBias = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Occlusion", meta = (ClampMin = "0.0"))
	float ShadowStartOffset = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShadowStrength = 1.0f;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	// Generated projection mesh.
	UPROPERTY() UProceduralMeshComponent* ProjMesh = nullptr;
	UPROPERTY() UProceduralMeshComponent* VolumeMesh = nullptr;
	UPROPERTY() USceneCaptureComponent2D* ShadowCapture = nullptr;
	UPROPERTY() UTextureRenderTarget2D* ShadowDepthTarget = nullptr;
	UPROPERTY() UMaterialInstanceDynamic* DynamicProjMaterial = nullptr;
	UPROPERTY() UMaterialInstanceDynamic* DynamicVolumeMaterial = nullptr;

	// 灯光方向
	FVector SunDir;
	FTransform LastWindowTransform = FTransform::Identity;
	FTransform LastActorTransform = FTransform::Identity;
	FVector2D LastEffectiveWindowSize = FVector2D::ZeroVector;
	FVector LastSunDir = FVector::ZeroVector;
	float LastProjectionDistance = 0.0f;
	EAxis::Type LastMeshNormalAxis = EAxis::None;
	bool bLastEnableOcclusionCapture = false;
	bool bHasProjectionCache = false;

	USceneComponent* ResolveWindowFrameComponent() const;
	UStaticMeshComponent* ResolveGlassMeshComponent() const;
	FVector2D GetScaledBaseWindowSize(const FVector& Scale) const;

	// 获取窗户的世界 Transform（从 GlassMeshComponent 或 Actor 自身）
	FTransform GetWindowWorldTransform() const;
	// 根据 Transform 和 BaseWindowSize 计算实际窗户尺寸
	FVector2D GetEffectiveWindowSize() const;
	// 获取窗户的三个轴向（考虑 MeshNormalAxis 配置）
	void GetWindowAxes(FVector& OutNormal, FVector& OutRight, FVector& OutUp) const;
	void UpdateSunDirection();
	bool ShouldRebuildProjectionMesh(
		const FTransform& WindowTransform,
		const FTransform& ActorTransform,
		const FVector2D& EffectiveSize) const;
	void CacheProjectionState(
		const FTransform& WindowTransform,
		const FTransform& ActorTransform,
		const FVector2D& EffectiveSize);
	void UpdateProjection(bool bRebuildMesh);
	void EnsureShadowRenderTarget();
	void UpdateShadowCapture(const FVector& WinCenter, const FVector& WinRight, const FVector& WinUp, const FVector2D& EffectiveSize);
	void UpdateMaterialParameters(
		const FVector& WinCenter,
		const FVector& WinNormal,
		const FVector& WinRight,
		const FVector& WinUp,
		const FVector2D& EffectiveSize);
	void SetProjectorCustomPrimitiveDataFloat(int32 Index, float Value);
	void ApplyProjectorMaterialParameters(
		UMaterialInstanceDynamic* Material,
		const FVector& CaptureOrigin,
		const FVector& LightRight,
		const FVector& LightUp,
		const FVector& LightDir,
		float OrthoWidth,
		float InvOrthoWidth,
		float OcclusionEnabled);
};
