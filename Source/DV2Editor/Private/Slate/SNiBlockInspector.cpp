#include "Slate/SNiBlockInspector.h"

#include "NetImmerse.h"
#include "TextUtils.h"
#include "NiMeta/NiMeta.h"

void SNiBlockInspector::Construct(const FArguments& InArgs)
{
	TargetBlock = InArgs._TargetBlock;
	TargetFile = InArgs._TargetFile;
	OnReloadBlockClicked = InArgs._OnReloadBlockClicked;

	ChildSlot
	[
		SAssignNew(ContentContainer, SBox)
		[
			GenerateInspectorContent()
		]
	];
}

void SNiBlockInspector::SetTargetBlock(const TSharedPtr<FNiBlock>& NewBlock)
{
	TargetBlock = NewBlock;

	Refresh();
}

void SNiBlockInspector::Refresh()
{
	if (ContentContainer.IsValid())
	{
		ContentContainer->SetContent(GenerateInspectorContent());
	}
}

TSharedRef<SWidget> SNiBlockInspector::GenerateInspectorContent()
{
	if (!TargetBlock.IsValid())
	{
		return SNew(SBox)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString("No Block Selected"))
			];
	}

	Fields.Reset();
	for (auto& field : TargetBlock->Fields)
	{
		Fields.Add(MakeShared<FNiInspectorRowItem>(&field));
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FORMAT_TEXT("SNifBlockInspector", "Type: {0}", FText::FromString(TargetBlock->Type->name)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		[
			SNew(STextBlock)
			.Text(FORMAT_TEXT("SNifBlockInspector", "Block index: {0}", FText::AsNumber(TargetBlock->BlockIndex)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		[
			SNew(STextBlock)
			.Text(FORMAT_TEXT("SNifBlockInspector", "Data offset: {0}", FText::AsNumber(TargetBlock->DataOffset)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		[
			SNew(STextBlock)
			.Text(FORMAT_TEXT("SNifBlockInspector", "Data size: {0}", FText::AsNumber(TargetBlock->DataSize)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		[
			SNew(STextBlock)
			.Text(FORMAT_TEXT("SNifBlockInspector", "Error: {0}", FText::FromString(TargetBlock->Error)))
			.AutoWrapText(true)
			.ColorAndOpacity(FColor::Red)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		[
			SNew(SButton)
			.OnClicked_Lambda([this]()
			{
				OnReloadBlockClicked.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(MAKE_TEXT("SNifBlockInspector", "Re-load block"))
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(0, 5, 0, 0)
		[
			SNew(STreeView<TSharedPtr<FNiInspectorRowItem>>)
			.TreeItemsSource(&Fields)
			.HeaderRow(SNew(SHeaderRow)
				+ SHeaderRow::Column("NameColumn")
				.DefaultLabel(FText::FromString("Name"))
				+ SHeaderRow::Column("ValueColumn")
				.DefaultLabel(FText::FromString("Value"))
				)
			.OnGetChildren_Lambda([](const TSharedPtr<FNiInspectorRowItem>& item, TArray<TSharedPtr<FNiInspectorRowItem>>& outChildren)
			{
				if (item->CachedChildren.Num() > 0)
				{
					outChildren.Append(item->CachedChildren);
					return;
				}

				auto length = item->Field->Size;
				if (length == 1 || item->Index != (uint32)-1)
				{
					// Show fields
					if (!!(item->Field->Meta->type->GetFieldType() & (NiMeta::EFieldType::BitFlags | NiMeta::EFieldType::BitField | NiMeta::EFieldType::BitField)))
					{
						auto& value = item->Field->SingleGroup();

						for (const auto& childField : value->Fields)
						{
							outChildren.Add(MakeShared<FNiInspectorRowItem>(&childField));
						}
					}
				}
				else if (length > 1)
				{
					if (item->Field->Meta->type->name != "BinaryBlob")
					{
						// Show list items
						for (uint32 i = 0; i < length; ++i)
						{
							outChildren.Add(MakeShared<FNiInspectorRowItem>(item->Field, i));
						}
					}
				}

				item->CachedChildren = outChildren;
			})
			.OnGenerateRow_Lambda([this](const TSharedPtr<FNiInspectorRowItem>& item, const TSharedRef<STableViewBase>& ownerTable)
			{
				return SNew(SNiBlockInspectorRow, ownerTable, item)
					.TargetBlock(TargetBlock)
					.TargetFile(TargetFile);
			})
		];
}

void SNiBlockInspectorRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, const TSharedPtr<FNiInspectorRowItem>& InTarget)
{
	TargetBlock = InArgs._TargetBlock;
	TargetFile = InArgs._TargetFile;

	Target = InTarget;

	SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SNiBlockInspectorRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == "NameColumn")
	{
		bool isSelf = Target->Index == (uint32)-1;
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(16)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(isSelf
					? FText::FromString(Target->Field->Meta->name)
					: FORMAT_TEXT("SNifBlockInspectorRow", "Item {0}", Target->Index))
				.ToolTip(SNew(SToolTip)
					.Text(GetEntryDescription())
					)
			];
	}
	else if (ColumnName == "ValueColumn")
	{
		const auto& TargetType = Target->Field->Meta->type;
		if (Target->Index == (uint32)-1)
		{
			if (Target->Field->Meta->type->name == "BinaryBlob")
			return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%d bytes"), Target->Field->Size)));

			if (Target->Field->Size == 0)
			return SNew(STextBlock)
					.Text(MAKE_TEXT("FNifField", "[empty]"));

			if (Target->Field->Size > 1)
			return SNew(STextBlock)
					.Text(FORMAT_TEXT("FNifField", "[{0} entries]", Target->Field->Size));

			if (TargetType->GenerateSlateWidget.IsSet())
				return TargetType->GenerateSlateWidget(*TargetFile, *Target->Field, 0);

			if (TargetType->ToString.IsSet())
			return SNew(STextBlock)
					.Text(FText::FromString(TargetType->ToString(*TargetFile, *Target->Field, 0)));

			return SNullWidget::NullWidget;
		}
		else
		{
			if (TargetType->GenerateSlateWidget.IsSet())
				return TargetType->GenerateSlateWidget(*TargetFile, *Target->Field, Target->Index);

			if (TargetType->ToString.IsSet())
			return SNew(STextBlock)
					.Text(FText::FromString(TargetType->ToString(*TargetFile, *Target->Field, Target->Index)));

			return SNullWidget::NullWidget;
		}
	}

	return SNullWidget::NullWidget;
}

FText SNiBlockInspectorRow::GetEntryDescription() const
{
	FString Result;

	bool IsSelf = Target->Index == (uint32)-1;
	const auto& TargetType = Target->Field->Meta->type;
	Result += Target->Field->Meta->description;

	if (!Result.IsEmpty())
		Result += "\n";

	Result += FString::Printf(TEXT("%s (size %s) on offset %s (%s)"),
	                          *TargetType->name,
	                          *TargetType->sizeString,
	                          IsSelf
	                          ? *Target->Field->SourceOffsetToString(0, 0)
	                          : *Target->Field->SourceOffsetToString(0, Target->Index),
	                          IsSelf
	                          ? *Target->Field->SourceOffsetToString(TargetBlock->DataOffset, 0)
	                          : *Target->Field->SourceOffsetToString(TargetBlock->DataOffset, Target->Index));

	return FText::FromString(Result);
}
