// Fill out your copyright notice in the Description page of Project Settings.


#include "NiSceneBlockHandlers/NiTriShapeHandler.h"

#include "NetImmerse.h"
#include "NiMeta/NiTools.h"

bool FNiTriShapeHandler::Handle(const FNiFile& File, const FNiFile::FSceneSpawnHandlerContext& Ctx)
{
	auto Data = Ctx.Block->GetField("Data").SingleReference().Resolve(File);
	if (!Data.IsValid() || Data->Error.IsValid())
		return true;

	FNiFile::FMeshDescriptor Mesh;
	
	auto VerticesField = Data->GetField("Vertices");
	auto NormalsField = Data->FindField("Normals");
	auto UVsField = Data->FindField("UV Sets");
	auto TrianglesField = Data->GetField("Triangles");

	Mesh.Vertices.Reserve(VerticesField.Size);
	for (uint32 i = 0; i < VerticesField.Size; i++)
	{
		Mesh.Vertices.Add(NiTools::ReadVector3(VerticesField, i));
	}

	if (NormalsField)
	{
		Mesh.Normals.Reserve(NormalsField->Size);
		for (uint32 i = 0; i < NormalsField->Size; i++)
		{
			Mesh.Normals.Add(NiTools::ReadVector3(*NormalsField, i));
		}
	}

	if (UVsField)
	{
		uint32 NumUVSets = FMath::Min((uint32)4, Data->GetField("Num UV Sets").SingleNumber<uint32>());
		uint32 NumUVs = Mesh.Vertices.Num();
		
		for (uint32 i = 0; i < NumUVSets; i++)
		{
			Mesh.UVs[i].Reserve(NumUVs);
			for (uint32 j = 0; j < NumUVs; j++)
			{
				Mesh.UVs[i].Add(NiTools::ReadUV(*UVsField, i * NumUVs + j));
			}
		}
	}

	Mesh.Triangles.Reserve(TrianglesField.Size);
	for (uint32 i = 0; i < TrianglesField.Size; i++)
	{
		const auto& obj = *TrianglesField.GroupAt(i);
		uint32 v1 = obj.GetField("v1").SingleNumber<uint32>();
		uint32 v2 = obj.GetField("v2").SingleNumber<uint32>();
		uint32 v3 = obj.GetField("v3").SingleNumber<uint32>();
		Mesh.Triangles.Add(v3);
		Mesh.Triangles.Add(v2);
		Mesh.Triangles.Add(v1);
	}

	auto MaterialProperty = Ctx.Block->FindBlockByType(NiMeta::GetNiObject("NiMaterialProperty"));
	if (MaterialProperty.IsValid() && !MaterialProperty->Error.IsValid())
	{
		Mesh.Material.DiffuseColor = NiTools::ReadColorFloat(MaterialProperty->GetField("Diffuse Color"));
		Mesh.Material.EmissiveColor = NiTools::ReadColorFloat(MaterialProperty->GetField("Emissive Color"));
		Mesh.Material.Glossiness = MaterialProperty->GetField("Glossiness").SingleNumber<float>();

		auto TextureProperty = Ctx.Block->FindBlockByType(NiMeta::GetNiObject("NiTexturingProperty"));
		if (TextureProperty.IsValid() && !TextureProperty->Error.IsValid())
		{
			if (auto TextureField = TextureProperty->FindField("Base Texture"))
			{
				auto TextureSourceIndex = TextureField->SingleGroup()->GetField("Source").SingleReference().Index;
				Mesh.Material.DiffuseTexture = FNiFile::FTextureDescriptor::CreateFromNiBlock(File.Path, TextureSourceIndex);
			}

			if (auto TextureField = TextureProperty->FindField("Glossiness Texture"))
			{
				auto TextureSourceIndex = TextureField->SingleGroup()->GetField("Source").SingleReference().Index;
				Mesh.Material.GlossinessTexture = FNiFile::FTextureDescriptor::CreateFromNiBlock(File.Path, TextureSourceIndex);
			}

			if (auto TextureField = TextureProperty->FindField("Normal Texture"))
			{
				auto TextureSourceIndex = TextureField->SingleGroup()->GetField("Source").SingleReference().Index;
				Mesh.Material.NormalTexture = FNiFile::FTextureDescriptor::CreateFromNiBlock(File.Path, TextureSourceIndex);
			}
		}
	}

	Ctx.AttachMesh(Mesh);

	return true;
}
