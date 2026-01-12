// Copyright Epic Games, Inc. All Rights Reserved.

#include "DV2.h"

#include "NiSceneComponents/NiAVObjectComponentConfigurator.h"
#include "NiSceneComponents/NiTriShapeComponentConfigurator.h"

#define LOCTEXT_NAMESPACE "FDV2Module"

DEFINE_LOG_CATEGORY(LogDV2)

void FDV2Module::StartupModule()
{
	RegisterNiComponentConfigurators();
}

void FDV2Module::ShutdownModule()
{
}

FDV2Module& FDV2Module::Get()
{
	return FModuleManager::GetModuleChecked<FDV2Module>("DV2");
}

void FDV2Module::RegisterNiComponentConfigurator(const FString& NiBlockName, const TSharedPtr<FNiComponentConfigurator>& Configurator)
{
	NiComponentConfigurators.Add(NiBlockName, Configurator);
}

void FDV2Module::RegisterNiComponentConfigurators()
{
	RegisterNiComponentConfigurator("NiAvObject", MakeShared<FNiAVObjectComponentConfigurator>());
	RegisterNiComponentConfigurator("NiTriShape", MakeShared<FNiTriShapeComponentConfigurator>());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDV2Module, DV2)