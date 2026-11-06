#include "WFCTownVolume.h"

void FWFCTownVolume::Initialize(const FWFCTownGrid& Grid, bool bReset)
{
	if (bReset || Occupied.Num() != Grid.Vertices.Num()) Occupied.Init(0, Grid.Vertices.Num());
	VertexCells.Empty();
	VertexCells.SetNum(Grid.Vertices.Num());
	for (int32 Cell = 0; Cell < Grid.Quads.Num(); ++Cell)
		for (int32 Vertex : Grid.Quads[Cell].Vertices) VertexCells[Vertex].Add(Cell);
}

bool FWFCTownVolume::SetOccupied(int32 Vertex, bool bOccupied)
{
	if (!Occupied.IsValidIndex(Vertex) || IsOccupied(Vertex) == bOccupied) return false;
	Occupied[Vertex] = bOccupied ? 1 : 0;
	return true;
}

bool FWFCTownVolume::IsOccupied(int32 Vertex) const
{
	return Occupied.IsValidIndex(Vertex) && Occupied[Vertex] != 0;
}

uint8 FWFCTownVolume::GetSquareMask(const FWFCTownQuad& Quad) const
{
	uint8 Mask = 0;
	for (int32 Corner = 0; Corner < 4; ++Corner)
		if (IsOccupied(Quad.Vertices[Corner])) Mask |= 1 << Corner;
	return Mask;
}
