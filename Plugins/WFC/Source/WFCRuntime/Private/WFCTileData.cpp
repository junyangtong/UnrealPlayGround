// ========================
// WFCTileData.cpp (WFCRuntime 模块)
// ========================
#include "WFCTileData.h"

// ========================
// UWFCTileData
// ========================

void UWFCTileData::PostInitProperties()
{
	Super::PostInitProperties();
	if (EdgeSockets.Num() != 4)
	{
		EdgeSockets.SetNum(4);
	}
}

void UWFCTileData::PostLoad()
{
	Super::PostLoad();
	if (EdgeSockets.Num() != 4)
	{
		EdgeSockets.SetNum(4);
	}
}

#if WITH_EDITOR
void UWFCTileData::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 编辑器中用户通过细节面板修改 EdgeSockets 时，保持 4 个元素
	if (EdgeSockets.Num() != 4)
	{
		EdgeSockets.SetNum(4);
	}
}
#endif

// ========================
// UWFCTileSet
// ========================

TArray<int32> UWFCTileSet::GetCompatibleTiles(int32 TileIndex, EWFCDirection Direction) const
{
	TArray<int32> Result;
	if (!Tiles.IsValidIndex(TileIndex) || !Tiles[TileIndex])
	{
		return Result;
	}

	const FWFCEdgeSocket& MySocket = Tiles[TileIndex]->GetEdge(Direction);
	if (MySocket.IsEmpty())
	{
		// "any" 可以连接任何 Tile
		for (int32 i = 0; i < Tiles.Num(); ++i)
		{
			if (i != TileIndex)
			{
				Result.Add(i);
			}
		}
		return Result;
	}

	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		if (i == TileIndex || !Tiles[i])
		{
			continue;
		}

		EWFCDirection OppDir = WFCDirectionUtils::GetOpposite(Direction);
		const FWFCEdgeSocket& OtherSocket = Tiles[i]->GetEdge(OppDir);

		if (MySocket.CanConnect(OtherSocket))
		{
			Result.Add(i);
		}
	}

	return Result;
}

bool UWFCTileSet::AreCompatible(int32 TileA, int32 TileB, EWFCDirection DirectionFromAToB) const
{
	if (!Tiles.IsValidIndex(TileA) || !Tiles.IsValidIndex(TileB) || !Tiles[TileA] || !Tiles[TileB])
	{
		return false;
	}

	const FWFCEdgeSocket& SocketA = Tiles[TileA]->GetEdge(DirectionFromAToB);
	EWFCDirection OppDir = WFCDirectionUtils::GetOpposite(DirectionFromAToB);
	const FWFCEdgeSocket& SocketB = Tiles[TileB]->GetEdge(OppDir);

	return SocketA.CanConnect(SocketB);
}

TArray<FString> UWFCTileSet::GetUniqueSocketIDs() const
{
	TSet<FString> UniqueIDs;
	for (UWFCTileData* Tile : Tiles)
	{
		if (Tile)
		{
			for (const FWFCEdgeSocket& Socket : Tile->EdgeSockets)
			{
				if (!Socket.SocketID.IsEmpty() && Socket.SocketID != TEXT("any"))
				{
					UniqueIDs.Add(Socket.SocketID);
				}
			}
		}
	}
	return UniqueIDs.Array();
}

#if WITH_EDITOR
void UWFCTileSet::RebuildCache()
{
	// 目前兼容性是实时计算的，此方法为将来缓存机制预留
}
#endif
