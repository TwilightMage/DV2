#pragma once

struct FNiFile;
struct FNiField;
class FNiBlock;

struct FNiInspectorRowItem
{
	FNiInspectorRowItem() = default;
	FNiInspectorRowItem(const FNiField* InField, uint32 InIndex = -1)
		: Field(InField)
		, Index(InIndex)
	{}
	
	const FNiField* Field;
	uint32 Index = -1;

	TArray<TSharedPtr<FNiInspectorRowItem>> CachedChildren;
};

class SNiBlockInspector : public SCompoundWidget
{
public:
	DECLARE_DELEGATE(FOnReloadBlockClicked)
	
	SLATE_BEGIN_ARGS(SNiBlockInspector)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FNiBlock>, TargetBlock)
		SLATE_ARGUMENT(TSharedPtr<FNiFile>, TargetFile)
		SLATE_EVENT(FOnReloadBlockClicked, OnReloadBlockClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetTargetBlock(const TSharedPtr<FNiBlock>& NewBlock);

	void Refresh();

private:
	TSharedPtr<FNiBlock> TargetBlock;
	TSharedPtr<FNiFile> TargetFile;
	FOnReloadBlockClicked OnReloadBlockClicked;
	TSharedPtr<SBox> ContentContainer;
	TArray<TSharedPtr<FNiInspectorRowItem>> Fields;

	TSharedRef<SWidget> GenerateInspectorContent();
};

class SNiBlockInspectorRow : public SMultiColumnTableRow<TSharedPtr<FNiInspectorRowItem>>
{
public:
	SLATE_BEGIN_ARGS(SNiBlockInspectorRow)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FNiBlock>, TargetBlock)
		SLATE_ARGUMENT(TSharedPtr<FNiFile>, TargetFile)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, const TSharedPtr<FNiInspectorRowItem>& InTarget);

private:
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	FText GetEntryDescription() const;

	TSharedPtr<FNiBlock> TargetBlock;
	TSharedPtr<FNiFile> TargetFile;
	TSharedPtr<FNiInspectorRowItem> Target;
};
