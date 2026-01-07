// Fill out your copyright notice in the Description page of Project Settings.

#include "DV2Ghost.h"

#include "Divinity2Assets.h"
#include "NetImmerse.h"

void UDV2GhostComponent::SetFile(const FString& InFilePath)
{
	FString FixedFilePath = FixPath(InFilePath);
	
	if (GetFilePath() == FixedFilePath)
		return;
		
	FilePath = FixedFilePath;
	SetFilePrivate(FixedFilePath);

	OnFileChanged.Broadcast();
}

FString UDV2GhostComponent::GetFilePath() const
{
	if (File.IsValid())
		return File->Path;
	return "";
}

#if WITH_EDITOR
void UDV2GhostComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_CHECKED(UDV2GhostComponent, FilePath))
	{
		ShowMask = FNiMask::CreateWhite();
		SetFilePrivate(FilePath);
	}
	else if (PropertyChangedEvent.Property->GetName() == GET_MEMBER_NAME_CHECKED(UDV2GhostComponent, ShowMask))
	{
		SetFilePrivate(FilePath);
	}
}
#endif

void UDV2GhostComponent::PostLoad()
{
	Super::PostLoad();
	
	SetFilePrivate(FilePath);
}

struct FSceneSpawnHandler : FNiFile::FSceneSpawnHandler
{
	UDV2GhostComponent* Ghost;

	struct HierarchyNode
	{
		FTransform RelativeTransform;
		FTransform GlobalTransform;
		TArray<TSharedPtr<HierarchyNode>> Children;
		TArray<USceneComponent*> SubComponents;
	};

	TSharedPtr<HierarchyNode> RootNode;
	TMap<TSharedPtr<FNiBlock>, TSharedPtr<HierarchyNode>> BlockToNodeMap;

	struct
	{
		TSharedPtr<FNiBlock> Block;
		TSharedPtr<FNiBlock> ParentBlock;
		TSharedPtr<HierarchyNode> Node;
	} Current;

	virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) override
	{
		if (!Ghost->GetShowMask().ShouldShow(Block->BlockIndex))
			return EBlockEnterResult::SkipThis;

		Current.Block = Block;
		Current.ParentBlock = ParentBlock;
		Current.Node = MakeShared<HierarchyNode>();
		Current.Node->Children.Reserve(Block->Children.Num());
		return EBlockEnterResult::Continue;
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

	bool HasAnyObjects() const
	{
		return RootNode.IsValid();
	}

	void CalculateTransforms(const TFunction<void(USceneComponent*)>& ComponentHandler)
	{
		CalculateTransform(*RootNode, nullptr, ComponentHandler);
	}

	static void CalculateTransform(HierarchyNode& Node, const FTransform* ParentTransform, const TFunction<void(USceneComponent*)>& ComponentHandler)
	{
		if (ParentTransform)
			Node.GlobalTransform = Node.RelativeTransform * *ParentTransform;
		else
			Node.GlobalTransform = Node.RelativeTransform;

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

void UDV2GhostComponent::AddFileSubComponents()
{
	if (!File.IsValid())
		return;

	FSceneSpawnHandler Handler;
	Handler.Ghost = this;
	Handler.BlockToNodeMap.Reserve(File->Blocks.Num());

	File->SpawnScene(&Handler);

	if (Handler.HasAnyObjects())
	{
		auto RootScale = Handler.RootNode->RelativeTransform.GetScale3D();
		RootScale.Y *= -1;
		Handler.RootNode->RelativeTransform.SetScale3D(RootScale);
		
		Handler.CalculateTransforms([&](USceneComponent* SubComponent)
		{
			SubComponent->RegisterComponent();
			SubComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		});	
	}
}

void UDV2GhostComponent::ClearFileSubComponents()
{
	TArray<USceneComponent*> SubComponents;
	GetChildrenComponents(true, SubComponents);
	for (USceneComponent* SubComponent : SubComponents)
	{
		if (!SubComponent)
			continue;
		SubComponent->UnregisterComponent();
		SubComponent->DestroyComponent();
	}
}

void UDV2GhostComponent::SetFilePrivate(const FString& InFilePath)
{
	ClearFileSubComponents();

	File = UNetImmerse::OpenNiFile(InFilePath, true);
	if (!File.IsValid())
		return;
	
	AddFileSubComponents();
}

ADV2GhostActor::ADV2GhostActor()
{
	Component = CreateDefaultSubobject<UDV2GhostComponent>("Ghost Component");
	SetRootComponent(Component);
}
