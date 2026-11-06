#if WITH_DEV_AUTOMATION_TESTS

#include "GrassSpringSimulation.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/AutomationTest.h"
#include "RenderingThread.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrassSpringGPUTest, "GrassInteraction.Spring.GPUFeedbackAndRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrassSpringGPUTest::RunTest(const FString& Parameters)
{
    // Exercise the actual GPU shader, including negative channels and alpha velocity.
    TStrongObjectPtr<UTextureRenderTarget2D> A(NewObject<UTextureRenderTarget2D>());
    TStrongObjectPtr<UTextureRenderTarget2D> B(NewObject<UTextureRenderTarget2D>());
    for (UTextureRenderTarget2D* RT : {A.Get(), B.Get()})
    {
        RT->RenderTargetFormat = RTF_RGBA16f;
        RT->ClearColor = FLinearColor::Transparent;
        RT->bCanCreateUAV = true;
        RT->InitAutoFormat(16, 16);
        RT->UpdateResourceImmediate(true);
    }
    UTextureRenderTarget2D* Current = A.Get();
    FGrassSpringStep Step{1.f / 120.f, 14.f, 3.f, 1100.f, 256.f, FVector2f::ZeroVector, 120.f};
    auto Advance = [&](int32 Count)
    {
        for (int32 I = 0; I < Count; ++I)
        {
            UTextureRenderTarget2D* Write = Current == A.Get() ? B.Get() : A.Get();
            DispatchGrassSpring(Current, Write, Step);
            Current = Write;
        }
        FlushRenderingCommands();
    };
    auto Read = [&]()
    {
        TArray<FFloat16Color> Pixels;
        const bool bRead = Current->GameThread_GetRenderTargetResource()->ReadFloat16Pixels(Pixels);
        TestTrue(TEXT("GPU float readback succeeded"), bRead && Pixels.Num() == 256);
        return Pixels;
    };
    Advance(19); // Odd count must leave B as the latest state.
    TestTrue(TEXT("Odd step swaps to B"), Current == B.Get());
    const auto Forced = Read();
    if (Forced.Num() != 256) return false;
    const FLinearColor Positive = Forced[8 * 16 + 10].GetFloats();
    const FLinearColor Negative = Forced[7 * 16 + 5].GetFloats();
    TestTrue(TEXT("Force creates positive XY displacement"), Positive.R > 1.f && Positive.G > 0.1f);
    TestTrue(TEXT("BA including alpha stores positive XY velocity"), Positive.B > 1.f && Positive.A > 0.1f);
    TestTrue(TEXT("All four channels preserve negative values"),
        Negative.R < -1.f && Negative.G < -0.1f && Negative.B < -1.f && Negative.A < -0.1f);
    TestEqual(TEXT("Unforced far corner stays at rest"), Forced[0].GetFloats(), FLinearColor::Transparent);

    // Remove force. Springs must still move, reverse velocity, and eventually settle.
    Step.ForceStrength = 0.f;
    Advance(120);
    const auto Returning = Read();
    if (Returning.Num() != 256) return false;
    TestTrue(TEXT("Velocity reverses after force removal"), Returning[8 * 16 + 10].B.GetFloat() < 0.f);
    Advance(600);
    const auto Rested = Read();
    if (Rested.Num() != 256) return false;
    float MaxPosition = 0.f;
    float MaxVelocity = 0.f;
    for (const FFloat16Color& Pixel : Rested)
    {
        const FLinearColor P = Pixel.GetFloats();
        if (!FMath::IsFinite(P.R) || !FMath::IsFinite(P.G) || !FMath::IsFinite(P.B) || !FMath::IsFinite(P.A))
        {
            AddError(TEXT("Nonfinite spring state"));
            return false;
        }
        MaxPosition = FMath::Max(MaxPosition, FMath::Max(FMath::Abs(P.R), FMath::Abs(P.G)));
        MaxVelocity = FMath::Max(MaxVelocity, FMath::Max(FMath::Abs(P.B), FMath::Abs(P.A)));
    }
    TestTrue(TEXT("Recovers to within 0.05 cm"), MaxPosition < 0.05f);
    TestTrue(TEXT("Velocity settles below 0.1 cm/s"), MaxVelocity < 0.1f);
    AddInfo(FString::Printf(TEXT("GPU spring verified: forced RGBA=(%.3f, %.3f, %.3f, %.3f), final max displacement=%.6f cm, velocity=%.6f cm/s"),
        Positive.R, Positive.G, Positive.B, Positive.A, MaxPosition, MaxVelocity));
    FlushRenderingCommands();
    return true;
}

#endif
