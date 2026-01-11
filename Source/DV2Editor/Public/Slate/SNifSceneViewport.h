#pragma once
#include "SEditorViewport.h"

class UProceduralMeshComponent;
class FNiBlock;
struct FNiFile;
class SNifSceneViewport;

class FNifViewportClient : public FEditorViewportClient
{
	friend struct FSceneSpawnHandler;

public:
	FNifViewportClient(const TSharedPtr<FPreviewScene>& InScene, const TSharedPtr<SNifSceneViewport>& Widget);

	void LoadNifFile(const TSharedPtr<FNiFile>& NifFile, uint32 RootBlockIndex = 0);

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

	template <typename T>
	struct TSpawnedComponent
	{
		TSpawnedComponent(T* InComponent, FNifViewportClient* InClient)
			: Component(InComponent)
			  , Client(InClient)
		{
		}

		~TSpawnedComponent()
		{
			if (!bIsCancelled)
			{
				FinishSpawn();
			}
		}

		void CancelSpawn()
		{
			if (ensureAlways(!bIsCancelled))
			{
				bIsCancelled = true;
				Client->GeneratedNifComponents.Remove(Component);
			}
		}

		void FinishSpawn()
		{
			if (!bIsFinished)
			{
				bIsFinished = true;
				Client->GetPreviewScene()->AddComponent(Component, Component->GetRelativeTransform());
			}
		}

		T* operator->() { return Component; }
		const T* operator->() const { return Component; }

		T& operator*() { return *Component; }
		const T& operator*() const { return *Component; }

		operator T*() { return Component; }

		T* Component;
		FNifViewportClient* Client;
		bool bIsCancelled = false;
		bool bIsFinished = false;
	};

	template <typename T, typename... TArgs>
	TSpawnedComponent<T> SpawnComponent(TArgs... Args)
	{
		T* Result = NewObject<T>(GetWorld(), Args...);
		GeneratedNifComponents.Add(Result);

		return TSpawnedComponent<T>(Result, this);
	}

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
