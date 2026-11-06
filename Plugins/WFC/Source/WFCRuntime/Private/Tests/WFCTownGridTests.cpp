#include "WFCTownGrid.h"
#include "HexData.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWFCTownTopologyTest, "WFC.Town.Topology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWFCTownTopologyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Default cube coordinate initializes to origin"), FHexCoord().ToWorldPosition(100), FVector::ZeroVector);
	for (int32 Radius : {1, 2, 4, 8, 12})
	{
		for (int32 Seed : {0, 1, 42, -17})
		{
			FWFCTownGrid Grid, Repeat, Unrelaxed;
			Grid.Generate(Radius, 100, Seed, 20);
			Repeat.Generate(Radius, 100, Seed, 20);
			Unrelaxed.Generate(Radius, 100, Seed, 0);
			TestTrue(TEXT("Convex cells and reciprocal adjacency"), Grid.IsValid());
			TestTrue(TEXT("Deterministic vertices"), Grid.Vertices == Repeat.Vertices);
			TestEqual(TEXT("Deterministic cell count"), Grid.Quads.Num(), Repeat.Quads.Num());
			TSet<FIntPoint> Edges;
			double Area = 0;
			for (int32 Q = 0; Q < Grid.Quads.Num(); ++Q)
			{
				const auto& Quad = Grid.Quads[Q];
				TestTrue(TEXT("Deterministic cell ordering"), Quad.Vertices == Repeat.Quads[Q].Vertices);
				for (int32 E = 0; E < 4; ++E)
				{
					const int32 A = Quad.Vertices[E], B = Quad.Vertices[(E + 1) % 4];
					Edges.Add(FIntPoint(FMath::Min(A, B), FMath::Max(A, B)));
					Area += (Grid.Vertices[A].X * Grid.Vertices[B].Y - Grid.Vertices[A].Y * Grid.Vertices[B].X) * 0.5;
				}
			}
			TestEqual(TEXT("Disk Euler characteristic: no holes or disconnected cells"), Grid.Vertices.Num() - Edges.Num() + Grid.Quads.Num(), 1);
			const double ExpectedArea = 4.5 * FMath::Sqrt(3.0) * 10000.0 * Radius * Radius;
			TestTrue(TEXT("Subdivision preserves full hexagonal area"), FMath::Abs(Area - ExpectedArea) < ExpectedArea * 1.e-5);
			for (int32 V : Grid.BoundaryVertices) TestEqual(TEXT("Boundary remains fixed"), Grid.Vertices[V], Unrelaxed.Vertices[V]);
		}
	}
	return true;
}
#endif
