// ========================
// WFCEditorModule.h
// ========================
#pragma once
#include "Modules/ModuleManager.h"

class FWFCEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedRef<SDockTab> OnSpawnTileCutterTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> OnSpawnRuleEditorTab(const FSpawnTabArgs& Args);

	static const FName TileCutterTabName;
	static const FName RuleEditorTabName;
};
