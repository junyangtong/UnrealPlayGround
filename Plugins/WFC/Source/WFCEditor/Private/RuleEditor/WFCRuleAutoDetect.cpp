// ========================
// WFCRuleAutoDetect.cpp (WFCEditor 模块)
// 基于像素比较的邻接规则自动检测
// ========================
#include "RuleEditor/WFCRuleAutoDetect.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "ImageCore.h"
#include "ImageCoreUtils.h"
#include "WFCTileData.h"

bool FWFCRuleAutoDetect::DetectRules(
	UWFCTileSet* TileSet,
	float Tolerance,
	FWFCRuleAutoDetectResult& OutResult)
{
	if (!TileSet || TileSet->Tiles.Num() == 0)
	{
		return false;
	}

	// Clamp tolerance
	Tolerance = FMath::Clamp(Tolerance, 0.0f, 1.0f);

	// Step 1: 提取所有 Tile 的所有方向的边缘像素
	TArray<FWFCEdgePixels> AllEdges;
	AllEdges.Reserve(TileSet->Tiles.Num() * 4);

	for (int32 TileIdx = 0; TileIdx < TileSet->Tiles.Num(); ++TileIdx)
	{
		UWFCTileData* Tile = TileSet->Tiles[TileIdx];
		if (!Tile || !Tile->TileTexture)
		{
			continue;
		}

		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			FWFCEdgePixels Edge = ExtractEdgePixels(
				Tile->TileTexture, TileIdx, static_cast<EWFCDirection>(Dir));
			if (Edge.Pixels.Num() > 0)
			{
				AllEdges.Add(MoveTemp(Edge));
			}
		}
	}

	if (AllEdges.Num() == 0)
	{
		return false;
	}

	// Step 2: 对所有边缘进行分组
	TArray<TArray<int32>> Groups;
	GroupEdges(AllEdges, Tolerance, Groups);

	// Step 3: 为每个分组分配 Socket ID，并写回 Tile 数据
	OutResult.EdgeToSocketMap.Empty();
	OutResult.NumUniqueEdges = AllEdges.Num();
	OutResult.NumSocketIDs = Groups.Num();

	for (int32 GroupIdx = 0; GroupIdx < Groups.Num(); ++GroupIdx)
	{
		FString SocketID = FString::Printf(TEXT("auto_%d"), GroupIdx);

		for (int32 EdgeIdx : Groups[GroupIdx])
		{
			const FWFCEdgePixels& Edge = AllEdges[EdgeIdx];
			OutResult.EdgeToSocketMap.Add(
				TPair<int32, EWFCDirection>(Edge.TileIndex, Edge.Direction), SocketID);

			if (UWFCTileData* Tile = TileSet->Tiles[Edge.TileIndex])
			{
				FWFCEdgeSocket& Socket = Tile->GetEdge(Edge.Direction);
				Socket.SocketID = SocketID;
				Socket.bSymmetric = true;
			}
		}
	}

	TileSet->Modify();
	TileSet->MarkPackageDirty();

	return true;
}

FWFCEdgePixels FWFCRuleAutoDetect::ExtractEdgePixels(
	UTexture2D* Texture,
	int32 TileIndex,
	EWFCDirection Direction)
{
	FWFCEdgePixels Result;
	Result.TileIndex = TileIndex;
	Result.Direction = Direction;
	Result.Length = 0;

	TArray<FColor> Pixels;
	int32 Width, Height;
	if (!GetTexturePixels(Texture, Pixels, Width, Height))
	{
		return Result;
	}

	switch (Direction)
	{
	case EWFCDirection::Up:
		// 上边缘：顶行，从左到右
		Result.Pixels.Reserve(Width);
		for (int32 X = 0; X < Width; ++X)
		{
			Result.Pixels.Add(Pixels[X]);
		}
		Result.Length = Width;
		break;

	case EWFCDirection::Down:
		// 下边缘：底行，从左到右
		Result.Pixels.Reserve(Width);
		for (int32 X = 0; X < Width; ++X)
		{
			Result.Pixels.Add(Pixels[(Height - 1) * Width + X]);
		}
		Result.Length = Width;
		break;

	case EWFCDirection::Left:
		// 左边缘：左列，从上到下
		Result.Pixels.Reserve(Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			Result.Pixels.Add(Pixels[Y * Width]);
		}
		Result.Length = Height;
		break;

	case EWFCDirection::Right:
		// 右边缘：右列，从上到下
		Result.Pixels.Reserve(Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			Result.Pixels.Add(Pixels[Y * Width + (Width - 1)]);
		}
		Result.Length = Height;
		break;
	}

	return Result;
}

