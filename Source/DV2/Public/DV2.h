// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FNiComponentConfigurator;
DV2_API DECLARE_LOG_CATEGORY_EXTERN(LogDV2, Display, Display)

class FDV2Module : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FDV2Module& Get();

	void RegisterNiComponentConfigurator(const FString& NiBlockName, const TSharedPtr<FNiComponentConfigurator>& Configurator);
	const TMap<FString, TSharedPtr<FNiComponentConfigurator>>& GetNiComponentConfigurators() const { return NiComponentConfigurators; }

	inline const static FString PluginName = TEXT("DV2");

private:
	void RegisterNiComponentConfigurators();
	
	TMap<FString, TSharedPtr<FNiComponentConfigurator>> NiComponentConfigurators;
};
