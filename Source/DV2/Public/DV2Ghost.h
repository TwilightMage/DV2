// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LandscapeEditLayerRenderer.h"
#include "GameFramework/Actor.h"
#include "DV2Ghost.generated.h"

struct FNiFile;

/**
 * Use this actor to spawn NIF scenes UE world
 */
UCLASS(DisplayName="DV2 Ghost Component", ClassGroup="Divinity 2", meta=(BlueprintSpawnableComponent))
class DV2_API UDV2GhostComponent : public USceneComponent
{
	GENERATED_BODY()

	friend class FDV2AssetPathCustomization;
	
public:
	DECLARE_MULTICAST_DELEGATE(FOnFileChanged);

	UFUNCTION(BlueprintCallable)
	void SetFile(UPARAM(meta=(DV2AssetPath)) const FString& InFilePath);

	UFUNCTION(BlueprintPure)
	FString GetFilePath() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	FOnFileChanged OnFileChanged;

private:
	virtual void PostLoad() override;
	
	void AddFileSubComponents();
	void ClearFileSubComponents();
	void SetFilePrivate(const FString& InFilePath);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DV2 Ghost Component", meta=(DV2AssetPath="dir, file", AllowPrivateAccess))
	FString FilePath;
	TSharedPtr<FNiFile> File;
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