bool FWFCRuleAutoDetect::GetTexturePixels(
	UTexture2D* Texture,
	TArray<FColor>& OutPixels,
	int32& OutWidth,
	int32& OutHeight)
{
	if (!Texture)
	{
		return false;
	}

#if WITH_EDITORONLY_DATA
	// UE5: 通过 FTextureSource::FMipLock 读取源图 mip0 像素
	FTextureSource& Source = Texture->Source;
	OutWidth = Source.GetSizeX();
	OutHeight = Source.GetSizeY();

	if (OutWidth <= 0 || OutHeight <= 0)
	{
		return false;
	}

	FTextureSource::FMipLock MipLock(
		FTextureSource::ELockState::ReadOnly, &Source, 0);

	if (!MipLock.IsValid() || MipLock.Image.RawData == nullptr)
	{
		return false;
	}

	// 将源图像数据转换为 BGRA8 FColor 数组
	int32 NumPixels = OutWidth * OutHeight;
	OutPixels.SetNumUninitialized(NumPixels);

	FImageView SrcView = MipLock.Image;
	ERawImageFormat::Type SrcFormat = SrcView.Format;

	if (SrcFormat == ERawImageFormat::BGRA8)
	{
		// 直接复制，格式已匹配
		FMemory::Memcpy(OutPixels.GetData(), SrcView.RawData, NumPixels * sizeof(FColor));
	}
	else
	{
		// 通过 FImage 转换到 BGRA8
		FImage TmpImage;
		TmpImage.Init(OutWidth, OutHeight, ERawImageFormat::BGRA8, SrcView.GammaSpace);
		SrcView.CopyTo(TmpImage);
		FMemory::Memcpy(OutPixels.GetData(), TmpImage.RawData.GetData(), NumPixels * sizeof(FColor));
	}

	return true;
#else
	OutWidth = 0;
	OutHeight = 0;
	return false;
#endif
}

float FWFCRuleAutoDetect::ComputeEdgeSimilarity(
	const FWFCEdgePixels& EdgeA,
	const FWFCEdgePixels& EdgeB)
{
	// 边缘长度必须相同才能比较
	if (EdgeA.Length != EdgeB.Length || EdgeA.Length == 0)
	{
		return 1.0f; // 完全不同
	}

	int32 Count = EdgeA.Pixels.Num();
	float TotalDiff = 0.0f;

	for (int32 i = 0; i < Count; ++i)
	{
		const FColor& CA = EdgeA.Pixels[i];
		const FColor& CB = EdgeB.Pixels[i];
		// RGB 通道差异归一化到 [0, 1]
		float DiffR = FMath::Abs((float)CA.R - (float)CB.R) / 255.0f;
		float DiffG = FMath::Abs((float)CA.G - (float)CB.G) / 255.0f;
		float DiffB = FMath::Abs((float)CA.B - (float)CB.B) / 255.0f;
		float DiffA = FMath::Abs((float)CA.A - (float)CB.A) / 255.0f;
		TotalDiff += (DiffR + DiffG + DiffB + DiffA) / 4.0f;
	}

	return TotalDiff / Count;
}

void FWFCRuleAutoDetect::GroupEdges(
	const TArray<FWFCEdgePixels>& AllEdges,
	float Tolerance,
	TArray<TArray<int32>>& OutGroups)
{
	OutGroups.Empty();
	int32 N = AllEdges.Num();

	// Union-Find 结构
	TArray<int32> Parent;
	Parent.SetNumUninitialized(N);
	for (int32 i = 0; i < N; ++i)
	{
		Parent[i] = i;
	}

	// 路径压缩 Find
	auto FindParent = [&Parent](int32 X) -> int32
	{
		while (Parent[X] != X)
		{
			Parent[X] = Parent[Parent[X]];
			X = Parent[X];
		}
		return X;
	};

	// Union
	auto Union = [&Parent, &FindParent](int32 A, int32 B)
	{
		int32 RootA = FindParent(A);
		int32 RootB = FindParent(B);
		if (RootA != RootB)
		{
			Parent[RootA] = RootB;
		}
	};

	// 两两比较所有边缘
	for (int32 i = 0; i < N; ++i)
	{
		for (int32 j = i + 1; j < N; ++j)
		{
			float Similarity = ComputeEdgeSimilarity(AllEdges[i], AllEdges[j]);
			if (Similarity <= Tolerance)
			{
				Union(i, j);
			}
		}
	}

	// 收集分组
	TMap<int32, TArray<int32>> GroupMap;
	for (int32 i = 0; i < N; ++i)
	{
		int32 Root = FindParent(i);
		GroupMap.FindOrAdd(Root).Add(i);
	}

	OutGroups.Reserve(GroupMap.Num());
	for (auto& Pair : GroupMap)
	{
		OutGroups.Add(MoveTemp(Pair.Value));
	}
}
