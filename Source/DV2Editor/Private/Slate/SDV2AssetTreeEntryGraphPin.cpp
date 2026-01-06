#include "Slate/SDV2AssetTreeEntryGraphPin.h"

#include "DV2AssetTree.h"
#include "DV2Importer/unpack.h"
#include "Slate/SDV2Explorer.h"

// Copied from SGraphPinObject.cpp
namespace GraphPinObjectDefs
{
	// Active Combo pin alpha
	static const float ActiveComboAlpha = 1.f;
	// InActive Combo pin alpha
	static const float InActiveComboAlpha = 0.6f;
	// Active foreground pin alpha
	static const float ActivePinForegroundAlpha = 1.f;
	// InActive foreground pin alpha
	static const float InactivePinForegroundAlpha = 0.15f;
	// Active background pin alpha
	static const float ActivePinBackgroundAlpha = 0.8f;
	// InActive background pin alpha
	static const float InactivePinBackgroundAlpha = 0.4f;
}

void SDV2AssetTreeEntryGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	PathTypes = InArgs._PathTypes;
	SGraphPin::Construct(SGraphPin::FArguments(), InPin);
}

TSharedRef<SWidget> SDV2AssetTreeEntryGraphPin::GetDefaultValueWidget()
{
	if (GraphPinObj->LinkedTo.Num() > 0)
		return SNullWidget::NullWidget;
	
	acceptDirectory = PathTypes.Contains("dir");
	acceptAnyFile = PathTypes.Contains("file") || PathTypes.IsEmpty();

	return SNew(SComboButton)
		.ButtonStyle(FAppStyle::Get(), "PropertyEditor.AssetComboStyle")
		.ForegroundColor_Lambda([this]() -> FSlateColor
		{
			float Alpha = (IsHovered() || bOnlyShowDefaultValue) ? GraphPinObjectDefs::ActiveComboAlpha : GraphPinObjectDefs::InActiveComboAlpha;
			return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
		})
		.ContentPadding(FMargin(2.0f, 2.0f, 2.0f, 1.0f))
		.ButtonColorAndOpacity_Lambda([this]() -> FSlateColor
		{
			float Alpha = (IsHovered() || bOnlyShowDefaultValue) ? GraphPinObjectDefs::ActivePinBackgroundAlpha : GraphPinObjectDefs::InactivePinBackgroundAlpha;
			return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
		})
		.MenuPlacement(MenuPlacement_BelowAnchor)
		.IsEnabled(this, &SGraphPin::IsEditingEnabled)
		.ButtonContent()
		[
			SNew(STextBlock)
			.ColorAndOpacity_Lambda([this]() -> FSlateColor
			{
				float Alpha = (IsHovered() || bOnlyShowDefaultValue) ? GraphPinObjectDefs::ActiveComboAlpha : GraphPinObjectDefs::InActiveComboAlpha;
				return FSlateColor(FLinearColor(1.f, 1.f, 1.f, Alpha));
			})
			.TextStyle(FAppStyle::Get(), "PropertyEditor.AssetClass")
			.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
			.Text_Lambda([this]()
			{
				return FText::FromString(GraphPinObj->DefaultValue);
			})
			.ToolTipText_Lambda([this]()
			{
				return FText::FromString(GraphPinObj->DefaultValue);
			})
		]
		.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
		{
			return SNew(SBox)
				.WidthOverride(280)
				[
					SNew(SDV2Explorer)
					.OnSelectionChanged_Lambda([this](const TSharedPtr<FDV2AssetTreeEntry>& Selection)
					{
						if (Selection.IsValid())
						{
							bool Match =
								(acceptDirectory && !Selection->IsFile()) ||
								(acceptAnyFile && Selection->IsFile());

							if (Match)
							{
								GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, Selection->GetPath());
							}
						}
					})
				];
		});
}
