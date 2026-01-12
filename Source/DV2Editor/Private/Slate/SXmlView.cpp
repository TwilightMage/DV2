#include "Slate/SXmlView.h"

#include "TextUtils.h"
#include "NiMeta/CStreamableNode.h"

void SXmlView::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	ConstructAssetView(InAsset);
}

void SXmlView::OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	File = UNetImmerse::OpenNiFile(InAsset, true);
	if (!File.IsValid())
		return;

	if (!ensure(File->Blocks.Num() == 1))
		return;

	if (File->Blocks[0]->Error.IsValid())
		return;

	if (!File->Blocks[0]->IsOfType("xml::dom::CStreamableNode"))
		return;

	Block = StaticCastSharedPtr<FNiBlock_CStreamableNode>(File->Blocks[0]);

	if (Block->UniqueParameterTypes.Num() > 20)
		if (FMessageDialog::Open(
			EAppMsgType::YesNo,
			FORMAT_TEXT("SXmlView", "This tree contains {0} unique properties, are you sure you want to proceed?", Block->UniqueParameterTypes.Num()),
			MAKE_TEXT("SXmlView", "Too many properties")
			) != EAppReturnType::Yes)
			return;

	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)
		+ SHeaderRow::Column("NameColumn")
		.DefaultLabel(FText::FromString("Node Name"));

	for (auto Type : Block->UniqueParameterTypes)
	{
		HeaderRow->AddColumn(
			SHeaderRow::Column(*FString::FromInt(Type))
			.DefaultLabel(FCStreamableNode::GetPropertyTitle(Type))
			);
	}

	ChildSlot
	[
		SNew(STreeView<TSharedPtr<FCStreamableNode>>)
		.TreeItemsSource(&Block->CStreamableNodes)
		.HeaderRow(HeaderRow)
		.OnGetChildren_Lambda([](const TSharedPtr<FCStreamableNode>& Item, TArray<TSharedPtr<FCStreamableNode>>& OutChildren)
		{
			OutChildren.Append(Item->Children);
		})
		.OnGenerateRow_Lambda([this](const TSharedPtr<FCStreamableNode>& Item, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
		{
			return SNew(SXmlViewTableRow, Owner, Item);
		})
	];
}

void SXmlViewTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& Owner, const TSharedPtr<FCStreamableNode>& InTarget)
{
	Target = InTarget;
	SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments(), Owner);
}

TSharedRef<SWidget> SXmlViewTableRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	if (InColumnName == "NameColumn")
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(16)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FCStreamableNode::GetNodeTitle(*Target))
				.AutoWrapText(true)
			];
	}
	else
	{
		uint32 Type;
		LexFromString(Type, InColumnName.ToString());

		for (const auto& Property : Target->Data.Properties)
		{
			if (Property.Type == Type)
			return SNew(STextBlock)
					.Text(FText::FromString(Property.Value))
					.AutoWrapText(true);
		}
	}

	return SNullWidget::NullWidget;
}
