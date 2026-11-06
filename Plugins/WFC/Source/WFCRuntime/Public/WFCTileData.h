// ========================
// WFCTileData.h (WFCRuntime 模块)
// 核心数据类型：Tile 定义、TileSet、边缘 Socket
// ========================
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "WFCTileData.generated.h"

// ========================
// 方向枚举
// ========================
UENUM(BlueprintType)
enum class EWFCDirection : uint8
{
	Up    = 0,
	Right = 1,
	Down  = 2,
	Left  = 3,
	MAX   UMETA(Hidden)
};

// 方向对应的相反方向
namespace WFCDirectionUtils
{
	inline EWFCDirection GetOpposite(EWFCDirection Dir)
	{
		return static_cast<EWFCDirection>((static_cast<uint8>(Dir) + 2) % 4);
	}
}

// ========================
// 边缘 Socket
// ========================
USTRUCT(BlueprintType)
struct WFCRUNTIME_API FWFCEdgeSocket
{
	GENERATED_BODY()

	// Socket 名称，相同名称的边可以相邻
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC")
	FString SocketID = TEXT("any");

	// 是否对称。true 时直接比较 SocketID，false 时遵循命名约定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC")
	bool bSymmetric = true;

	bool CanConnect(const FWFCEdgeSocket& Other) const
	{
		if (bSymmetric && Other.bSymmetric)
		{
			return SocketID == Other.SocketID;
		}
		// 非对称时也直接比较 SocketID（由 auto-detect 或用户保证命名一致）
		return SocketID == Other.SocketID;
	}

	bool IsEmpty() const { return SocketID.IsEmpty() || SocketID == TEXT("any"); }
};

// ========================
// 单个 Tile 数据定义
// ========================
UCLASS()
class WFCRUNTIME_API UWFCTileData : public UObject
{
	GENERATED_BODY()

public:
	// 显示名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC|Tile")
	FString DisplayName;

	// Tile 纹理
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC|Tile")
	UTexture2D* TileTexture = nullptr;

	// 四个方向的边缘 Socket [Up, Right, Down, Left]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC|Edges", meta=(ArrayClamp="4"))
	TArray<FWFCEdgeSocket> EdgeSockets;

	// 生成权重
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC|Tile", meta=(ClampMin="0.01"))
	float Weight = 1.0f;

	// 便捷访问（数组在 PostInitProperties 中保证为 4 元素）
	FWFCEdgeSocket& GetEdge(EWFCDirection Dir) { return EdgeSockets[static_cast<uint8>(Dir)]; }
	const FWFCEdgeSocket& GetEdge(EWFCDirection Dir) const { return EdgeSockets[static_cast<uint8>(Dir)]; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
};

// ========================
// TileSet 数据资产
// ========================
UCLASS(BlueprintType)
class WFCRUNTIME_API UWFCTileSet : public UDataAsset
{
	GENERATED_BODY()

public:
	// TileSet 名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WFC")
	FString TileSetName;

	// 所有 Tile
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WFC", meta=(TitleProperty="DisplayName"))
	TArray<UWFCTileData*> Tiles;

	// 查询某个 Tile 在指定方向上的兼容 Tile 索引列表
	TArray<int32> GetCompatibleTiles(int32 TileIndex, EWFCDirection Direction) const;

	// 查询两个 Tile 是否在指定方向上兼容
	bool AreCompatible(int32 TileA, int32 TileB, EWFCDirection DirectionFromAToB) const;

	// 获取所有已使用的唯一 Socket ID
	TArray<FString> GetUniqueSocketIDs() const;

#if WITH_EDITOR
	// 编辑器中重建兼容性缓存
	void RebuildCache();
#endif
};
