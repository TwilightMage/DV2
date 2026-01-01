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
UCLASS(DisplayName="DV2 Ghost")
class DV2_API ADV2Ghost : public AActor, public ILandscapeEditLayerRenderer
{
	GENERATED_BODY()

	friend class FDV2GhostCustomization;

public:
	DECLARE_MULTICAST_DELEGATE(FOnFileChanged);
	
	ADV2Ghost();

	UFUNCTION(BlueprintCallable)
	void SetFile(const FString& InFilePath);

	UFUNCTION(BlueprintPure)
	FString GetFilePath() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	FOnFileChanged OnFileChanged;

protected:
	void AddFileSubComponents();
	void ClearFileSubComponents();
	void SetFilePrivate(const FString& InFilePath);

	UPROPERTY()
	FString FilePath;
	TSharedPtr<FNiFile> File;

	UPROPERTY()
	USceneComponent* SceneRoot;
};
