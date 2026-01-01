#include "FDV2GhostCustomization.h"

#include "DV2AssetTree.h"
#include "DV2Ghost.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "NetImmerse.h"
#include "DV2Importer/unpack.h"
#include "Slate/SDV2Explorer.h"

void FDV2GhostCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("DV2 Ghost");

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	Target = nullptr;

	if (ObjectsBeingCustomized.Num() > 0 && ObjectsBeingCustomized[0].IsValid())
	{
		Target = Cast<ADV2Ghost>(ObjectsBeingCustomized[0].Get());
	}

	if (!Target)
		return;

	Target->OnFileChanged.AddSP(this, &FDV2GhostCustomization::Refresh);

	Category.AddCustomRow(FText::FromString("NI File"))
	        .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString("NI File"))
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(0, 0, 5, 0)
			[
				SAssignNew(PathDisplay, SEditableTextBox)
				.IsReadOnly(true)
				.Text(FText::FromString(Target->File.IsValid()
					? Target->File->Path
					: "None"))
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.ToolTipText(FText::FromString(Target->File.IsValid()
					? Target->File->Path
					: "None"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(MenuAnchor, SMenuAnchor)
				.MenuContent(
					SNew(SBox)
					.WidthOverride(250)
					.HeightOverride(300)
					[
						SNew(SDV2Explorer)
						.OnSelectionChanged_Lambda([this](const TSharedPtr<FDV2AssetTreeEntry>& Selection)
						{
							if (Selection.IsValid() && Selection->IsFile())
							{
								SetFile(Selection->GetAssetReference().GetFilePath());
							}
						})
					]
					)
				[
					SNew(SButton)
					.OnClicked_Lambda([this]()
					{
						MenuAnchor->SetIsOpen(true);
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString("..."))
					]
				]
			]
		];
}

void FDV2GhostCustomization::Refresh()
{
	FString NewPath = Target->File.IsValid()
		? Target->File->Path
		: "None";

	PathDisplay->SetText(FText::FromString(NewPath));
	PathDisplay->SetToolTipText(FText::FromString(NewPath));
}

void FDV2GhostCustomization::SetFile(const FString& NewPath)
{
	FScopedTransaction Transaction(FText::FromString("Set file on DV2 Ghost"));

	Target->Modify();

	FString OldPath = Target->File.IsValid()
		? Target->File->Path
		: "";
	TUniquePtr<FDV2GhostSetFile> Change = MakeUnique<FDV2GhostSetFile>(OldPath, NewPath);

	if (GUndo)
		GUndo->StoreUndo(Target, MoveTemp(Change));

	Target->SetFile(NewPath);
}
