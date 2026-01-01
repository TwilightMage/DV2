// Fill out your copyright notice in the Description page of Project Settings.


#include "Kismet/K2Node_TraverseCStreamable.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "KismetCompiler.h"
#include "NiMeta/CStreamableNode.h"

void UK2Node_TraverseCStreamable::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Input
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UCStreamableNodeHandle::StaticClass(), PN_Root);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, PN_Break);

	// Output
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, StaticStruct<FCStreamableNodeData>(), PN_Data);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Completed);
}

void UK2Node_TraverseCStreamable::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

    // 1. Create a temporary boolean variable for bBreakRequested
    UK2Node_TemporaryVariable* BreakRequestedVar = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
    BreakRequestedVar->VariableType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    BreakRequestedVar->AllocateDefaultPins();
    
    // Initialize to false
    BreakRequestedVar->GetVariablePin()->DefaultValue = TEXT("false");

    // 2. Create Break Handler to set variable to true
    UK2Node_AssignmentStatement* BreakHandler = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
    BreakHandler->AllocateDefaultPins();
    Schema->TryCreateConnection(BreakHandler->GetVariablePin(), BreakRequestedVar->GetVariablePin());
    BreakHandler->GetValuePin()->DefaultValue = TEXT("true");

    // 3. Create the TraverseTree function call
    UK2Node_CallFunction* CallTraverse = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CallTraverse->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UCStreamableNodeHandle, TraverseTree), 
        UCStreamableNodeHandle::StaticClass()
    );
    CallTraverse->AllocateDefaultPins();

    // 4. Create the delegate (custom event) for the loop body
    UK2Node_CustomEvent* LoopEvent = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);
    LoopEvent->CustomFunctionName = FName(*FString::Printf(TEXT("LoopBody_%s"), *CompilerContext.GetGuid(this)));
	UFunction* Sig = UCStreamableNodeHandle::StaticClass()->FindFunctionByName(FName("OnTreeIteration__DelegateSignature"));
	LoopEvent->SetDelegateSignature(Sig);
    LoopEvent->AllocateDefaultPins();

    // --- Wire Everything ---

    // Execution flow: Input Exec -> TraverseTree call -> Completed pin
    CompilerContext.MovePinLinksToIntermediate(*FindPin(UEdGraphSchema_K2::PN_Execute), *CallTraverse->GetExecPin());
    CompilerContext.MovePinLinksToIntermediate(*FindPin(UEdGraphSchema_K2::PN_Completed), *CallTraverse->GetThenPin());

    // Break pin -> Break handler
    CompilerContext.MovePinLinksToIntermediate(*FindPin(PN_Break), *BreakHandler->GetExecPin());

    // Root object input
    CompilerContext.MovePinLinksToIntermediate(*FindPin(PN_Root), *CallTraverse->FindPinChecked(FName("Node")));

    // Delegate binding
    Schema->TryCreateConnection(
        LoopEvent->GetDelegatePin(), 
        CallTraverse->FindPinChecked(FName("LoopDelegate"))
    );

    //// bBreakRequested parameter (pass by reference)
    //// Connect our variable to the function's bBreakRequested input pin
    //UK2Node_VariableGet* GetBreakVar = CompilerContext.SpawnIntermediateNode<UK2Node_VariableGet>(this, SourceGraph);
    //GetBreakVar->VariableReference.SetSelfMember(BreakRequestedVar->GetVariableName());
    //GetBreakVar->AllocateDefaultPins();
    
    Schema->TryCreateConnection(
        BreakRequestedVar->GetVariablePin(),
        CallTraverse->FindPinChecked(FName("bBreakRequested"))
    );

    // Loop pin wiring
    // The loop body's execution output goes to our Loop pin
    CompilerContext.MovePinLinksToIntermediate(*FindPin(UEdGraphSchema_K2::PN_Then), *LoopEvent->FindPinChecked(Schema->PN_Then));

    // Data pin wiring
    CompilerContext.MovePinLinksToIntermediate(*FindPin(PN_Data), *LoopEvent->FindPinChecked(TEXT("Data")));

    // BreakAllNodeLinks to clean up
    BreakAllNodeLinks();
}

void UK2Node_TraverseCStreamable::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}
