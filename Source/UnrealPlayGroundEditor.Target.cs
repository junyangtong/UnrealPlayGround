using UnrealBuildTool;
public class UnrealPlayGroundEditorTarget : TargetRules
{
    public UnrealPlayGroundEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("UnrealPlayGround");
    }
}
