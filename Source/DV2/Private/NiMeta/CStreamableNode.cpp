#include "NiMeta/CStreamableNode.h"

#include "DV2.h"
#include "NetImmerse.h"

void FCStreamableNode::ReadFrom(TSharedPtr<FNiBlock>& Block, FMemoryReader& Reader)
{
	constexpr uint8 NF_HAS_CHILDREN = 0x1;
	constexpr uint8 NF_HAS_PROPERTIES = 0x2;
	constexpr uint8 NF_HAS_NAME = 0x4;
	constexpr uint8 NF_COMPRESSED_INTS = 0x8;

	uint32 TotalNodes;
	uint32 TotalProperties;
	uint32 StringPoolSize;

	Reader << TotalNodes << TotalProperties << StringPoolSize;

	TArray<uint8> StringPool;
	StringPool.SetNum(StringPoolSize);
	Reader.ByteOrderSerialize(StringPool.GetData(), StringPoolSize);

	uint32 CurrentStringOffset = 0;
	TQueue<TSharedPtr<FCStreamableNode>> ParentQueue;
	TArray<TSharedPtr<FCStreamableNode>> RootNodes;
	TArray<uint32> UniquePropertyTypes; // TArray instead of TSet to preserve insertion order

	auto ReadInt = [&](uint8 NodeFlags) -> uint32
	{
		if (NodeFlags & NF_COMPRESSED_INTS)
		{
			uint8 Val8;
			Reader << Val8;
			return Val8;
		}
		else
		{
			uint8 Val32;
			Reader << Val32;
			return Val32;
		}
	};

	auto GetNextString = [&]() -> FString
	{
		FString Result = UTF8_TO_TCHAR(StringPool.GetData() + CurrentStringOffset);
		CurrentStringOffset += Result.Len() + 1;
		return Result;
	};

	for (uint32 i = 0; i < TotalNodes; i++)
	{
		TSharedPtr<FCStreamableNode> Node = MakeShared<FCStreamableNode>();
		uint8 NodeFlags;
		Reader << NodeFlags << Node->Data.TypeId;

		TSharedPtr<FCStreamableNode> CurrentParent = nullptr;
		if (!ParentQueue.IsEmpty())
			CurrentParent = *ParentQueue.Peek();
		else
			RootNodes.Add(Node);

		if (NodeFlags & NF_HAS_NAME)
		{
			Node->Data.Name = GetNextString();
		}

		if (NodeFlags & NF_HAS_PROPERTIES)
		{
			uint32 PropertyCount = ReadInt(NodeFlags);
			Node->Data.Properties.SetNum(PropertyCount);

			for (uint32 j = 0; j < PropertyCount; j++)
			{
				Reader << Node->Data.Properties[j].Type;
				Node->Data.Properties[j].Value = GetNextString();

				UniquePropertyTypes.AddUnique(Node->Data.Properties[j].Type);
			}
		}

		if (NodeFlags & NF_HAS_CHILDREN)
		{
			uint32 ChildrenCount = ReadInt(NodeFlags);
			Node->Children.Reserve(ChildrenCount);

			ParentQueue.Enqueue(Node);
		}

		if (CurrentParent)
		{
			CurrentParent->Children.Add(Node);
			Node->Parent = CurrentParent;

			if (CurrentParent->Children.Num() >= CurrentParent->Children.Max())
			{
				ParentQueue.Pop();
			}
		}
	}

	auto NewBlock = MakeShared<FNiBlock_CStreamableNode>(*Block);
	NewBlock->CStreamableNodes = RootNodes;
	NewBlock->UniqueParameterTypes = UniquePropertyTypes;
	Block = NewBlock;
	Block->Error = "";

	UE_LOG(LogDV2, Display, TEXT("Parsed CStreamableNode: %d nodes total, %d root node, %d properties total, %d unique properties"), TotalNodes, RootNodes.Num(), TotalProperties, UniquePropertyTypes.Num());
}

FText FCStreamableNode::GetNodeTitle(const FCStreamableNode& InNode)
{
	if (InNode.Data.Name.IsEmpty())
		return FText::FromString(FString::Printf(TEXT("Node #%d"), InNode.Data.TypeId));
	return FText::FromString(InNode.Data.Name);
}

FText FCStreamableNode::GetPropertyTitle(uint32 InPropertyType)
{
	return FText::FromString(FString::Printf(TEXT("Property #%d"), InPropertyType));
}

bool FCStreamableNode::TraverseTree(const TFunction<bool(const TSharedPtr<FCStreamableNode>& InNodeData)>& InHandler)
{
	if (!InHandler(AsShared()))
		return false;

	for (const auto& Child : Children)
	{
		if (!Child->TraverseTree(InHandler))
			return false;
	}
	
	return true;
}

void UCStreamableNodeHandle::TraverseTree(UCStreamableNodeHandle* Node, const FOnTreeIteration& LoopDelegate, bool& bBreakRequested)
{
	if (IsValid(Node) && Node->Handle.IsValid() && LoopDelegate.IsBound())
	{
		Node->Handle->TraverseTree([&](const TSharedPtr<FCStreamableNode>& InNode) -> bool
		{
			LoopDelegate.Execute(InNode->Data);
			return !bBreakRequested;
		});
	}
}
