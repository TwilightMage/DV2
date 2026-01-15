// Copyright Epic Games, Inc. All Rights Reserved.

#include "DV2.h"

#include "NiSceneBlockHandlers/NiAVObjectHandler.h"
#include "NiSceneBlockHandlers/NiTriShapeHandler.h"

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

void FDV2Module::RegisterNiComponentConfigurators()
{
	RegisterNiSceneBlockHandler("NiAvObject", MakeShared<FNiAVObjectHandler>());
	RegisterNiSceneBlockHandler("NiTriShape", MakeShared<FNiTriShapeHandler>());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDV2Module, DV2)