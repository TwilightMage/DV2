// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetImmerse.h"
#include "NiMeta/NiMeta.h"
#include "NiBlockComponentConfigurator.generated.h"

struct FNiFile;
class FNiBlock;

UCLASS(Abstract)
class DV2_API UNiBlockComponentConfigurator : public UObject
{
	GENERATED_BODY()

public:
	
	virtual TSharedPtr<NiMeta::niobject> GetTargetBlockType() const { return nullptr; }
	virtual bool Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx) { return true; }
};
