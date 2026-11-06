using UnrealBuildTool;

public class WFCEditor : ModuleRules
{
	public WFCEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"WFCRuntime"    // 依赖运行时模块的数据类型
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd",
			"EditorStyle",
			"InputCore",
			"ToolMenus",
			"PropertyEditor",
			"AssetTools",
			"ContentBrowser",
			"DesktopPlatform",   // 文件对话框
			"ImageWrapper",      // 图片读写
			"ImageCore",         // FImage, FImageView 等图像处理类
			"AssetRegistry",
			"EditorScriptingUtilities",
			"MeshDescription",
			"StaticMeshDescription"
		});
	}
}
