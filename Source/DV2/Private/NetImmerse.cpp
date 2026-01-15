#include "NetImmerse.h"

#include "DV2.h"
#include "DV2AssetTree.h"
#include "DesktopPlatformModule.h"
#include "Divinity2Assets.h"
#include "IDesktopPlatform.h"
#include "ProceduralMeshComponent.h"
#include "SourceCodeNavigation.h"
#include "Containers/Deque.h"
#include "DV2Importer/DV2Settings.h"
#include "DV2Importer/unpack.h"
#include "NiMeta/NiException.h"
#include "NiMeta/NiMeta.h"
#include "NiSceneBlockHandlers/NiSceneBlockHandler.h"

const FNiFile::FMaterialDescriptor FNiFile::FMaterialDescriptor::DefaultMaterial = {};

FString NetImmerse::ReadStringNoSize(FMemoryReader& MemoryReader, uint32 SizeLimit)
{
	FString result;
	while (!MemoryReader.AtEnd())
	{
		if ((uint32)result.Len() >= SizeLimit)
			throw MakeNiException("Size limit exceeded");

		uint8 byte;
		MemoryReader << byte;

		if (byte == '\n')
		{
			break;
		}

		result.AppendChar(byte);
	}

	return result;
}

FString NetImmerse::ReadString(FMemoryReader& memoryReader)
{
	uint32 length;
	memoryReader << length;

	TArray<uint8> bytes;
	bytes.SetNum(length);

	memoryReader.ByteOrderSerialize(bytes.GetData(), length);

	return FString(length, (UTF8CHAR*)bytes.GetData());
}

TSharedPtr<FNiBlock> FNiBlockReference::Resolve(const FNiFile& file) const
{
	if (Index == (uint32)-1)
		return nullptr;
	if (Index >= (uint32)file.Blocks.Num())
		return nullptr;

	return file.Blocks[Index];
}

