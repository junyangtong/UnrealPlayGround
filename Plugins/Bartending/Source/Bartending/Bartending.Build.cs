using UnrealBuildTool;
public class Bartending : ModuleRules
{
    public Bartending(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent"});
    }
}
