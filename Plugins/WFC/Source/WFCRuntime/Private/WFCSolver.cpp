// ========================
// WFCSolver.cpp (WFCRuntime 模块)
// WFC 核心求解器实现
// ========================
#include "WFCSolver.h"

DEFINE_LOG_CATEGORY_STATIC(LogWFCSolver, Log, All);

// ========================
// 初始化
// ========================
bool UWFCSolver::Initialize(UWFCTileSet* InTileSet, int32 InGridWidth, int32 InGridHeight, int32 InSeed)
{
	bSolved = false;

	if (!InTileSet || InTileSet->Tiles.Num() == 0 || InGridWidth <= 0 || InGridHeight <= 0)
	{
		UE_LOG(LogWFCSolver, Error, TEXT("Initialize failed: invalid parameters (TileSet=%s, Width=%d, Height=%d)"),
			InTileSet ? *InTileSet->GetName() : TEXT("null"), InGridWidth, InGridHeight);
		return false;
	}

	TileSet = InTileSet;
	GridWidth = InGridWidth;
	GridHeight = InGridHeight;
	NumTiles = TileSet->Tiles.Num();

	// 收集有效（非 null）的 Tile 索引
	ValidTileIndices.Empty();
	for (int32 i = 0; i < NumTiles; ++i)
	{
		if (TileSet->Tiles[i] != nullptr)
		{
			ValidTileIndices.Add(i);
		}
	}

	if (ValidTileIndices.Num() == 0)
	{
		UE_LOG(LogWFCSolver, Error, TEXT("Initialize failed: TileSet has no valid (non-null) tiles"));
		return false;
	}

	// 设置随机流
	if (InSeed == 0)
	{
		RandomStream.GenerateNewSeed();
	}
	else
	{
		RandomStream.Initialize(InSeed);
	}

	// 初始化网格
	ResetGrid();

	UE_LOG(LogWFCSolver, Log, TEXT("Solver initialized: %dx%d grid, %d valid tiles, seed=%d"),
		GridWidth, GridHeight, ValidTileIndices.Num(), RandomStream.GetCurrentSeed());

	return true;
}

void UWFCSolver::ResetGrid()
{
	int32 TotalCells = GridWidth * GridHeight;
	Grid.SetNum(TotalCells);

	for (int32 i = 0; i < TotalCells; ++i)
	{
		Grid[i].Init(NumTiles);

		// 将无效 Tile（null）从候选集中移除
		for (int32 t = 0; t < NumTiles; ++t)
		{
			if (TileSet->Tiles[t] == nullptr)
			{
				Grid[i].RemoveCandidate(t);
			}
		}
	}
}

