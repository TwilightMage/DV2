#include "FNiMaskCustomization.h"

#include "DV2Ghost.h"
#include "DetailWidgetRow.h"
#include "NetImmerse.h"
#include "Slate/SNiOutliner.h"

void FNiMaskCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	void* StructData;
	if (PropertyHandle->GetValueData(StructData) == FPropertyAccess::Success && StructData != nullptr)
	{
		FString ErrorMessage;

		FString AssetPropertyName = PropertyHandle->GetMetaData("AssetProperty");
		if (AssetPropertyName.IsEmpty())
			ErrorMessage = "Missing 'FilePath' metadata on this property.";
		else
		{
			TSharedPtr<IPropertyHandle> AssetPropertyHandle = PropertyHandle->GetParentHandle()->GetChildHandle(*AssetPropertyName);

			if (!AssetPropertyHandle.IsValid())
				ErrorMessage = FString::Printf(TEXT("Property '%s' not found."), *AssetPropertyName);
			else
			{
				FString AssetPath;
				if (AssetPropertyHandle->GetValue(AssetPath) == FPropertyAccess::Success)
				{
					File = UNetImmerse::OpenNiFile(AssetPath, true);
					if (!File.IsValid())
						ErrorMessage = FString::Printf(TEXT("Failed to open file '%s'."), *AssetPath);
				}
				else
					ErrorMessage = "Failed to open file.";
			}
		}

		FNiMask& Mask = *(FNiMask*)StructData;

		TSharedPtr<SWidget> ValueWidget;
		if (!ErrorMessage.IsEmpty())
			ValueWidget = SNew(STextBlock)
				.Text(FText::FromString(ErrorMessage))
				.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.ColorAndOpacity(FColor::Red);
		else
			ValueWidget = SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, PropertyHandle, &Mask]() -> TSharedRef<SWidget>
				{
					return SNew(SBox)
						.MinDesiredWidth(280)
						[
							SNew(SNiOutliner, File)
							.AddColumnsLeft({SNiOutliner::FColumn{
									.Key = "Check",
									.Title = FText::GetEmpty(),
									.Width = 25}}
								)
							.GenerateCell_Lambda([PropertyHandle, &Mask](FString ColumnName, TSharedPtr<FNiBlock> Block) -> TSharedRef<SWidget>
							{
								return SNew(SBox)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(SImage)
										.Image(FAppStyle::Get().GetBrush("Icons.Success"))
										.ColorAndOpacity_Lambda([&Mask, Block]() -> FSlateColor
										{
											return Mask.ShouldShow(Block->BlockIndex)
												? FLinearColor::White
												: FLinearColor::Transparent;
										})
										.OnMouseButtonDown_Lambda([PropertyHandle, &Mask, Block](const FGeometry&, const FPointerEvent&) -> FReply
										{
											Mask.Toggle(Block->BlockIndex);
											PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
											return FReply::Handled();
										})
									];
							})
						];
				})
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([PropertyHandle, &Mask]() -> FText
					{
						return FText::FromString(FString::Printf(TEXT("%d %hs"),
						                                         Mask.BlockIndexList.Num(),
						                                         Mask.bIsWhiteList ? "whitelisted" : "blacklisted"
							));
					})
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				];

		HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
			.ValueContent()[
				ValueWidget.ToSharedRef()
			];

		return;
	}

	HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
		.ValueContent()[PropertyHandle->CreatePropertyValueWidget()];
}

void FNiMaskCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}
