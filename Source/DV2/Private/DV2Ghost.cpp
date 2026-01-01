// Fill out your copyright notice in the Description page of Project Settings.

#include "DV2Ghost.h"

#include "Divinity2Assets.h"
#include "NetImmerse.h"

ADV2Ghost::ADV2Ghost()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>("Scene Root");
	SetRootComponent(SceneRoot);

	PrimaryActorTick.bCanEverTick = false;
}

void ADV2Ghost::SetFile(const FString& InFilePath)
{
	FString FixedFilePath = FixPath(InFilePath);
	
	if (GetFilePath() == FixedFilePath)
		return;
		
	FilePath = FixedFilePath;
	SetFilePrivate(FixedFilePath);

	OnFileChanged.Broadcast();
}

FString ADV2Ghost::GetFilePath() const
{
	if (File.IsValid())
		return File->Path;
	return "";
}

#if WITH_EDITOR
void ADV2Ghost::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_CHECKED(ADV2Ghost, FilePath))
	{
		SetFilePrivate(FilePath);
	}
}
#endif

struct FSceneSpawnHandler : FNiFile::FSceneSpawnHandler
{
	ADV2Ghost* Ghost;

	struct HierarchyNode
	{
		FTransform RelativeTransform;
		FTransform GlobalTransform;
		TArray<TSharedPtr<HierarchyNode>> Children;
		TArray<USceneComponent*> SubComponents;
	};

	TSharedPtr<HierarchyNode> RootNode;
	TMap<TSharedPtr<FNiBlock>, TSharedPtr<HierarchyNode>> BlockToNodeMap;
	USceneComponent* Component;

	struct
	{
		TSharedPtr<FNiBlock> Block;
		TSharedPtr<FNiBlock> ParentBlock;
		TSharedPtr<HierarchyNode> Node;
	} Current;

	virtual void OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) override
	{
		Current.Block = Block;
		Current.ParentBlock = ParentBlock;
		Current.Node = MakeShared<HierarchyNode>();
		Current.Node->Children.Reserve(Block->Children.Num());
	}

	virtual void OnExitBlock(bool bSuccess) override
	{
		if (bSuccess)
		{
			if (!RootNode.IsValid())
				RootNode = Current.Node;

			if (Current.ParentBlock.IsValid())
				BlockToNodeMap[Current.ParentBlock]->Children.Add(Current.Node);

			BlockToNodeMap.Add(Current.Block, Current.Node);
		}
	}

	virtual void OnAttachSubComponent(USceneComponent* SubComponent) override
	{
		if (!SubComponent)
			return;
		
		Current.Node->SubComponents.Add(SubComponent);
	}

	virtual void OnSetComponentTransform(const FTransform& Transform) override
	{
		Current.Node->RelativeTransform = Transform;
	}

	virtual UObject* GetWCO() override
	{
		return Ghost;
	}

	void CalculateTransforms(const TFunction<void(USceneComponent*)>& ComponentHandler)
	{
		if (!RootNode)
			return;

		CalculateTransform(*RootNode, nullptr, ComponentHandler);
	}

	static void CalculateTransform(HierarchyNode& Node, const FTransform* ParentTransform, const TFunction<void(USceneComponent*)>& ComponentHandler)
	{
		if (ParentTransform)
		{
			Node.GlobalTransform = Node.RelativeTransform * *ParentTransform;
		}

		for (const auto& SubComponent : Node.SubComponents)
		{
			auto SubComponentTransform = SubComponent->GetComponentTransform();
			SubComponentTransform = SubComponentTransform * Node.GlobalTransform;
			SubComponent->SetWorldTransform(SubComponentTransform);
			ComponentHandler(SubComponent);
		}

		for (auto& Child : Node.Children)
			CalculateTransform(*Child, &Node.GlobalTransform, ComponentHandler);
	}
};

void ADV2Ghost::AddFileSubComponents()
{
	if (!File.IsValid())
		return;

	FSceneSpawnHandler Handler;
	Handler.Ghost = this;
	Handler.BlockToNodeMap.Reserve(File->Blocks.Num());
	Handler.Component = SceneRoot;

	File->SpawnScene(&Handler);
	Handler.CalculateTransforms([&](USceneComponent* SubComponent)
	{
		SubComponent->RegisterComponent();
		SubComponent->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
	});
}

void ADV2Ghost::ClearFileSubComponents()
{
	TArray<USceneComponent*> SubComponents;
	SceneRoot->GetChildrenComponents(true, SubComponents);
	for (USceneComponent* SubComponent : SubComponents)
	{
		if (!SubComponent)
			continue;
		SubComponent->UnregisterComponent();
		SubComponent->DestroyComponent();
	}
}

void ADV2Ghost::SetFilePrivate(const FString& InFilePath)
{
	ClearFileSubComponents();

	File = UNetImmerse::OpenNiFile(InFilePath, true);
	if (!File.IsValid())
		return;
	
	AddFileSubComponents();
}
