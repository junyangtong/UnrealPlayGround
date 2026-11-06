#pragma once

#include "CoreMinimal.h"

/** 共享顶点的四边形；顶点在局部 XY 平面按逆时针排列。 */
struct WFCRUNTIME_API FWFCTownQuad
{
	TArray<int32> Vertices;
	// Neighbor across each corresponding edge; INDEX_NONE at the boundary.
	TArray<int32> Neighbors;
};

struct WFCRUNTIME_API FWFCTownGrid
{
	TArray<FVector> Vertices;
	TArray<FWFCTownQuad> Quads;
	TSet<int32> BoundaryVertices;

	/** Deterministic triangle pairing, conforming subdivision and bounded relaxation. */
	void Generate(int32 Radius, float Spacing, int32 Seed, int32 RelaxIterations);
	bool IsValid() const;

private:
	// 中间数据只存在于 Generate 调用内，不混入最终网格状态。
	void BuildHexPoints(int32 Radius, float Spacing);
	// 连接第 r 圈和第 r-1 圈，按六条边逐段生成三角形。
	void BuildTriangles(int32 Radius, TArray<TArray<int32>>& Triangles) const;
	void MergeTriangles(const TArray<TArray<int32>>& Triangles, int32 Seed,
		TArray<TArray<int32>>& Polygons) const;
	void SubdivideToQuads(const TArray<TArray<int32>>& Polygons);
	void BuildAdjacency(TArray<TArray<int32>>& VertexNeighbors);
	void RelaxVertices(const TArray<TArray<int32>>& VertexNeighbors, int32 RelaxIterations);
};