// ========================
// 熵计算
// ========================
float UWFCSolver::CalculateEntropy(int32 CellIndex) const
{
	const FWFCCell& Cell = Grid[CellIndex];

	if (Cell.NumCandidates <= 1)
	{
		return 0.0f;
	}

	// Shannon 熵: H = log(sum(w)) - sum(w * log(w)) / sum(w)
	float SumWeight = 0.0f;
	float SumWeightLogWeight = 0.0f;

	for (int32 t = 0; t < NumTiles; ++t)
	{
		if (Cell.Candidates[t])
		{
			float W = FMath::Max(TileSet->Tiles[t]->Weight, 0.01f);
			SumWeight += W;
			SumWeightLogWeight += W * FMath::Loge(W);
		}
	}

	if (SumWeight <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Loge(SumWeight) - SumWeightLogWeight / SumWeight;
}

// ========================
// 最小熵选择
// ========================
int32 UWFCSolver::SelectMinEntropyCell()
{
	float MinEntropy = TNumericLimits<float>::Max();
	int32 BestCell = INDEX_NONE;
	int32 TieCount = 0;

	for (int32 i = 0; i < Grid.Num(); ++i)
	{
		if (Grid[i].bCollapsed || Grid[i].NumCandidates <= 1)
		{
			continue;
		}

		float Entropy = CalculateEntropy(i);

		// 加一个小随机噪声打破平局
		Entropy += RandomStream.FRandRange(0.0f, 0.0001f);

		if (Entropy < MinEntropy)
		{
			MinEntropy = Entropy;
			BestCell = i;
		}
	}

	return BestCell;
}

// ========================
// 坍缩
// ========================
void UWFCSolver::CollapseCell(int32 CellIndex)
{
	FWFCCell& Cell = Grid[CellIndex];

	// 收集候选和对应权重
	TArray<int32> CandidateTiles;
	TArray<float> Weights;
	float TotalWeight = 0.0f;

	for (int32 t = 0; t < NumTiles; ++t)
	{
		if (Cell.Candidates[t])
		{
			CandidateTiles.Add(t);
			float W = FMath::Max(TileSet->Tiles[t]->Weight, 0.01f);
			Weights.Add(W);
			TotalWeight += W;
		}
	}

	if (CandidateTiles.Num() == 0)
	{
		return;  // 不应该到这里，调用者应该先检查
	}

	// 按权重随机选择
	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	float Cumulative = 0.0f;
	int32 ChosenTile = CandidateTiles.Last();  // 默认最后一个（防止浮点误差）

	for (int32 i = 0; i < CandidateTiles.Num(); ++i)
	{
		Cumulative += Weights[i];
		if (Roll <= Cumulative)
		{
			ChosenTile = CandidateTiles[i];
			break;
		}
	}

	Cell.CollapseToTile(ChosenTile);
}

// ========================
// 邻居坐标获取
// ========================
void UWFCSolver::GetNeighbor(int32 X, int32 Y, EWFCDirection Dir, int32& OutNX, int32& OutNY) const
{
	OutNX = X;
	OutNY = Y;

	switch (Dir)
	{
	case EWFCDirection::Up:    OutNY = Y - 1; break;
	case EWFCDirection::Down:  OutNY = Y + 1; break;
	case EWFCDirection::Left:  OutNX = X - 1; break;
	case EWFCDirection::Right: OutNX = X + 1; break;
	default: break;
	}
}

// ========================
// 约束传播
// ========================
bool UWFCSolver::Propagate(int32 StartCellIndex)
{
	// 使用队列式传播（AC-3 变体）
	TQueue<int32> Queue;
	TSet<int32> InQueue;

	Queue.Enqueue(StartCellIndex);
	InQueue.Add(StartCellIndex);

	while (!Queue.IsEmpty())
	{
		int32 Current;
		Queue.Dequeue(Current);
		InQueue.Remove(Current);

		int32 CX = CellX(Current);
		int32 CY = CellY(Current);

		// 对 4 个方向的邻居做约束检查
		for (int32 d = 0; d < 4; ++d)
		{
			EWFCDirection Dir = static_cast<EWFCDirection>(d);
			int32 NX, NY;
			GetNeighbor(CX, CY, Dir, NX, NY);

			if (!IsValidCoord(NX, NY))
			{
				continue;
			}

			int32 NeighborIdx = CellIndex(NX, NY);
			FWFCCell& NeighborCell = Grid[NeighborIdx];

			if (NeighborCell.bCollapsed)
			{
				continue;
			}

			EWFCDirection OppositeDir = WFCDirectionUtils::GetOpposite(Dir);
			bool bChanged = false;

			// 对邻居的每个候选 Tile，检查是否至少有一个当前格的候选 Tile 与之兼容
			for (int32 nt = 0; nt < NumTiles; ++nt)
			{
				if (!NeighborCell.Candidates[nt])
				{
					continue;
				}

				// 检查 nt 是否与当前格的任意候选兼容
				bool bCompatible = false;
				for (int32 ct = 0; ct < NumTiles; ++ct)
				{
					if (!Grid[Current].Candidates[ct])
					{
						continue;
					}

					// 当前格的 Tile ct 在 Dir 方向上，邻居的 Tile nt 在 OppositeDir 方向上
					// 需要检查: ct 的 Dir 边 与 nt 的 OppositeDir 边 是否兼容
					if (TileSet->Tiles[ct] && TileSet->Tiles[nt])
					{
						const FWFCEdgeSocket& SocketCT = TileSet->Tiles[ct]->GetEdge(Dir);
						const FWFCEdgeSocket& SocketNT = TileSet->Tiles[nt]->GetEdge(OppositeDir);
						if (SocketCT.CanConnect(SocketNT))
						{
							bCompatible = true;
							break;
						}
					}
				}

				if (!bCompatible)
				{
					NeighborCell.RemoveCandidate(nt);
					bChanged = true;
				}
			}

			// 矛盾检测
			if (NeighborCell.NumCandidates == 0)
			{
				UE_LOG(LogWFCSolver, Verbose, TEXT("Contradiction at cell (%d, %d)"), NX, NY);
				return false;  // 矛盾
			}

			// 如果邻居发生了变化，加入队列继续传播
			if (bChanged && !InQueue.Contains(NeighborIdx))
			{
				Queue.Enqueue(NeighborIdx);
				InQueue.Add(NeighborIdx);
			}
		}
	}

	return true;  // 传播成功，无矛盾
}

// ========================
// 求解主循环
// ========================
bool UWFCSolver::Solve(int32 MaxRetries)
{
	bSolved = false;

	if (!TileSet || GridWidth <= 0 || GridHeight <= 0)
	{
		UE_LOG(LogWFCSolver, Error, TEXT("Solve failed: solver not initialized"));
		return false;
	}

	for (int32 Attempt = 0; Attempt <= MaxRetries; ++Attempt)
	{
		if (Attempt > 0)
		{
			UE_LOG(LogWFCSolver, Log, TEXT("Retry attempt %d/%d"), Attempt, MaxRetries);
			ResetGrid();
		}

		bool bContradiction = false;
		int32 StepCount = 0;

		while (true)
		{
			// 选择熵最小的未坍缩格子
			int32 CellIdx = SelectMinEntropyCell();

			if (CellIdx == INDEX_NONE)
			{
				// 没有可选格子了——检查是否全部已坍缩
				bool bAllCollapsed = true;
				for (int32 i = 0; i < Grid.Num(); ++i)
				{
					if (!Grid[i].bCollapsed && Grid[i].NumCandidates > 1)
					{
						bAllCollapsed = false;
						break;
					}
				}

				if (bAllCollapsed)
				{
					// 将只有 1 个候选的格子标记为已坍缩
					for (int32 i = 0; i < Grid.Num(); ++i)
					{
						if (!Grid[i].bCollapsed && Grid[i].NumCandidates == 1)
						{
							Grid[i].bCollapsed = true;
						}
					}
					// 求解成功
					bSolved = true;
					UE_LOG(LogWFCSolver, Log, TEXT("WFC Solve succeeded in %d steps, %d retries"),
						StepCount, Attempt);
					return true;
				}

				// 有问题但 SelectMinEntropyCell 没找到
				bContradiction = true;
				break;
			}

			// 坍缩选定格子
			CollapseCell(CellIdx);
			++StepCount;

			// 约束传播
			if (!Propagate(CellIdx))
			{
				bContradiction = true;
				break;
			}
		}

		if (bContradiction)
		{
			UE_LOG(LogWFCSolver, Warning, TEXT("Contradiction encountered at step %d"), StepCount);
			continue;  // 重试
		}
	}

	UE_LOG(LogWFCSolver, Warning, TEXT("WFC Solve failed after %d retries"), MaxRetries);
	return false;
}

// ========================
// 获取结果
// ========================
TArray<int32> UWFCSolver::GetResult() const
{
	TArray<int32> Result;
	Result.SetNum(GridWidth * GridHeight);

	for (int32 i = 0; i < Grid.Num(); ++i)
	{
		Result[i] = Grid[i].GetCollapsedTile();
	}

	return Result;
}

// ========================
// 交互模式
// ========================

bool UWFCSolver::InitializeInteractive(UWFCTileSet* InTileSet, int32 InGridWidth, int32 InGridHeight, int32 InSeed)
{
	// 复用已有的初始化逻辑
	bool bResult = Initialize(InTileSet, InGridWidth, InGridHeight, InSeed);
	if (bResult)
	{
		UE_LOG(LogWFCSolver, Log, TEXT("Interactive mode initialized: %dx%d grid, %d valid tiles"),
			GridWidth, GridHeight, ValidTileIndices.Num());
	}
	return bResult;
}

bool UWFCSolver::CollapseAt(int32 X, int32 Y)
{
	if (!IsValidCoord(X, Y))
	{
		UE_LOG(LogWFCSolver, Warning, TEXT("CollapseAt: invalid coord (%d, %d)"), X, Y);
		return false;
	}

	int32 Idx = CellIndex(X, Y);
	FWFCCell& Cell = Grid[Idx];

	if (Cell.bCollapsed)
	{
		UE_LOG(LogWFCSolver, Warning, TEXT("CollapseAt: cell (%d, %d) already collapsed"), X, Y);
		return false;
	}

	if (Cell.NumCandidates == 0)
	{
		UE_LOG(LogWFCSolver, Warning, TEXT("CollapseAt: cell (%d, %d) has no candidates (contradiction)"), X, Y);
		return false;
	}

	// 坍缩该格子（随机选一个候选）
	CollapseCell(Idx);

	UE_LOG(LogWFCSolver, Log, TEXT("CollapseAt (%d, %d): collapsed to tile %d"), X, Y, Cell.GetCollapsedTile());

	// 约束传播
	bool bSuccess = Propagate(Idx);

	if (!bSuccess)
	{
		UE_LOG(LogWFCSolver, Warning, TEXT("CollapseAt (%d, %d): propagation caused contradiction!"), X, Y);
	}

	// 传播后，将所有 NumCandidates==1 但尚未标记 bCollapsed 的格子标记为已坍缩
	for (int32 i = 0; i < Grid.Num(); ++i)
	{
		if (!Grid[i].bCollapsed && Grid[i].NumCandidates == 1)
		{
			Grid[i].bCollapsed = true;
		}
	}

	// 检查是否全部完成
	if (IsFullyCollapsed())
	{
		bSolved = true;
		UE_LOG(LogWFCSolver, Log, TEXT("Interactive WFC: all cells collapsed!"));
	}

	return bSuccess;
}

int32 UWFCSolver::GetCellCandidateCount(int32 X, int32 Y) const
{
	if (!IsValidCoord(X, Y)) return 0;
	return Grid[CellIndex(X, Y)].NumCandidates;
}

bool UWFCSolver::IsCellCollapsed(int32 X, int32 Y) const
{
	if (!IsValidCoord(X, Y)) return false;
	return Grid[CellIndex(X, Y)].bCollapsed;
}

int32 UWFCSolver::GetCellCollapsedTile(int32 X, int32 Y) const
{
	if (!IsValidCoord(X, Y)) return INDEX_NONE;
	const FWFCCell& Cell = Grid[CellIndex(X, Y)];
	if (!Cell.bCollapsed) return INDEX_NONE;
	return Cell.GetCollapsedTile();
}

bool UWFCSolver::IsFullyCollapsed() const
{
	for (int32 i = 0; i < Grid.Num(); ++i)
	{
		if (!Grid[i].bCollapsed && Grid[i].NumCandidates > 1)
		{
			return false;
		}
	}
	return true;
}

TArray<int32> UWFCSolver::GetAllCandidateCounts() const
{
	TArray<int32> Counts;
	Counts.SetNum(Grid.Num());
	for (int32 i = 0; i < Grid.Num(); ++i)
	{
		Counts[i] = Grid[i].NumCandidates;
	}
	return Counts;
}
