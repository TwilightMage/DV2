// Fill out your copyright notice in the Description page of Project Settings.

#include "Kismet/K2Node_GetNiFieldValue.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_SwitchEnum.h"
#include "KismetCompiler.h"
#include "NetImmerseFunctionLibrary.h"

FEdGraphPinType CreatePinType(const FName& Category, const TWeakObjectPtr<UObject>& SubCategoryObject = nullptr)
{
	FEdGraphPinType Result;
	Result.PinCategory = Category;
	Result.PinSubCategoryObject = SubCategoryObject;

	return Result;
}

const TArray<UK2Node_GetNiFieldValue::FType> UK2Node_GetNiFieldValue::Types = {
	{ENiFieldValueType::Boolean, CreatePinType(UEdGraphSchema_K2::PC_Boolean)},
	{ENiFieldValueType::Byte, CreatePinType(UEdGraphSchema_K2::PC_Byte)},
	{ENiFieldValueType::Integer, CreatePinType(UEdGraphSchema_K2::PC_Int)},
	{ENiFieldValueType::Integer64, CreatePinType(UEdGraphSchema_K2::PC_Int64)},
	{ENiFieldValueType::Float, []()
	{
		auto Pin = CreatePinType(UEdGraphSchema_K2::PC_Real);
		Pin.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		return Pin;
	}()},
	{ENiFieldValueType::Name, CreatePinType(UEdGraphSchema_K2::PC_Name)},
	{ENiFieldValueType::String, CreatePinType(UEdGraphSchema_K2::PC_String)},
	{ENiFieldValueType::Text, CreatePinType(UEdGraphSchema_K2::PC_Text)},
	{ENiFieldValueType::Vector, CreatePinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get())},
	{ENiFieldValueType::Vector2D, CreatePinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector2D>::Get())},
	{ENiFieldValueType::Rotator, CreatePinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FRotator>::Get())},
	{ENiFieldValueType::Transform, CreatePinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FTransform>::Get())},
};

void UK2Node_GetNiFieldValue::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Input
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UNiFileHandle::StaticClass(), PN_File);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, PN_BlockIndex);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, PN_FieldName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, PN_ValueIndex);

	// Output
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, PN_Value);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_BadBlockIndex);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_BadFieldName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_BadType);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_BadValueIndex);
}

void UK2Node_GetNiFieldValue::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	// Get all our pins
	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* FilePin = FindPinChecked(PN_File);
	UEdGraphPin* BlockIndexPin = FindPinChecked(PN_BlockIndex);
	UEdGraphPin* FieldNamePin = FindPinChecked(PN_FieldName);
	UEdGraphPin* ValueIndexPin = FindPinChecked(PN_ValueIndex);
	UEdGraphPin* ThenPin = GetThenPin();
	UEdGraphPin* ValuePin = FindPinChecked(PN_Value);
	UEdGraphPin* BadBlockIndexPin = FindPinChecked(PN_BadBlockIndex);
	UEdGraphPin* BadFieldNamePin = FindPinChecked(PN_BadFieldName);
	UEdGraphPin* BadTypePin = FindPinChecked(PN_BadType);
	UEdGraphPin* BadValueIndexPin = FindPinChecked(PN_ValueIndex);

	// Determine which function to call based on the ValuePin type
	UFunction* FunctionToCall = PinTypeToFunction(ValuePin->PinType);

	if (!FunctionToCall)
	{
		CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Unknown pin type for value pin"), this));
		return;
	}

	// Create the function call node
	UK2Node_CallFunction* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunctionNode->SetFromFunction(FunctionToCall);
	CallFunctionNode->AllocateDefaultPins();

	// Move execution connections
	CompilerContext.MovePinLinksToIntermediate(*ExecPin, *CallFunctionNode->GetExecPin());

	// Move parameter connections
	CompilerContext.MovePinLinksToIntermediate(*FilePin, *CallFunctionNode->FindPinChecked(TEXT("self")));
	CompilerContext.MovePinLinksToIntermediate(*BlockIndexPin, *CallFunctionNode->FindPinChecked(TEXT("BlockIndex")));
	CompilerContext.MovePinLinksToIntermediate(*FieldNamePin, *CallFunctionNode->FindPinChecked(TEXT("FieldName")));
	CompilerContext.MovePinLinksToIntermediate(*ValueIndexPin, *CallFunctionNode->FindPinChecked(TEXT("ValueIndex")));

	// Get the return value pin (the enum)
	UEdGraphPin* FunctionReturnPin = CallFunctionNode->GetReturnValuePin();

	// Create a switch node for the enum result
	UK2Node_SwitchEnum* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchEnum>(this, SourceGraph);
	SwitchNode->Enum = StaticEnum<EGetFieldValueResult>();
	SwitchNode->AllocateDefaultPins();

	// Connect the function's return value to the switch
	FunctionReturnPin->MakeLinkTo(SwitchNode->GetSelectionPin());

	// Move the value output
	UEdGraphPin* FunctionOutValuePin = CallFunctionNode->FindPinChecked(FName("OutValue"));
	CompilerContext.MovePinLinksToIntermediate(*ValuePin, *FunctionOutValuePin);

	// Connect function's then pin to sequence
	UEdGraphPin* FunctionThenPin = CallFunctionNode->GetThenPin();
	FunctionThenPin->MakeLinkTo(SwitchNode->GetExecPin());

	// Connect switch enum values to our output pins
	CompilerContext.MovePinLinksToIntermediate(*ThenPin, *SwitchNode->FindPinChecked(FName("Success")));
	CompilerContext.MovePinLinksToIntermediate(*BadBlockIndexPin, *SwitchNode->FindPinChecked(FName("BadBlockIndex")));
	CompilerContext.MovePinLinksToIntermediate(*BadFieldNamePin, *SwitchNode->FindPinChecked(FName("BadFieldName")));
	CompilerContext.MovePinLinksToIntermediate(*BadValueIndexPin, *SwitchNode->FindPinChecked(FName("BadValueIndex")));
	CompilerContext.MovePinLinksToIntermediate(*BadTypePin, *SwitchNode->FindPinChecked(FName("BadType")));

	// Break all links to our original pins (they've been moved to intermediate nodes)
	BreakAllNodeLinks();
}

