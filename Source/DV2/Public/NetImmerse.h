#pragma once
#include "NiMeta/NiException.h"
#include "NiMeta/NiVersion.h"
#include "NiMeta/NiMeta.h"
#include "NetImmerse.generated.h"

class UProceduralMeshComponent;
struct Dv2File;

namespace NiMeta
{
	struct niobject;
	struct memberBase;
	struct metaObject;
	struct fieldType;
	struct basicType;
	struct enumType;
	struct bitfieldType;
	struct structType;
}

struct FNiField;
class FNiFieldStorage;
struct FNiFile;
class FNiBlock;
struct FDV2AssetTreeEntry;
class FNiFieldGroup;
class UTexture2D;

namespace NetImmerse
{
	FString ReadStringNoSize(FMemoryReader& MemoryReader, uint32 SizeLimit = -1);
	FString ReadString(FMemoryReader& memoryReader);
}

struct DV2_API FNiBlockReference
{
	uint32 Index = 0;

	TSharedPtr<FNiBlock> Resolve(const FNiFile& file) const;
};

typedef TVariant<TArray<uint64>, TArray<uint8>, TArray<FNiBlockReference>, TArray<TSharedPtr<FNiFieldGroup>>> TNifFieldStorageType;

// Attribute field, used by block or struct
struct DV2_API FNiField
{
	typedef TArray<uint64> TArrNum;
	typedef TArray<uint8> TArrByte;
	typedef TArray<FNiBlockReference> TArrReference;
	typedef TArray<TSharedPtr<FNiFieldGroup>> TArrGroup;

#pragma region Generic Getters
#pragma region Indexed

	template <typename T>
	T& ValueAt(uint32 i) { return Values.Get<TArray<T>>()[i]; }

	template <typename T>
	const T& ValueAt(uint32 i) const { return Values.Get<TArray<T>>()[i]; }

#pragma endregion

#pragma region Single

	template <typename T>
	T& SingleValue() { return ValueAt<T>(0); }

	template <typename T>
	const T& SingleValue() const { return ValueAt<T>(0); }

#pragma endregion
#pragma endregion

#pragma region Specific getters
#pragma region Indexed

	template <typename TNum>
	TNum NumberAt(uint32 i) const { return *(TNum*)&ValueAt<uint64>(i); }

	template <typename TNum>
	TNum& NumberAtRef(uint32 i) { return *(TNum*)&ValueAt<uint64>(i); }

	uint8& ByteAt(uint32 i) { return ValueAt<uint8>(i); }
	uint8 ByteAt(uint32 i) const { return ValueAt<uint8>(i); }

	FNiBlockReference& ReferenceAt(uint32 i) { return ValueAt<FNiBlockReference>(i); }
	const FNiBlockReference& ReferenceAt(uint32 i) const { return ValueAt<FNiBlockReference>(i); }

	TSharedPtr<FNiFieldGroup>& GroupAt(uint32 i) { return ValueAt<TSharedPtr<FNiFieldGroup>>(i); }
	const TSharedPtr<FNiFieldGroup>& GroupAt(uint32 i) const { return ValueAt<TSharedPtr<FNiFieldGroup>>(i); }

#pragma endregion

#pragma region Single

	template <typename TNum>
	TNum SingleNumber() const { return NumberAt<TNum>(0); }

	uint8& SingleByte() { return ByteAt(0); }
	uint8 SingleByte() const { return ByteAt(0); }

	FNiBlockReference& SingleReference() { return ReferenceAt(0); }
	const FNiBlockReference& SingleReference() const { return ReferenceAt(0); }

	TSharedPtr<FNiFieldGroup>& SingleGroup() { return GroupAt(0); }
	const TSharedPtr<FNiFieldGroup>& SingleGroup() const { return GroupAt(0); }

#pragma endregion
#pragma endregion

#pragma region Type checks

	bool IsNumeric() const { return Values.IsType<TArrNum>(); }
	bool IsByte() const { return Values.IsType<TArrByte>(); }
	bool IsReference() const { return Values.IsType<TArrReference>(); }
	bool IsGroup() const { return Values.IsType<TArrGroup>(); }

#pragma endregion

#pragma region Array Access
	TArrNum& GetNumberArray() { return Values.Get<TArrNum>(); }
	const TArrNum& GetNumberArray() const { return Values.Get<TArrNum>(); }

	TArrByte& GetByteArray() { return Values.Get<TArrByte>(); }
	const TArrByte& GetByteArray() const { return Values.Get<TArrByte>(); }

	TArrReference& GetReferenceArray() { return Values.Get<TArrReference>(); }
	const TArrReference& GetReferenceArray() const { return Values.Get<TArrReference>(); }

