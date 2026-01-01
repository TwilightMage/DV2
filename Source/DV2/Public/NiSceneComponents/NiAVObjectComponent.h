// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiBlockComponentConfigurator.h"
#include "Components/SceneComponent.h"
#include "NiAVObjectComponent.generated.h"

UCLASS(Abstract)
class DV2_API UNiAVObjectComponent : public UNiBlockComponentConfigurator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<NiMeta::niobject> GetTargetBlockType() const override;
	virtual bool Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx) override;
};
