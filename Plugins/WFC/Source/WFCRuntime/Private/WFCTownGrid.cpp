#include "WFCTownGrid.h"
#include "HexData.h"

namespace
{
	FIntPoint EdgeKey(int32 A, int32 B) { return FIntPoint(FMath::Min(A, B), FMath::Max(A, B)); }
	double Cross(const FVector& A, const FVector& B, const FVector& C)
	{
		return (B.X - A.X) * (C.Y - B.Y) - (B.Y - A.Y) * (C.X - B.X);
	}
	bool Convex(const TArray<int32>& Face, const TArray<FVector>& Points)
	{
		for (int32 I = 0; I < Face.Num(); ++I)
		{
			if (Cross(Points[Face[I]], Points[Face[(I + 1) % Face.Num()]],
				Points[Face[(I + 2) % Face.Num()]]) <= UE_SMALL_NUMBER) return false;
		}
		return true;
	}
}

void FWFCTownGrid::Generate(int32 Radius, float Spacing, int32 Seed, int32 RelaxIterations)
{
	Vertices.Reset();
	Quads.Reset();
	BoundaryVertices.Reset();
	Radius = FMath::Clamp(Radius, 1, 12);
	Spacing = FMath::IsFinite(Spacing) ? FMath::Clamp(Spacing, 10.f, 10000.f) : 100.f;

	// 教程顺序：点阵 → 三角形 → 随机合并 → 四边形细分 → 平滑。
	BuildHexPoints(Radius, Spacing);

	TArray<TArray<int32>> Triangles;
	BuildTriangles(Radius, Triangles);

	TArray<TArray<int32>> Polygons;
	MergeTriangles(Triangles, Seed, Polygons);
	SubdivideToQuads(Polygons);

	TArray<TArray<int32>> VertexNeighbors;
	BuildAdjacency(VertexNeighbors);
	RelaxVertices(VertexNeighbors, RelaxIterations);
}

void FWFCTownGrid::BuildHexPoints(int32 Radius, float Spacing)
{
	Vertices = FHexCoord::BuildSpiralPoints(Radius, Spacing);
}

void FWFCTownGrid::BuildTriangles(int32 Radius, TArray<TArray<int32>>& Triangles) const
{
	Triangles.Reserve(6 * Radius * Radius);
	auto AddTriangle = [&](int32 A, int32 B, int32 C)
	{
		// 环的遍历方向来自 HexData；统一为后续合并需要的逆时针绕序。
		if (Cross(Vertices[A], Vertices[B], Vertices[C]) < 0) Swap(B, C);
		Triangles.Add({A, B, C});
	};

	// 与 GrabRing 相同的螺旋布局：中心为 0，第 r 圈从 1+3*r*(r-1) 开始。
	// 这里保留全局顶点索引，避免复制坐标后再查找对应的顶点。
	for (int32 Ring = 1; Ring <= Radius; ++Ring)
	{
		const int32 OuterStart = 1 + 3 * Ring * (Ring - 1);
		const int32 OuterCount = 6 * Ring;
		const int32 InnerSideLength = Ring - 1;
		const int32 InnerStart = Ring == 1 ? 0 : 1 + 3 * (Ring - 1) * (Ring - 2);
		const int32 InnerCount = Ring == 1 ? 1 : 6 * InnerSideLength;

		for (int32 Side = 0; Side < 6; ++Side)
		{
			for (int32 Step = 0; Step < Ring; ++Step)
			{
				const int32 OuterOffset = Side * Ring + Step;
				const int32 InnerOffset = Side * InnerSideLength + Step;
				const int32 OuterA = OuterStart + OuterOffset;
				const int32 OuterB = OuterStart + (OuterOffset + 1) % OuterCount;
				const int32 InnerA = InnerStart + InnerOffset % InnerCount;

				// 每条外圈边连向内圈一点；第一圈自然形成中心扇形的六个三角形。
				AddTriangle(InnerA, OuterA, OuterB);

				// 内圈每条边再补一个三角形，填满内外圈之间的带状区域。
				// 每侧最后一步已到内圈拐角，不再补面；取模闭合第六侧首尾。
				if (Step < InnerSideLength)
				{
					const int32 InnerB = InnerStart + (InnerOffset + 1) % InnerCount;
					AddTriangle(InnerA, OuterB, InnerB);
				}
			}
		}
	}
}

void FWFCTownGrid::MergeTriangles(const TArray<TArray<int32>>& Triangles, int32 Seed,
	TArray<TArray<int32>>& Polygons) const
{
	TMap<FIntPoint, TArray<int32>> TriangleEdges;
	for (int32 T = 0; T < Triangles.Num(); ++T)
		for (int32 E = 0; E < 3; ++E)
			TriangleEdges.FindOrAdd(EdgeKey(Triangles[T][E], Triangles[T][(E + 1) % 3])).Add(T);
	TArray<FIntPoint> Edges;
	TriangleEdges.GetKeys(Edges);
	Edges.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
	FRandomStream Random(Seed);
	for (int32 I = Edges.Num() - 1; I > 0; --I) Edges.Swap(I, Random.RandRange(0, I));
	TSet<int32> Paired;
	for (const FIntPoint& Edge : Edges)
	{
		const TArray<int32>& Owners = TriangleEdges[Edge];
		if (Owners.Num() != 2 || Paired.Contains(Owners[0]) || Paired.Contains(Owners[1])) continue;
		TArray<int32> Face = Triangles[Owners[0]];
		for (int32 V : Triangles[Owners[1]]) Face.AddUnique(V);
		FVector Center = FVector::ZeroVector;
		for (int32 V : Face) Center += Vertices[V];
		Center /= Face.Num();
		Face.Sort([&](int32 A, int32 B) { return FMath::Atan2(Vertices[A].Y - Center.Y, Vertices[A].X - Center.X)
			< FMath::Atan2(Vertices[B].Y - Center.Y, Vertices[B].X - Center.X); });
		if (!Convex(Face, Vertices)) continue;
		Polygons.Add(MoveTemp(Face));
		Paired.Add(Owners[0]); Paired.Add(Owners[1]);
	}
	for (int32 T = 0; T < Triangles.Num(); ++T) if (!Paired.Contains(T)) Polygons.Add(Triangles[T]);

}