	TArrGroup& GetGroupArray() { return Values.Get<TArrGroup>(); }
	const TArrGroup& GetGroupArray() const { return Values.Get<TArrGroup>(); }
#pragma endregion

	template <typename T>
	void SetType(uint32 InSize)
	{
		Size = InSize;

		Values.Set<TArray<T>>(TArray<T>());
		Values.Get<TArray<T>>().Reserve(Size);
	}

	template <typename T>
	T& PushValue(T defaultValue = T())
	{
		Values.Get<TArray<T>>().Add(defaultValue);
		return Values.Get<TArray<T>>().Last();
	}

	TSharedPtr<NiMeta::memberBase> Meta;
	uint32 Size;
	TNifFieldStorageType Values;

	FString SourceOffsetToString(uint32 offsetBias, uint32 index) const
	{
		if (SourceSubOffset == 0)
			return FString::FromInt(offsetBias + SourceOffset + index * SourceItemSize);
		else
			return FString::Printf(TEXT("%d+%d"), offsetBias + SourceOffset + index * SourceItemSize, SourceSubOffset);
	}

	uint32 SourceOffset = 0;
	uint8 SourceSubOffset = 0;
	uint32 SourceItemSize = 0;
};

struct FNiFieldBuilder
{
	FNiFieldBuilder& Meta(const TSharedPtr<NiMeta::memberBase>& InMeta);

	FNiFieldBuilder& Value(uint64 InNum);
	FNiFieldBuilder& Value(const FNiBlockReference& InReference);
	FNiFieldBuilder& Value(const TSharedPtr<FNiFieldGroup>& InGroup);

	template <typename T>
	FNiFieldBuilder& NumFromReader(FMemoryReader& Reader, T* OutCopy = nullptr)
	{
		T Local;
		T* Buff = OutCopy ? OutCopy : &Local;

		Reader << *Buff;
		return Value((uint64)*Buff);
	}

	FNiFieldBuilder& ByteArrayFromReader(FMemoryReader& Reader, uint32 Num, TArray<uint8>* OutCopy = nullptr)
	{
		TArray<uint8> Local;
		TArray<uint8>* Buff = OutCopy ? OutCopy : &Local;

		Buff->SetNum(Num);
		Reader.ByteOrderSerialize(Buff->GetData(), Num);
		return Values(*Buff);
	}

	FNiFieldBuilder& Values(const TArray<uint64>& InNumArray);
	FNiFieldBuilder& Values(const TArray<uint8>& InByteArray);
	FNiFieldBuilder& Values(const TArray<FNiBlockReference>& InReferenceArray);
	FNiFieldBuilder& Values(const TArray<TSharedPtr<FNiFieldGroup>>& InGroupArray);

	FNiFieldBuilder& Offset(uint32 Bytes, uint8 Bits);
	FNiFieldBuilder& Offset(FMemoryReader& Reader);
	FNiFieldBuilder& SourceSize(uint32 Size);

	FNiField* Field;
};

class DV2_API FNiFieldStorage
{
public:
	FNiField& GetField(const FString& fieldName) { return *Fields.FindByPredicate([&](const FNiField& fld) { return fld.Meta->name == fieldName; }); }
	const FNiField& GetField(const FString& fieldName) const { return *Fields.FindByPredicate([&](const FNiField& fld) { return fld.Meta->name == fieldName; }); }

	FNiField& operator[](const FString& fieldName) { return GetField(fieldName); }
	const FNiField& operator[](const FString& fieldName) const { return GetField(fieldName); }

	FNiField* FindField(const FString& fieldName) { return Fields.FindByPredicate([&](const FNiField& fld) { return fld.Meta->name == fieldName; }); }
	const FNiField* FindField(const FString& fieldName) const { return Fields.FindByPredicate([&](const FNiField& fld) { return fld.Meta->name == fieldName; }); }

	int32 FindFieldIndex(const FString& fieldName) const { return Fields.IndexOfByPredicate([&](const FNiField& fld) { return fld.Meta->name == fieldName; }); }

	void TraverseFields(const TFunction<void(const FNiField& field)>& handler) const;

	template <typename T>
	T NumberAt(const FString& FieldName, uint32 Index = 0) const { return GetField(FieldName).NumberAt<T>(Index); }

	TArray<uint8>& ByteArrayAt(const FString& FieldName) { return GetField(FieldName).GetByteArray(); }
	const TArray<uint8>& ByteArrayAt(const FString& FieldName) const { return GetField(FieldName).GetByteArray(); }

