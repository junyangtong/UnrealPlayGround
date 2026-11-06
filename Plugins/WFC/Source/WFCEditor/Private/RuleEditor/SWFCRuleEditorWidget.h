// ========================
// SWFCRuleEditorWidget.h (WFCEditor 模块)
// 邻接规则编辑器 - 可视化编辑 Tile 的边缘 Socket 并自动检测规则
// ========================
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SScrollBox.h"

class UWFCTileSet;
class UWFCTileData;
struct FSlateBrush;

// 列表中每个 Tile 的显示信息
struct FWFCRuleTileListItem
{
	UWFCTileData* Tile = nullptr;
	int32 Index = INDEX_NONE;
};

class SWFCRuleEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWFCRuleEditorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// === 回调 ===
	FReply OnBrowseTileSet();
	FReply OnAutoDetectRules();
	FReply OnSaveTileSet();
	FReply OnClearRules();
	void OnToleranceChanged(float NewValue);
	void OnTileSelectionChanged(TSharedPtr<FWFCRuleTileListItem> InItem, ESelectInfo::Type SelectInfo);
	void OnEdgeSocketCommitted(const FText& NewText, ETextCommit::Type CommitType, int32 Direction);

	// === UI 属性获取 ===
	FText GetTileSetPathText() const;
	FText GetStatusText() const;
	TOptional<float> GetTolerance() const;
	EVisibility GetDetailPanelVisibility() const;
	FText GetEdgeSocketText(int32 Direction) const;
	FText GetSelectedTileName() const;
	const FSlateBrush* GetSelectedTileBrush() const;

	// === 内部方法 ===
	void LoadTileSet(const FString& AssetPath);
	void RefreshTileList();
	void RefreshSelectedTilePreview();
	void UpdateCompatibleTilesDisplay();
	void SetStatus(const FString& Message);
	TSharedRef<SWidget> MakeEdgeSocketRow(const FString& Label, int32 Direction);
	TSharedRef<ITableRow> OnGenerateTileListRow(
		TSharedPtr<FWFCRuleTileListItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// === 数据 ===
	TWeakObjectPtr<UWFCTileSet> CurrentTileSet;
	FString CurrentTileSetPath;
	float AutoDetectTolerance = 0.05f;

	TArray<TSharedPtr<FWFCRuleTileListItem>> TileListItems;
	TSharedPtr<SListView<TSharedPtr<FWFCRuleTileListItem>>> TileListView;

	TSharedPtr<FWFCRuleTileListItem> SelectedItem;
	int32 SelectedTileIndex = INDEX_NONE;

	TSharedPtr<SEditableTextBox> TileSetPathTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SImage> SelectedTilePreview;
	TSharedPtr<SBorder> DetailPanelBorder;
	TSharedPtr<SScrollBox> DetailScrollBox;
	TSharedPtr<SEditableTextBox> EdgeSocketTextBoxes[4];
	TSharedPtr<FSlateBrush> SelectedTilePreviewBrush;
	TSharedPtr<STextBlock> CompatibleTilesText;
	TArray<TSharedPtr<FSlateBrush>> TileThumbnailBrushes;
	FString StatusMessage;
};
