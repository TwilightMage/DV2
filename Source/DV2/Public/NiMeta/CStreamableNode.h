#pragma once
#include "NetImmerse.h"
#include "CStreamableNode.generated.h"

USTRUCT(Blueprintable, DisplayName="CStreamableNode Property")
struct FCStreamableNodeProperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Value;
};

USTRUCT(BlueprintType, DisplayName="CStreamableNode Data")
struct FCStreamableNodeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TypeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCStreamableNodeProperty> Properties;
};

class DV2_API FCStreamableNode : public TSharedFromThis<FCStreamableNode>
{
public:
	static void ReadFrom(TSharedPtr<FNiBlock>& Block, FMemoryReader& Reader);

	static FText GetNodeTitle(const FCStreamableNode& InNode);
	static FText GetPropertyTitle(uint32 InPropertyType);

	bool TraverseTree(const TFunction<bool(const TSharedPtr<FCStreamableNode>& InNodeData)>& InHandler);

	static void RefreshAliases();

	FCStreamableNodeData Data = {};

	TArray<TSharedPtr<FCStreamableNode>> Children;

	TWeakPtr<FCStreamableNode> Parent;

private:
	inline static TMap<int32, FText> Aliases;
};

UCLASS(DisplayName="CStreamableNode")
class DV2_API UCStreamableNodeHandle : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTreeIteration, const FCStreamableNodeData&, Data);
	
	TSharedPtr<FCStreamableNode> Handle;

	UFUNCTION(BlueprintPure, Category="Divinity 2|NiMeta|CStreamableNode")
	FCStreamableNodeData GetData() const { return Handle.IsValid() ? Handle->Data : FCStreamableNodeData{}; }

	UFUNCTION(BlueprintPure, Category="Divinity 2|NiMeta|CStreamableNode")
	int32 NumChildren() const { return Handle.IsValid() ? Handle->Children.Num() : 0; }

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, meta=(AdvancedDisplay="bBreakRequested"))
    static void TraverseTree(UCStreamableNodeHandle* Node, const FOnTreeIteration& LoopDelegate, UPARAM(Ref) bool& bBreakRequested);
};

class FNiBlock_CStreamableNode : public FNiBlock
{
public:
	TArray<TSharedPtr<FCStreamableNode>> CStreamableNodes;
	TArray<uint32> UniqueParameterTypes;
};

UCLASS()
class UCStreamableNodeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

};
