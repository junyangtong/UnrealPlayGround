#include "WFCTownMeshGenerator.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCTownMeshTest, "WFC.Town.MeshAndPicking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWFCTownMeshTest::RunTest(const FString& Parameters)
{
	// 两个共享边的单位方格，用确定的几何检查内墙剔除和最近命中。
	FWFCTownGrid Grid;
	Grid.Vertices = {{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}, {2,0,0}, {2,1,0}};
	FWFCTownQuad Left, Right;
	Left.Vertices = {0,1,2,3}; Left.Neighbors = {-1,1,-1,-1};
	Right.Vertices = {1,4,5,2}; Right.Neighbors = {-1,-1,-1,0};
	Grid.Quads = {Left, Right};
	const TArray<int32> EqualHeights = {1,1};
	const auto EqualMesh = FWFCTownMeshGenerator::BuildExtrudedMesh(Grid, EqualHeights, 10.f);
	TestEqual(TEXT("Two top surfaces with rims and six exterior walls"), EqualMesh.Positions.Num(), 64);
	TestEqual(TEXT("Consistent normal count"), EqualMesh.Normals.Num(), EqualMesh.Positions.Num());
	TestEqual(TEXT("Consistent UV count"), EqualMesh.UVs.Num(), EqualMesh.Positions.Num());
	TestEqual(TEXT("Consistent color count"), EqualMesh.Colors.Num(), EqualMesh.Positions.Num());
	for (int32 Index : EqualMesh.Indices) TestTrue(TEXT("Valid triangle index"), EqualMesh.Positions.IsValidIndex(Index));
	const TArray<int32> UnequalHeights = {1,2};
	const auto UnequalMesh = FWFCTownMeshGenerator::BuildExtrudedMesh(Grid, UnequalHeights, 10.f);
	TestEqual(TEXT("Height difference exposes one extra wall"), UnequalMesh.Positions.Num(), 68);
	TestEqual(TEXT("Ray hits left roof"), FWFCTownMeshGenerator::RaycastCell(Grid, UnequalHeights, 10.f,
		FVector(0.5,0.5,100), FVector(0,0,-1)), 0);
	TestEqual(TEXT("Ray hits nearest exterior wall"), FWFCTownMeshGenerator::RaycastCell(Grid, UnequalHeights, 10.f,
		FVector(3,0.5,5), FVector(-1,0,0)), 1);
	TestEqual(TEXT("Ray misses the town"), FWFCTownMeshGenerator::RaycastCell(Grid, UnequalHeights, 10.f,
		FVector(3,3,100), FVector(0,0,-1)), INDEX_NONE);
	return true;
}
#endif
