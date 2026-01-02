// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LandscapeEditLayer.h"
#include "DV2LandscapeBrush.generated.h"

class ADV2GhostActor;

UCLASS(DisplayName="DV2 Landscape Brush")
class DV2_API UDV2LandscapeBrush : public ULandscapeEditLayerProcedural
{
	GENERATED_BODY()

public:
	UDV2LandscapeBrush();
	
	UPROPERTY(EditAnywhere)
	TArray<ADV2GhostActor*> UsedActors;

private:
	UPROPERTY()
	UTextureRenderTarget2D* InternalCaptureTarget;
};
