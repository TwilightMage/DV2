#include "FDV2AssetPathCustomization.h"

#include "DV2AssetTree.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "K2Node_Variable.h"
#include "TextUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Slate/SDV2Explorer.h"

FText GetPathTitle(const TSharedRef<IPropertyHandle>& PropertyHandle)
{
	FString Value;
	PropertyHandle->GetValue(Value);
	if (Value.IsEmpty())
		return MAKE_TEXT("FDV2AssetPathCustomization", "none");
	return FText::FromString(Value);
}

void FDV2AssetPathCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (PropertyHandle->HasMetaData("DV2AssetPath"))
	{
		TArray<FString> PathTypes;
		PropertyHandle->GetMetaData("DV2AssetPath").ParseIntoArray(PathTypes, TEXT(","));
		for (auto& T : PathTypes)
			T = T.TrimStartAndEnd().ToLower();

		bool acceptDirectory = PathTypes.Contains("dir");
		bool acceptAnyFile = PathTypes.Contains("file") || PathTypes.IsEmpty();

		HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
			.ValueContent()[
				SNew(SComboButton)
				.OnGetMenuContent_Lambda([this, PropertyHandle, acceptDirectory, acceptAnyFile]() -> TSharedRef<SWidget>
				{
					return SNew(SBox)
						.WidthOverride(280)
						[
							SNew(SDV2Explorer)
							.OnSelectionChanged_Lambda([this, PropertyHandle, acceptDirectory, acceptAnyFile](const TSharedPtr<FDV2AssetTreeEntry>& Selection)
							{
								if (Selection.IsValid())
								{
									bool Match =
										(acceptDirectory && !Selection->IsFile()) ||
										(acceptAnyFile && Selection->IsFile());

									if (Match)
									{
										PropertyHandle->SetValue(Selection->GetPath());
									}
								}
							})
						];
				})
				.ToolTipText_Lambda([PropertyHandle]() -> FText
				{
					return GetPathTitle(PropertyHandle);
				})
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([PropertyHandle]() -> FText
					{
						return GetPathTitle(PropertyHandle);
					})
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				]
			];
	}
	else
	{
		HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
			.ValueContent()[PropertyHandle->CreatePropertyValueWidget()];
	}
}

void FDV2AssetPathCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

#define CHECKBOX_ROW(Title, Var) \
.NameContent() \
[ \
	SNew(STextBlock) \
	.Text(FText::FromString(Title)) \
	.Font(IDetailLayoutBuilder::GetDetailFont()) \
] \
.ValueContent() \
[ \
	SNew(SCheckBox) \
	.IsChecked_Lambda([this]() \
	{ \
		return Var ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; \
	}) \
	.OnCheckStateChanged_Lambda([this](ECheckBoxState InNewState) \
	{ \
		Var = InNewState == ECheckBoxState::Checked; \
		SaveMetaData(); \
	}) \
];

void FDV2AssetPathBlueprintEditor::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> EditedObjects;
	DetailBuilder.GetObjectsBeingCustomized(EditedObjects);

	if (EditedObjects.Num() == 0 && EditedObjects[0] == nullptr)
		return;

	if (UPropertyWrapper* Wrapper = Cast<UPropertyWrapper>(EditedObjects[0]))
		Property = Wrapper->GetProperty();
	else if (UK2Node_Variable* Variable = Cast<UK2Node_Variable>(EditedObjects[0]))
		Property = Variable->GetPropertyForVariable();

	if (Property == nullptr)
		return;

	CachedVariableName = Property->GetName();

	LoadMetaData();

	auto& CategoryBuilder = DetailBuilder.EditCategory("Variable");

	CategoryBuilder.AddCustomRow(FText::FromString("DV2AssetPath"))
	               .Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this]()
	               {
		               return bAssetPathEnabled ? EVisibility::Collapsed : EVisibility::Visible;
	               })))
		CHECKBOX_ROW("DV2 Asset Path", bAssetPathEnabled);

	auto& Group = CategoryBuilder.AddGroup("DV2AssetPathSettings", FText::FromString("DV2 Asset Path Settings"));

	Group.HeaderRow()
	     .Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this]()
	     {
		     return bAssetPathEnabled ? EVisibility::Visible : EVisibility::Collapsed;
	     })))
		CHECKBOX_ROW("DV2 Asset Path", bAssetPathEnabled);

	Group.AddWidgetRow()
		CHECKBOX_ROW("Directories", bDirectories);

	Group.AddWidgetRow()
		CHECKBOX_ROW("Any File", bAnyFile);
}

void FDV2AssetPathBlueprintEditor::LoadMetaData()
{
	FString Value;
	if (FBlueprintEditorUtils::GetBlueprintVariableMetaData(
		GetBlueprintObj(),
		*CachedVariableName,
		GetLocalVariableScope(),
		MD_DV2AssetPath,
		Value))
	{
		bAssetPathEnabled = true;
		
		TArray<FString> PathTypes;
		Value.ParseIntoArray(PathTypes, TEXT(","));
		for (auto& T : PathTypes)
			T = T.TrimStartAndEnd().ToLower();

		bDirectories = PathTypes.Contains("dir");
		bAnyFile = PathTypes.Contains("file") || PathTypes.IsEmpty();	
	}
	else
		bAssetPathEnabled = false;
}

void FDV2AssetPathBlueprintEditor::SaveMetaData()
{
	if (bAssetPathEnabled)
	{
		TArray<FString> Identifiers;

		if (bDirectories)
			Identifiers.Add("dir");

		if (bAnyFile)
			Identifiers.Add("file");

		FString NewMetaData = FString::Join(Identifiers, TEXT(", "));

		FBlueprintEditorUtils::SetBlueprintVariableMetaData(
			GetBlueprintObj(),
			*CachedVariableName,
			GetLocalVariableScope(),
			MD_DV2AssetPath,
			*NewMetaData
		);
	}
	else
		FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(
			GetBlueprintObj(),
			*CachedVariableName,
			GetLocalVariableScope(),
			MD_DV2AssetPath
		);
}

UStruct* FDV2AssetPathBlueprintEditor::GetLocalVariableScope() const
{
	if(Property && Property->GetOwner<UFunction>() != nullptr)
	{
		return Property->GetOwner<UFunction>();
	}

	return nullptr;
}
