#include "SNiOutliner.h"

#include "DV2Style.h"
#include "NetImmerse.h"
#include "TextUtils.h"

void SNiOutliner::Construct(const FArguments& InArgs, const TSharedPtr<FNiFile>& InFile)
{
	GenerateCell = InArgs._GenerateCell;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	File = InFile;

	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow);

	for (const auto& Column : InArgs._AddColumnsLeft)
	{
		auto ColumnArgs = SHeaderRow::Column(*Column.Key)
			.DefaultLabel(Column.Title);

		if (Column.Width > 0)
			ColumnArgs.FixedWidth(Column.Width);
		
		HeaderRow->AddColumn(ColumnArgs);
	}

	HeaderRow->AddColumn(
		SHeaderRow::Column("Main")
		.DefaultLabel(FText::FromString("Node Tree"))
		);

	for (const auto& Column : InArgs._AddColumnsRight)
	{
		auto ColumnArgs = SHeaderRow::Column(*Column.Key)
			.DefaultLabel(Column.Title);

		if (Column.Width > 0)
			ColumnArgs.FixedWidth(Column.Width);
		
		HeaderRow->AddColumn(ColumnArgs);
	}

	RefreshRootNodes();

	ChildSlot
	[
		SNew(STreeView<TSharedPtr<FBlockWrapper>>)
		.TreeItemsSource(&RootNodes)
		.HeaderRow(HeaderRow)
		.OnGetChildren_Lambda(
			[](const TSharedPtr<FBlockWrapper>& entry,
			   TArray<TSharedPtr<FBlockWrapper>>& outChildren)
			{
				entry->GetChildren(outChildren);
			})
		.OnContextMenuOpening_Lambda([this]() -> TSharedPtr<SWidget>
		{
			if (!SelectedBlock.IsValid())
				return SNullWidget::NullWidget;

			FMenuBuilder MenuBuilder(true, nullptr);

			if (SelectedBlock->Block->Type->BuildContextMenu.IsSet())
				SelectedBlock->Block->Type->BuildContextMenu(MenuBuilder, File, SelectedBlock->Block);

			return MenuBuilder.MakeWidget();
		})
		.OnGenerateRow_Lambda([this](const TSharedPtr<FBlockWrapper>& Entry, const TSharedRef<STableViewBase>& Owner)
		{
			return SNew(SNiOutlinerRow, Owner, Entry, File)
				.GenerateCell(GenerateCell);
		})
		.OnRowReleased_Lambda([](const TSharedRef<ITableRow>& Row)
		{

		})
		.SelectionMode(ESelectionMode::Single)
		.OnSelectionChanged_Lambda(
			[this](const TSharedPtr<FBlockWrapper>& Selection, ESelectInfo::Type Type)
			{
				SelectedBlock = Selection;
				OnSelectionChanged.ExecuteIfBound(Selection.IsValid() ? Selection->Block : nullptr);
			})
	];
}

void SNiOutliner::RefreshRootNodes()
{
	RootNodes.Reset();

	TArray<TSharedPtr<FNiBlock>> RootCandidates;
	RootCandidates.Append(File->Blocks);

	for (int32 i = 0; i < File->Blocks.Num(); i++)
	{
		const auto& block = File->Blocks[i];
		for (const auto& child : block->Referenced)
		{
			if (child->BlockIndex == 0)
				continue;

			RootCandidates.RemoveSingle(child);
		}
	}

	for (const auto& candidate : RootCandidates)
	{
		RootNodes.Add(MakeShareable(new FBlockWrapper{
			.Block = candidate,
		}));
	}
}

void SNiOutliner::FBlockWrapper::GetChildren(TArray<TSharedPtr<FBlockWrapper>>& OutChildren)
{
	if (CachedChildren.IsEmpty())
	{
		OutChildren.Reserve(Block->Referenced.Num());
		CachedChildren.Reserve(Block->Referenced.Num());
		for (const auto& Child : Block->Referenced)
		{
			TSharedPtr<FBlockWrapper> WrappedChild = MakeShareable(new FBlockWrapper{
				.Block = Child
			});
			OutChildren.Add(WrappedChild);
			CachedChildren.Add(WrappedChild);
		}
	}
	else
	{
		OutChildren.Append(CachedChildren);
	}
}

void SNiOutlinerRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner, const TSharedPtr<SNiOutliner::FBlockWrapper>& InTarget, const TSharedPtr<FNiFile>& InFile)
{
	GenerateCell = InArgs._GenerateCell;

	Target = InTarget;
	File = InFile;

	SMultiColumnTableRow::Construct(FTableRowArgs(), InOwner);
}

TSharedRef<SWidget> SNiOutlinerRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName == "Main")
		return GenerateMainCell();

	if (GenerateCell.IsBound())
		return GenerateCell.Execute(InColumnName.ToString(), Target->Block);

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SNiOutlinerRow::GenerateMainCell()
{
	auto horizontalBox = SNew(SHorizontalBox);

	auto typeNameTextBlock = SNew(STextBlock)
		.Text(FText::FromString(Target->Block->Type->name))
		.ToolTipText(FText::FromString(Target->Block->Type->description));

	if (Target->Block->Error.IsValid())
		typeNameTextBlock->SetColorAndOpacity(FColor::Red);

	const FSlateBrush* Icon = nullptr;
	bool HaveIcon = FDV2Style::FindNifBlockIcon(Target->Block->Type, Icon);

	horizontalBox->AddSlot()
	             .AutoWidth()
	[
		SNew(SExpanderArrow, SharedThis(this))
		.ShouldDrawWires(true)
		.IndentAmount(16)
	];

	horizontalBox->AddSlot()
	             .AutoWidth()
	             .VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(14)
		.HeightOverride(14)
		[
			SNew(SImage)
			.Image(Icon)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Visibility(HaveIcon ? EVisibility::Visible : EVisibility::Hidden)
		]
	];

	horizontalBox->AddSlot()
	             .AutoWidth()
	             .Padding(2.0f, 0.0f)
	             .VAlign(VAlign_Center)
	[
		typeNameTextBlock
	];

	FString name = Target->Block->GetTitle(*File);
	if (!name.IsEmpty())
	{
		horizontalBox->AddSlot()
		             .AutoWidth()
		             .Padding(2.0f, 0.0f)
		             .VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString("\"" + name + "\""))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		];
	}

	horizontalBox->AddSlot()
	             .AutoWidth()
	             .Padding(2.0f, 0.0f)
	             .VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FORMAT_TEXT("SNiOutlinerRow", "[{0}]", Target->Block->BlockIndex))
	];

	return horizontalBox;
}
