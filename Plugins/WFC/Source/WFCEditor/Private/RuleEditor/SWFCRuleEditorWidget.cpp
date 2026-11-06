// ========================
// SWFCRuleEditorWidget.cpp (WFCEditor 模块)
// 邻接规则编辑器实现
// ========================
#include "RuleEditor/SWFCRuleEditorWidget.h"
#include "RuleEditor/WFCRuleAutoDetect.h"
#include "WFCTileData.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#include "DesktopPlatformModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "SWFCRuleEditorWidget"

// Tile 列表行的 Widget
class SWFCTileListRow : public SMultiColumnTableRow<TSharedPtr<FWFCRuleTileListItem>>
{
public:
	SLATE_BEGIN_ARGS(SWFCTileListRow) {}
		SLATE_ARGUMENT(TSharedPtr<FWFCRuleTileListItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
	{
		Item = InArgs._Item;

		// 预先创建 Brush 并保存为成员变量，确保生命周期与行一致
		if (Item.IsValid() && Item->Tile && Item->Tile->TileTexture)
		{
			ThumbnailBrush = MakeShareable(new FSlateBrush());
			ThumbnailBrush->SetResourceObject(Item->Tile->TileTexture);
			ThumbnailBrush->ImageSize = FVector2D(32, 32);
		}

		SMultiColumnTableRow::Construct(
			FSuperRowType::FArguments(),
			InOwnerTable
		);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		FString ColId = InColumnName.ToString();

		if (ColId == TEXT("Preview"))
		{
			return SNew(SBox)
				.WidthOverride(40)
				.HeightOverride(40)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(ThumbnailBrush.IsValid() ? ThumbnailBrush.Get() : nullptr)
				];
		}
		else if (ColId == TEXT("Name"))
		{
			FString Name = (Item.IsValid() && Item->Tile) ? (Item->Tile->DisplayName.IsEmpty()
				? FString::Printf(TEXT("Tile_%d"), Item->Index)
				: Item->Tile->DisplayName)
				: TEXT("None");
			return SNew(STextBlock)
				.Text(FText::FromString(Name));
		}
		else if (ColId == TEXT("Index"))
		{
			return SNew(STextBlock)
				.Text(FText::FromString(FString::FromInt(Item.IsValid() ? Item->Index : INDEX_NONE)));
		}

		return SNew(STextBlock).Text(FText::FromString(TEXT("")));
	}

private:
	TSharedPtr<FWFCRuleTileListItem> Item;
	TSharedPtr<FSlateBrush> ThumbnailBrush;  // 保持 Brush 生命周期与行一致
};

// ==============================
// SWFCRuleEditorWidget 实现
// ==============================

