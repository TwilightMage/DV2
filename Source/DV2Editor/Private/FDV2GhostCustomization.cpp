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
		Target = Cast<UDV2GhostComponent>(ObjectsBeingCustomized[0].Get());
	}

	if (!Target)
		return;

	Category.AddCustomRow(FText::FromString("NI File"))
	        .NameContent()
		[
			SNew(STextBlock).Text(FText::FromString("NI File"))
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SComboButton)
			.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
			{
				return SNew(SBox)
					.WidthOverride(280)
					[
						SNew(SDV2Explorer)
						.OnSelectionChanged_Lambda([this](const TSharedPtr<FDV2AssetTreeEntry>& Selection)
						{
							if (Selection.IsValid() && Selection->IsFile())
							{
								SetFile(Selection->GetAssetReference().GetFilePath());
							}
						})
					];
			})
			.ToolTipText(this, &FDV2GhostCustomization::GetNiPathTitle)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FDV2GhostCustomization::GetNiPathTitle)
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			]
		];
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

FText FDV2GhostCustomization::GetNiPathTitle() const
{
	return FText::FromString(Target->File.IsValid() ? Target->File->Path : "None");
}
