#include "GrassSpringSimulation.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "ShaderParameterStruct.h"

class FGrassSpringCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FGrassSpringCS);
    SHADER_USE_PARAMETER_STRUCT(FGrassSpringCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, PreviousState)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, NextState)
        SHADER_PARAMETER(uint32, Resolution)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, Stiffness)
        SHADER_PARAMETER(float, Damping)
        SHADER_PARAMETER(float, ForceStrength)
        SHADER_PARAMETER(float, FieldSize)
        SHADER_PARAMETER(FVector2f, BrushPosition)
        SHADER_PARAMETER(float, BrushRadius)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

IMPLEMENT_GLOBAL_SHADER(FGrassSpringCS, "/GrassInteraction/GrassSpring.usf", "MainCS", SF_Compute);

void DispatchGrassSpring(UTextureRenderTarget2D* ReadRT, UTextureRenderTarget2D* WriteRT, FGrassSpringStep Step)
{
    check(ReadRT != WriteRT);
    FTextureRenderTargetResource* ReadResource = ReadRT->GameThread_GetRenderTargetResource();
    FTextureRenderTargetResource* WriteResource = WriteRT->GameThread_GetRenderTargetResource();
    const int32 Resolution = ReadRT->SizeX;

    ENQUEUE_RENDER_COMMAND(GrassSpring)(
        [ReadResource, WriteResource, Resolution, Step](FRHICommandListImmediate& RHICmdList)
        {
            FRDGBuilder Graph(RHICmdList);
            FRDGTextureRef Read = Graph.RegisterExternalTexture(CreateRenderTarget(ReadResource->GetRenderTargetTexture(), TEXT("Grass.Read")));
            FRDGTextureRef Write = Graph.RegisterExternalTexture(CreateRenderTarget(WriteResource->GetRenderTargetTexture(), TEXT("Grass.Write")));
            auto* Params = Graph.AllocParameters<FGrassSpringCS::FParameters>();
            Params->PreviousState = Read;
            Params->NextState = Graph.CreateUAV(Write);
            Params->Resolution = Resolution;
            Params->DeltaTime = Step.DeltaTime;
            Params->Stiffness = Step.Stiffness;
            Params->Damping = Step.Damping;
            Params->ForceStrength = Step.ForceStrength;
            Params->FieldSize = Step.FieldSize;
            Params->BrushPosition = Step.BrushPosition;
            Params->BrushRadius = Step.BrushRadius;
            TShaderMapRef<FGrassSpringCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
            FComputeShaderUtils::AddPass(Graph, RDG_EVENT_NAME("Grass Spring: Read -> Write"), Shader, Params,
                FIntVector(FMath::DivideAndRoundUp(Resolution, 8), FMath::DivideAndRoundUp(Resolution, 8), 1));
            // WPO samples this texture on the graphics pipeline after this compute pass.
            Graph.SetTextureAccessFinal(Write, ERHIAccess::SRVMask);
            Graph.Execute();
        });
}
