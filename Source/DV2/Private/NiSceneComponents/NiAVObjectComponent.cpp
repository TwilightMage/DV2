// Fill out your copyright notice in the Description page of Project Settings.

#include "NiSceneComponents/NiAVObjectComponent.h"

#include "NiMeta/NiTools.h"

TSharedPtr<NiMeta::niobject> UNiAVObjectComponent::GetTargetBlockType() const
{
	return NiMeta::GetNiObject("NiAVObject");
}

bool UNiAVObjectComponent::Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx)
{
	FVector Translation = NiTools::ReadVector3(Ctx.Block->GetField("Translation"));
	FRotator Rotation = NiTools::ReadRotator(Ctx.Block->GetField("Rotation"));
	float Scale = Ctx.Block->GetField("Scale").SingleNumber<float>();

	Ctx.SetTransform(FTransform(Rotation, Translation, FVector(Scale)));

	return true;
}
