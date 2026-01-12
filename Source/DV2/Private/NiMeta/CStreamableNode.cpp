#include "NiMeta/CStreamableNode.h"

#include "DV2.h"
#include "NetImmerse.h"

#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#endif

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
	Block->ClearError();

	UE_LOG(LogDV2, Display, TEXT("Parsed CStreamableNode: %d nodes total, %d root node, %d properties total, %d unique properties"), TotalNodes, RootNodes.Num(), TotalProperties, UniquePropertyTypes.Num());
}

FText FCStreamableNode::GetNodeTitle(const FCStreamableNode& InNode)
{
	FString Result;
	if (auto Alias = Aliases.Find(InNode.Data.TypeId))
		Result = *Alias;
	else
		Result = FString::Printf(TEXT("Node #%d"), InNode.Data.TypeId);
	
	if (!InNode.Data.Name.IsEmpty())
		Result += " \"" + InNode.Data.Name + "\"";
	
	return FText::FromString(Result);
}

FText FCStreamableNode::GetPropertyTitle(uint32 InPropertyType)
{
	if (auto Value = Aliases.Find(InPropertyType))
		return FText::FromString(*Value);
	
	return FText::FromString(FString::Printf(TEXT("Property #%d"), InPropertyType));
}

FString FCStreamableNode::GetNodeTypeName(uint32 InNodeType, const FString& NumberPrefix)
{
	if (auto Value = Aliases.Find(InNodeType))
		return Value->Replace(TEXT(" "), TEXT(""));
	
	return NumberPrefix + FString::FromInt(InNodeType);
}

FString FCStreamableNode::GetPropertyTypeName(uint32 InPropertyType, const FString& NumberPrefix)
{
	if (auto Value = Aliases.Find(InPropertyType))
		return Value->Replace(TEXT(" "), TEXT(""));
	
	return NumberPrefix + FString::FromInt(InPropertyType);
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

void FCStreamableNode::RefreshAliases()
{
#if WITH_EDITOR
	Aliases.Empty();

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(*FDV2Module::PluginName);
	FString ConfigPath = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir() / TEXT("Resources") / TEXT("NiMeta") / TEXT("CStreamableAliases.ini"));

	GConfig->LoadFile(ConfigPath);
	
	FConfigFile* ConfigFile = GConfig->FindConfigFile(ConfigPath);
	if (ConfigFile)
	{
		// Iterate through the map of sections: Key = Section Name, Value = Section Data
		for (auto& SectionPair : *ConfigFile)
		{
			const FConfigSection& Section = SectionPair.Value;

			for (FConfigSection::TConstIterator It(Section); It; ++It)
			{
				int32 Key;
				LexFromString(Key, *It.Key().ToString());
				
				FString Value = It.Value().GetValue();

				Aliases.Add(Key, Value);
			}
		}
	}
#endif
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
