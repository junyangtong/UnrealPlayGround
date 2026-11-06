using UnrealBuildTool;
public class StainedGlass : ModuleRules
{
    public StainedGlass(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine", "ProceduralMeshComponent"});
    }
}
