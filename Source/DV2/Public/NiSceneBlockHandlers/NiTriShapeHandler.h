// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NiSceneBlockHandler.h"

class FNiTriShapeHandler : public FNiSceneBlockHandler
{
public:
	virtual bool Handle(const FNiFile& File, const FNiFile::FSceneSpawnHandlerContext& Ctx) override;
};
