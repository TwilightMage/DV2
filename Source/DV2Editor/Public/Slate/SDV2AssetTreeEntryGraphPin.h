#pragma once
#include "SGraphPin.h"

class SDV2AssetTreeEntryGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDV2AssetTreeEntryGraphPin)
		{
		}

		SLATE_ARGUMENT(TArray<FString>, PathTypes)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

private:
	TArray<FString> PathTypes;

	bool acceptDirectory = false;
	bool acceptAnyFile = true;
};