void SWFCRuleEditorWidget::Construct(const FArguments& InArgs)
{
	static const FName ColumnPreview("Preview");
	static const FName ColumnName("Name");
	static const FName ColumnIndex("Index");

	ChildSlot
	[
		SNew(SVerticalBox)
		// --- 工具栏 ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5)
			[
				SNew(SVerticalBox)
				// TileSet 路径行
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString("TileSet:"))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.VAlign(VAlign_Center)
					[
						SAssignNew(TileSetPathTextBox, SEditableTextBox)
						.Text(this, &SWFCRuleEditorWidget::GetTileSetPathText)
						.IsReadOnly(true)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0)
					[
						SNew(SButton)
						.Text(FText::FromString("Browse"))
						.OnClicked(this, &SWFCRuleEditorWidget::OnBrowseTileSet)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0)
					[
						SNew(SButton)
						.Text(FText::FromString("Save"))
						.OnClicked(this, &SWFCRuleEditorWidget::OnSaveTileSet)
					]
				]
				// 操作按钮行
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString("Tolerance:"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 15, 0)
					[
						SNew(SBox)
						.WidthOverride(100)
						[
							SNew(SNumericEntryBox<float>)
							.Value(this, &SWFCRuleEditorWidget::GetTolerance)
							.OnValueChanged(this, &SWFCRuleEditorWidget::OnToleranceChanged)
							.MinValue_Lambda([]() { return TOptional<float>(0.0f); })
							.MaxValue_Lambda([]() { return TOptional<float>(1.0f); })
							.Delta(0.01f)
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString("Auto Detect Rules"))
						.OnClicked(this, &SWFCRuleEditorWidget::OnAutoDetectRules)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0, 0, 0)
					[
						SNew(SButton)
						.Text(FText::FromString("Clear Rules"))
						.OnClicked(this, &SWFCRuleEditorWidget::OnClearRules)
					]
				]
			]
		]
		// --- 主内容区 ---
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(5)
		[
			SNew(SHorizontalBox)
			// 左侧：Tile 列表
			+ SHorizontalBox::Slot()
			.FillWidth(0.4f)
			.Padding(0, 0, 5, 0)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SAssignNew(TileListView, SListView<TSharedPtr<FWFCRuleTileListItem>>)
					.ListItemsSource(&TileListItems)
					.OnGenerateRow(this, &SWFCRuleEditorWidget::OnGenerateTileListRow)
					.OnSelectionChanged(this, &SWFCRuleEditorWidget::OnTileSelectionChanged)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+SHeaderRow::Column(ColumnPreview).FixedWidth(50.f).DefaultLabel(LOCTEXT("ColPreview", ""))
						+SHeaderRow::Column(ColumnName).FillWidth(1.f).DefaultLabel(LOCTEXT("ColName", "Tile Name"))
						+SHeaderRow::Column(ColumnIndex).FixedWidth(50.f).DefaultLabel(LOCTEXT("ColIndex", "#"))
					)
				]
			]
			// 右侧：详情面板
			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			[
				SAssignNew(DetailPanelBorder, SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Visibility(this, &SWFCRuleEditorWidget::GetDetailPanelVisibility)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(10)
					[
						SNew(SVerticalBox)
						// 选中 Tile 名称
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 10)
						[
							SNew(STextBlock)
							.Text(this, &SWFCRuleEditorWidget::GetSelectedTileName)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						]
						// 纹理预览
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 10)
						[
							SNew(SBox)
							.WidthOverride(128)
							.HeightOverride(128)
							.HAlign(HAlign_Center)
							[
								SAssignNew(SelectedTilePreview, SImage)
								.Image(this, &SWFCRuleEditorWidget::GetSelectedTileBrush)
							]
						]
						// 权重
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (SelectedItem.IsValid() && SelectedItem->Tile)
								{
									return FText::FromString(FString::Printf(
										TEXT("Weight: %.2f"), SelectedItem->Tile->Weight));
								}
								return FText::FromString("Weight: -");
							})
						]
						// 分隔线
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 5)
						[
							SNew(SSeparator)
						]
						// 边缘 Socket 编辑区
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 5)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Edge Sockets (SocketID):"))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						]
						// 四个方向的 Socket 输入
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 5)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(2)
							[
								MakeEdgeSocketRow(TEXT("Up    [0]:"), 0)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(2)
							[
								MakeEdgeSocketRow(TEXT("Right [1]:"), 1)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(2)
							[
								MakeEdgeSocketRow(TEXT("Down  [2]:"), 2)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(2)
							[
								MakeEdgeSocketRow(TEXT("Left  [3]:"), 3)
							]
						]
						// 分隔线
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 5)
						[
							SNew(SSeparator)
						]
						// 兼容 Tile 显示
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 5)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Compatible Tiles:"))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(CompatibleTilesText, STextBlock)
							.AutoWrapText(true)
							.Text(FText::FromString("Select a tile to see compatible tiles."))
						]
					]
				]
			]
		]
		// --- 状态栏 ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 2)
		[
			SAssignNew(StatusTextBlock, STextBlock)
			.Text(this, &SWFCRuleEditorWidget::GetStatusText)
		]
	];

	SetStatus(TEXT("Ready. Browse a TileSet asset to begin."));
}

TSharedRef<SWidget> SWFCRuleEditorWidget::MakeEdgeSocketRow(const FString& Label, int32 Direction)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 5, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.MinDesiredWidth(100)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SAssignNew(EdgeSocketTextBoxes[Direction], SEditableTextBox)
			.Text(this, &SWFCRuleEditorWidget::GetEdgeSocketText, Direction)
			.OnTextCommitted(this, &SWFCRuleEditorWidget::OnEdgeSocketCommitted, Direction)
		];
}

// === 回调 ===

FReply SWFCRuleEditorWidget::OnBrowseTileSet()
{
	// 直接从 ProjectContentDir 拼接，确保得到有效的磁盘绝对路径
	FString DefaultDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("WFC"));

	// 如果 WFC 目录不存在，回退到 Content 根目录
	if (!FPaths::DirectoryExists(DefaultDir))
	{
		DefaultDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	}

	UE_LOG(LogTemp, Log, TEXT("[WFC RuleEditor] Browse default dir: %s"), *DefaultDir);

	TArray<FString> OutFiles;
	if (FDesktopPlatformModule::Get()->OpenFileDialog(
		nullptr,
		TEXT("Select TileSet Asset"),
		DefaultDir,
		TEXT(""),
		TEXT("TileSet Files (*.uasset)|*.uasset|All Files (*.*)|*.*"),
		EFileDialogFlags::None,
		OutFiles))
	{
		if (OutFiles.Num() > 0)
		{
			FString FullPath = OutFiles[0];
			FString PackageName;
			if (FPackageName::TryConvertFilenameToLongPackageName(FullPath, PackageName))
			{
				LoadTileSet(PackageName);
			}
			else
			{
				SetStatus(TEXT("Error: Invalid asset path."));
			}
		}
	}
	return FReply::Handled();
}

