// Private/TileCutter/SWFCTileCutterWidget.cpp
#include "TileCutter/SWFCTileCutterWidget.h"
#include "DesktopPlatformModule.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/Texture2D.h"
#include "TileCutter/WFCTileCutterLogic.h"
#include "EditorStyleSet.h"
#include "Misc/PackageName.h"
#include "WFCTileData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

void SWFCTileCutterWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SBorder)
            .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
            .Content()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString("Source Texture:"))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(2)
                    [
                        SNew(SEditableTextBox)
                        .Text(this, &SWFCTileCutterWidget::GetSourceTexturePathText)
                        .IsReadOnly(true)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(5, 0, 0, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString("Browse"))
                        .OnClicked(this, &SWFCTileCutterWidget::OnBrowseSourceTexture)
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString("Rows:"))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SNumericEntryBox<int32>)
                        .Value(this, &SWFCTileCutterWidget::GetNumRows)
                        .OnValueChanged(this, &SWFCTileCutterWidget::OnRowsChanged)
                        .MinValue(1)
                        .MaxValue(100)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString("Columns:"))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SNumericEntryBox<int32>)
                        .Value(this, &SWFCTileCutterWidget::GetNumCols)
                        .OnValueChanged(this, &SWFCTileCutterWidget::OnColsChanged)
                        .MinValue(1)
                        .MaxValue(100)
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString("Save Directory:"))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(3)
                    [
                        SNew(SEditableTextBox)
                        .Text(this, &SWFCTileCutterWidget::GetSaveDirectoryText)
                        .OnTextCommitted(this, &SWFCTileCutterWidget::OnSavePathCommitted)
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString("Asset Prefix:"))
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(3)
                    [
                        SNew(SEditableTextBox)
                        .Text(this, &SWFCTileCutterWidget::GetAssetPrefixText)
                        .OnTextCommitted(this, &SWFCTileCutterWidget::OnAssetPrefixCommitted)
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(SButton)
                        .Text(FText::FromString("Cut Tiles"))
                        .OnClicked(this, &SWFCTileCutterWidget::OnCutClicked)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1)
                    [
                        SNew(SButton)
                        .Text(FText::FromString("Auto Create TileSet"))
                        .OnClicked(this, &SWFCTileCutterWidget::OnAutoCreateTileSetClicked)
                    ]
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .FillHeight(1)
        .Padding(10)
        [
            SNew(SBorder)
            .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
            .Content()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Preview:"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SAssignNew(PreviewImage, SImage)
                    .Image(this, &SWFCTileCutterWidget::GetPreviewBrush)
                    .ColorAndOpacity(FLinearColor::White)
                ]
                + SVerticalBox::Slot()
                .FillHeight(1)
                .Padding(5)
                [
                    SAssignNew(ResultGridPanel, SUniformGridPanel)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SAssignNew(StatusText, STextBlock)
            .Text(FText::FromString("Ready"))
        ]
    ];

    // 初始刷新预览
    RefreshPreview();
}

FText SWFCTileCutterWidget::GetSourceTexturePathText() const
{
    return FText::FromString(SourceTexturePath);
}

FText SWFCTileCutterWidget::GetSaveDirectoryText() const
{
    return FText::FromString(SaveDirectory);
}

FText SWFCTileCutterWidget::GetAssetPrefixText() const
{
    return FText::FromString(AssetPrefix);
}

TOptional<int32> SWFCTileCutterWidget::GetNumRows() const
{
    return NumRows;
}

TOptional<int32> SWFCTileCutterWidget::GetNumCols() const
{
    return NumCols;
}

const FSlateBrush* SWFCTileCutterWidget::GetPreviewBrush() const
{
    return PreviewBrush.IsValid() ? PreviewBrush.Get() : nullptr;
}

FReply SWFCTileCutterWidget::OnBrowseSourceTexture()
{
    // 默认定位到项目 Content/WFC 目录，不存在则回退到 Content 根目录
    FString DefaultDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("WFC"));
    if (!FPaths::DirectoryExists(DefaultDir))
    {
        DefaultDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    }

    TArray<FString> OutFiles;
    if (FDesktopPlatformModule::Get()->OpenFileDialog(
        nullptr,
        TEXT("Select Source Texture"),
        DefaultDir,
        TEXT(""),
        TEXT("Texture Files (*.uasset)|*.uasset|All Files (*.*)|*.*"),
        EFileDialogFlags::None,
        OutFiles
    ))
    {
        if (OutFiles.Num() > 0)
        {
            FString FullPath = OutFiles[0];
            FString PackageName = FPackageName::FilenameToLongPackageName(FullPath);
            FString AssetPath = PackageName;
            if (!PackageName.StartsWith(TEXT("/Game/")) && !PackageName.StartsWith(TEXT("/Engine/")))
            {
                FString ProjectName = FApp::GetProjectName();
                FString RelativePath = FullPath;
                if (FPackageName::TryConvertFilenameToLongPackageName(FullPath, PackageName))
                {
                    AssetPath = PackageName;
                }
            }
            SourceTexturePath = AssetPath;
            SourceTexture = LoadObject<UTexture2D>(nullptr, *AssetPath);
            if (SourceTexture.IsValid())
            {
                StatusText->SetText(FText::FromString(FString::Printf(TEXT("Loaded: %s"), *SourceTexturePath)));
                RefreshPreview();
            }
            else
            {
                StatusText->SetText(FText::FromString(FString::Printf(TEXT("Failed to load: %s"), *SourceTexturePath)));
            }
        }
    }

    return FReply::Handled();
}

