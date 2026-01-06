#pragma once

#include "BlueprintEditor.h"
#include "BlueprintEditorModule.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"

class FDV2AssetPathCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<FDV2AssetPathCustomization>();
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};

class FDV2AssetPathBlueprintEditor : public IDetailCustomization
{
public:
	FDV2AssetPathBlueprintEditor(const TWeakObjectPtr<UBlueprint>& InBlueprint)
		: Blueprint(InBlueprint)
	{}
	
	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<IBlueprintEditor> BlueprintEditor)
	{
		return MakeShared<FDV2AssetPathBlueprintEditor>(StaticCastSharedPtr<FBlueprintEditor>(BlueprintEditor)->GetBlueprintObj());
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void LoadMetaData();
	void SaveMetaData();

	UBlueprint* GetBlueprintObj() const { return Blueprint.Get(); }
	UStruct* GetLocalVariableScope() const;

	inline static const FName MD_DV2AssetPath = "DV2AssetPath";

	TWeakObjectPtr<UBlueprint> Blueprint;
	FString CachedVariableName;
	FProperty* Property = nullptr;
	
	bool bAssetPathEnabled = false;
	bool bDirectories = false;
	bool bAnyFile = true;
};
