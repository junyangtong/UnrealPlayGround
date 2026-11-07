#include "BartendingDemo.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "ProceduralMeshComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCocktailTest, "Bartending.Recipe.PourStirReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCocktailTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Demo = World->SpawnActor<ABartendingDemo>();
    Demo->ResetDrink();
    TestFalse(TEXT("Invalid recipe is rejected"), Demo->PourLayer(-1));
    TestTrue(TEXT("First pour starts"), Demo->PourLayer(0));
    TestFalse(TEXT("Concurrent pour is rejected"), Demo->PourLayer(1));
    Demo->Tick(100.f);
    TestEqual(TEXT("One slot has one fifth height"), Demo->Fill, .2f);
    TestFalse(TEXT("Hitch completes animation"), Demo->bBusy);
    for (int32 I=1; I<5; ++I) { TestTrue(TEXT("Remaining slots accepted"),Demo->PourLayer(I)); Demo->Tick(2.f); }
    TestFalse(TEXT("Full cup rejects overflow"), Demo->PourLayer(0));
    FLinearColor Mean(0,0,0,0);
    for (const auto& L : Demo->LiquidLayers) Mean += L.Color / 5.f;
    Demo->Stir(); Demo->Tick(4.f);
    for (const auto& L : Demo->LiquidLayers) TestTrue(TEXT("Stir conserves mean RGBA"),L.Color.Equals(Mean,.0001f));
    TestEqual(TEXT("Stir conserves fill height"), Demo->Fill, 1.f);
    Demo->ResetDrink(); Demo->PourLayer(2); Demo->Tick(.3f); Demo->ResetDrink(); Demo->Tick(3.f);
    TestEqual(TEXT("Reset cancels in-flight pour"),Demo->LiquidLayers.Num(),0);
    TestEqual(TEXT("Reset remains empty"),Demo->Fill,0.f);
    TestFalse(TEXT("Empty stir is a no-op"),Demo->bBusy);
    Demo->Stir(); TestFalse(TEXT("Empty stir does not start"),Demo->bBusy);
    Demo->PourLayer(0); Demo->Tick(2.f); Demo->PourLayer(1); Demo->Tick(2.f);
    Demo->Stir(); Demo->Tick(4.f);
    TestTrue(TEXT("Pour after stirring remains supported"),Demo->PourLayer(2)); Demo->Tick(2.f);
    TestEqual(TEXT("Mixed recipe retains correct fill height"),Demo->Fill,.6f);
    World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOriginalCocktailMeshTest, "Bartending.Geometry.OriginalFBXClipping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOriginalCocktailMeshTest::RunTest(const FString& Parameters)
{
    UStaticMesh* Source=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Demos/Bartending/Meshes/Liquid.Liquid"));
    if (!TestNotNull(TEXT("Original FBX liquid asset exists"),Source)) return false;
    TestTrue(TEXT("CPU clipping source is accessible"),Source->bAllowCPUAccess);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Demo=World->SpawnActor<ABartendingDemo>();
    Demo->LiquidSourceMesh=Source;
    Demo->ResetDrink();
    Demo->PourLayer(0); Demo->Tick(2.f);
    TArray<UProceduralMeshComponent*> Meshes; Demo->GetComponents(Meshes);
    auto* Liquid=*Meshes.FindByPredicate([](auto* M){return M->GetName()==TEXT("Liquid");});
    auto* Section=Liquid->GetProcMeshSection(0);
    TestTrue(TEXT("Clipped original mesh has triangles"),Section && Section->ProcIndexBuffer.Num()>0);
    if (Section)
    {
        for (const auto& Vertex:Section->ProcVertexBuffer)
        {
            if (Vertex.Position.Z>90.51f || !FMath::IsFinite(Vertex.Position.Z))
            { AddError(TEXT("Clipped vertex exceeds the raised contact-line height")); break; }
        }
        TestTrue(TEXT("Cap and side preserve original curved topology"),Section->ProcVertexBuffer.Num()>100);
    }
    Demo->PreviewFull();
    Section=Liquid->GetProcMeshSection(0);
    TestTrue(TEXT("Full source outline reaches original vessel width"),Section && Section->SectionLocalBox.GetSize().X>150.f);
    World->DestroyWorld(false);
    return true;
}
#endif
