// Fill out your copyright notice in the Description page of Project Settings.

#include "DV2Ghost.h"

#include "Divinity2Assets.h"
#include "NetImmerse.h"
#include "ProceduralMeshComponent.h"

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

struct FSceneSpawnHandlerData
{
	TArray<USceneComponent*> SubComponents;
};

struct FGhostSceneSpawnHandler : FVirtualNiSceneSpawnHandler<FSceneSpawnHandlerData>
{
	UDV2GhostComponent* Ghost;

	virtual void OnAttachMesh(const FNiFile::FMeshDescriptor& Mesh) override
	{
		auto MeshComponent = NewObject<UProceduralMeshComponent>(GetWCO());
		MeshComponent->CastShadow = true;

		Mesh.AddSectionToProceduralMeshComponent(MeshComponent);

		Current.Node->Data.SubComponents.Add(MeshComponent);
	}

	virtual UObject* GetWCO() override
	{
		return Ghost;
	}

	virtual void OnFinalizeNode(HierarchyNode& Node) override
	{
		for (const auto& SubComponent : Node.Data.SubComponents)
		{
			auto SubComponentTransform = SubComponent->GetRelativeTransform();
			SubComponentTransform = SubComponentTransform * Node.GlobalTransform;
			SubComponent->RegisterComponent();
			SubComponent->AttachToComponent(Ghost, FAttachmentTransformRules::SnapToTargetIncludingScale);
			SubComponent->SetRelativeTransform(SubComponentTransform);
			Ghost->SpawnedComponents.Add(SubComponent);
		}
	}
};

void UDV2GhostComponent::AddFileComponents()
{
	if (!File.IsValid())
		return;

	FGhostSceneSpawnHandler Handler;
	Handler.Mask = &ShowMask;
	Handler.Ghost = this;
	Handler.BlockToNodeMap.Reserve(File->Blocks.Num());

	File->SpawnScene(&Handler);

	if (Handler.HasAnyObjects())
	{
		auto RootScale = Handler.RootNode->RelativeTransform.GetScale3D();
		RootScale.Y *= -1;
		Handler.RootNode->RelativeTransform.SetScale3D(RootScale);
		
		Handler.Finalize();
	}
}

void UDV2GhostComponent::ClearSpawnedComponents()
{
	for (USceneComponent* Component : SpawnedComponents)
	{
		if (!Component)
			continue;
		Component->UnregisterComponent();
		Component->DestroyComponent();
	}
}

void UDV2GhostComponent::SetFilePrivate(const FString& InFilePath)
{
	ClearSpawnedComponents();

	File = UNetImmerse::OpenNiFile(InFilePath, true);
	if (!File.IsValid())
		return;
	
	AddFileComponents();
}

ADV2GhostActor::ADV2GhostActor()
{
	Component = CreateDefaultSubobject<UDV2GhostComponent>("Ghost Component");
	SetRootComponent(Component);
}
