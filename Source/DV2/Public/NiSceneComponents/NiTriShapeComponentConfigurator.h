// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NiComponentConfigurator.h"

class FNiTriShapeComponentConfigurator : public FNiComponentConfigurator
{
public:
	virtual bool Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx) override;
};
