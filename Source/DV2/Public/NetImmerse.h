#pragma once
#include "NiMeta/NiVersion.h"
#include "NiMeta/NiMeta.h"
#include "NetImmerse.generated.h"

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

class UNiBlockComponentConfigurator;
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
	void SetType(uint32 length, uint32 width = 1)
	{
		Size = length * width;

		//if (Size > 4096)
		//	throw NiException(TEXT("Potentially wrong size %d"), Size);

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

class FNiBitfield : public FNiFieldGroup
{
public:
	TSharedPtr<NiMeta::bitfieldType> Type;
};

class FNiBitflags : public FNiFieldGroup
{
public:
	TSharedPtr<NiMeta::bitflagsType> Type;
};

class FNiStruct : public FNiFieldGroup
{
public:
	TSharedPtr<NiMeta::structType> Type;
};

// Hierarchy element, stores fields
class DV2_API FNiBlock : public FNiFieldStorage, public TSharedFromThis<FNiBlock>
{
public:
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

	TSharedPtr<NiMeta::niobject> Type;
	TArray<TSharedPtr<FNiBlock>> Referenced;
	TArray<TSharedPtr<FNiBlock>> Children;
	FString Error;
	uint32 DataOffset = 0;
	uint32 DataSize = 0;
	uint32 BlockIndex = 0;

private:
	void TraverseReferencedPrivate(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler, const TSharedPtr<FNiBlock>& parent);
	void TraverseChildrenPrivate(const TFunction<bool(const TSharedPtr<FNiBlock>& block, const TSharedPtr<FNiBlock>& parent)>& handler, const TSharedPtr<FNiBlock>& parent);
};

struct DV2_API FNiFile
{
	struct FSceneSpawnConfiguratorContext
	{
		friend FNiFile;

		TSharedPtr<FNiBlock> Block;
		UObject* WCO;

		struct FCallbacks
		{
			TFunction<void(USceneComponent* Component)> OnAttachComponent;
			TFunction<void(const FTransform& Transform)> OnSetTransform;
		};

		void AttachComponent(USceneComponent* Component) const { return Callbacks.OnAttachComponent(Component); }
		void SetTransform(const FTransform& Transform) const { Callbacks.OnSetTransform(Transform); }

	private:
		FCallbacks Callbacks;
	};

	struct FSceneSpawnHandler
	{
		enum class EBlockEnterResult
		{
			Continue,     // As is
			SkipChildren, // Setup block but skip children, exit with success = true
			SkipThis,     // Skip this block, exit with success = false
		};

		virtual EBlockEnterResult OnEnterBlock(const TSharedPtr<FNiBlock>& Block, const TSharedPtr<FNiBlock>& ParentBlock) = 0;
		virtual void OnExitBlock(bool bSuccess) = 0;
		virtual void OnAttachSubComponent(USceneComponent* SubComponent) = 0;
		virtual void OnSetComponentTransform(const FTransform& Transform) = 0;
		virtual UObject* GetWCO() = 0;
	};

	bool ReadFrom(FMemoryReader& memoryReader);
	bool ShouldByteSwapOnThisMachine() const;

	void SpawnScene(FSceneSpawnHandler* Handler);

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
	                                 const FString& FilePath, int32 BlockIndex, bool ForceLoadFile);

private:
	inline static TMap<FString, TWeakPtr<FNiFile>> LoadedNiFiles;

	UPROPERTY()
	TMap<FName, UTexture2D*> LoadedNiTextures;
};
