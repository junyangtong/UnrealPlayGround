#include "WFCMarchingSquares.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCSquaresTest, "WFC.Town.MarchingSquares",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWFCSquaresTest::RunTest(const FString& Parameters)
{
	FWFCTownGrid Grid;
	Grid.Vertices = {{0,0,0},{100,0,0},{100,100,0},{0,100,0},{200,0,0},{200,100,0}};
	FWFCTownQuad Left, Right;
	Left.Vertices = {0,1,2,3}; Left.Neighbors = {-1,1,-1,-1};
	Right.Vertices = {1,4,5,2}; Right.Neighbors = {-1,-1,-1,0};
	Grid.Quads = {Left,Right};
	FWFCTownVolume Volume;
	Volume.Initialize(Grid,true);
	const double ExpectedArea[] = {0,1250,1250,5000,1250,2500,5000,8750,1250,5000,2500,8750,5000,8750,8750,10000};
	for (int32 Mask = 0; Mask < 16; ++Mask)
	{
		for (int32 V = 0; V < 4; ++V) Volume.SetOccupied(V, (Mask & (1 << V)) != 0);
		TestEqual(TEXT("All four-bit cases encode correctly"), int32(Volume.GetSquareMask(Left)), Mask);
		const auto Sample = FWFCMarchingSquares::BuildSample(Mask);
		double TopArea = 0;
		for (int32 I = 0; I < Sample.Indices.Num(); I += 3)
		{
			const FVector A = Sample.Positions[Sample.Indices[I]];
			const FVector B = Sample.Positions[Sample.Indices[I+1]];
			const FVector C = Sample.Positions[Sample.Indices[I+2]];
			const double Area = FVector::CrossProduct(B-A,C-A).Size() * 0.5;
			TestTrue(TEXT("No degenerate sample triangles"), Area > 0.001);
			if (A.Z == 100 && B.Z == 100 && C.Z == 100) TopArea += Area;
		}
		TestTrue(TEXT("Correct occupied footprint including diagonal cases"), FMath::IsNearlyEqual(TopArea,ExpectedArea[Mask],0.01));
	}
	Volume.Initialize(Grid,true);
	Volume.SetOccupied(1,true);
	TestEqual(TEXT("Shared vertex uses bit1 in left cell"), int32(Volume.GetSquareMask(Left)),2);
	TestEqual(TEXT("Shared vertex uses bit0 in right cell"), int32(Volume.GetSquareMask(Right)),1);
	TestEqual(TEXT("Shared vertex tracks both cells"), Volume.VertexCells[1].Num(),2);
	TestFalse(TEXT("Invalid vertex rejected"),Volume.SetOccupied(-1,true));
	Volume.Initialize(Grid,false);
	TestTrue(TEXT("Rebuild preserves occupancy"),Volume.IsOccupied(1));
	// 扭曲共享边后，两侧参数化仍需落在同一条边上。
	Grid.Vertices[2] = FVector(120,130,0);
	for (int32 Step = 0; Step <= 10; ++Step)
	{
		const double Y = Step * 10.;
		TestTrue(TEXT("Deformed shared edges stay watertight"),
			FWFCMarchingSquares::DeformPoint(FVector(100,Y,50),Left,Grid,40).Equals(
			FWFCMarchingSquares::DeformPoint(FVector(0,Y,50),Right,Grid,40),0.001));
	}
	return true;
}
#endif