FReply SWFCTileCutterWidget::OnCutClicked()
{
    if (!SourceTexture.IsValid())
    {
        StatusText->SetText(FText::FromString("Error: No source texture selected!"));
        return FReply::Handled();
    }

    StatusText->SetText(FText::FromString("Cutting tiles..."));

    // 执行切割
    bool Success = FWFCTileCutterLogic::CutAtlas(
        SourceTexture.Get(),
        NumRows,
        NumCols,
        SaveDirectory,
        AssetPrefix,
        CutResult
    );

    if (Success)
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("Success! Created %d tiles."), CutResult.Tiles.Num())));
        RefreshResultGrid();
    }
    else
    {
        StatusText->SetText(FText::FromString("Error: Failed to cut tiles!"));
    }

    return FReply::Handled();
}

FReply SWFCTileCutterWidget::OnAutoCreateTileSetClicked()
{
    if (CutResult.Tiles.Num() == 0)
    {
        StatusText->SetText(FText::FromString("Error: No tiles cut yet! Click 'Cut Tiles' first."));
        return FReply::Handled();
    }

    StatusText->SetText(FText::FromString("Creating TileSet..."));

    // 构建 TileSet 保存路径
    FString TileSetDir = SaveDirectory;
    FString TileSetAssetName = AssetPrefix + TEXT("TileSet");
    FString FullPackagePath = TileSetDir / TileSetAssetName;
    FullPackagePath = FPackageName::ObjectPathToPackageName(FullPackagePath);

    UPackage* Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        StatusText->SetText(FText::FromString(TEXT("Error: Failed to create TileSet package.")));
        return FReply::Handled();
    }
    Package->FullyLoad();

    // 创建 TileSet DataAsset
    UWFCTileSet* TileSet = NewObject<UWFCTileSet>(Package, *TileSetAssetName,
        RF_Public | RF_Standalone);
    TileSet->TileSetName = TileSetAssetName;

    // 为每个切割结果创建 UWFCTileData
    for (int32 i = 0; i < CutResult.Tiles.Num(); ++i)
    {
        UWFCTileData* TileData = NewObject<UWFCTileData>(TileSet,
            *FString::Printf(TEXT("Tile_%d"), i), RF_Public);

        TileData->DisplayName = FString::Printf(TEXT("Tile_%d_%d"),
            CutResult.GridPositions[i].Y, CutResult.GridPositions[i].X);
        TileData->TileTexture = CutResult.Tiles[i];
        TileData->Weight = 1.0f;

        // 初始化所有边缘为 "any"（后续在 Rule Editor 中 auto-detect）
        for (int32 Dir = 0; Dir < 4; ++Dir)
        {
            TileData->EdgeSockets[Dir].SocketID = TEXT("any");
            TileData->EdgeSockets[Dir].bSymmetric = true;
        }

        TileSet->Tiles.Add(TileData);
    }

    TileSet->PostEditChange();
    Package->MarkPackageDirty();

    FAssetRegistryModule::AssetCreated(TileSet);

    // 保存到磁盘
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    FString PackageFilename = FPackageName::LongPackageNameToFilename(
        FullPackagePath, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, TileSet, *PackageFilename, SaveArgs);

    StatusText->SetText(FText::FromString(FString::Printf(
        TEXT("TileSet created: %s (%d tiles). Open Rule Editor to auto-detect adjacency rules."),
        *FullPackagePath, CutResult.Tiles.Num())));

    return FReply::Handled();
}

void SWFCTileCutterWidget::OnRowsChanged(int32 NewValue)
{
    NumRows = NewValue;
}

void SWFCTileCutterWidget::OnColsChanged(int32 NewValue)
{
    NumCols = NewValue;
}

void SWFCTileCutterWidget::OnSavePathCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
    SaveDirectory = NewText.ToString();
}

void SWFCTileCutterWidget::OnAssetPrefixCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
    AssetPrefix = NewText.ToString();
}

void SWFCTileCutterWidget::RefreshPreview()
{
    if (SourceTexture.IsValid())
    {
        // 创建预览 Brush
        PreviewBrush = MakeShareable(new FSlateBrush());
        PreviewBrush->SetResourceObject(SourceTexture.Get());
        PreviewBrush->ImageSize = FVector2D(200, 200);
    }
    else
    {
        PreviewBrush.Reset();
    }
}

void SWFCTileCutterWidget::RefreshResultGrid()
{
    if (!ResultGridPanel.IsValid())
    {
        return;
    }

    // 清空网格
    ResultGridPanel->ClearChildren();

    // 清空旧的 brush 引用
    ResultTileBrushes.Empty();

    // 添加切割结果
    for (int32 i = 0; i < CutResult.Tiles.Num(); ++i)
    {
        UTexture2D* TileTexture = CutResult.Tiles[i];
        FIntPoint GridPos = CutResult.GridPositions[i];

        if (TileTexture)
        {
            // 创建每个 tile 的预览并存储引用
            TSharedPtr<FSlateBrush> TileBrush = MakeShareable(new FSlateBrush());
            TileBrush->SetResourceObject(TileTexture);
            TileBrush->ImageSize = FVector2D(64, 64);

            // 保存引用防止被销毁
            ResultTileBrushes.Add(TileBrush);

            ResultGridPanel->AddSlot(GridPos.X, GridPos.Y)
            [
                SNew(SBox)
                .WidthOverride(80)
                .HeightOverride(80)
                .Content()
                [
                    SNew(SImage)
                    .Image(TileBrush.Get())
                ]
            ];
        }
    }
}
