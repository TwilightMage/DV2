#include "Slate/SNifSceneViewport.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "EngineUtils.h"
#include "NetImmerse.h"
#include "ProceduralMeshComponent.h"

FNifViewportClient::FNifViewportClient(const TSharedPtr<FPreviewScene>& InScene, const TSharedPtr<SNifSceneViewport>& Widget)
	: FEditorViewportClient(nullptr, InScene.Get(), Widget)
{
	EngineShowFlags.SetEditor(true);
	EngineShowFlags.SetCompositeEditorPrimitives(true);

	SetViewMode(VMI_Unlit);
	SetRealtime(true);

	UStaticMeshComponent* SkyComp = NewObject<UStaticMeshComponent>(GetWorld());
	GetPreviewScene()->AddComponent(SkyComp, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(400)));
	SkyComp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineSky/SM_SkySphere.SM_SkySphere")));
	SkyComp->SetMaterial(0, LoadObject<UMaterial>(nullptr, TEXT("/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox")));
	SkyComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyComp->SetMobility(EComponentMobility::Static);
	SkyComp->CastShadow = false;
	SkyComp->bVisibleInRayTracing = false;

	UseRootTransform = FTransform(
		FRotator::ZeroRotator,
		FVector::ZeroVector,
		FVector(1, -1, 1)
		);
}

struct FViewportSceneSpawnHandler : FNiFile::FSceneSpawnHandler
{
	FViewportSceneSpawnHandler(FNifViewportClient* InViewportClient, TMap<TSharedPtr<FNiBlock>, USceneComponent*>& InBlockToComponentMap)
		: ViewportClient(InViewportClient)
		  , BlockToComponentMap(InBlockToComponentMap)
	{
	}

	struct
	{
		TSharedPtr<FNiBlock> Block;
		TSharedPtr<FNiBlock> ParentBlock;

		FTransform RelativeTransform;
		TArray<TSharedPtr<FNiFile::FMeshDescriptor>> Meshes;
	} Current;

	FNifViewportClient* ViewportClient;
	TMap<TSharedPtr<FNiBlock>, USceneComponent*>& BlockToComponentMap;

	virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) override
	{
		auto Result = FSceneSpawnHandler::OnEnterBlock(Block, ParentBlock);
		if (Result != EBlockEnterResult::Continue)
			return Result;

		Current.Block = Block;
		Current.ParentBlock = ParentBlock;
		Current.RelativeTransform = FTransform::Identity;
		Current.Meshes.Reset();

		return EBlockEnterResult::Continue;
	}

	virtual void OnExitBlock(bool bSuccess) override
	{
		if (bSuccess)
		{
			auto SceneComponent = NewObject<USceneComponent>(GetWCO());
			SceneComponent->SetRelativeTransform(Current.RelativeTransform);
			SceneComponent->SetMobility(EComponentMobility::Static);
			//SceneComponent->Rename(*FString::Printf(TEXT("BLOCK_%d"), Block->BlockIndex));

			if (Current.ParentBlock.IsValid())
				SceneComponent->AttachToComponent(BlockToComponentMap[Current.ParentBlock], FAttachmentTransformRules::KeepRelativeTransform);
			
			ViewportClient->GetPreviewScene()->AddComponent(SceneComponent, FTransform::Identity);
			ViewportClient->GeneratedNifComponents.Add(SceneComponent);
			
			ViewportClient->BlockToComponentMap.Add(Current.Block, SceneComponent);
			ViewportClient->ComponentToBlockMap.Add(SceneComponent, Current.Block);

			for (const auto& Mesh : Current.Meshes)
			{
				auto MeshComponent = NewObject<UProceduralMeshComponent>(GetWCO());
				MeshComponent->SetMobility(EComponentMobility::Static);
				MeshComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
				MeshComponent->CastShadow = true;

				Mesh->AddSectionToProceduralMeshComponent(MeshComponent);
				
				ViewportClient->GetPreviewScene()->AddComponent(MeshComponent, FTransform::Identity);
				ViewportClient->GeneratedNifComponents.Add(MeshComponent);
			}

			if (!ViewportClient->SceneRoot)
				ViewportClient->SceneRoot = SceneComponent;
		}
	}

	virtual void OnAttachMesh(const FNiFile::FMeshDescriptor& Mesh) override
	{
		Current.Meshes.Add(MakeShared<FNiFile::FMeshDescriptor>(Mesh));
	}

	virtual void OnSetComponentTransform(const FTransform& Transform) override
	{
		Current.RelativeTransform = Transform;
	}

	virtual UObject* GetWCO() override
	{
		return ViewportClient->GetWorld();
	}
};

void FNifViewportClient::LoadNifFile(const TSharedPtr<FNiFile>& NifFile, uint32 RootBlockIndex, const FNiMask* Mask)
{
	Clear();

	if (!NifFile->Blocks.IsEmpty())
	{
		FViewportSceneSpawnHandler Handler(this, BlockToComponentMap);
		Handler.Mask = Mask;

		NifFile->SpawnScene(&Handler, RootBlockIndex);

		if (SceneRoot)
			SceneRoot->SetRelativeTransform(UseRootTransform);	
	}

	Viewport->Invalidate();
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

void FNifViewportClient::SetRootTransform(const FTransform& InRootTransform)
{
	UseRootTransform = InRootTransform;
	UseRootTransform.SetScale3D(UseRootTransform.GetScale3D() * FVector(1, -1, 1));
	
	if (SceneRoot)
	{
		SceneRoot->SetRelativeTransform(UseRootTransform);
	}
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
	SceneRoot = nullptr;
	BlockToComponentMap.Reset();
	ComponentToBlockMap.Reset();
}

void SNifSceneViewport::Construct(const FArguments& InArgs)
{
	OrbitingCamera = InArgs._OrbitingCamera;
	RootTransform = FTransform(InArgs._RootRotation, InArgs._RootOffset, InArgs._RootScale);

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SNifSceneViewport::MakeEditorViewportClient()
{
	Scene = MakeShared<FPreviewScene>();
	ViewportClient = MakeShared<FNifViewportClient>(Scene, StaticCastSharedRef<SNifSceneViewport>(AsShared()));
	ViewportClient->ViewportType = LVT_Perspective;
	if (OrbitingCamera)
	{
		ViewportClient->SetCameraLock();
		ViewportClient->SetViewLocationForOrbiting(FVector::ZeroVector);
	}
	ViewportClient->SetRootTransform(RootTransform);

	return ViewportClient.ToSharedRef();
}