	FNiBlockReference& ReferenceAt(const FString& FieldName, uint32 Index = 0) { return GetField(FieldName).ReferenceAt(Index); }
	const FNiBlockReference& ReferenceAt(const FString& FieldName, uint32 Index = 0) const { return GetField(FieldName).ReferenceAt(Index); }

	TSharedPtr<FNiFieldGroup>& GroupAt(const FString& FieldName, uint32 Index = 0) { return GetField(FieldName).GroupAt(Index); }
	const TSharedPtr<FNiFieldGroup>& GroupAt(const FString& FieldName, uint32 Index = 0) const { return GetField(FieldName).GroupAt(Index); }

	FString ToStringAt(const FString& FieldName, const FNiFile& File, uint32 Index = 0) const
	{
		const auto& Field = GetField(FieldName);
		return Field.Meta->type->ToString(File, Field, Index);
	}

	template <typename T>
	T* AtPath(const FString& Path);

	template <typename T>
	const T* AtPath(const FString& Path) const;

	FNiFieldBuilder AddField();

	TArray<FNiField> Fields;

private:
	FNiField* FieldAtPrivate(const FString& Path, uint32& OutLastFieldIndex) const;
};

// Set of fields in block or other group
class DV2_API FNiFieldGroup : public FNiFieldStorage
{
public:
	TSharedPtr<NiMeta::memberStorage> Type;
};

template <typename T>
T* FNiFieldStorage::AtPath(const FString& Path)
{
	uint32 LastFieldIndex;
	if (auto Field = FieldAtPrivate(Path, LastFieldIndex))
	{
		if constexpr (std::is_arithmetic_v<T>)
		{
			return &Field->NumberAtRef<T>(LastFieldIndex);
		}
		else if constexpr (std::is_same_v<T, FNiField::TArrByte>)
		{
			return &Field->GetByteArray();
		}
		else if constexpr (std::is_same_v<T, FNiBlockReference>)
		{
			return &Field->ReferenceAt(LastFieldIndex);
		}
		else if constexpr (std::is_same_v<T, FNiFieldGroup>)
		{
			return &Field->GroupAt(LastFieldIndex);
		}
	}

	return nullptr;
}

template <typename T>
const T* FNiFieldStorage::AtPath(const FString& Path) const
{
	uint32 LastFieldIndex;
	if (auto Field = FieldAtPrivate(Path, LastFieldIndex))
	{
		if constexpr (std::is_arithmetic_v<T>)
		{
			return Field->NumberAt<T>(LastFieldIndex);
		}
		else if constexpr (std::is_same_v<T, FNiField::TArrByte>)
		{
			return Field->GetByteArray();
		}
		else if constexpr (std::is_same_v<T, FNiBlockReference>)
		{
			return Field->ReferenceAt(LastFieldIndex);
		}
		else if constexpr (std::is_same_v<T, FNiFieldGroup>)
		{
			return Field->GroupAt(LastFieldIndex);
		}
	}

	return nullptr;
}

// Hierarchy element, stores fields
class DV2_API FNiBlock : public FNiFieldStorage, public TSharedFromThis<FNiBlock>
{
public:
	struct DV2_API FError
	{
		void OpenSource(const FString& Name) const;

		FString Message;
		TMap<FString, TTuple<FString, int32>> Sources;
	};

	FString GetTitle(const FNiFile& File) const;
	FString GetFullName(const FNiFile& File) const;
	void TraverseReferenced(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler);
	void TraverseChildren(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler);

	TSharedPtr<FNiBlock> FindBlockByType(const TSharedPtr<NiMeta::niobject>& BlockType) const
	{
		if (auto chld = Referenced.FindByPredicate([&](const TSharedPtr<FNiBlock>& chld) { return chld->Type == BlockType; }))
			return *chld;
		return nullptr;
	}

	bool IsOfType(const FString& TypeName) const
	{
		return Type.IsValid() && Type->name == TypeName;
	}

	bool IsChildOfType(const FString& TypeName) const
	{
		auto Ty = Type;
		while (Ty.IsValid())
		{
			if (Ty->name == TypeName)
				return true;
			Ty = Ty->inherit;
		}
		
		return false;
	}

	void SetError(const FString& InMessage)
	{
		Error = MakeShared<FError>(InMessage);
	}

	void SetError(const FString& InMessage, const FString& InFile, int32 InLine)
	{
		Error = MakeShared<FError>(InMessage);
		Error->Sources.Add("Source", {InFile, InLine});
	}

#define SetErrorManual(Message) SetError(Message, __FILE__, __LINE__)

