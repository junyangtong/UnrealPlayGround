#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WFCTownMeshGenerator.h"
#include "WFCTownVolume.h"
#include "WFCMarchingSquares.generated.h"

class UStaticMesh;

/** 模型坐标范围：XY=[0,100]，Z=[0,100]；四角 0,1,2,3 逆时针。
 * 3----2
 * |    |
 * 0----1
 * 数组索引就是 V0 + 2*V1 + 4*V2 + 8*V3。0 表示空，不需要模型。
 */
UCLASS(BlueprintType)
class WFCRUNTIME_API UWFCSquareModuleSet : public UDataAsset
{
	GENERATED_BODY()
public:
	UWFCSquareModuleSet() { Modules.SetNum(16); }
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Marching Squares", meta=(EditFixedSize))
	TArray<TObjectPtr<UStaticMesh>> Modules;
};

/** 占用编码 → 模型 → 双线性变形。与鼠标输入和 Actor 生命周期无关。 */
struct WFCRUNTIME_API FWFCMarchingSquares
{
	// 5/10（对角占用）固定使用两个不连通角块，避免歧义。
	static FWFCTownMeshData BuildSample(uint8 Mask);
	static FVector DeformPoint(const FVector& Point, const FWFCTownQuad& Quad,
		const FWFCTownGrid& Grid, float Height);
	static FWFCTownMeshData BuildMesh(const FWFCTownGrid& Grid, const FWFCTownVolume& Volume,
		const UWFCSquareModuleSet* Modules, float Height, int32 HoverVertex);
};
