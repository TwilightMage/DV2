#include "PinFactory.h"

#include "K2Node_CallFunction.h"
#include "Slate/SDV2AssetTreeEntryGraphPin.h"

FProperty* GetAssociatedProperty(UEdGraphPin* Pin)
{
	if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Pin->GetOwningNode()))
	{
		if (UFunction* TargetFunc = CallNode->GetTargetFunction())
		{
			if (FProperty* ParamProp = TargetFunc->FindPropertyByName(Pin->PinName))
			{
				return ParamProp;
			}
		}
	}

	return nullptr;
}

TSharedPtr<SGraphPin> FPinFactory::CreatePin(UEdGraphPin* Pin) const
{
	if (Pin)
	{
		if (Pin->Direction == EGPD_Input)
		{
			auto Property = GetAssociatedProperty(Pin);
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String && Property->HasMetaData("DV2AssetPath"))
				return SNew(SDV2AssetTreeEntryGraphPin, Pin);
		}
	}
	
	return FGraphPanelPinFactory::CreatePin(Pin);
}
