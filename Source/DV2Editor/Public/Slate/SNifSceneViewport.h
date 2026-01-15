#pragma once
#include "SEditorViewport.h"

struct FNiMask;
class UProceduralMeshComponent;
class FNiBlock;
struct FNiFile;
class SNifSceneViewport;

class FNifViewportClient : public FEditorViewportClient
{
	friend struct FViewportSceneSpawnHandler;

public:
	FNifViewportClient(const TSharedPtr<FPreviewScene>& InScene, const TSharedPtr<SNifSceneViewport>& Widget);

	void LoadNifFile(const TSharedPtr<FNiFile>& NifFile, uint32 RootBlockIndex = 0, const FNiMask* Mask = nullptr);

	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override;
	virtual void Tick(float DeltaSeconds) override;

	const TSharedPtr<FNiBlock>& GetSelectedBlock() const { return SelectedBlock; }
	const USceneComponent* GetSelectedComponent() const { return CachedSelectedComponent; }
	void SetSelectedBlock(const TSharedPtr<FNiBlock>& Block);

	void SetRootTransform(const FTransform& InRootTransform);
	
	virtual bool RequiresHitProxyStorage() override;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;

private:
	void Clear();

	USceneComponent* BlockToComponent(const TSharedPtr<FNiBlock>& Block) const { return BlockToComponentMap.FindRef(Block); }
	TSharedPtr<FNiBlock> ComponentToBlock(const USceneComponent* Component) const { return ComponentToBlockMap.FindRef(Component); }

	TSharedPtr<FNiBlock> SelectedBlock;
	USceneComponent* CachedSelectedComponent;

	TSet<USceneComponent*> GeneratedNifComponents;
	USceneComponent* SceneRoot = nullptr;
	TMap<TSharedPtr<FNiBlock>, USceneComponent*> BlockToComponentMap;
	TMap<USceneComponent*, TSharedPtr<FNiBlock>> ComponentToBlockMap;

	FTransform UseRootTransform;
};

class SNifSceneViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SNifSceneViewport)
			: _OrbitingCamera(false)
			, _RootOffset(FVector::ZeroVector)
			, _RootRotation(FRotator::ZeroRotator)
			, _RootScale(FVector::OneVector)
		{
		}

		SLATE_ARGUMENT(bool, OrbitingCamera)
		SLATE_ARGUMENT(FVector, RootOffset)
		SLATE_ARGUMENT(FRotator, RootRotation)
		SLATE_ARGUMENT(FVector, RootScale)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

	const TSharedPtr<FNifViewportClient>& GetNifViewportClient() const { return ViewportClient; }

private:
	TSharedPtr<FNifViewportClient> ViewportClient;
	TSharedPtr<FPreviewScene> Scene;

	bool OrbitingCamera;
	FTransform RootTransform;
};
