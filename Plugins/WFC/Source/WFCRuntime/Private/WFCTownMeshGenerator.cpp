#include "WFCTownMeshGenerator.h"

namespace
{
	// Two-sided ray triangle test, used only by the cursor; no collision cooking per edit.
	bool RayTriangle(const FVector& O, const FVector& D, const FVector& A, const FVector& B, const FVector& C, double& Best)
	{
		const FVector E1 = B - A, E2 = C - A, P = FVector::CrossProduct(D, E2);
		const double Det = FVector::DotProduct(E1, P);
		if (FMath::Abs(Det) < 1.e-9) return false;
		const FVector T = O - A;
		const double U = FVector::DotProduct(T, P) / Det;
		if (U < 0 || U > 1) return false;
		const FVector Q = FVector::CrossProduct(T, E1);
		const double V = FVector::DotProduct(D, Q) / Det;
		if (V < 0 || U + V > 1) return false;
		const double Distance = FVector::DotProduct(E2, Q) / Det;
		if (Distance < 0 || Distance >= Best) return false;
		Best = Distance; return true;
	}
}

FWFCTownMeshData FWFCTownMeshGenerator::BuildExtrudedMesh(const FWFCTownGrid& Grid,
	const TArray<int32>& Heights, float FloorHeight, int32 HoverCell)
{
	FWFCTownMeshData Data;
	auto& Positions = Data.Positions;
	auto& Normals = Data.Normals;
	auto& Indices = Data.Indices;
	auto& UVs = Data.UVs;
	auto& Colors = Data.Colors;
	auto AddFace = [&](FVector A, FVector B, FVector C, FVector D, const FLinearColor& Color)
	{
		const int32 Base = Positions.Num();
		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		Positions.Append({A, B, C, D});
		for (int32 I = 0; I < 4; ++I) { Normals.Add(Normal); Colors.Add(Color); }
		UVs.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)});
		// UE front faces are clockwise; topology is mathematical CCW.
		Indices.Append({Base, Base + 2, Base + 1, Base, Base + 3, Base + 2});
	};
	for (int32 Cell = 0; Cell < Grid.Quads.Num(); ++Cell)
	{
		const auto& Quad = Grid.Quads[Cell];
		const float Top = Heights[Cell] * FloorHeight;
		FVector P[4];
		FVector Center = FVector::ZeroVector;
		for (int32 I = 0; I < 4; ++I) { P[I] = Grid.Vertices[Quad.Vertices[I]] + FVector(0, 0, Top); Center += P[I] / 4; }
		const FLinearColor Roof = Cell == HoverCell ? FLinearColor(1.f, 0.75f, 0.12f) :
			(Top > 0 ? FLinearColor(0.6f, 0.18f, 0.1f) : FLinearColor(0.12f, 0.32f, 0.38f));
		// Recessed rim makes individual cells legible even with the default gray material.
		FVector Inner[4];
		for (int32 I = 0; I < 4; ++I) Inner[I] = FMath::Lerp(P[I], Center, 0.055) + FVector(0, 0, Cell == HoverCell ? 5.f : 1.f);
		AddFace(Inner[0], Inner[1], Inner[2], Inner[3], Roof);
		for (int32 E = 0; E < 4; ++E)
		{
			const int32 Next = (E + 1) % 4;
			AddFace(P[E], P[Next], Inner[Next], Inner[E], Roof * 0.65f);
			const int32 Neighbor = Quad.Neighbors[E];
			const float Bottom = Neighbor == INDEX_NONE ? -12.f : Heights[Neighbor] * FloorHeight;
			if (Bottom >= Top) continue; // Shared interior walls are omitted.
			const FVector A(P[E].X, P[E].Y, Bottom), B(P[Next].X, P[Next].Y, Bottom);
			AddFace(A, B, P[Next], P[E], FLinearColor(0.8f, 0.68f, 0.48f));
		}
	}
	return Data;
}

int32 FWFCTownMeshGenerator::RaycastCell(const FWFCTownGrid& Grid, const TArray<int32>& Heights,
	float FloorHeight, const FVector& Origin, const FVector& Direction)
{
	double Best = TNumericLimits<double>::Max();
	int32 Result = INDEX_NONE;
	for (int32 Cell = 0; Cell < Grid.Quads.Num(); ++Cell)
	{
		const auto& Quad = Grid.Quads[Cell];
		FVector P[4];
		for (int32 I = 0; I < 4; ++I) P[I] = Grid.Vertices[Quad.Vertices[I]] + FVector(0, 0, Heights[Cell] * FloorHeight);
		auto Test = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D)
		{
			if (RayTriangle(Origin, Direction, A, B, C, Best)) Result = Cell;
			if (RayTriangle(Origin, Direction, A, C, D, Best)) Result = Cell;
		};
		Test(P[0], P[1], P[2], P[3]);
		for (int32 E = 0; E < 4; ++E)
		{
			const int32 N = Quad.Neighbors[E], Next = (E + 1) % 4;
			const float Bottom = N == INDEX_NONE ? -12.f : Heights[N] * FloorHeight;
			if (Bottom >= P[E].Z) continue;
			Test(FVector(P[E].X, P[E].Y, Bottom), FVector(P[Next].X, P[Next].Y, Bottom), P[Next], P[E]);
		}
	}
	return Result;
}
