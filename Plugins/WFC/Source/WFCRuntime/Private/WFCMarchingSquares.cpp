#include "WFCMarchingSquares.h"
#include "Engine/StaticMesh.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"

namespace
{
	void AddTriangle(FWFCTownMeshData& Mesh, FVector A, FVector B, FVector C, FLinearColor Color)
	{
		const int32 Start = Mesh.Positions.Num();
		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		Mesh.Positions.Append({A,B,C});
		Mesh.Indices.Append({Start,Start+2,Start+1}); // UE 正面绕序
		for (const FVector& P : {A,B,C})
		{
			Mesh.Normals.Add(Normal);
			// 按面法线选择投影平面，竖墙也有非退化 UV。
			if (FMath::Abs(Normal.Z) > 0.5) Mesh.UVs.Add(FVector2D(P.X, P.Y) / 100.);
			else if (FMath::Abs(Normal.X) > FMath::Abs(Normal.Y)) Mesh.UVs.Add(FVector2D(P.Y, P.Z) / 100.);
			else Mesh.UVs.Add(FVector2D(P.X, P.Z) / 100.);
			Mesh.Colors.Add(Color);
		}
	}
}

FWFCTownMeshData FWFCMarchingSquares::BuildSample(uint8 Mask)
{
	// 角点 0..3，边中点 4..7；每个轮廓按逆时针排列。
	static const TArray<TArray<int32>> Cases[16] = {
		{}, {{0,4,7}}, {{4,1,5}}, {{0,1,5,7}},
		{{5,2,6}}, {{0,4,7},{5,2,6}}, {{4,1,2,6}}, {{0,1,2,6,7}},
		{{7,6,3}}, {{0,4,6,3}}, {{4,1,5},{7,6,3}}, {{0,1,5,6,3}},
		{{7,5,2,3}}, {{0,4,5,2,3}}, {{4,1,2,3,7}}, {{0,1,2,3}}
	};
	const FVector Corners[] = {{0,0,0},{100,0,0},{100,100,0},{0,100,0},
		{50,0,0},{100,50,0},{50,100,0},{0,50,0}};
	FWFCTownMeshData Mesh;
	if (Mask > 15) return Mesh;
	for (const auto& Polygon : Cases[Mask])
	{
		const FVector Up(0,0,100);
		// 凸轮廓用扇形三角剖分，顶面与底面闭合。
		for (int32 I = 1; I + 1 < Polygon.Num(); ++I)
		{
			AddTriangle(Mesh, Corners[Polygon[0]] + Up, Corners[Polygon[I]] + Up,
				Corners[Polygon[I+1]] + Up, FLinearColor(0.65f,0.2f,0.12f));
			AddTriangle(Mesh, Corners[Polygon[0]], Corners[Polygon[I+1]], Corners[Polygon[I]], FLinearColor::Gray);
		}
		for (int32 I = 0; I < Polygon.Num(); ++I)
		{
			const FVector A = Corners[Polygon[I]], B = Corners[Polygon[(I+1)%Polygon.Num()]];
			const FLinearColor Wall(0.85f,0.72f,0.5f);
			AddTriangle(Mesh,A,B,B+Up,Wall);
			AddTriangle(Mesh,A,B+Up,A+Up,Wall);
		}
	}
	return Mesh;
}

FVector FWFCMarchingSquares::DeformPoint(const FVector& Point, const FWFCTownQuad& Quad,
	const FWFCTownGrid& Grid, float Height)
{
	const double U = Point.X / 100., V = Point.Y / 100.;
	const FVector Bottom = FMath::Lerp(Grid.Vertices[Quad.Vertices[0]], Grid.Vertices[Quad.Vertices[1]], U);
	const FVector Top = FMath::Lerp(Grid.Vertices[Quad.Vertices[3]], Grid.Vertices[Quad.Vertices[2]], U);
	return FMath::Lerp(Bottom, Top, V) + FVector(0,0,Point.Z / 100. * Height + 2.);
}

FWFCTownMeshData FWFCMarchingSquares::BuildMesh(const FWFCTownGrid& Grid, const FWFCTownVolume& Volume,
	const UWFCSquareModuleSet* Modules, float Height, int32 HoverVertex)
{
	// 每个编号最多读一次静态网格，再复用于所有同编号单元。
	FWFCTownMeshData Templates[16];
	for (int32 Mask = 1; Mask < 16; ++Mask)
	{
		UStaticMesh* Model = Modules && Modules->Modules.IsValidIndex(Mask) ? Modules->Modules[Mask].Get() : nullptr;
		if (Model && Model->bAllowCPUAccess)
		{
			TArray<FProcMeshTangent> Tangents;
			UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(Model,0,0,Templates[Mask].Positions,
				Templates[Mask].Indices,Templates[Mask].Normals,Templates[Mask].UVs,Tangents);
		}
		// 未指定资产时提供同一套程序化样例，方便先学习编码。
		if (Templates[Mask].Positions.IsEmpty()) Templates[Mask] = BuildSample(Mask);
	}
	TArray<int32> GroundHeights;
	GroundHeights.Init(0, Grid.Quads.Num());
	FWFCTownMeshData Result = FWFCTownMeshGenerator::BuildExtrudedMesh(Grid,GroundHeights,Height,INDEX_NONE);
	for (const auto& Quad : Grid.Quads)
	{
		const uint8 Mask = Volume.GetSquareMask(Quad);
		const auto& Model = Templates[Mask];
		for (int32 I = 0; I + 2 < Model.Indices.Num(); I += 3)
		{
			const FVector A = DeformPoint(Model.Positions[Model.Indices[I]],Quad,Grid,Height);
			const FVector B = DeformPoint(Model.Positions[Model.Indices[I+2]],Quad,Grid,Height);
			const FVector C = DeformPoint(Model.Positions[Model.Indices[I+1]],Quad,Grid,Height);
			const FVector Normal = FVector::CrossProduct(B-A,C-A).GetSafeNormal();
			AddTriangle(Result,A,B,C,Normal.Z > 0.5 ? FLinearColor(0.65f,0.2f,0.12f) : FLinearColor(0.85f,0.72f,0.5f));
		}
	}
	// 顶点标记放在模型上方；点击时也按这些标记的屏幕位置选取。
	for (int32 V = 0; V < Grid.Vertices.Num(); ++V)
	{
		const FVector Center = Grid.Vertices[V] + FVector(0,0,Height+8);
		const float Size = V == HoverVertex ? 7.f : 4.f;
		const FLinearColor Color = V == HoverVertex ? FLinearColor::Yellow :
			(Volume.IsOccupied(V) ? FLinearColor(1.f,0.35f,0.1f) : FLinearColor(0.35f,0.75f,0.85f));
		AddTriangle(Result,Center+FVector(-Size,0,0),Center+FVector(0,-Size,0),Center+FVector(Size,0,0),Color);
		AddTriangle(Result,Center+FVector(-Size,0,0),Center+FVector(Size,0,0),Center+FVector(0,Size,0),Color);
	}
	return Result;
}
