#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WFCSquareSampleLibrary.generated.h"

class UWFCSquareModuleSet;
class UMaterialInterface;

/** 编辑器工具：把程序化样例保存为可替换、可拖入场景的模型资产。 */
UCLASS()
class WFCEDITOR_API UWFCSquareSampleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="WFC|Samples")
	static UWFCSquareModuleSet* CreateSquareSamples(UMaterialInterface* Material, bool bRebuildSamples = false);
};
