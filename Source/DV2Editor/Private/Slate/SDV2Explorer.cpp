#include "Slate/SDV2Explorer.h"

#include "DV2AssetTree.h"
#include "Divinity2Assets.h"
#include "TextUtils.h"

void SDV2Explorer::Construct(const FArguments& InArgs)
{
	OnSelectionChanged = InArgs._OnSelectionChanged;
	AllowExport = InArgs._AllowExport;
	FormatFilter = InArgs._FormatFilter;

	RootEntries = MakeShared<UE::Slate::Containers::TObservableArray<TSharedPtr<FDV2AssetTreeEntry>>>();

	UDivinity2Assets::OnAssetsReloaded().AddSP(this, &SDV2Explorer::ReloadRootEntries);
	ReloadRootEntries();

	ChildSlot
	[
		SAssignNew(TreeView, STreeView<TSharedPtr<FDV2AssetTreeEntry>>)
		.TreeItemsSource(RootEntries)
		.OnGetChildren_Lambda(
			[this](const TSharedPtr<FDV2AssetTreeEntry>& Entry, TArray<TSharedPtr<FDV2AssetTreeEntry>>& OutChildren)
			{
				if (FormatFilter.IsEmpty())
					Entry->Children.GenerateValueArray(OutChildren);
				else
					for (const auto& Child : Entry->Children)
						if (!Child.Value->IsFile() || Child.Value->Name.EndsWith(FormatFilter))
							OutChildren.Add(Child.Value);

			})
		.OnGenerateRow_Lambda([](const TSharedPtr<FDV2AssetTreeEntry>& entry, const TSharedRef<STableViewBase>& ownerTable)
		{
			return SNew(STableRow<TSharedPtr<FDV2AssetTreeEntry>>, ownerTable)
				.ShowWires(true)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(entry->GetTitle())
					]
				];
		})
		.SelectionMode(ESelectionMode::Single)
		.OnSelectionChanged_Lambda([this](const TSharedPtr<FDV2AssetTreeEntry>& selection, ESelectInfo::Type type)
		{
			OnSelectionChanged.ExecuteIfBound(selection);
		})
		.OnContextMenuOpening_Lambda([this]() -> TSharedPtr<SWidget>
		{
			const auto& Selection = TreeView->GetSelectedItems();
			if (Selection.Num() == 0)
				return nullptr;

			FMenuBuilder MenuBuilder(true, nullptr);

			if (AllowExport)
			{
				MenuBuilder.AddMenuEntry(
					MAKE_TEXT("SDV2Explorer", "Export to disk"),
					MAKE_TEXT("SDV2Explorer", "Export asset from DV2 file to disk"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this, Selection]()
					{
						for (const auto& Asset : Selection)
						{
							Asset->ExportToDiskRecursive(true);
						}
					})));

				if (Selection.Num() == 1 && Selection[0]->IsFile())
				{
					MenuBuilder.AddMenuEntry(
						MAKE_TEXT("SDV2Explorer", "Open in external program"),
						MAKE_TEXT("SDV2Explorer", "Export to disk and open in external program"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([this, Asset = Selection[0]]()
						{
							Asset->ExportToDisk(nullptr, false, true);
						})));
				}
			}

			return MenuBuilder.MakeWidget();
		})
	];
}

void SDV2Explorer::SetSelectedEntry(const TSharedPtr<FDV2AssetTreeEntry>& InEntry) const
{
	TreeView->SetSelection(InEntry);
}

void SDV2Explorer::ReloadRootEntries() const
{
	auto RootDir = GetDefault<UDivinity2Assets>()->GetRootDir();
	if (RootDir.IsValid())
	{
		TArray<TSharedPtr<FDV2AssetTreeEntry>> newRootEntries;

		if (FormatFilter.IsEmpty())
			RootDir->Children.GenerateValueArray(newRootEntries);
		else
			for (const auto& Child : RootDir->Children)
				if (!Child.Value->IsFile() || Child.Value->Name.EndsWith(FormatFilter))
					newRootEntries.Add(Child.Value);
		
		RootEntries->Reset();
		RootEntries->Append(newRootEntries);
	}
}
