// Fill out your copyright notice in the Description page of Project Settings.


#include "NiSceneComponents/NiTriShapeComponentConfigurator.h"

#include "NetImmerse.h"
#include "ProceduralMeshComponent.h"
#include "DV2Importer/DV2Settings.h"
#include "NiMeta/NiTools.h"

bool FNiTriShapeComponentConfigurator::Configure(const FNiFile& File, const FNiFile::FSceneSpawnConfiguratorContext& Ctx)
{
	auto MeshComponent = NewObject<UProceduralMeshComponent>(Ctx.WCO);
	Ctx.AttachComponent(MeshComponent);
	MeshComponent->SetMobility(EComponentMobility::Static);
	MeshComponent->CastShadow = true;

	auto Data = Ctx.Block->GetField("Data").SingleReference().Resolve(File);
	if (!Data.IsValid() || Data->Error.IsValid())
		return true;

	auto VerticesField = Data->GetField("Vertices");
	auto NormalsField = Data->FindField("Normals");
	auto UVsField = Data->FindField("UV Sets");
	auto TrianglesField = Data->GetField("Triangles");

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs[4];
	TArray<int32> Triangles;

	Vertices.Reserve(VerticesField.Size);
	for (uint32 i = 0; i < VerticesField.Size; i++)
	{
		Vertices.Add(NiTools::ReadVector3(VerticesField, i));
	}

	if (NormalsField)
	{
		Normals.Reserve(NormalsField->Size);
		for (uint32 i = 0; i < NormalsField->Size; i++)
		{
			Normals.Add(NiTools::ReadVector3(*NormalsField, i));
		}
	}

	if (UVsField)
	{
		uint32 NumUVSets = FMath::Min((uint32)4, Data->GetField("Num UV Sets").SingleNumber<uint32>());
		uint32 NumUVs = Vertices.Num();
		
		for (uint32 i = 0; i < NumUVSets; i++)
		{
			UVs[i].Reserve(NumUVs);
			for (uint32 j = 0; j < NumUVs; j++)
			{
				UVs[i].Add(NiTools::ReadUV(*UVsField, i * NumUVs + j));
			}
		}
	}

	Triangles.Reserve(TrianglesField.Size);
	for (uint32 i = 0; i < TrianglesField.Size; i++)
	{
		const auto& obj = *TrianglesField.GroupAt(i);
		uint32 v1 = obj.GetField("v1").SingleNumber<uint32>();
		uint32 v2 = obj.GetField("v2").SingleNumber<uint32>();
		uint32 v3 = obj.GetField("v3").SingleNumber<uint32>();
		Triangles.Add(v3);
		Triangles.Add(v2);
		Triangles.Add(v1);
	}

	MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs[0], UVs[1], UVs[2], UVs[2], {}, {}, true);
	MeshComponent->UpdateBounds();

	auto MaterialProperty = Ctx.Block->FindBlockByType(NiMeta::GetNiObject("NiMaterialProperty"));
	if (MaterialProperty.IsValid() && !MaterialProperty->Error.IsValid())
	{
		FColor DiffuseColor = NiTools::ReadColorFloat(MaterialProperty->GetField("Diffuse Color"));
		FColor EmissiveColor = NiTools::ReadColorFloat(MaterialProperty->GetField("Emissive Color"));
		float Glossiness = MaterialProperty->GetField("Glossiness").SingleNumber<float>();

		if (auto BaseMaterial = GetDefault<UDV2Settings>()->BaseMaterial.LoadSynchronous())
		{
			auto BaseMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, Ctx.WCO);

			BaseMaterialInstance->SetVectorParameterValue("Diffuse Color", DiffuseColor);
			BaseMaterialInstance->SetVectorParameterValue("Emissive Color", EmissiveColor);
			BaseMaterialInstance->SetScalarParameterValue("Glossiness", Glossiness);

			auto TextureProperty = Ctx.Block->FindBlockByType(NiMeta::GetNiObject("NiTexturingProperty"));
			if (TextureProperty.IsValid() && !TextureProperty->Error.IsValid())
			{
				if (auto TextureField = TextureProperty->FindField("Base Texture"))
				{
					auto TextureSourceIndex = TextureField->SingleGroup()->GetField("Source").SingleReference().Index;
					auto Texture = UNetImmerse::LoadNiTexture(File.Path, TextureSourceIndex, true);
					BaseMaterialInstance->SetTextureParameterValue("Diffuse Texture", Texture);
				}

				if (auto TextureField = TextureProperty->FindField("Glossiness Texture"))
				{
					auto TextureSourceIndex = TextureField->SingleGroup()->GetField("Source").SingleReference().Index;
					auto Texture = UNetImmerse::LoadNiTexture(File.Path, TextureSourceIndex, true);
					BaseMaterialInstance->SetTextureParameterValue("Glossiness Texture", Texture);
				}

				if (auto TextureField = TextureProperty->FindField("Normal Texture"))
				{
					auto TextureSourceIndex = TextureField->SingleGroup()->GetField("Source").SingleReference().Index;
					auto Texture = UNetImmerse::LoadNiTexture(File.Path, TextureSourceIndex, true);
					BaseMaterialInstance->SetTextureParameterValue("Normal Texture", Texture);
				}
			}
		
			MeshComponent->SetMaterial(0, BaseMaterialInstance);	
		}
	}

	return true;
}
