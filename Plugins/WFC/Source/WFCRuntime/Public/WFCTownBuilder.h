#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WFCTownGrid.h"
#include "WFCTownVolume.h"
#include "WFCTownBuilder.generated.h"

class UProceduralMeshComponent;
class UCameraComponent;
class APlayerController;
class UWFCSquareModuleSet;

/** 关卡入口：维护建造状态和撤销历史，处理输入，调用网格与几何生成器。 */
UCLASS(Blueprintable, meta=(DisplayName="WFC Town Builder"))
class WFCRUNTIME_API AWFCTownBuilder : public AActor
{
	GENERATED_BODY()
public:
	AWFCTownBuilder();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Grid", meta=(ClampMin="1", ClampMax="12"))
	int32 Radius = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Grid", meta=(ClampMin="10", ClampMax="10000"))
	float Spacing = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Grid")
	int32 Seed = 42;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Grid", meta=(ClampMin="0", ClampMax="50"))
	int32 RelaxIterations = 12;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Building", meta=(ClampMin="10", ClampMax="1000"))
	float FloorHeight = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Building", meta=(ClampMin="1", ClampMax="20"))
	int32 MaxFloors = 6;
	// Supply a material that reads VertexColor for colored roofs, walls and hover.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Town|Appearance")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Interaction")
	bool bEnableDemoControls = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Town|Marching Squares")
	bool bUseMarchingSquares = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Town|Marching Squares")
	TObjectPtr<UWFCSquareModuleSet> SquareModules;
	UFUNCTION(BlueprintCallable, Category="Town|Marching Squares")
	bool SetVertexOccupied(int32 Vertex, bool bOccupied);
	UFUNCTION(BlueprintPure, Category="Town|Marching Squares")
	int32 GetSquareMask(int32 Cell) const;
	UFUNCTION(BlueprintPure, Category="Town|Marching Squares")
	int32 GetVertexCount() const { return Grid.Vertices.Num(); }
	UFUNCTION(BlueprintPure, Category="Town|Marching Squares")
	bool IsVertexOccupied(int32 Vertex) const { return Volume.IsOccupied(Vertex); }

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Town")
	void ResetTown();
	UFUNCTION(BlueprintCallable, Category="Town")
	bool EditCell(int32 Cell, int32 Delta);
	UFUNCTION(BlueprintCallable, Category="Town")
	void Undo();
	UFUNCTION(BlueprintCallable, Category="Town")
	void Redo();
	UFUNCTION(BlueprintPure, Category="Town")
	int32 GetCellCount() const { return Grid.Quads.Num(); }
	UFUNCTION(BlueprintPure, Category="Town")
	int32 GetCellHeight(int32 Cell) const { return Heights.IsValidIndex(Cell) ? Heights[Cell] : 0; }

private:
	UPROPERTY(VisibleAnywhere, Category="Town")
	TObjectPtr<UProceduralMeshComponent> TownMesh;
	UPROPERTY(VisibleAnywhere, Category="Town")
	TObjectPtr<UCameraComponent> Camera;
	// 旧挤出模式的单元高度；Marching Squares 使用下方 Volume 的共享顶点占用。
	// 保留属性名，兼容已有演示关卡的序列化数据。
	UPROPERTY()
	TArray<int32> Heights;
	UPROPERTY()
	uint32 GridSignature = 0;
	UPROPERTY()
	FWFCTownVolume Volume;
	FWFCTownGrid Grid;
	struct FEdit { int32 Cell; int32 Before; int32 After; bool bVertexEdit = false; };
	TArray<FEdit> UndoStack, RedoStack;
	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<AActor> PreviousView;
	bool bPreviousCursor = false;
	int32 HoverCell = INDEX_NONE;
	int32 HoverVertex = INDEX_NONE;
	float Yaw = -45.f;
	float Pitch = 55.f;
	float Distance = 2500.f;
	// 生命周期和调度。几何计算放在 FWFCTownMeshGenerator 中。
	void InitializeTown();
	void HandleCameraInput();
	void HandleBuildInput();
	void ShowControlsHint() const;
	void Rebuild();
	void UpdateCamera();
	int32 PickCell() const;
	int32 PickVertex() const;
};