	void SetError(const NiException& InException)
	{
		TMap<FString, TTuple<FString, int32>> Sources;
		InException.GetSources(Sources);

		Error = MakeShared<FError>(InException.GetWhatFormatted(), Sources);
	}

	void ClearError()
	{
		Error = nullptr;
	}

	TSharedPtr<NiMeta::niobject> Type;
	TArray<TSharedPtr<FNiBlock>> Referenced;
	TArray<TSharedPtr<FNiBlock>> Children;
	TSharedPtr<FError> Error;
	uint32 DataOffset = 0;
	uint32 DataSize = 0;
	uint32 BlockIndex = 0;

private:
	void TraverseReferencedPrivate(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler, const TSharedPtr<FNiBlock>& parent);
	void TraverseChildrenPrivate(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler, const TSharedPtr<FNiBlock>& parent);
};

USTRUCT()
struct FNiMask
{
	GENERATED_BODY()

	static FNiMask CreateBlack() { return FNiMask{.bIsWhiteList = true}; }
	static FNiMask CreateWhite() { return FNiMask{.bIsWhiteList = false}; }

	bool ShouldShow(uint32 InBlockIndex) const
	{
		return bIsWhiteList
			? BlockIndexList.Contains(InBlockIndex)
			: !BlockIndexList.Contains(InBlockIndex);
	}

	void Toggle(uint32 InBlockIndex)
	{
		if (BlockIndexList.Remove(InBlockIndex) == 0)
			BlockIndexList.Add(InBlockIndex);
	}

	void Reset()
	{
		BlockIndexList.Reset();
	}

	UPROPERTY()
	TSet<uint32> BlockIndexList;

	UPROPERTY()
	bool bIsWhiteList = false;
};

struct DV2_API FNiFile
{
	struct DV2_API FTextureDescriptor
	{
		using TTexturePtr = TStrongObjectPtr<UTexture2D>;
		using TNiBlock = struct
		{
			FString AssetPath;
			uint32 BlockIndex;
		};
		using TAssetPath = FString;

		bool IsSet() const;
		UTexture2D* ResolveTexture() const;

		static FTextureDescriptor CreateFromTexture(UTexture2D* Texture);
		static FTextureDescriptor CreateFromNiBlock(const FString& AssetPath, uint32 BlockIndex);
		static FTextureDescriptor CreateFromAssetPath(const FString& AssetPath);

		bool operator==(const FTextureDescriptor& Rhs) const;

		TVariant<nullptr_t, TTexturePtr, TNiBlock, TAssetPath> Data = TVariant<nullptr_t, TTexturePtr, TNiBlock, TAssetPath>(TInPlaceType<nullptr_t>());
	};

	struct DV2_API FMaterialDescriptor
	{
		FLinearColor DiffuseColor = FLinearColor::White;
		FLinearColor EmissiveColor = FLinearColor::Black;
		float Glossiness = 0;

		FTextureDescriptor DiffuseTexture;
		FTextureDescriptor GlossinessTexture;
		FTextureDescriptor NormalTexture;

		static const FMaterialDescriptor DefaultMaterial;

		bool operator==(const FMaterialDescriptor& Rhs) const;
	};

	struct DV2_API FMeshDescriptor
	{
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		TArray<FVector> Tangents;
		TArray<FVector2D> UVs[4];
		TArray<int32> Triangles;

		bool CheckValid() const;

		void AddSectionToProceduralMeshComponent(UProceduralMeshComponent* PMC) const;

		FMaterialDescriptor Material;
	};

	struct FSceneSpawnHandlerContext
	{
		friend FNiFile;

		TSharedPtr<FNiBlock> Block;
		UObject* WCO;

		struct FCallbacks
		{
			TFunction<void(const FMeshDescriptor& Mesh)> OnAttachMesh;
			TFunction<void(const FTransform& Transform)> OnSetTransform;
		};

		void AttachMesh(const FMeshDescriptor& Mesh) const { return Callbacks.OnAttachMesh(Mesh); }
		void SetTransform(const FTransform& Transform) const { Callbacks.OnSetTransform(Transform); }

	private:
		FCallbacks Callbacks;
	};

	struct FSceneSpawnHandler
	{
		enum class EBlockEnterResult
		{
			Continue, // As is
			SkipChildren, // Setup block but skip children, exit with success = true
			SkipThis, // Skip this block, exit with success = false
		};

		const FNiMask* Mask = nullptr;

		virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock)
		{
			if (Mask && !Mask->ShouldShow(Block->BlockIndex))
				return EBlockEnterResult::SkipThis;

			return EBlockEnterResult::Continue;
		}
		
