using UnrealBuildTool;

public class GrassInteraction : ModuleRules
{
    public GrassInteraction(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "ProceduralMeshComponent" });
        PrivateDependencyModuleNames.AddRange(new[] { "Projects", "RenderCore", "RHI", "InputCore" });
    }
}