FNiFieldBuilder& FNiFieldBuilder::Meta(const TSharedPtr<NiMeta::memberBase>& InMeta)
{
	Field->Meta = InMeta;
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Value(uint64 InNum)
{
	if (!Field->Values.IsType<FNiField::TArrNum>())
		Field->Values.Set<FNiField::TArrNum>({});
	Field->PushValue(InNum);
	Field->Size = Field->GetNumberArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Value(const FNiBlockReference& InReference)
{
	if (!Field->Values.IsType<FNiField::TArrReference>())
		Field->Values.Set<FNiField::TArrReference>({});
	Field->PushValue(InReference);
	Field->Size = Field->GetReferenceArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Value(const TSharedPtr<FNiFieldGroup>& InGroup)
{
	if (!Field->Values.IsType<FNiField::TArrGroup>())
		Field->Values.Set<FNiField::TArrGroup>({});
	Field->PushValue(InGroup);
	Field->Size = Field->GetGroupArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Values(const TArray<uint64>& InNumArray)
{
	if (!Field->Values.IsType<FNiField::TArrNum>())
		Field->Values.Set<FNiField::TArrNum>({});
	Field->GetNumberArray().Append(InNumArray);
	Field->Size = Field->GetNumberArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Values(const TArray<uint8>& InByteArray)
{
	if (!Field->Values.IsType<FNiField::TArrByte>())
		Field->Values.Set<FNiField::TArrByte>({});
	Field->GetByteArray().Append(InByteArray);
	Field->Size = Field->GetByteArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Values(const TArray<FNiBlockReference>& InReferenceArray)
{
	if (!Field->Values.IsType<FNiField::TArrReference>())
		Field->Values.Set<FNiField::TArrReference>({});
	Field->GetReferenceArray().Append(InReferenceArray);
	Field->Size = Field->GetReferenceArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Values(const TArray<TSharedPtr<FNiFieldGroup>>& InGroupArray)
{
	if (!Field->Values.IsType<FNiField::TArrGroup>())
		Field->Values.Set<FNiField::TArrGroup>({});
	Field->GetGroupArray().Append(InGroupArray);
	Field->Size = Field->GetGroupArray().Num();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Offset(uint32 Bytes, uint8 Bits)
{
	Field->SourceOffset = Bytes;
	Field->SourceSubOffset = Bits;
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::Offset(FMemoryReader& Reader)
{
	Field->SourceOffset = Reader.Tell();
	return *this;
}

FNiFieldBuilder& FNiFieldBuilder::SourceSize(uint32 Size)
{
	Field->SourceItemSize = Size;
	return *this;
}

void FNiFieldStorage::TraverseFields(const TFunction<void(const FNiField& field)>& handler) const
{
	for (const auto& field : Fields)
	{
		handler(field);

		if (field.IsGroup())
		{
			const auto& arr = field.GetGroupArray();
			for (const auto& val : arr)
			{
				val->TraverseFields(handler);
			}
		}
	}
}

void FieldToNameIndex(const FString& Field, FString& OutName, uint32& OutIndex)
{
	const TCHAR* chr = *Field;
	const TCHAR* end = *Field + Field.Len();
	const TCHAR* fragmentStart = *Field;
	bool state = false;

	while (chr != end)
	{
		if (state
			? !(TChar<TCHAR>::IsDigit(*chr))
			: !(TChar<TCHAR>::IsAlnum(*chr) || *chr == '_'))
		{
			if (state == 0)
			{
				if (*chr != '[')
				{
					OutName = "";
					OutIndex = 0;
					return;
				}

				OutName = FString(chr - fragmentStart, fragmentStart);
				fragmentStart = chr + 1;
				state = true;
			}
			else
			{
				if (*chr != ']')
				{
					OutName = "";
					OutIndex = 0;
					return;
				}

				LexFromString(OutIndex, fragmentStart);
				return;
			}
		}

		++chr;
	}

	OutName = Field;
	OutIndex = 0;
}

FNiFieldBuilder FNiFieldStorage::AddField()
{
	return {&Fields.Add_GetRef(FNiField())};
}

FNiField* FNiFieldStorage::FieldAtPrivate(const FString& Path, uint32& OutLastFieldIndex) const
{
	TArray<FString> Bits;
	Path.ParseIntoArray(Bits, TEXT("."));

	FNiFieldStorage* Cur = const_cast<FNiFieldStorage*>(this);
	for (int32 i = 0; i < Bits.Num(); ++i)
	{
		FString FieldName;
		uint32 FieldIndex;
		FieldToNameIndex(Bits[i], FieldName, FieldIndex);

		if (auto Field = Cur->FindField(FieldName))
		{
			bool IsLast = i == Bits.Num() - 1;
			if (!IsLast)
			{
				Cur = Field->GroupAt(FieldIndex).Get();
				if (!Cur)
					return nullptr;
			}
			else
			{
				OutLastFieldIndex = FieldIndex;
				return Cur->FindField(FieldName);
			}
		}
	}

	return nullptr;
}

void FNiBlock::FError::OpenSource(const FString& Name) const
{
#if WITH_EDITOR
	if (auto Source = Sources.Find(Name))
		FSourceCodeNavigation::OpenSourceFile(Source->Get<0>(), Source->Get<1>());
#endif
}

FString FNiBlock::GetTitle(const FNiFile& File) const
{
	if (auto NameField = FindField("Name"))
		return StaticCastSharedPtr<NiMeta::structType>(NameField->SingleGroup()->Type)->ToString(File, *NameField, 0);
	return "";
}

FString FNiBlock::GetFullName(const FNiFile& File) const
{
	FString TypeName = Type->name;
	FString Title = GetTitle(File);
	if (Title.IsEmpty())
		return FString::Printf(TEXT("%s \"%s\" [%d]"), *TypeName, *Title, BlockIndex);
	else
		return FString::Printf(TEXT("%s [%d]"), *TypeName, BlockIndex);
}

void FNiBlock::TraverseReferenced(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler)
{
	TraverseReferencedPrivate(handler, nullptr);
}

void FNiBlock::TraverseChildren(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler)
{
	TraverseChildrenPrivate(handler, nullptr);
}

void FNiBlock::TraverseReferencedPrivate(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler, const TSharedPtr<FNiBlock>& parent)
{
	auto self = AsShared();
	if (!handler(self, parent))
		return;

	for (const auto& Child : Referenced)
	{
		Child->TraverseReferencedPrivate(handler, self);
	}
}

void FNiBlock::TraverseChildrenPrivate(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler, const TSharedPtr<FNiBlock>& parent)
{
	auto self = AsShared();
	if (!handler(self, parent))
		return;

	for (const auto& Child : Children)
	{
		Child->TraverseChildrenPrivate(handler, self);
	}
}

bool FNiFile::FTextureDescriptor::IsSet() const
{
	return !Data.IsType<nullptr_t>();
}

UTexture2D* FNiFile::FTextureDescriptor::ResolveTexture() const
{
	if (Data.IsType<TTexturePtr>())
		return Data.Get<TTexturePtr>().Get();
	if (Data.IsType<TNiBlock>())
		return UNetImmerse::LoadNiTexture(Data.Get<TNiBlock>().AssetPath, Data.Get<TNiBlock>().BlockIndex);
	if (Data.IsType<TAssetPath>())
		return nullptr;
	return nullptr;
}

FNiFile::FTextureDescriptor FNiFile::FTextureDescriptor::CreateFromTexture(UTexture2D* Texture)
{
	FTextureDescriptor Result;
	Result.Data.Emplace<TTexturePtr>(Texture);
	return Result;
}

FNiFile::FTextureDescriptor FNiFile::FTextureDescriptor::CreateFromNiBlock(const FString& AssetPath, uint32 BlockIndex)
{
	FTextureDescriptor Result;
	Result.Data.Emplace<TNiBlock>(AssetPath, BlockIndex);
	return Result;
}

FNiFile::FTextureDescriptor FNiFile::FTextureDescriptor::CreateFromAssetPath(const FString& AssetPath)
{
	FTextureDescriptor Result;
	Result.Data.Emplace<TAssetPath>(AssetPath);
	return Result;
}

bool FNiFile::FTextureDescriptor::operator==(const FTextureDescriptor& Rhs) const
{
	if (Data.GetIndex() != Rhs.Data.GetIndex())
		return false;

	if (Data.IsType<TTexturePtr>())
		return Data.Get<TTexturePtr>() == Rhs.Data.Get<TTexturePtr>();
	if (Data.IsType<TNiBlock>())
		return Data.Get<TNiBlock>().AssetPath == Rhs.Data.Get<TNiBlock>().AssetPath && Data.Get<TNiBlock>().BlockIndex == Rhs.Data.Get<TNiBlock>().BlockIndex;
	if (Data.IsType<TAssetPath>())
		return Data.Get<TAssetPath>() == Rhs.Data.Get<TAssetPath>();
	return true;
}

bool FNiFile::FMaterialDescriptor::operator==(const FMaterialDescriptor& Rhs) const
{
	return
		DiffuseColor == Rhs.DiffuseColor &&
		EmissiveColor == Rhs.EmissiveColor &&
		Glossiness == Rhs.Glossiness &&
		DiffuseTexture == Rhs.DiffuseTexture &&
		GlossinessTexture == Rhs.GlossinessTexture &&
		NormalTexture == Rhs.NormalTexture;
}

bool FNiFile::FMeshDescriptor::CheckValid() const
{
	return
		(Normals.Num() == Vertices.Num() || Normals.Num() == 0) &&
		(Tangents.Num() == Vertices.Num() || Tangents.Num() == 0) &&
		(UVs[0].Num() == Vertices.Num() || UVs[0].Num() == 0) &&
		(UVs[1].Num() == Vertices.Num() || UVs[1].Num() == 0) &&
		(UVs[2].Num() == Vertices.Num() || UVs[2].Num() == 0) &&
		(UVs[3].Num() == Vertices.Num() || UVs[3].Num() == 0) &&
		Triangles.Num() % 3 == 0;
}

void FNiFile::FMeshDescriptor::AddSectionToProceduralMeshComponent(UProceduralMeshComponent* PMC) const
{
	int32 SectionIndex = PMC->GetNumSections();
	PMC->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs[0], UVs[1], UVs[2], UVs[2], {}, {}, true);
	PMC->UpdateBounds();

	if (Material != FMaterialDescriptor::DefaultMaterial)
	{
		if (auto BaseMaterial = GetDefault<UDV2Settings>()->BaseMaterial.LoadSynchronous())
		{
			auto BaseMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, PMC);

			BaseMaterialInstance->SetVectorParameterValue("Diffuse Color", Material.DiffuseColor);
			BaseMaterialInstance->SetVectorParameterValue("Emissive Color", Material.EmissiveColor);
			BaseMaterialInstance->SetScalarParameterValue("Glossiness", Material.Glossiness);

			if (Material.DiffuseTexture.IsSet())
				BaseMaterialInstance->SetTextureParameterValue("Diffuse Texture", Material.DiffuseTexture.ResolveTexture());

			if (Material.GlossinessTexture.IsSet())
				BaseMaterialInstance->SetTextureParameterValue("Glossiness Texture", Material.GlossinessTexture.ResolveTexture());

			if (Material.NormalTexture.IsSet())
				BaseMaterialInstance->SetTextureParameterValue("Normal Texture", Material.NormalTexture.ResolveTexture());

			PMC->SetMaterial(SectionIndex, BaseMaterialInstance);
		}
	}
}

bool FNiFile::ReadFrom(FMemoryReader& memoryReader)
{
	VersionString = "";

	try
	{
		VersionString = NetImmerse::ReadStringNoSize(memoryReader, 64);
	}
	catch (NiException e)
	{
		return false;
	}
	if (!VersionString.StartsWith("Gamebryo File Format"))
		return false;

	memoryReader << Version.v1;
	memoryReader << Version.v2;
	memoryReader << Version.v3;
	memoryReader << Version.v4;

	uint8 isLittleEndian;
	memoryReader << isLittleEndian;
	IsLittleEndian = isLittleEndian != 0;

	if (IsLittleEndian)
	{
		Swap(Version.v1, Version.v4);
		Swap(Version.v2, Version.v3);
	}

	memoryReader.SetByteSwapping(ShouldByteSwapOnThisMachine());

	memoryReader << UserVersion;
	memoryReader << NumBlocks;

	uint16 numBlockTypes;
	memoryReader << numBlockTypes;
	BlockTypes.SetNum(numBlockTypes);
	for (uint32 i = 0; i < numBlockTypes; i++)
	{
		FString typeName = NetImmerse::ReadString(memoryReader);
		BlockTypes[i] = NiMeta::GetNiObject(typeName);
	}

	Blocks.SetNum(NumBlocks);

	for (uint32 i = 0; i < NumBlocks; i++)
	{
		uint16 blockTypeId;
		memoryReader << blockTypeId;

		TSharedPtr<NiMeta::niobject> blockType = BlockTypes[blockTypeId];

		auto block = MakeShared<FNiBlock>();
		block->Type = blockType;
		block->BlockIndex = i;

		Blocks[i] = block;
	}

	for (uint32 i = 0; i < NumBlocks; i++)
	{
		uint32 blockSize;
		memoryReader << blockSize;

		Blocks[i]->DataSize = blockSize;
	}

	uint32 numStrings;
	uint32 maxStringLength; // Used for buffer allocation, but unnecessary for us
	memoryReader << numStrings;
	memoryReader << maxStringLength;
	Strings.SetNum(numStrings);
	for (uint32 i = 0; i < numStrings; i++)
	{
		Strings[i] = NetImmerse::ReadString(memoryReader);
	}

	uint32 numGroups;
	memoryReader << numGroups;
	if (numGroups > 0)
		throw MakeNiException("NIF Groups are not supported");

	for (uint32 i = 0; i < NumBlocks; i++)
	{
		Blocks[i]->DataOffset = memoryReader.Tell();

		TArray<uint8> blockBytes;
		blockBytes.SetNum(Blocks[i]->DataSize);
		memoryReader.ByteOrderSerialize(blockBytes.GetData(), Blocks[i]->DataSize);

		FMemoryReader blockReader(blockBytes);
		blockReader.SetByteSwapping(memoryReader.IsByteSwapping());
		if (Blocks[i]->Type.IsValid())
		{
			NiMeta::BlockReadContext Ctx(*this, Blocks[i]);
			Ctx.Read(blockReader);
			Blocks[i] = Ctx.Block;
		}
		else
			Blocks[i]->SetErrorManual("Unknown nif block type");

		if (Blocks[i]->Error.IsValid())
			UE_LOG(LogDV2, Error, TEXT("Failed to construct nif block on index %d with error %s"), i, *Blocks[i]->Error->Message);
	}

	for (auto& Block : Blocks)
	{
		Block->TraverseFields([&](const FNiField& Field)
		{
			if (Field.IsReference())
			{
				const auto& Arr = Field.GetReferenceArray();
				for (const auto& Val : Arr)
				{
					if (auto Ptr = Val.Resolve(*this); Ptr.IsValid())
						Block->Referenced.AddUnique(Ptr);
				}
			}
		});

		if (auto ChildrenField = Block->FindField("Children"))
		{
			const auto& Arr = ChildrenField->GetReferenceArray();
			for (const auto& Val : Arr)
			{
				if (auto Ptr = Val.Resolve(*this); Ptr.IsValid())
					Block->Children.AddUnique(Ptr);
			}
		}
	}

	return true;
}

bool FNiFile::ShouldByteSwapOnThisMachine() const
{
	return FPlatformProperties::IsLittleEndian() != IsLittleEndian;
}

void FNiFile::SpawnScene(FSceneSpawnHandler* Handler, uint32 RootBlockIndex)
{
	struct BlockComponentConfig
	{
		TDeque<TSharedPtr<FNiSceneBlockHandler>> Handlers;
	};

	TMap<TSharedPtr<NiMeta::niobject>, BlockComponentConfig> BlockComponentConfigs;
	TSet<TSharedPtr<FNiBlock>> ProcessedBlocks;

	for (const auto& BlockType : BlockTypes)
	{
		BlockComponentConfig Config;

		TSharedPtr<NiMeta::niobject> BT = BlockType;
		while (BT.IsValid())
		{
			if (BT->SceneHandler.IsValid())
			{
				Config.Handlers.PushFirst(BT->SceneHandler);
			}

			BT = BT->inherit;
		}

		BlockComponentConfigs.Add(BlockType, Config);
	}

	auto Root = Blocks[RootBlockIndex];
	Root->TraverseChildren([&](const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& Parent) -> bool
	{
		if (ProcessedBlocks.Contains(Block))
			return false; // Cycle
		ProcessedBlocks.Add(Block);

		if (Block->Error.IsValid())
			return false; // Error, no need to go deeper

		const auto& Config = BlockComponentConfigs[Block->Type];

		auto EnterState = Handler->OnEnterBlock(Block, Parent);

		if (EnterState == FSceneSpawnHandler::EBlockEnterResult::SkipThis)
		{
			Handler->OnExitBlock(false);
			return false;
		}

		FSceneSpawnHandlerContext Ctx;
		Ctx.Block = Block;
		Ctx.WCO = Handler->GetWCO();
		Ctx.Callbacks.OnAttachMesh = [&](const FMeshDescriptor& Mesh)
		{
			Handler->OnAttachMesh(Mesh);
		};
		Ctx.Callbacks.OnSetTransform = [&](const FTransform& Transform)
		{
			Handler->OnSetComponentTransform(Transform);
		};

		for (const auto& BlockHandler : Config.Handlers)
			if (!BlockHandler->Handle(*this, Ctx))
			{
				Handler->OnExitBlock(false);
				return false;
			}

		Handler->OnExitBlock(true);
		return EnterState != FSceneSpawnHandler::EBlockEnterResult::SkipChildren;
	});
}

TSharedPtr<FNiFile> UNetImmerse::OpenNiFile(const FString& Path, bool bForceLoad)
{
	return OpenNiFile(UDivinity2Assets::GetAssetEntryFromPath(Path), bForceLoad);
}

TSharedPtr<FNiFile> UNetImmerse::OpenNiFile(const TSharedPtr<FDV2AssetTreeEntry>& AssetEntry, bool bForceLoad)
{
	if (!AssetEntry.IsValid())
		return nullptr;

	if (!AssetEntry->IsFile())
		return nullptr;

	const auto& Ref = AssetEntry->GetAssetReference();
	FString FilePath = Ref.GetFilePath();

	auto WeakFile = LoadedNiFiles.FindRef(FilePath);
	if (auto File = WeakFile.Pin(); File.IsValid())
		return File;

	if (bForceLoad)
	{
		TArray<uint8> Bytes;
		if (!Ref.Read(*AssetEntry->File, Bytes))
		{
			return nullptr;
		}

		FMemoryReader Reader(Bytes);

		auto File = MakeShared<FNiFile>();
		File->Path = FilePath;
		if (!File->ReadFrom(Reader))
			return nullptr;

		LoadedNiFiles.Add(FilePath, File);

		return File;
	}
	return nullptr;
}

UTexture2D* UNetImmerse::LoadNiTexture(const FString& FilePath, int32 BlockIndex)
{
	auto FixedFilePath = FixPath(FilePath);
	auto NI = GetMutableDefault<UNetImmerse>();

	FName BlockPath = *FString::Printf(TEXT("%s+%d"), *FixedFilePath, BlockIndex);
	if (auto Texture = NI->LoadedNiTextures.FindRef(BlockPath))
		return Texture;

	FString ActualFilePath = FixedFilePath;
	if (
		//FixedFilePath.ToLower().EndsWith(".dds") ||
		FixedFilePath.ToLower().EndsWith(".tga")
	)
	{
		ActualFilePath = FixedFilePath.Mid(0, FixedFilePath.Len() - 4) + ".nif";
	}

	if (auto File = OpenNiFile(ActualFilePath, true); File.IsValid())
	{
		if (BlockIndex >= File->Blocks.Num())
			return nullptr;

		auto Block = File->Blocks[BlockIndex];

		if (Block->Type == NiMeta::GetNiObject("NiSourceTexture"))
		{
			bool UseExternal = Block->NumberAt<bool>("Use External");

			if (UseExternal)
			{
				FString TextureName = Block->ToStringAt("File Name", *File);
				FString TexturePath = "Win32/Textures/" + TextureName;
				return LoadNiTexture(TexturePath, 0);
			}
			else
			{
				Block = Block->ReferenceAt("Pixel Data").Resolve(*File);
			}
		}

		if (Block->Type == NiMeta::GetNiObject("NiPersistentSrcTextureRendererData"))
		{
			const TArray<uint8>& DataBlob = Block->ByteArrayAt("Pixel Data");

			auto PixelFormat = Block->ToStringAt("Pixel Format", *File);

			EPixelFormat PF = EPixelFormat::PF_Unknown;
			if (PixelFormat == "FMT_RGB")
				PF = EPixelFormat::PF_R8G8B8;
			else if (PixelFormat == "FMT_RGBA")
				PF = EPixelFormat::PF_R8G8B8A8;
			else if (PixelFormat == "FMT_PAL")
				PF = EPixelFormat::PF_Unknown;
			else if (PixelFormat == "FMT_PALA")
				PF = EPixelFormat::PF_Unknown;
			else if (PixelFormat == "FMT_DXT1")
				PF = EPixelFormat::PF_DXT1;
			else if (PixelFormat == "FMT_DXT3")
				PF = EPixelFormat::PF_DXT3;
			else if (PixelFormat == "FMT_DXT5")
				PF = EPixelFormat::PF_DXT5;
			else if (PixelFormat == "FMT_RGB24NONINT")
				PF = EPixelFormat::PF_Unknown;
			else if (PixelFormat == "FMT_BUMP")
				PF = EPixelFormat::PF_Unknown; // Fix
			else if (PixelFormat == "FMT_BUMPLUMA")
				PF = EPixelFormat::PF_Unknown; // Fix
			else if (PixelFormat == "FMT_RENDERSPEC")
				PF = EPixelFormat::PF_Unknown; // Fix
			else if (PixelFormat == "FMT_1CH")
				PF = EPixelFormat::PF_R8;
			else if (PixelFormat == "FMT_2CH")
				PF = EPixelFormat::PF_R8G8;
			else if (PixelFormat == "FMT_3CH")
				PF = EPixelFormat::PF_R8G8B8;
			else if (PixelFormat == "FMT_4CH")
				PF = EPixelFormat::PF_R8G8B8A8;
			else if (PixelFormat == "FMT_DEPTH_STENCIL")
				PF = EPixelFormat::PF_Unknown; // Fix
			else if (PixelFormat == "FMT_UNKNOWN")
				PF = EPixelFormat::PF_Unknown;

			if (PF == PF_Unknown)
			{
				UE_LOG(LogDV2, Error, TEXT("Unsupported or unknown texture format %s on texture %s"), *PixelFormat, *FixedFilePath);
				return nullptr;
			}

			const auto& Mip = Block->GroupAt("Mipmaps", 0);
			uint32 Width = Mip->NumberAt<uint32>("Width");
			uint32 Height = Mip->NumberAt<uint32>("Height");

			UTexture2D* Result = UTexture2D::CreateTransient(Width, Height, PF);

			void* TextureData = Result->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			switch (PF)
			{
			case PF_R8:
				FMemory::Memcpy(TextureData, DataBlob.GetData(), Width * Height * 1);
				break;
			case PF_R8G8:
				FMemory::Memcpy(TextureData, DataBlob.GetData(), Width * Height * 2);
				break;
			case PF_R8G8B8:
				FMemory::Memcpy(TextureData, DataBlob.GetData(), Width * Height * 3);
				break;
			case PF_R8G8B8A8:
				FMemory::Memcpy(TextureData, DataBlob.GetData(), Width * Height * 4);
				break;
			case PF_DXT1:
				FMemory::Memcpy(TextureData, DataBlob.GetData(), (Width * Height) >> 1);
				break;
			case PF_DXT3:
			case PF_DXT5:
				FMemory::Memcpy(TextureData, DataBlob.GetData(), Width * Height);
				break;
			}
			Result->GetPlatformData()->Mips[0].BulkData.Unlock();

			Result->UpdateResource();

			return NI->LoadedNiTextures.Add(BlockPath, Result);
		}
	}

	return nullptr;
}

struct FExportObjHandlerData
{
	FString Name;
	TSharedPtr<FNiFile::FMeshDescriptor> Mesh;
};

struct FExportObjHandler : FVirtualNiSceneSpawnHandler<FExportObjHandlerData>
{
	FNiFile* File;
	TFunction<void(const FString& Str)> Print;

	uint32 VertexAccum = 0;

	virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) override
	{
		auto Result = FVirtualNiSceneSpawnHandler::OnEnterBlock(Block, ParentBlock);
		if (Result != EBlockEnterResult::Continue)
			return Result;

		auto Title = Block->GetTitle(*File);
		Current.Node->Data.Name = Title.IsEmpty() ? FString::Printf(TEXT("BLOCK_%d"), Block->BlockIndex) : Title;

		return EBlockEnterResult::Continue;
	}

	virtual void OnAttachMesh(const FNiFile::FMeshDescriptor& Mesh) override
	{
		Current.Node->Data.Mesh = MakeShared<FNiFile::FMeshDescriptor>(Mesh);
	}

	virtual UObject* GetWCO() override
	{
		return nullptr;
	}

	virtual void OnFinalizeNode(HierarchyNode& Node) override
	{
		if (Node.Data.Mesh.IsValid())
		{
			Print(FString::Printf(TEXT("o %s\n"), *Node.Data.Name));

			for (const auto& Vertex : Node.Data.Mesh->Vertices)
			{
				auto GlobalVertex = Node.GlobalTransform.TransformPosition(Vertex);
				Print(FString::Printf(TEXT("v %f %f %f\n"), GlobalVertex.X, GlobalVertex.Y, GlobalVertex.Z));
			}

			for (const auto& Normal : Node.Data.Mesh->Normals)
			{
				auto GlobalNormal = Node.GlobalTransform.TransformVector(Normal);
				Print(FString::Printf(TEXT("vn %f %f %f\n"), GlobalNormal.X, GlobalNormal.Y, GlobalNormal.Z));
			}

			for (const auto& UV : Node.Data.Mesh->UVs[0])
			{
				Print(FString::Printf(TEXT("vt %f %f\n"), UV.X, UV.Y));
			}

			for (int32 i = 0; i < Node.Data.Mesh->Triangles.Num(); i += 3)
			{
				uint32 i0 = VertexAccum + Node.Data.Mesh->Triangles[i + 0] + 1;
				uint32 i1 = VertexAccum + Node.Data.Mesh->Triangles[i + 1] + 1;
				uint32 i2 = VertexAccum + Node.Data.Mesh->Triangles[i + 2] + 1;
				Print(FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d\n"),
				                      i0, i0, i0,
				                      i1, i1, i1,
				                      i2, i2, i2
					));
			}

			Print("\n");

			VertexAccum += Node.Data.Mesh->Vertices.Num();
		}
	}
};

void UNetImmerse::ExportScene(const FString& FilePath, int32 BlockIndex, const FString& DiskPath)
{
	auto File = OpenNiFile(FilePath, true);
	if (!File.IsValid())
		return;

	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*DiskPath);

	TFunction<void(const FString& Str)> Print = [FileWriter](const FString& Str)
	{
		FTCHARToUTF8 Converter(*Str, Str.Len());
		FileWriter->Serialize((UTF8CHAR*)Converter.Get(), Converter.Length());
	};

	FExportObjHandler Handler;
	Handler.File = File.Get();
	Handler.Print = Print;
	File->SpawnScene(&Handler, BlockIndex);
	Handler.Finalize();

	FileWriter->Close();
	delete FileWriter;
}

void UNetImmerse::ExportSceneWithWizard(const FString& FilePath, int32 BlockIndex)
{
	auto Asset = UDivinity2Assets::GetAssetEntryFromPath(FilePath);
	if (!Asset.IsValid())
		return;
	
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
		return;
	
	TArray<FString> OutFilenames;
	FString FileTypes = TEXT("OBJ Files (*.obj)|*.obj");

	if (!DesktopPlatform->SaveFileDialog(
		nullptr,
		TEXT("Export Model"),
		TEXT(""),
		FPaths::ChangeExtension(Asset->Name, "obj"),
		FileTypes,
		EFileDialogFlags::None,
		OutFilenames
		))
		return;

	if (OutFilenames.Num() != 1)
		return;

	ExportScene(FilePath, BlockIndex, OutFilenames[0]);
}
