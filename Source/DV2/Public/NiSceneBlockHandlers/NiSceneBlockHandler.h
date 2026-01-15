// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

struct FNiFile;
class FNiBlock;

#include "NetImmerse.h"

class FNiSceneBlockHandler
{
public:
	virtual ~FNiSceneBlockHandler() = default;
	
	virtual bool Handle(const FNiFile& File, const FNiFile::FSceneSpawnHandlerContext& Ctx) = 0;
};
