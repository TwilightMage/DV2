// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FNiSceneBlockHandler;
DV2_API DECLARE_LOG_CATEGORY_EXTERN(LogDV2, Display, Display)

class FDV2Module : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FDV2Module& Get();

	void RegisterNiSceneBlockHandler(const FString& NiBlockName, const TSharedPtr<FNiSceneBlockHandler>& Handler) { NiSceneBlockHandlers.Add(NiBlockName, Handler); }
	void UnregisterNiSceneBlockHandler(const FString& NiBlockName) { NiSceneBlockHandlers.Remove(NiBlockName); }
	const TMap<FString, TSharedPtr<FNiSceneBlockHandler>>& GetNiSceneBlockHandlers() const { return NiSceneBlockHandlers; }
	
	inline const static FString PluginName = TEXT("DV2");

private:
	void RegisterNiComponentConfigurators();
	
	TMap<FString, TSharedPtr<FNiSceneBlockHandler>> NiSceneBlockHandlers;
};
