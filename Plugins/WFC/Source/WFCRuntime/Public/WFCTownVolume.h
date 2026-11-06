#pragma once
#include "CoreMinimal.h"
#include "WFCTownGrid.h"
#include "WFCTownVolume.generated.h"

/** 当前是单层共享顶点占用；以后增加高度层，扩展为八角编码。 */
USTRUCT()
struct WFCRUNTIME_API FWFCTownVolume
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<uint8> Occupied;
	// 非序列化索引，在网格初始化后重建。
	TArray<TArray<int32>> VertexCells;
	void Initialize(const FWFCTownGrid& Grid, bool bReset);
	bool SetOccupied(int32 Vertex, bool bOccupied);
	bool IsOccupied(int32 Vertex) const;
	uint8 GetSquareMask(const FWFCTownQuad& Quad) const;
};
