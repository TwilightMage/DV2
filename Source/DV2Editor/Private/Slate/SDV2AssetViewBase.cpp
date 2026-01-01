#include "Slate/SDV2AssetViewBase.h"

#include "DV2.h"
#include "DV2AssetTree.h"
#include "DV2Importer/unpack.h"

void SDV2AssetViewBase::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	Asset = InAsset;

	const auto& Ref = InAsset->GetAssetReference();

	try
	{
		OnConstructView(InAsset);
	}
	catch (...)
	{
	}

	if (ChildSlot.GetWidget() == SNullWidget::NullWidget)
		FailConstruct(FString::Printf(TEXT("Error constructing asset view %s"), *Ref.name));
}

void SDV2AssetViewBase::FailConstruct(const FString& Message)
{
	UE_LOG(LogDV2, Error, TEXT("%s"), *Message);
	
	ChildSlot
	[
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Message))
		]
	];
}

bool SDV2AssetViewBase::ReadAsset(TArray<uint8>& OutBytes) const
{
	const auto& Ref = Asset->GetAssetReference();
	return Ref.Read(*Asset->File, OutBytes);
}
