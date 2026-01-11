#include "DV2Browser.h"

#include "DV2Editor.h"
#include "DV2Importer/unpack.h"
#include "Interfaces/IMainFrameModule.h"
#include "Slate/SDV2Explorer.h"
#include "Slate/SItemView.h"
#include "Slate/SKfView.h"
#include "Slate/SNifView.h"
#include "Slate/SXmlView.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

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

TSharedPtr<SWidget> FDV2Browser::GetViewForAsset(const TSharedPtr<FDV2AssetTreeEntry>& asset)
{
	if (!asset->File.IsValid())
		return SNullWidget::NullWidget;

	FString Extension = FPaths::GetExtension(asset->Name);

	if (Extension == "lua")
	{
		return GetViewForAssetText(asset);
	}

	if (Extension == "nif" || Extension == "dds")
	{
		return SNew(SNifView, asset);
	}

	if (Extension == "item")
	{
		return SNew(SItemView, asset);
	}

	if (Extension == "kf")
	{
		return SNew(SKfView, asset);
	}

	if (Extension == "xml")
	{
		return SNew(SXmlView, asset);
	}

	return GetViewForAssetUnknown(asset);
}

TSharedPtr<SWidget> FDV2Browser::GetViewForAssetUnknown(const TSharedPtr<FDV2AssetTreeEntry>& asset) const
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.OnClicked_Lambda([this, asset]()
			{
				asset->ExportToDisk(nullptr, false, true);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DV2Browser", "Open in external program", "Open in external program"))
				.Margin(5)
			]
		];
}

TSharedPtr<SWidget> FDV2Browser::GetViewForAssetText(const TSharedPtr<FDV2AssetTreeEntry>& asset) const
{
	TArray<uint8> bytes;
	if (asset->GetAssetReference().Read(*asset->File, bytes))
	{
		return SNew(SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.Text(FText::FromString(UTF8_TO_TCHAR(bytes.GetData())));
	}

	return GetMessageViewForAsset("Failed to open file");
}

TSharedPtr<SWidget> FDV2Browser::GetMessageViewForAsset(const FString& message) const
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(message))
		];
}
