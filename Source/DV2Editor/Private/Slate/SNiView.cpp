#include "Slate/SNiView.h"

#include "DV2AssetTree.h"
#include "NetImmerse.h"
#include "Slate/SNiBlockInspector.h"
#include "Slate/SNiOutliner.h"

void SNiView::OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	File = UNetImmerse::OpenNiFile(InAsset, true);
	if (!File.IsValid())
		return;

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
	return SAssignNew(NiOutliner, SNiOutliner, File)
		.OnSelectionChanged_Lambda([this](const TSharedPtr<FNiBlock>& InBlock)
		{
			SelectedBlock = InBlock;
			NiBlockInspector->SetTargetBlock(InBlock);
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
				fileReader.Seek(SelectedBlock->DataOffset);

				blockBytes.SetNum(SelectedBlock->DataSize);
				FMemory::Memcpy(blockBytes.GetData(), FileBytes.GetData() + SelectedBlock->DataOffset, blockBytes.Num());
			}

			FMemoryReader blockReader(blockBytes);
			blockReader.SetByteSwapping(File->ShouldByteSwapOnThisMachine());

			if (SelectedBlock->Type.IsValid())
			{
				// TODO: if block was changed inside (passed by ref), we should emit event
				NiMeta::BlockReadContext Ctx(*File, SelectedBlock);
				Ctx.Read(blockReader);
			}
			else
				SelectedBlock->SetErrorManual("Unknown nif block type");

			SelectedBlock->Referenced.Empty();
			SelectedBlock->TraverseFields([&](const FNiField& Field)
			{
				if (Field.IsReference())
				{
					const auto& Arr = Field.GetReferenceArray();
					for (const auto& Val : Arr)
					{
						if (auto Ptr = Val.Resolve(*File); Ptr.IsValid())
							SelectedBlock->Referenced.AddUnique(Ptr);
					}
				}
			});

			SelectedBlock->Children.Empty();
			if (auto ChildrenField = SelectedBlock->FindField("Children"))
			{
				const auto& Arr = ChildrenField->GetReferenceArray();
				for (const auto& Val : Arr)
				{
					if (auto Ptr = Val.Resolve(*File); Ptr.IsValid())
						SelectedBlock->Children.AddUnique(Ptr);
				}
			}

			//NifBlockInspector->Refresh();
		});
}
