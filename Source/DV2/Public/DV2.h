// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

DV2_API DECLARE_LOG_CATEGORY_EXTERN(LogDV2, Display, Display)

class FDV2Module : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	inline const static FString PluginName = TEXT("DV2");
};
