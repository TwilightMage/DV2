#pragma once

struct FNiFile;
class SNiOutlinerRow;
class FNiBlock;

class SNiOutliner : public SCompoundWidget
{
	friend SNiOutlinerRow;

public:
	struct FColumn
	{
		FString Key;
		FText Title;
		float Width;
	};
	
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SWidget>, FGenerateCell, FString ColumnName , TSharedPtr<FNiBlock> Block)
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, TSharedPtr<FNiBlock> Block)

	SLATE_BEGIN_ARGS(SNiOutliner)
		{
		}

		SLATE_ARGUMENT(TArray<FColumn>, AddColumnsLeft)
		SLATE_ARGUMENT(TArray<FColumn>, AddColumnsRight)
		SLATE_EVENT(FGenerateCell, GenerateCell)
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FNiFile>& InFile);

	void RefreshRootNodes();

private:
	struct FBlockWrapper
	{
		TSharedPtr<FNiBlock> Block;
		TArray<TSharedPtr<FBlockWrapper>> CachedChildren;

		void GetChildren(TArray<TSharedPtr<FBlockWrapper>>& OutChildren);
	};

	FGenerateCell GenerateCell;
	FOnSelectionChanged OnSelectionChanged;

	TArray<TSharedPtr<FBlockWrapper>> RootNodes;
	TSharedPtr<FNiFile> File;
	TSharedPtr<FBlockWrapper> SelectedBlock;
};

class SNiOutlinerRow : public SMultiColumnTableRow<TSharedPtr<SNiOutliner::FBlockWrapper>>
{
public:
	SLATE_BEGIN_ARGS(SNiOutlinerRow)
		{
		}

		SLATE_EVENT(SNiOutliner::FGenerateCell, GenerateCell)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner, const TSharedPtr<SNiOutliner::FBlockWrapper>& InTarget, const TSharedPtr<FNiFile>& InFile);
	
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedRef<SWidget> GenerateMainCell();
	
	SNiOutliner::FGenerateCell GenerateCell;
	TSharedPtr<SNiOutliner::FBlockWrapper> Target;
	TSharedPtr<FNiFile> File;
};
