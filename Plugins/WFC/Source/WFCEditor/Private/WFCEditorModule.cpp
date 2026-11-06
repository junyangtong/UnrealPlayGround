// ========================
// WFCEditorModule.cpp
// ========================
#include "WFCEditorModule.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "TileCutter/SWFCTileCutterWidget.h"
#include "RuleEditor/SWFCRuleEditorWidget.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FWFCEditorModule"

const FName FWFCEditorModule::TileCutterTabName(TEXT("WFCTileCutter"));
const FName FWFCEditorModule::RuleEditorTabName(TEXT("WFCRuleEditor"));

void FWFCEditorModule::StartupModule()
{
    // 注册 Tab Spawner
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TileCutterTabName,
        FOnSpawnTab::CreateRaw(this, &FWFCEditorModule::OnSpawnTileCutterTab))
        .SetDisplayName(LOCTEXT("TileCutterTitle", "WFC Tile Cutter"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        RuleEditorTabName,
        FOnSpawnTab::CreateRaw(this, &FWFCEditorModule::OnSpawnRuleEditorTab))
        .SetDisplayName(LOCTEXT("RuleEditorTitle", "WFC Rule Editor"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    // 延迟注册菜单
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWFCEditorModule::RegisterMenus));
}

void FWFCEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TileCutterTabName);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuleEditorTabName);
}

void FWFCEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    // 在顶部菜单栏添加 "WFC Tools" 菜单
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
    UToolMenu* WFCMenu = Menu->AddSubMenu(
        "LevelEditor.MainMenu",
        NAME_None,
        "WFCTools",
        LOCTEXT("WFCToolsMenu", "WFC Tools"),
        LOCTEXT("WFCToolsMenuTooltip", "Wave Function Collapse Tools")
    );

    FToolMenuSection& Section = WFCMenu->AddSection("WFCToolsSection",
        LOCTEXT("WFCToolsSectionLabel", "Tools"));

    Section.AddMenuEntry(
        "OpenTileCutter",
        LOCTEXT("TileCutterMenuEntry", "Tile Cutter"),
        LOCTEXT("TileCutterMenuEntryTooltip", "Open the tile cutting tool"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            FGlobalTabmanager::Get()->TryInvokeTab(TileCutterTabName);
        }))
    );

    Section.AddMenuEntry(
        "OpenRuleEditor",
        LOCTEXT("RuleEditorMenuEntry", "Rule Editor"),
        LOCTEXT("RuleEditorMenuEntryTooltip", "Open the adjacency rule editor"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([]()
        {
            FGlobalTabmanager::Get()->TryInvokeTab(RuleEditorTabName);
        }))
    );
}

TSharedRef<SDockTab> FWFCEditorModule::OnSpawnTileCutterTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SWFCTileCutterWidget)
        ];
}

TSharedRef<SDockTab> FWFCEditorModule::OnSpawnRuleEditorTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SWFCRuleEditorWidget)
		];
}

IMPLEMENT_MODULE(FWFCEditorModule, WFCEditor)

#undef LOCTEXT_NAMESPACE
