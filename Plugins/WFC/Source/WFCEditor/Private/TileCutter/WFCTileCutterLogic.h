// Private/TileCutter/WFCTileCutterLogic.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

struct FWFCCutResult
{
    // 切出的子纹理列表
    TArray<UTexture2D*> Tiles;
    // 每个 tile 的网格坐标 (row, col)
    TArray<FIntPoint> GridPositions;
    // 原图的行列数
    int32 NumRows = 0;
    int32 NumCols = 0;
    // 每张 tile 的尺寸
    int32 TileWidth = 0;
    int32 TileHeight = 0;
};

class FWFCTileCutterLogic
{
public:
    /**
     * 从源纹理按行列切分
     * @param SourceTexture    源图集纹理
     * @param Rows             行数
     * @param Cols             列数
     * @param SaveDirectory    保存资产的路径 (e.g. "/Game/Demos/WFC/Tiles/")
     * @param AssetPrefix      资产名前缀 (e.g. "Tile_")
     * @param OutResult        输出切割结果
     * @return 是否成功
     */
    static bool CutAtlas(
        UTexture2D* SourceTexture,
        int32 Rows,
        int32 Cols,
        const FString& SaveDirectory,
        const FString& AssetPrefix,
        FWFCCutResult& OutResult
    );

    /**
     * 从源纹理按像素尺寸切分
     */
    static bool CutAtlasByPixelSize(
        UTexture2D* SourceTexture,
        int32 TilePixelWidth,
        int32 TilePixelHeight,
        const FString& SaveDirectory,
        const FString& AssetPrefix,
        FWFCCutResult& OutResult
    );

    /**
     * 获取纹理像素数据（解锁Mip0）
     */
    static bool GetTextureRawData(
        UTexture2D* Texture,
        TArray<FColor>& OutPixels,
        int32& OutWidth,
        int32& OutHeight
    );

    /**
     * 从像素数组中截取子区域
     */
    static TArray<FColor> ExtractSubRegion(
        const TArray<FColor>& SourcePixels,
        int32 SourceWidth,
        int32 SourceHeight,
        int32 StartX,
        int32 StartY,
        int32 RegionWidth,
        int32 RegionHeight
    );

    /**
     * 从像素数据创建并保存 UTexture2D 资产
     */
    static UTexture2D* CreateTextureAsset(
        const TArray<FColor>& Pixels,
        int32 Width,
        int32 Height,
        const FString& PackagePath,
        const FString& AssetName
    );
};
