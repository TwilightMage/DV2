#include "DV2Browser.h"

#include "DV2Editor.h"
#include "TextUtils.h"
#include "FileHandlers/FFileHandlerBase.h"
#include "Interfaces/IMainFrameModule.h"
#include "Slate/SDV2Explorer.h"
#include "Slate/SKfView.h"

TSharedRef<SDockTab> FDV2Browser::Open(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.ContentPadding(FMargin(5))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(AssetExplorer, SDV2Explorer)
				.AllowExport(true)
				.OnSelectionChanged_Lambda([this](const TSharedPtr<FDV2AssetTreeEntry>& Entry)
				{
					SelectAsset(Entry, true);
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(4)
			.Padding(5, 0, 0, 0)
			[
				SAssignNew(AssetViewPanel, SBox)
			]
		];

	TabManager = FGlobalTabmanager::Get()->NewTabManager(NewTab);
	TabManager->SetAllowWindowMenuBar(true);
	FToolMenuContext ToolMenuContext;
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
	MainFrameModule.MakeMainMenu(TabManager, MenuName, ToolMenuContext);

	return NewTab;
}

void FDV2Browser::Init()
{
	NiMeta::OnReset().AddSPLambda(this, [this]()
	{
		SelectAsset(nullptr, false);
	});

	NiMeta::OnReload().AddSPLambda(this, [this]()
	{
		SelectAsset(nullptr, false);
	});
}

FDV2Browser& FDV2Browser::Get()
{
	return *FDV2EditorModule::Get().DV2Browser;
}

void FDV2Browser::SelectAsset(const TSharedPtr<FDV2AssetTreeEntry>& Entry, bool bSkipExplorer)
{
	if (AssetExplorer.IsValid() && !bSkipExplorer)
		AssetExplorer->SetSelectedEntry(Entry);

	if (AssetViewPanel.IsValid())
	{
		AssetViewPanel->SetContent(Entry.IsValid()
			? GetViewForAsset(Entry).ToSharedRef()
			: SNullWidget::NullWidget);
	}
}

TSharedPtr<SWidget> FDV2Browser::GetViewForAsset(const TSharedPtr<FDV2AssetTreeEntry>& asset) const
{
	if (!asset->File.IsValid())
		return SNullWidget::NullWidget;

	FString Extension = FPaths::GetExtension(asset->Name);
	if (auto Handler = FDV2EditorModule::Get().GetFileHandler(Extension); Handler.IsValid())
	{
		auto View = Handler->CreateViewForFile(asset);
		if (View == SNullWidget::NullWidget)
			return GetMessageViewForAsset("Failed to open asset");
		return View;
	}

	return GetViewForAssetUnknown(asset);
}

TSharedPtr<SWidget> FDV2Browser::GetViewForAssetUnknown(const TSharedPtr<FDV2AssetTreeEntry>& asset)
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.OnClicked_Lambda([asset]()
			{
				asset->ExportToDisk(nullptr, false, true);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(MAKE_TEXT("DV2Browser", "Open in external program"))
				.Margin(5)
			]
		];
}

TSharedPtr<SWidget> FDV2Browser::GetMessageViewForAsset(const FString& message)
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(message))
		];
}
