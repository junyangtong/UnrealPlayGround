// Private/TileCutter/SWFCTileCutterWidget.h
#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "TileCutter/WFCTileCutterLogic.h"

class UTexture2D;
class UWFCTileSet;
class SUniformGridPanel;
struct FSlateBrush;

class SWFCTileCutterWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWFCTileCutterWidget) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);

private:
	// --- UI 回调 ---
	FReply OnBrowseSourceTexture();
	FReply OnCutClicked();
	FReply OnAutoCreateTileSetClicked();
	void OnRowsChanged(int32 NewValue);
	void OnColsChanged(int32 NewValue);
	void OnSavePathCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void OnAssetPrefixCommitted(const FText& NewText, ETextCommit::Type CommitType);

	// --- UI 属性获取 ---
	FText GetSourceTexturePathText() const;
	FText GetSaveDirectoryText() const;
	FText GetAssetPrefixText() const;
	TOptional<int32> GetNumRows() const;
	TOptional<int32> GetNumCols() const;
	const FSlateBrush* GetPreviewBrush() const;

	// 缩略图刷新
	void RefreshPreview();
	void RefreshResultGrid();

	// --- 数据 ---
	// 源纹理
	TWeakObjectPtr<UTexture2D> SourceTexture;
	FString SourceTexturePath;

	// 参数
	int32 NumRows = 4;
	int32 NumCols = 4;
	FString SaveDirectory = TEXT("/Game/Demos/WFC/Tiles/");
	FString AssetPrefix = TEXT("Tile_");

	// 切图结果
	FWFCCutResult CutResult;

	// Slate 引用
	TSharedPtr<SImage> PreviewImage;
	TSharedPtr<SUniformGridPanel> ResultGridPanel;
	TSharedPtr<STextBlock> StatusText;

	// 预览 Brush
	TSharedPtr<FSlateBrush> PreviewBrush;

	// 结果网格的 Brush 列表（保持引用防止被销毁）
	TArray<TSharedPtr<FSlateBrush>> ResultTileBrushes;
};
