// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "TextUtils.h"
#include "K2Node_TraverseCStreamable.generated.h"

UCLASS()
class DV2UNCOOKEDONLY_API UK2Node_TraverseCStreamable : public UK2Node
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return MAKE_TEXT_INCLASS("Traverse CStreamableNode Tree"); }
	virtual FText GetMenuCategory() const override { return MAKE_TEXT_INCLASS("Flow Control|Divinity 2"); }

	inline static FName PN_Root = "Root";
	inline static FName PN_Data = "Data";
	inline static FName PN_Break = "Break";
};
