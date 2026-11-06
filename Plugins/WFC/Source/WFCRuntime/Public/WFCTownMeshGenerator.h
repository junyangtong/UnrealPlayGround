#pragma once

#include "CoreMinimal.h"
#include "WFCTownGrid.h"

/** 可交给 ProceduralMeshComponent 的纯数据；全部使用 Actor 局部坐标。 */
struct WFCRUNTIME_API FWFCTownMeshData
{
	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<int32> Indices;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
};

/** 无状态的几何计算，不持有 Actor、组件或输入控制器。
 * 当前只生成挤出原型；以后在这里接入编码驱动的模型匹配与变形。
 */
struct WFCRUNTIME_API FWFCTownMeshGenerator
{
	// Heights 与 Grid.Quads 一一对应；不修改输入的网格和建造状态。
	static FWFCTownMeshData BuildExtrudedMesh(const FWFCTownGrid& Grid,
		const TArray<int32>& Heights, float FloorHeight, int32 HoverCell = INDEX_NONE);

	// 双面拾取逻辑顶面和外墙；射线输入为局部坐标，返回最近单元或 INDEX_NONE。
	static int32 RaycastCell(const FWFCTownGrid& Grid, const TArray<int32>& Heights,
		float FloorHeight, const FVector& Origin, const FVector& Direction);
};
