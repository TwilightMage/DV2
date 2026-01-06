// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "TextUtils.h"
#include "K2Node_GetNiFieldValue.generated.h"

UENUM()
enum class ENiFieldValueType : uint8
{
	Boolean,
	Byte,
	Integer,
	Integer64,
	Float,
	Name,
	String,
	Text,
	Vector,
	Vector2D,
	Rotator,
	Transform,
	Invalid,
};

UCLASS()
class DV2UNCOOKEDONLY_API UK2Node_GetNiFieldValue : public UK2Node
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void PostReconstructNode() override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return MAKE_TEXT_INCLASS("Get NI Field Value"); }
	virtual FText GetMenuCategory() const override { return MAKE_TEXT_INCLASS("Divinity 2|Net Immerse"); }

	inline static FName PN_File = "File";
	inline static FName PN_BlockIndex = "BlockIndex";
	inline static FName PN_FieldName = "FieldName";
	inline static FName PN_ValueIndex = "ValueIndex";
	inline static FName PN_Value = "Value";
	inline static FName PN_BadBlockIndex = "BadBlockIndex";
	inline static FName PN_BadFieldName = "BadFieldName";
	inline static FName PN_BadType = "BadType";
	inline static FName PN_BadValueIndex = "BadValueIndex";

private:
	struct FType
	{
		ENiFieldValueType ValueType;
		FEdGraphPinType PinType;
	};

	UFUNCTION()
	void SetValueType(ENiFieldValueType ValueType);

	static UFunction* PinTypeToFunction(const FEdGraphPinType& PinType);

	static const TArray<FType> Types;
};
