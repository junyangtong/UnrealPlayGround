// ========================
// WFCGridGenerator.h (WFCRuntime 模块)
// 关卡内 WFC 可视化生成器 Actor
// ========================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WFCGridGenerator.generated.h"

class UWFCTileSet;
class UWFCSolver;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UTextRenderComponent;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "WFC Grid Generator"))
class WFCRUNTIME_API AWFCGridGenerator : public AActor
{
	GENERATED_BODY()

public:
	AWFCGridGenerator();

	// === 可编辑属性 ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config")
	TObjectPtr<UWFCTileSet> TileSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config", meta = (ClampMin = "1", ClampMax = "64"))
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config", meta = (ClampMin = "1", ClampMax = "64"))
	int32 GridHeight = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config", meta = (ClampMin = "1.0"))
	float TileSize = 100.0f;

	// 随机种子，0 = 随机
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config", meta = (ClampMin = "0", ClampMax = "100"))
	int32 MaxRetries = 10;

	// 可选基础材质（需包含名为 "BaseTexture" 的纹理参数），为空时使用默认 Unlit 材质
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Config")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	// 网格线颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|GridLines")
	FLinearColor GridLineColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

	// 网格线宽度（世界单位）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|GridLines", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float GridLineWidth = 2.0f;

	// 是否显示网格线
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|GridLines")
	bool bShowGridLines = true;

	// 六边形网格半径（环数），0 = 只有中心一格
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|HexGrid", meta = (ClampMin = "0", ClampMax = "20"))
	int32 HexGridRadius = 4;

	// === 蓝图接口 ===

	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void Generate();

	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void ClearGrid();

	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void Regenerate();

	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void DrawGridLines();

	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void DrawHexGridLines();

	// ========================
	// 交互模式
	// ========================

	// 初始化交互模式：生成空白网格，等待用户点击
	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void InitInteractiveMode();

	// 处理用户点击格子 (由鼠标点击检测调用)
	UFUNCTION(BlueprintCallable, Category = "WFC")
	bool InteractiveCollapseAt(int32 X, int32 Y);

	// 重置交互模式
	UFUNCTION(BlueprintCallable, Category = "WFC", CallInEditor)
	void ResetInteractiveMode();

	// 是否处于交互模式
	UFUNCTION(BlueprintPure, Category = "WFC")
	bool IsInInteractiveMode() const { return bInteractiveMode; }

	// 是否全部完成
	UFUNCTION(BlueprintPure, Category = "WFC")
	bool IsInteractiveComplete() const;

	// 未确定格子的颜色（会根据候选数渐变）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Interactive")
	FLinearColor UncollapsedColorLow = FLinearColor(0.2f, 0.6f, 1.0f, 0.5f);  // 候选少（即将确定）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Interactive")
	FLinearColor UncollapsedColorHigh = FLinearColor(0.1f, 0.1f, 0.3f, 0.3f);  // 候选多（完全不确定）

	// 矛盾格子颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Interactive")
	FLinearColor ContradictionColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.8f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// 从鼠标位置计算格子坐标
	bool GetGridCoordsFromMouse(int32& OutX, int32& OutY) const;

	// 刷新所有格子的可视化
	void RefreshInteractiveVisuals();

	// 刷新单个格子的可视化
	void RefreshCellVisual(int32 X, int32 Y);

	// 获取格子颜色（根据候选数插值）
	FLinearColor GetCellColor(int32 CandidateCount, int32 MaxCandidates) const;

	void BuildVisualGrid(const TArray<int32>& Result);
	UMaterialInterface* GetOrCreateDefaultMaterial();

	// 根组件
	UPROPERTY(VisibleAnywhere, Category = "WFC")
	TObjectPtr<USceneComponent> GridRoot;

	// 运行时创建的组件
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TileMeshComponents;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TileMaterials;

	// 网格线 Mesh
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> GridLineMesh;

	// 缓存的默认材质和 Plane Mesh
	UPROPERTY()
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;

	// ========================
	// 交互模式数据
	// ========================
	UPROPERTY()
	TObjectPtr<UWFCSolver> InteractiveSolver;

	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> CandidateCountLabels;

	bool bInteractiveMode = false;
	int32 MaxCandidateCount = 1;  // 初始时的最大候选数
};