		virtual void OnExitBlock(bool bSuccess)
		{
		}
		
		virtual void OnAttachMesh(const FMeshDescriptor& Mesh)
		{
		}
		
		virtual void OnSetComponentTransform(const FTransform& Transform)
		{
		}
		
		virtual UObject* GetWCO() = 0;
	};

	bool ReadFrom(FMemoryReader& memoryReader);
	bool ShouldByteSwapOnThisMachine() const;

	void SpawnScene(FSceneSpawnHandler* Handler, uint32 RootBlockIndex = 0);

	FString Path;
	FString VersionString;
	FNiVersion Version;
	bool IsLittleEndian;
	uint32 UserVersion;
	uint32 NumBlocks;
	TArray<TSharedPtr<NiMeta::niobject>> BlockTypes;
	TArray<FString> Strings;

	TArray<TSharedPtr<FNiBlock>> Blocks;
};

UCLASS(Abstract)
class DV2_API UNetImmerse : public UObject
{
	GENERATED_BODY()

public:
	static TSharedPtr<FNiFile> OpenNiFile(const FString& Path, bool bForceLoad);
	static TSharedPtr<FNiFile> OpenNiFile(const TSharedPtr<FDV2AssetTreeEntry>& AssetEntry, bool bForceLoad);

	UFUNCTION(BlueprintCallable, Category="Divinity 2|Net Immerse")
	static UTexture2D* LoadNiTexture(UPARAM(meta=(DV2AssetPath))
	                                 const FString& FilePath, int32 BlockIndex);

	UFUNCTION(BlueprintCallable, Category="Divinity 2|Net Immerse")
	static void ExportScene(UPARAM(meta=(DV2AssetPath))
	                      const FString& FilePath, int32 BlockIndex, const FString& DiskPath);

	UFUNCTION(BlueprintCallable, Category="Divinity 2|Net Immerse")
	static void ExportSceneWithWizard(UPARAM(meta=(DV2AssetPath))
	                      const FString& FilePath, int32 BlockIndex);

private:
	inline static TMap<FString, TWeakPtr<FNiFile>> LoadedNiFiles;

	UPROPERTY()
	TMap<FName, UTexture2D*> LoadedNiTextures;
};

// Base class for constructing virtual tree from NI scene
template <typename TData>
struct FVirtualNiSceneSpawnHandler : FNiFile::FSceneSpawnHandler
{
	struct HierarchyNode
	{
		FTransform RelativeTransform;
		FTransform GlobalTransform;
		TArray<TSharedPtr<HierarchyNode>> Children;
		TData Data;
	};

	TSharedPtr<HierarchyNode> RootNode;
	TMap<TSharedPtr<FNiBlock>, TSharedPtr<HierarchyNode>> BlockToNodeMap;

	struct
	{
		TSharedPtr<FNiBlock> Block;
		TSharedPtr<FNiBlock> ParentBlock;
		TSharedPtr<HierarchyNode> Node;
	} Current;

	virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) override
	{
		auto Result = FSceneSpawnHandler::OnEnterBlock(Block, ParentBlock);
		if (Result != EBlockEnterResult::Continue)
			return Result;
		
		Current.Block = Block;
		Current.ParentBlock = ParentBlock;
		Current.Node = MakeShared<HierarchyNode>();
		Current.Node->Children.Reserve(Block->Children.Num());
		return EBlockEnterResult::Continue;
	}

	virtual void OnExitBlock(bool bSuccess) override
	{
		if (bSuccess)
		{
			if (!RootNode.IsValid())
				RootNode = Current.Node;

			if (Current.ParentBlock.IsValid())
				BlockToNodeMap[Current.ParentBlock]->Children.Add(Current.Node);

			BlockToNodeMap.Add(Current.Block, Current.Node);
		}
	}

	virtual void OnSetComponentTransform(const FTransform& Transform) override
	{
		Current.Node->RelativeTransform = Transform;
	}

	bool HasAnyObjects() const
	{
		return RootNode.IsValid();
	}

	void Finalize()
	{
		FinalizeNode(*RootNode, nullptr);
	}
	
protected:
	virtual void OnFinalizeNode(HierarchyNode& Node)
	{
	}

private:
	void FinalizeNode(HierarchyNode& Node, const FTransform* ParentTransform)
	{
		if (ParentTransform)
			Node.GlobalTransform = Node.RelativeTransform * *ParentTransform;
		else
			Node.GlobalTransform = Node.RelativeTransform;

		OnFinalizeNode(Node);

		for (auto& Child : Node.Children)
			FinalizeNode(*Child, &Node.GlobalTransform);
	}
};
