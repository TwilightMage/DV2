// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

struct FNiFile;
class FNiBlock;

#include "NetImmerse.h"

class FNiComponentConfigurator
{
public:
	virtual ~FNiComponentConfigurator() = default;
	
	virtual bool Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx) = 0;
};
