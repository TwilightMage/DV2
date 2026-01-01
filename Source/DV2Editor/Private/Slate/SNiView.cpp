#include "Slate/SNiView.h"

#include "DV2AssetTree.h"
#include "DV2Style.h"
#include "NetImmerse.h"
#include "TextUtils.h"
#include "Slate/SNiBlockInspector.h"

void SNiView::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	RootNifNodes = MakeShared<UE::Slate::Containers::TObservableArray<TSharedPtr<FBlockWrapper>>>();

	SDV2AssetViewBase::Construct(SDV2AssetViewBase::FArguments(), InAsset);
}

void SNiView::OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	File = UNetImmerse::OpenNiFile(InAsset, true);

	RefreshRootNifNodes();

	auto ViewportWidget = MakeViewportWidget(File);
	if (!ViewportWidget.IsValid())
		return;

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.MinSize(500)
		[
			ViewportWidget.ToSharedRef()
		]
		+ SSplitter::Slot()
		.Resizable(true)
		.OnSlotResized_Lambda([this](float NewSize)
		{
			InspectorWidth = NewSize;
		})
		.SizeRule(SSplitter::FractionOfParent)
		.Value(0.3)
		.MinSize(200)
		[
			SNew(SBox)
			.WidthOverride(InspectorWidth)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				+ SSplitter::Slot()
				.MinSize(300)
				[
					MakeOutlinerWidget()
				]
				+ SSplitter::Slot()
				.Resizable(true)
				.OnSlotResized_Lambda([this](float NewSize)
				{
					InspectorHeight = NewSize;
				})
				.SizeRule(SSplitter::FractionOfParent)
				.Value(0.5)
				.MinSize(300)
				[
					SNew(SBox)
					.HeightOverride(InspectorHeight)
					[
						MakeInspectorWidget()
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SNiView::MakeOutlinerWidget()
{
	return SAssignNew(NiOutliner, STreeView<TSharedPtr<FBlockWrapper>>)
		.TreeItemsSource(RootNifNodes)
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
		.OnGenerateRow_Lambda(
			[this](const TSharedPtr<FBlockWrapper>& entry, const TSharedRef<STableViewBase>& ownerTable)
			{
				auto horizontalBox = SNew(SHorizontalBox);

				auto typeNameTextBlock = SNew(STextBlock)
					.Text(FText::FromString(entry->Block->Type->name))
					.ToolTipText(FText::FromString(entry->Block->Type->description));

				if (!entry->Block->Error.IsEmpty())
					typeNameTextBlock->SetColorAndOpacity(FColor::Red);

				const FSlateBrush* Icon = nullptr;
				bool HaveIcon = FDV2Style::FindNifBlockIcon(entry->Block->Type, Icon);

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

				FString name = entry->Block->GetTitle(*File);
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
					.Text(FORMAT_TEXT("SNifView", "[{0}]", entry->Block->BlockIndex))
				];

				return SNew(STableRow<TSharedPtr<FBlockWrapper>>, ownerTable)
					.ShowWires(true)
					[
						horizontalBox
					];
			})
		.OnRowReleased_Lambda([](const TSharedRef<ITableRow>& Row)
		{

		})
		.SelectionMode(ESelectionMode::Single)
		.OnSelectionChanged_Lambda(
			[this](const TSharedPtr<FBlockWrapper>& selection, ESelectInfo::Type type)
			{
				SelectedBlock = selection;

				auto Block = selection.IsValid() ? selection->Block : nullptr;
				NiBlockInspector->SetTargetBlock(Block);
				OnSelectedBlockChanged(Block);
			});
}

TSharedRef<SWidget> SNiView::MakeInspectorWidget()
{
	return SAssignNew(NiBlockInspector, SNiBlockInspector)
		.TargetFile(File)
		.OnReloadBlockClicked_Lambda([this]()
		{
			TArray<uint8> blockBytes;

			{
				TArray<uint8> FileBytes;
				if (!ReadAsset(FileBytes))
					return;

				FMemoryReader fileReader(FileBytes);
				fileReader.SetByteSwapping(File->ShouldByteSwapOnThisMachine());
				fileReader.Seek(SelectedBlock->Block->DataOffset);

				blockBytes.SetNum(SelectedBlock->Block->DataSize);
				FMemory::Memcpy(blockBytes.GetData(), FileBytes.GetData() + SelectedBlock->Block->DataOffset, blockBytes.Num());
			}

			FMemoryReader blockReader(blockBytes);
			blockReader.SetByteSwapping(File->ShouldByteSwapOnThisMachine());

			if (SelectedBlock->Block->Type.IsValid())
				// TODO: if block was changed inside (passed by ref), we should emit event
				SelectedBlock->Block->Type->ReadFrom(*File, blockReader, SelectedBlock->Block);
			else
				SelectedBlock->Block->Error = "Unknown nif block type";

			SelectedBlock->Block->Referenced.Empty();
			SelectedBlock->Block->TraverseFields([&](const FNiField& Field)
			{
				if (Field.IsReference())
				{
					const auto& Arr = Field.GetReferenceArray();
					for (const auto& Val : Arr)
					{
						if (auto Ptr = Val.Resolve(*File); Ptr.IsValid())
							SelectedBlock->Block->Referenced.AddUnique(Ptr);
					}
				}
			});

			SelectedBlock->Block->Children.Empty();
			if (auto ChildrenField = SelectedBlock->Block->FindField("Children"))
			{
				const auto& Arr = ChildrenField->GetReferenceArray();
				for (const auto& Val : Arr)
				{
					if (auto Ptr = Val.Resolve(*File); Ptr.IsValid())
						SelectedBlock->Block->Children.AddUnique(Ptr);
				}
			}

			RefreshRootNifNodes();

			//NifBlockInspector->Refresh();
		});
}

void SNiView::RefreshRootNifNodes() const
{
	RootNifNodes->Reset();

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
		RootNifNodes->Add(MakeShareable(new FBlockWrapper{
			.Block = candidate,
		}));
	}
}
