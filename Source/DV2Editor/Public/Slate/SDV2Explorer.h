#pragma once

struct FDV2AssetTreeEntry;

class SDV2Explorer : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, TSharedPtr<FDV2AssetTreeEntry>);
	
	SLATE_BEGIN_ARGS(SDV2Explorer)
		: _AllowExport(false)
		{
		}
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
		SLATE_ARGUMENT(bool, AllowExport)
		SLATE_ARGUMENT(FString, FormatFilter)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetSelectedEntry(const TSharedPtr<FDV2AssetTreeEntry>& InEntry) const;

private:
	void ReloadRootEntries();
	void Search(const FText& Prompt);

	static void SearchRecursive(const TSharedPtr<FDV2AssetTreeEntry>& Target, const FString& Prompt, const TFunction<void(const TSharedPtr<FDV2AssetTreeEntry>& Entry)> FindHandler);

	FOnSelectionChanged OnSelectionChanged;
	bool AllowExport = false;
	FString FormatFilter;

	TSharedPtr<STreeView<TSharedPtr<FDV2AssetTreeEntry>>> TreeView;
	TSharedPtr<FDV2AssetTreeEntry> SelectedEntry;
	TSharedPtr<UE::Slate::Containers::TObservableArray<TSharedPtr<FDV2AssetTreeEntry>>> ShowRootEntries;
	TArray<TSharedPtr<FDV2AssetTreeEntry>> RootEntries;
};
