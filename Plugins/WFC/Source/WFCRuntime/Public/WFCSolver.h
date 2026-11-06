// ========================
// WFCSolver.h (WFCRuntime 模块)
// WFC 核心求解器 - Wave Function Collapse 算法实现
// ========================
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WFCTileData.h"
#include "WFCSolver.generated.h"

// 单个格子的状态
struct FWFCCell
{
	TArray<bool> Candidates;  // 候选集，Candidates[i] == true 表示 Tile i 仍是候选
	int32 NumCandidates = 0;  // 剩余候选数量
	bool bCollapsed = false;  // 是否已坍缩

	void Init(int32 NumTiles)
	{
		Candidates.SetNumUninitialized(NumTiles);
		for (int32 i = 0; i < NumTiles; ++i)
		{
			Candidates[i] = true;
		}
		NumCandidates = NumTiles;
		bCollapsed = false;
	}

	void RemoveCandidate(int32 TileIndex)
	{
		if (Candidates[TileIndex])
		{
			Candidates[TileIndex] = false;
			--NumCandidates;
		}
	}

	void CollapseToTile(int32 TileIndex)
	{
		for (int32 i = 0; i < Candidates.Num(); ++i)
		{
			Candidates[i] = (i == TileIndex);
		}
		NumCandidates = 1;
		bCollapsed = true;
	}

	int32 GetCollapsedTile() const
	{
		for (int32 i = 0; i < Candidates.Num(); ++i)
		{
			if (Candidates[i]) return i;
		}
		return INDEX_NONE;
	}
};

UCLASS(BlueprintType)
class WFCRUNTIME_API UWFCSolver : public UObject
{
	GENERATED_BODY()

public:
	// 初始化求解器
	UFUNCTION(BlueprintCallable, Category = "WFC")
	bool Initialize(UWFCTileSet* InTileSet, int32 InGridWidth, int32 InGridHeight, int32 InSeed = 0);

	// 执行求解
	UFUNCTION(BlueprintCallable, Category = "WFC")
	bool Solve(int32 MaxRetries = 10);

	// 获取结果（行优先的 Tile 索引数组）
	UFUNCTION(BlueprintPure, Category = "WFC")
	TArray<int32> GetResult() const;

	// ========================
	// 交互模式 API
	// ========================

	// 初始化交互模式：所有格子候选全开
	UFUNCTION(BlueprintCallable, Category = "WFC|Interactive")
	bool InitializeInteractive(UWFCTileSet* InTileSet, int32 InGridWidth, int32 InGridHeight, int32 InSeed = 0);

	// 用户点击坍缩指定格子，返回 true 表示成功（无矛盾）
	UFUNCTION(BlueprintCallable, Category = "WFC|Interactive")
	bool CollapseAt(int32 X, int32 Y);

	// 查询某格子的候选数量（0=矛盾, 1=已确定, >1=未确定）
	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	int32 GetCellCandidateCount(int32 X, int32 Y) const;

	// 查询某格子是否已坍缩
	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	bool IsCellCollapsed(int32 X, int32 Y) const;

	// 获取某格子坍缩后的 Tile 索引（未坍缩返回 INDEX_NONE）
	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	int32 GetCellCollapsedTile(int32 X, int32 Y) const;

	// 检查全图是否全部已确定
	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	bool IsFullyCollapsed() const;

	// 获取网格尺寸
	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	int32 GetGridWidth() const { return GridWidth; }

	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	int32 GetGridHeight() const { return GridHeight; }

	// 获取全图所有格子的候选数量数组（用于批量刷新可视化）
	UFUNCTION(BlueprintPure, Category = "WFC|Interactive")
	TArray<int32> GetAllCandidateCounts() const;

private:
	// 核心算法步骤
	float CalculateEntropy(int32 CellIndex) const;
	int32 SelectMinEntropyCell();
	void CollapseCell(int32 CellIndex);
	bool Propagate(int32 StartCellIndex);

	// 辅助方法
	void ResetGrid();
	int32 CellIndex(int32 X, int32 Y) const { return Y * GridWidth + X; }
	int32 CellX(int32 Index) const { return Index % GridWidth; }
	int32 CellY(int32 Index) const { return Index / GridWidth; }
	bool IsValidCoord(int32 X, int32 Y) const { return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight; }
	void GetNeighbor(int32 X, int32 Y, EWFCDirection Dir, int32& OutNX, int32& OutNY) const;

	// 数据
	UPROPERTY()
	TObjectPtr<UWFCTileSet> TileSet;

	TArray<FWFCCell> Grid;
	TArray<int32> ValidTileIndices;  // TileSet 中非 null 的 Tile 索引
	int32 GridWidth = 0;
	int32 GridHeight = 0;
	int32 NumTiles = 0;  // TileSet->Tiles.Num()
	FRandomStream RandomStream;
	bool bSolved = false;
};
