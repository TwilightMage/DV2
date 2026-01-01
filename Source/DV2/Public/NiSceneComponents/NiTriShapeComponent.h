// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiAVObjectComponent.h"
#include "NiTriShapeComponent.generated.h"

UCLASS(Abstract)
class DV2_API UNiTriShapeComponent : public UNiBlockComponentConfigurator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<NiMeta::niobject> GetTargetBlockType() const override;
	virtual bool Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx) override;
};