FReply SWFCRuleEditorWidget::OnAutoDetectRules()
{
	if (!CurrentTileSet.IsValid())
	{
		SetStatus(TEXT("Error: No TileSet loaded!"));
		return FReply::Handled();
	}

	SetStatus(TEXT("Auto-detecting rules..."));

	FWFCRuleAutoDetectResult Result;
	bool Success = FWFCRuleAutoDetect::DetectRules(
		CurrentTileSet.Get(), AutoDetectTolerance, Result);

	if (Success)
	{
		SetStatus(FString::Printf(
			TEXT("Auto-detect complete: %d edges grouped into %d unique sockets."),
			Result.NumUniqueEdges, Result.NumSocketIDs));

		// 刷新 UI
		RefreshTileList();
		if (SelectedItem.IsValid())
		{
			RefreshSelectedTilePreview();
			UpdateCompatibleTilesDisplay();
		}
	}
	else
	{
		SetStatus(TEXT("Error: Auto-detect failed. Ensure all tiles have textures."));
	}

	return FReply::Handled();
}

FReply SWFCRuleEditorWidget::OnSaveTileSet()
{
	if (!CurrentTileSet.IsValid())
	{
		SetStatus(TEXT("Error: No TileSet loaded!"));
		return FReply::Handled();
	}

	CurrentTileSet->MarkPackageDirty();

	UPackage* Package = CurrentTileSet->GetOutermost();
	if (Package)
	{
		FString PackageFilename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (UPackage::SavePackage(Package, CurrentTileSet.Get(), *PackageFilename, SaveArgs))
		{
			SetStatus(TEXT("TileSet saved successfully."));
		}
		else
		{
			SetStatus(TEXT("Error: Failed to save TileSet."));
		}
	}
	return FReply::Handled();
}

FReply SWFCRuleEditorWidget::OnClearRules()
{
	if (!CurrentTileSet.IsValid())
	{
		SetStatus(TEXT("Error: No TileSet loaded!"));
		return FReply::Handled();
	}

	for (UWFCTileData* Tile : CurrentTileSet->Tiles)
	{
		if (Tile)
		{
			for (int32 i = 0; i < 4; ++i)
			{
				Tile->EdgeSockets[i].SocketID = TEXT("any");
				Tile->EdgeSockets[i].bSymmetric = true;
			}
		}
	}

	CurrentTileSet->Modify();
	CurrentTileSet->MarkPackageDirty();

	SetStatus(TEXT("All edge sockets reset to 'any' (no restrictions)."));
	RefreshTileList();
	if (SelectedItem.IsValid())
	{
		RefreshSelectedTilePreview();
		UpdateCompatibleTilesDisplay();
	}
	return FReply::Handled();
}

void SWFCRuleEditorWidget::OnToleranceChanged(float NewValue)
{
	AutoDetectTolerance = NewValue;
}

void SWFCRuleEditorWidget::OnTileSelectionChanged(
	TSharedPtr<FWFCRuleTileListItem> InItem, ESelectInfo::Type SelectInfo)
{
	SelectedItem = InItem;
	SelectedTileIndex = InItem.IsValid() ? InItem->Index : INDEX_NONE;
	RefreshSelectedTilePreview();
	UpdateCompatibleTilesDisplay();
}

void SWFCRuleEditorWidget::OnEdgeSocketCommitted(
	const FText& NewText, ETextCommit::Type CommitType, int32 Direction)
{
	if (!CurrentTileSet.IsValid() || !SelectedItem.IsValid() || !SelectedItem->Tile)
	{
		return;
	}

	UWFCTileData* Tile = SelectedItem->Tile;
	FWFCEdgeSocket& Socket = Tile->GetEdge(static_cast<EWFCDirection>(Direction));
	Socket.SocketID = NewText.ToString();

	Tile->Modify();
	CurrentTileSet->MarkPackageDirty();

	UpdateCompatibleTilesDisplay();
}

// === UI 属性获取 ===

FText SWFCRuleEditorWidget::GetTileSetPathText() const
{
	return FText::FromString(CurrentTileSetPath);
}

FText SWFCRuleEditorWidget::GetStatusText() const
{
	return FText::FromString(StatusMessage);
}

TOptional<float> SWFCRuleEditorWidget::GetTolerance() const
{
	return AutoDetectTolerance;
}

