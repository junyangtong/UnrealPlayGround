// Private/TileCutter/WFCTileCutterLogic.cpp
#include "TileCutter/WFCTileCutterLogic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "TextureResource.h"
bool FWFCTileCutterLogic::CutAtlas(
UTexture2D* SourceTexture,
int32 Rows,
int32 Cols,
const FString& SaveDirectory,
const FString& AssetPrefix,
FWFCCutResult& OutResult)
{
if (!SourceTexture || Rows <= 0 || Cols <= 0)
{
UE_LOG(LogTemp, Error, TEXT("WFCTileCutter: Invalid input parameters."));
return false;
}
// 读取源纹理像素
TArray<FColor> SourcePixels;
int32 TexWidth, TexHeight;
if (!GetTextureRawData(SourceTexture, SourcePixels, TexWidth, TexHeight))
{
    return false;
}

int32 TileW = TexWidth / Cols;
int32 TileH = TexHeight / Rows;

if (TileW <= 0 || TileH <= 0)
{
    UE_LOG(LogTemp, Error, TEXT("WFCTileCutter: Tile size too small. Tex=%dx%d, Grid=%dx%d"),
        TexWidth, TexHeight, Cols, Rows);
    return false;
}

OutResult.NumRows = Rows;
OutResult.NumCols = Cols;
OutResult.TileWidth = TileW;
OutResult.TileHeight = TileH;
OutResult.Tiles.Empty();
OutResult.GridPositions.Empty();

for (int32 Row = 0; Row < Rows; ++Row)
{
    for (int32 Col = 0; Col < Cols; ++Col)
    {
        int32 StartX = Col * TileW;
        int32 StartY = Row * TileH;

        TArray<FColor> TilePixels = ExtractSubRegion(
            SourcePixels, TexWidth, TexHeight,
            StartX, StartY, TileW, TileH
        );

        FString AssetName = FString::Printf(TEXT("%s%d_%d"), *AssetPrefix, Row, Col);

        UTexture2D* NewTile = CreateTextureAsset(
            TilePixels, TileW, TileH,
            SaveDirectory, AssetName
        );

        if (NewTile)
        {
            OutResult.Tiles.Add(NewTile);
            OutResult.GridPositions.Add(FIntPoint(Col, Row));
            UE_LOG(LogTemp, Log, TEXT("WFCTileCutter: Created tile [%d,%d] -> %s"),
                Row, Col, *NewTile->GetPathName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("WFCTileCutter: Failed to create tile [%d,%d]"), Row, Col);
        }
    }
}

UE_LOG(LogTemp, Log, TEXT("WFCTileCutter: Cut complete. %d tiles created."), OutResult.Tiles.Num());
return OutResult.Tiles.Num() > 0;

}
bool FWFCTileCutterLogic::CutAtlasByPixelSize(
UTexture2D* SourceTexture,
int32 TilePixelWidth,
int32 TilePixelHeight,
const FString& SaveDirectory,
const FString& AssetPrefix,
FWFCCutResult& OutResult)
{
if (!SourceTexture || TilePixelWidth <= 0 || TilePixelHeight <= 0)
{
return false;
}
TArray<FColor> SourcePixels;
int32 TexWidth, TexHeight;
if (!GetTextureRawData(SourceTexture, SourcePixels, TexWidth, TexHeight))
{
    return false;
}

int32 Cols = TexWidth / TilePixelWidth;
int32 Rows = TexHeight / TilePixelHeight;

return CutAtlas(SourceTexture, Rows, Cols, SaveDirectory, AssetPrefix, OutResult);

}
bool FWFCTileCutterLogic::GetTextureRawData(
UTexture2D* Texture,
TArray<FColor>& OutPixels,
int32& OutWidth,
int32& OutHeight)
{
if (!Texture)
{
return false;
}
// 确保纹理设置允许 CPU 读取
TextureCompressionSettings OldCompression = Texture->CompressionSettings;
TextureMipGenSettings OldMipGenSettings = Texture->MipGenSettings;
bool OldSRGB = Texture->SRGB;

Texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
Texture->SRGB = false;
Texture->UpdateResource();

// 锁定 Mip0 读取
FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
void* MipData = Mip.BulkData.Lock(LOCK_READ_ONLY);

OutWidth = Mip.SizeX;
OutHeight = Mip.SizeY;

int32 NumPixels = OutWidth * OutHeight;
OutPixels.SetNum(NumPixels);

if (MipData)
{
    // 假设 BGRA8 格式
    FMemory::Memcpy(OutPixels.GetData(), MipData, NumPixels * sizeof(FColor));
}

Mip.BulkData.Unlock();

// 恢复纹理设置
Texture->CompressionSettings = OldCompression;
Texture->MipGenSettings = OldMipGenSettings;
Texture->SRGB = OldSRGB;
Texture->UpdateResource();

return MipData != nullptr;

}
TArray<FColor> FWFCTileCutterLogic::ExtractSubRegion(
const TArray<FColor>& SourcePixels,
int32 SourceWidth,
int32 SourceHeight,
int32 StartX,
int32 StartY,
int32 RegionWidth,
int32 RegionHeight)
{
TArray<FColor> Result;
Result.SetNum(RegionWidth * RegionHeight);
for (int32 Y = 0; Y < RegionHeight; ++Y)
{
    for (int32 X = 0; X < RegionWidth; ++X)
    {
        int32 SrcIdx = (StartY + Y) * SourceWidth + (StartX + X);
        int32 DstIdx = Y * RegionWidth + X;

        if (SrcIdx < SourcePixels.Num())
        {
            Result[DstIdx] = SourcePixels[SrcIdx];
        }
        else
        {
            Result[DstIdx] = FColor::Magenta; // 越界用品红标记
        }
    }
}

return Result;

}
UTexture2D* FWFCTileCutterLogic::CreateTextureAsset(
const TArray<FColor>& Pixels,
int32 Width,
int32 Height,
const FString& PackagePath,
const FString& AssetName)
{
// 创建 Package
FString FullPackagePath = PackagePath / AssetName;
FullPackagePath = FPackageName::ObjectPathToPackageName(FullPackagePath);
UPackage* Package = CreatePackage(*FullPackagePath);
if (!Package)
{
    return nullptr;
}

Package->FullyLoad();

// 创建 Texture2D
UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *AssetName,
    RF_Public | RF_Standalone);

NewTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8);

// 写入像素数据到 Source
uint8* MipData = NewTexture->Source.LockMip(0);
FMemory::Memcpy(MipData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
NewTexture->Source.UnlockMip(0);

// 设置纹理属性（像素风格推荐关滤波）
NewTexture->MipGenSettings = TMGS_NoMipmaps;
NewTexture->CompressionSettings = TC_EditorIcon;
NewTexture->Filter = TF_Nearest;
NewTexture->LODGroup = TEXTUREGROUP_Pixels2D;
NewTexture->SRGB = true;
NewTexture->NeverStream = true;

NewTexture->UpdateResource();
NewTexture->PostEditChange();

// 标记脏并保存
Package->MarkPackageDirty();

FAssetRegistryModule::AssetCreated(NewTexture);

FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
FString PackageFilename = FPackageName::LongPackageNameToFilename(
    FullPackagePath, FPackageName::GetAssetPackageExtension());
UPackage::SavePackage(Package, NewTexture, *PackageFilename, SaveArgs);

return NewTexture;

}
