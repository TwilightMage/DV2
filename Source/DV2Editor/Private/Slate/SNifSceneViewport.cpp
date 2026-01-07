#include "Slate/SNifSceneViewport.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "EngineUtils.h"
#include "NetImmerse.h"

FNifViewportClient::FNifViewportClient(const TSharedPtr<FPreviewScene>& InScene, const TSharedPtr<SNifSceneViewport>& Widget)
	: FEditorViewportClient(nullptr, InScene.Get(), Widget)
{
	EngineShowFlags.SetEditor(true);
	EngineShowFlags.SetCompositeEditorPrimitives(true);

	SetRealtime(true);

	UStaticMeshComponent* SkyComp = NewObject<UStaticMeshComponent>(GetWorld());
	GetPreviewScene()->AddComponent(SkyComp, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(400)));
	SkyComp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineSky/SM_SkySphere.SM_SkySphere")));
	SkyComp->SetMaterial(0, LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox")));
	SkyComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyComp->CastShadow = false;
	SkyComp->bVisibleInRayTracing = false;
}

struct FSceneSpawnHandler : FNiFile::FSceneSpawnHandler
{
	FSceneSpawnHandler(FNifViewportClient* InViewportClient, TMap<TSharedPtr<FNiBlock>, USceneComponent*>& InBlockToComponentMap)
		: ViewportClient(InViewportClient)
		, BlockToComponentMap(InBlockToComponentMap)
	{}
	
	struct
	{
		TSharedPtr<FNiBlock> Block;
		TSharedPtr<FNiBlock> ParentBlock;
		USceneComponent* Component;
		TArray<USceneComponent*> SubComponents;
	} Current;

	FNifViewportClient* ViewportClient;
	TMap<TSharedPtr<FNiBlock>, USceneComponent*>& BlockToComponentMap;

	virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) override
	{
		auto Component = ViewportClient->SpawnComponent<USceneComponent>();
		Component->Rename(*FString::Printf(TEXT("BLOCK_%d"), Block->BlockIndex));
		Component.FinishSpawn();

		Current.Block = Block;
		Current.ParentBlock = ParentBlock;
		Current.Component = Component;
		Current.SubComponents.Reset();

		return EBlockEnterResult::Continue;
	}

	virtual void OnExitBlock(bool bSuccess) override
	{
		if (bSuccess)
		{
			if (Current.ParentBlock.IsValid())
				Current.Component->AttachToComponent(BlockToComponentMap[Current.ParentBlock], FAttachmentTransformRules::KeepRelativeTransform);

			ViewportClient->BlockToComponentMap.Add(Current.Block, Current.Component);
			ViewportClient->ComponentToBlockMap.Add(Current.Component, Current.Block);
			
			for (auto SubComponent : Current.SubComponents)
			{
				SubComponent->AttachToComponent(Current.Component, FAttachmentTransformRules::KeepRelativeTransform);
				ViewportClient->GetPreviewScene()->AddComponent(SubComponent, FTransform::Identity);
				
				TArray<USceneComponent*> InnerSubComponents;
				SubComponent->GetChildrenComponents(true, InnerSubComponents);
				for (auto InnerSubComponent : InnerSubComponents)
					ViewportClient->GetPreviewScene()->AddComponent(InnerSubComponent, FTransform::Identity);
			}
		}
		else
		{
			ViewportClient->GetPreviewScene()->RemoveComponent(Current.Component);
			ViewportClient->GeneratedNifComponents.Remove(Current.Component);
		}
	}

	virtual void OnAttachSubComponent(USceneComponent* SubComponent) override
	{
		Current.SubComponents.Add(SubComponent);
	}

	virtual void OnSetComponentTransform(const FTransform& Transform) override
	{
		Current.Component->SetRelativeTransform(Transform);
	}

	virtual UObject* GetWCO() override
	{
		return ViewportClient->GetWorld();
	}
};

void FNifViewportClient::LoadNifFile(const TSharedPtr<FNiFile>& NifFile)
{
	Clear();

	if (NifFile->Blocks.IsEmpty())
		return;

	FSceneSpawnHandler Handler(this, BlockToComponentMap);

	NifFile->SpawnScene(&Handler);

	if (!NifFile->Blocks.IsEmpty())
	{
		if (auto RootComponent = BlockToComponentMap.FindRef(NifFile->Blocks[0]))
		{
			auto RootScale = RootComponent->GetRelativeScale3D();
			RootScale.Y *= -1;
			RootComponent->SetRelativeScale3D(RootScale);
		}
	}

	Viewport->InvalidateHitProxy();
}

void FNifViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (SelectedBlock.IsValid())
	{
		if (CachedSelectedComponent)
		{
			PDI->DrawLine(GetViewLocation() + FVector::UpVector * -100, CachedSelectedComponent->GetComponentLocation(), FColor::Red, SDPG_Foreground);
		}
	}
}

void FNifViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas)
{
	FEditorViewportClient::Draw(InViewport, Canvas);

	if (!Canvas)
		return;

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			InViewport,
			GetScene(),
			EngineShowFlags)
		.SetRealtimeUpdate(IsRealtime()));
	FSceneView* SceneView = CalcSceneView(&ViewFamily);

	if (SelectedBlock.IsValid())
	{
		if (CachedSelectedComponent)
		{

			FVector Location = CachedSelectedComponent->GetComponentLocation();
			FVector2D TextPos;
			if (SceneView->WorldToPixel(Location, TextPos))
			{
				FText DebugText = FText::FromString(FString::Printf(TEXT("X: %f\nY: %f\nZ: %f"), Location.X, Location.Y, Location.Z));
				FCanvasTextItem TextItem(TextPos, DebugText, GEngine->GetSmallFont(), FLinearColor::White);
				TextItem.EnableShadow(FLinearColor::Black);

				Canvas->DrawItem(TextItem);
			}
		}
	}
}

void FNifViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
}

void FNifViewportClient::SetSelectedBlock(const TSharedPtr<FNiBlock>& Block)
{
	SelectedBlock = Block;
	CachedSelectedComponent = BlockToComponent(SelectedBlock);
}

bool FNifViewportClient::RequiresHitProxyStorage()
{
	return true;
}

void FNifViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);

	if (Event == IE_Released && Key == EKeys::LeftMouseButton)
	{
		HitProxy = Viewport->GetHitProxy(HitX, HitY);
		if (HitProxy)
		{
			HActor* ActorProxy = HitProxyCast<HActor>(HitProxy);
			if (ActorProxy && ActorProxy->Actor)
			{
				AActor* ClickedActor = ActorProxy->Actor;
			}
		}
	}
}

void FNifViewportClient::Clear()
{
	for (const auto& Component : GeneratedNifComponents)
	{
		GetPreviewScene()->RemoveComponent(Component);
		Component->DestroyComponent();
	}

	GeneratedNifComponents.Reset();
	BlockToComponentMap.Reset();
	ComponentToBlockMap.Reset();
}

void SNifSceneViewport::Construct(const FArguments& InArgs)
{
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SNifSceneViewport::MakeEditorViewportClient()
{
	Scene = MakeShared<FPreviewScene>();
	ViewportClient = MakeShared<FNifViewportClient>(Scene, StaticCastSharedRef<SNifSceneViewport>(AsShared()));
	ViewportClient->ViewportType = LVT_Perspective;

	return ViewportClient.ToSharedRef();
}
