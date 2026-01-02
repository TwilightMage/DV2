#pragma once
#include "SGraphPin.h"

class SDV2AssetTreeEntryGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDV2AssetTreeEntryGraphPin)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
};