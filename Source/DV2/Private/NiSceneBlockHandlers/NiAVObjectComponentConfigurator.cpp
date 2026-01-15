// Fill out your copyright notice in the Description page of Project Settings.

#include "NiSceneBlockHandlers/NiAVObjectHandler.h"

#include "NiMeta/NiTools.h"

bool FNiAVObjectHandler::Handle(const FNiFile& File, const FNiFile::FSceneSpawnHandlerContext& Ctx)
{
	FVector Translation = NiTools::ReadVector3(Ctx.Block->GetField("Translation"));
	FRotator Rotation = NiTools::ReadRotator(Ctx.Block->GetField("Rotation"));
	float Scale = Ctx.Block->GetField("Scale").SingleNumber<float>();

	Ctx.SetTransform(FTransform(Rotation, Translation, FVector(Scale)));

	return true;
}
