// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DV2Settings.generated.h"

UCLASS(config=DV2, defaultconfig, meta=(DisplayName="Divinity 2"))
class DV2_API UDV2Settings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDV2Settings();
	
	UPROPERTY(EditAnywhere, config, DisplayName="Divinity 2 Asset Location")
	FDirectoryPath Divinity2AssetLocation;

	UPROPERTY(EditAnywhere, config, meta=(RelativePath))
	FDirectoryPath ExportLocation;

	// Material to use on preview scenes as Base material
	UPROPERTY(EditAnywhere, config, meta=(RelativePath))
	TSoftObjectPtr<UMaterial> BaseMaterial;

	UPROPERTY(EditAnywhere, config, meta=(RelativePath))
	TArray<FFilePath> NifMetaFiles;
};
