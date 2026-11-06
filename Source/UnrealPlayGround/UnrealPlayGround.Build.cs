using UnrealBuildTool;
public class UnrealPlayGround : ModuleRules
{
    public UnrealPlayGround(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine"});
    }
}