void UK2Node_GetNiFieldValue::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		NodeSpawner->DefaultMenuSignature.Keywords = FText::FromString("NiFileHandle, Custom");
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UK2Node_GetNiFieldValue::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	if (Context->Pin && Context->Pin->Direction == EGPD_Output && Context->Pin->PinName == PN_Value)
	{
		FToolMenuSection& Section = Menu->AddSection("TypeSelection", FText::FromString("Set Type"));

		for (int32 i = 0; i < StaticEnum<ENiFieldValueType>()->NumEnums() - 2; i++)
		{
			FText Name = StaticEnum<ENiFieldValueType>()->GetDisplayNameTextByIndex(i);
			Section.AddMenuEntry(
				NAME_None,
				Name,
				FORMAT_TEXT_INCLASS("Set output type to {0}", Name),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateUObject(const_cast<UK2Node_GetNiFieldValue*>(this), &UK2Node_GetNiFieldValue::SetValueType, (ENiFieldValueType)i))
				);
		}
	}
}

void UK2Node_GetNiFieldValue::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin)
	{
		UEdGraphPin* ThenPin = FromPin->GetOwningNode()->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* ExecutePin = FindPin(UEdGraphSchema_K2::PN_Execute);
		if (ExecutePin && ThenPin && ThenPin->LinkedTo.IsEmpty() && GetSchema()->ArePinsCompatible(ThenPin, ExecutePin, nullptr))
		{
			GetSchema()->TryCreateConnection(ThenPin, ExecutePin);
			GetGraph()->NotifyGraphChanged();
		}

		UEdGraphPin* FilePin = FindPin(PN_File);
		if (FilePin && GetSchema()->ArePinsCompatible(FromPin, FilePin, nullptr))
		{
			GetSchema()->TryCreateConnection(FromPin, FilePin);
			GetGraph()->NotifyGraphChanged();
		}
	}
}

void UK2Node_GetNiFieldValue::PostReconstructNode()
{
	Super::PostReconstructNode();

	const UEdGraphSchema* Schema = GetSchema();
	UEdGraphPin* ValuePin = FindPinChecked(PN_Value);

	if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard && ValuePin->LinkedTo.Num() > 0)
	{
		if (UEdGraphPin* ConnectedPin = ValuePin->LinkedTo[0])
		{
			for (const auto& Type : Types)
			{
				if (Schema->ArePinTypesEquivalent(ConnectedPin->PinType, Type.PinType))
				{
					SetValueType(Type.ValueType);
					break;
				}
			}
		}
	}
}

bool UK2Node_GetNiFieldValue::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (MyPin->Direction == EGPD_Output && MyPin->PinName == PN_Value)
	{
		const UEdGraphSchema* Schema = GetSchema();

		OutReason = "Invalid Type";
		for (const auto& Type : Types)
		{
			if (Schema->ArePinTypesEquivalent(Type.PinType, OtherPin->PinType))
			{
				OutReason = Schema->ArePinTypesEquivalent(Type.PinType, MyPin->PinType)
					? ""
					: FString::Printf(TEXT("Change value type to %s"), *StaticEnum<ENiFieldValueType>()->GetDisplayNameTextByIndex((int32)Type.ValueType).ToString());
				return false;
			}
		}
		return true;
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_GetNiFieldValue::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->Direction == EGPD_Output && Pin->PinName == PN_Value)
	{
		const UEdGraphSchema* Schema = GetSchema();
		
		if (Pin->LinkedTo.Num() > 0)
		{
			auto OtherPin = Pin->LinkedTo[0];
			for (const auto& Type : Types)
			{
				if (Schema->ArePinTypesEquivalent(Type.PinType, OtherPin->PinType))
				{
					SetValueType(Type.ValueType);
					return;
				}
			}
			SetValueType(ENiFieldValueType::Invalid);
		}
	}
}

FSlateIcon UK2Node_GetNiFieldValue::GetIconAndTint(FLinearColor& OutColor) const
{
	return Super::GetIconAndTint(OutColor);
}

FLinearColor UK2Node_GetNiFieldValue::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

void UK2Node_GetNiFieldValue::SetValueType(ENiFieldValueType ValueType)
{
	UEdGraphPin* ValuePin = FindPin(PN_Value);

	FEdGraphPinType NewPinType;
	NewPinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;

	for (const auto& Type : Types)
	{
		if (Type.ValueType == ValueType)
		{
			NewPinType = Type.PinType;
			break;
		}
	}

	ValuePin->PinType = NewPinType;
	GetGraph()->NotifyGraphChanged();
}

UFunction* UK2Node_GetNiFieldValue::PinTypeToFunction(const FEdGraphPinType& PinType)
{
	UClass* FunctionClass = UNiFileHandle::StaticClass();

	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
		return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Bool));
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
		return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Int32));
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
		return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Int64));
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		if (PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
			return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Float));
	}
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_String)
		return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_String));
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
		return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Name));
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
		return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Text));
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		if (PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
			return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Vector));
		if (PinType.PinSubCategoryObject == TBaseStructure<FVector2D>::Get())
			return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Vector2D));
		if (PinType.PinSubCategoryObject == TBaseStructure<FRotator>::Get())
			return FunctionClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UNiFileHandle, GetFieldValue_Rotator));
	}

	return nullptr;
}