void FWFCTownGrid::SubdivideToQuads(const TArray<TArray<int32>>& Polygons)
{
	// Every polygon is subdivided: even unpaired triangles become three quads.
	TMap<FIntPoint, int32> Midpoints;
	for (const TArray<int32>& Face : Polygons)
	{
		FVector Center = FVector::ZeroVector;
		for (int32 V : Face) Center += Vertices[V];
		const int32 CenterIndex = Vertices.Add(Center / Face.Num());
		TArray<int32> Mids;
		for (int32 E = 0; E < Face.Num(); ++E)
		{
			const FIntPoint Key = EdgeKey(Face[E], Face[(E + 1) % Face.Num()]);
			int32* Existing = Midpoints.Find(Key);
			if (Existing) Mids.Add(*Existing);
			else
			{
				const int32 Mid = Vertices.Add((Vertices[Key.X] + Vertices[Key.Y]) * 0.5);
				Midpoints.Add(Key, Mid); Mids.Add(Mid);
			}
		}
		for (int32 E = 0; E < Face.Num(); ++E)
		{
			FWFCTownQuad Quad;
			Quad.Vertices = {Face[E], Mids[E], CenterIndex, Mids[(E + Face.Num() - 1) % Face.Num()]};
			Quad.Neighbors.Init(INDEX_NONE, 4);
			Quads.Add(MoveTemp(Quad));
		}
	}
}

void FWFCTownGrid::BuildAdjacency(TArray<TArray<int32>>& VertexNeighbors)
{
	TMap<FIntPoint, FIntPoint> QuadEdges;
	VertexNeighbors.SetNum(Vertices.Num());
	for (int32 Q = 0; Q < Quads.Num(); ++Q)
	{
		for (int32 E = 0; E < 4; ++E)
		{
			const FIntPoint Key = EdgeKey(Quads[Q].Vertices[E], Quads[Q].Vertices[(E + 1) % 4]);
			VertexNeighbors[Key.X].AddUnique(Key.Y); VertexNeighbors[Key.Y].AddUnique(Key.X);
			if (const FIntPoint* Owner = QuadEdges.Find(Key))
			{
				Quads[Q].Neighbors[E] = Owner->X;
				Quads[Owner->X].Neighbors[Owner->Y] = Q;
			}
			else QuadEdges.Add(Key, FIntPoint(Q, E));
		}
	}
	for (const auto& Entry : QuadEdges)
		if (Quads[Entry.Value.X].Neighbors[Entry.Value.Y] == INDEX_NONE)
		{ BoundaryVertices.Add(Entry.Key.X); BoundaryVertices.Add(Entry.Key.Y); }
}

void FWFCTownGrid::RelaxVertices(const TArray<TArray<int32>>& VertexNeighbors, int32 RelaxIterations)
{
	for (int32 Iteration = 0; Iteration < FMath::Clamp(RelaxIterations, 0, 50); ++Iteration)
	{
		TArray<FVector> Next = Vertices;
		for (int32 V = 0; V < Vertices.Num(); ++V)
		{
			if (BoundaryVertices.Contains(V) || VertexNeighbors[V].IsEmpty()) continue;
			FVector Average = FVector::ZeroVector;
			for (int32 N : VertexNeighbors[V]) Average += Vertices[N];
			Next[V] = FMath::Lerp(Vertices[V], Average / VertexNeighbors[V].Num(), 0.35);
		}
		bool bValid = true;
		for (const auto& Quad : Quads) if (!Convex(Quad.Vertices, Next)) { bValid = false; break; }
		if (!bValid) break; // Never introduce inverted or concave cells.
		Vertices = MoveTemp(Next);
	}
}

bool FWFCTownGrid::IsValid() const
{
	if (Vertices.IsEmpty() || Quads.IsEmpty()) return false;
	for (int32 Q = 0; Q < Quads.Num(); ++Q)
	{
		const auto& Quad = Quads[Q];
		if (Quad.Vertices.Num() != 4 || Quad.Neighbors.Num() != 4) return false;
		for (int32 V : Quad.Vertices) if (!Vertices.IsValidIndex(V) || Vertices[V].ContainsNaN()) return false;
		if (!Convex(Quad.Vertices, Vertices)) return false;
		for (int32 E = 0; E < 4; ++E)
		{
			const int32 N = Quad.Neighbors[E];
			if (N == INDEX_NONE) continue;
			if (!Quads.IsValidIndex(N)) return false;
			bool bReciprocal = false;
			for (int32 NE = 0; NE < 4; ++NE)
				if (Quads[N].Neighbors[NE] == Q && Quad.Vertices[E] == Quads[N].Vertices[(NE + 1) % 4]
					&& Quad.Vertices[(E + 1) % 4] == Quads[N].Vertices[NE]) bReciprocal = true;
			if (!bReciprocal) return false;
		}
	}
	return true;
}
