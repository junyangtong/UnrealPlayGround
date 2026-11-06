// ========================
// WFCRuleAutoDetect.h (WFCEditor 模块)
// 基于像素比较的邻接规则自动检测
// ========================
#pragma once

#include "CoreMinimal.h"
#include "WFCTileData.h"

class UTexture2D;
struct FTexture2DMipMap;

// 一条边的像素数据（1 像素宽的条带）
struct FWFCEdgePixels
{
	int32 TileIndex;
	EWFCDirection Direction;
	TArray<FColor> Pixels;
	int32 Length;
};

// 自动检测结果
struct FWFCRuleAutoDetectResult
{
	int32 NumUniqueEdges = 0;
	int32 NumSocketIDs = 0;
	TMap<TPair<int32, EWFCDirection>, FString> EdgeToSocketMap;
};

class FWFCRuleAutoDetect
{
public:
	static bool DetectRules(
		UWFCTileSet* TileSet,
		float Tolerance,
		FWFCRuleAutoDetectResult& OutResult
	);

private:
	static FWFCEdgePixels ExtractEdgePixels(
		UTexture2D* Texture,
		int32 TileIndex,
		EWFCDirection Direction
	);

	static bool GetTexturePixels(
		UTexture2D* Texture,
		TArray<FColor>& OutPixels,
		int32& OutWidth,
		int32& OutHeight
	);

	static float ComputeEdgeSimilarity(
		const FWFCEdgePixels& EdgeA,
		const FWFCEdgePixels& EdgeB
	);

	static void GroupEdges(
		const TArray<FWFCEdgePixels>& AllEdges,
		float Tolerance,
		TArray<TArray<int32>>& OutGroups
	);
};
