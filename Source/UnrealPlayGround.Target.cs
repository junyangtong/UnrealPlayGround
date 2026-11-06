using UnrealBuildTool;
public class UnrealPlayGroundTarget : TargetRules
{
    public UnrealPlayGroundTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("UnrealPlayGround");
    }
}
