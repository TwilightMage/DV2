// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LandscapeEditLayerRenderer.h"
#include "NetImmerse.h"
#include "GameFramework/Actor.h"
#include "DV2Ghost.generated.h"

struct FNiFile;

/**
 * Use this actor to spawn NIF scenes in UE world
 */
UCLASS(DisplayName="DV2 Ghost Component", ClassGroup="Divinity 2", meta=(BlueprintSpawnableComponent))
class DV2_API UDV2GhostComponent : public USceneComponent
{
	GENERATED_BODY()

	friend class FDV2AssetPathCustomization;
	friend struct FGhostSceneSpawnHandler;

public:
	DECLARE_MULTICAST_DELEGATE(FOnFileChanged);

	UFUNCTION(BlueprintCallable)
	void SetFile(UPARAM(meta=(DV2AssetPath))
		const FString& InFilePath);

	UFUNCTION(BlueprintPure)
	FString GetFilePath() const;

	const FNiMask& GetShowMask() const { return ShowMask; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	FOnFileChanged OnFileChanged;

private:
	virtual void PostLoad() override;

	void AddFileComponents();
	void ClearSpawnedComponents();
	void SetFilePrivate(const FString& InFilePath);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DV2 Ghost Component", meta=(DV2AssetPath="dir, file", AllowPrivateAccess))
	FString FilePath;
	TSharedPtr<FNiFile> File;

	UPROPERTY(EditAnywhere, Category="DV2 Ghost Component", meta=(AssetProperty=FilePath))
	FNiMask ShowMask = FNiMask::CreateWhite();

	UPROPERTY(Transient)
	TArray<USceneComponent*> SpawnedComponents;
};

UCLASS(DisplayName="DV2 Ghost Actor", ConversionRoot, ComponentWrapperClass)
class DV2_API ADV2GhostActor : public AActor, public ILandscapeEditLayerRenderer
{
	GENERATED_BODY()

public:
	ADV2GhostActor();

	UDV2GhostComponent* GetGhostComponent() const { return Component; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=DV2GhostActor, meta=(AllowPrivateAccess))
	UDV2GhostComponent* Component;
};