EVisibility SWFCRuleEditorWidget::GetDetailPanelVisibility() const
{
	return SelectedItem.IsValid() && SelectedItem->Tile ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SWFCRuleEditorWidget::GetEdgeSocketText(int32 Direction) const
{
	if (SelectedItem.IsValid() && SelectedItem->Tile)
	{
		const FWFCEdgeSocket& Socket = SelectedItem->Tile->EdgeSockets[Direction];
		return FText::FromString(Socket.SocketID);
	}
	return FText::GetEmpty();
}

FText SWFCRuleEditorWidget::GetSelectedTileName() const
{
	if (SelectedItem.IsValid() && SelectedItem->Tile)
	{
		if (!SelectedItem->Tile->DisplayName.IsEmpty())
		{
			return FText::FromString(SelectedItem->Tile->DisplayName);
		}
		return FText::FromString(FString::Printf(TEXT("Tile_%d"), SelectedItem->Index));
	}
	return FText::FromString("No Tile Selected");
}

const FSlateBrush* SWFCRuleEditorWidget::GetSelectedTileBrush() const
{
	return SelectedTilePreviewBrush.IsValid() ? SelectedTilePreviewBrush.Get() : nullptr;
}

// === 内部方法 ===

void SWFCRuleEditorWidget::LoadTileSet(const FString& AssetPath)
{
	CurrentTileSetPath = AssetPath;
	CurrentTileSet = LoadObject<UWFCTileSet>(nullptr, *AssetPath);

	if (CurrentTileSet.IsValid())
	{
		SetStatus(FString::Printf(TEXT("Loaded TileSet: %s (%d tiles)"),
			*AssetPath, CurrentTileSet->Tiles.Num()));
		RefreshTileList();
		SelectedItem.Reset();
		SelectedTileIndex = INDEX_NONE;
		SelectedTilePreviewBrush.Reset();
	}
	else
	{
		SetStatus(TEXT("Error: Failed to load TileSet asset."));
	}
}

void SWFCRuleEditorWidget::RefreshTileList()
{
	TileListItems.Empty();
	TileThumbnailBrushes.Empty();

	if (!CurrentTileSet.IsValid())
	{
		TileListView->RequestListRefresh();
		return;
	}

	for (int32 i = 0; i < CurrentTileSet->Tiles.Num(); ++i)
	{
		TSharedPtr<FWFCRuleTileListItem> Item = MakeShareable(new FWFCRuleTileListItem);
		Item->Tile = CurrentTileSet->Tiles[i];
		Item->Index = i;
		TileListItems.Add(Item);
	}

	TileListView->RequestListRefresh();
}

void SWFCRuleEditorWidget::RefreshSelectedTilePreview()
{
	if (SelectedItem.IsValid() && SelectedItem->Tile && SelectedItem->Tile->TileTexture)
	{
		SelectedTilePreviewBrush = MakeShareable(new FSlateBrush());
		SelectedTilePreviewBrush->SetResourceObject(SelectedItem->Tile->TileTexture);
		SelectedTilePreviewBrush->ImageSize = FVector2D(128, 128);
	}
	else
	{
		SelectedTilePreviewBrush.Reset();
	}
}

void SWFCRuleEditorWidget::UpdateCompatibleTilesDisplay()
{
	if (!CurrentTileSet.IsValid() || !SelectedItem.IsValid() || !SelectedItem->Tile)
	{
		if (CompatibleTilesText.IsValid())
		{
			CompatibleTilesText->SetText(FText::FromString("Select a tile to see compatible tiles."));
		}
		return;
	}

	FString ResultStr;
	int32 TileIdx = SelectedItem->Index;
	static const TCHAR* DirNames[] = { TEXT("Up"), TEXT("Right"), TEXT("Down"), TEXT("Left") };

	for (int32 Dir = 0; Dir < 4; ++Dir)
	{
		TArray<int32> Compatible = CurrentTileSet->GetCompatibleTiles(
			TileIdx, static_cast<EWFCDirection>(Dir));

		ResultStr += FString::Printf(TEXT("%s: "), DirNames[Dir]);
		if (Compatible.Num() == 0)
		{
			ResultStr += TEXT("None");
		}
		else
		{
			for (int32 i = 0; i < Compatible.Num(); ++i)
			{
				if (i > 0) ResultStr += TEXT(", ");
				ResultStr += FString::FromInt(Compatible[i]);
			}
		}
		ResultStr += TEXT("\n");
	}

	if (CompatibleTilesText.IsValid())
	{
		CompatibleTilesText->SetText(FText::FromString(ResultStr));
	}
}

void SWFCRuleEditorWidget::SetStatus(const FString& Message)
{
	StatusMessage = Message;
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(FText::FromString(StatusMessage));
	}
}

TSharedRef<ITableRow> SWFCRuleEditorWidget::OnGenerateTileListRow(
	TSharedPtr<FWFCRuleTileListItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SWFCTileListRow, OwnerTable)
		.Item(Item);
}

#undef LOCTEXT_NAMESPACE
