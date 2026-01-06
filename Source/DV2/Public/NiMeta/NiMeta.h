#pragma once
#include "NiVersion.h"

class FNiBlock;
class UNiBlockComponentConfigurator;
struct FNiField;
struct FNiFile;

namespace NiMeta
{
	struct version;
	struct module;
	struct basicType;
	struct enumType;
	struct bitflagsType;
	struct bitfieldType;
	struct structType;
	struct niobject;

#define FAST_MAP(ItemType, Name)\
	TMap<FString, ItemType>& Name##Map();\

	FAST_MAP(FString, Token)
	FAST_MAP(TSharedPtr<version>, Version)
	FAST_MAP(TSharedPtr<module>, Module)
	FAST_MAP(TSharedPtr<basicType>, Basic)
	FAST_MAP(TSharedPtr<enumType>, Enum)
	FAST_MAP(TSharedPtr<bitflagsType>, Bitflags)
	FAST_MAP(TSharedPtr<bitfieldType>, Bitfield)
	FAST_MAP(TSharedPtr<structType>, Struct)
	FAST_MAP(TSharedPtr<niobject>, NiObject)

	struct metaObject
	{
		virtual ~metaObject() = default;
		
		virtual size_t typeId() const = 0;

		template<typename T>
		static size_t staticTypeId()
		{
			static uint8* value = new uint8(0);
			return (size_t)value;
		}

		template<typename T>
		bool isType() const
		{
			return typeId() == staticTypeId<T>();
		}
		
		FString name;
		FString description;
	};

	struct version : metaObject
	{
		virtual size_t typeId() const override { return staticTypeId<version>(); }
		
		FString num;
		uint32 user;
		FNiVersion ver;
	};

	struct module : metaObject
	{
		virtual size_t typeId() const override { return staticTypeId<module>(); }
		
		uint32 priority = 0;
		TArray<TSharedPtr<module>> depends;
		bool custom = false;
	};

	enum class EFieldType : uint8
	{
		Basic    = 0x01,
		Enum     = 0x02,
		BitFlags = 0x04,
		BitField = 0x08,
		Struct   = 0x0F,
	};
	ENUM_CLASS_FLAGS(EFieldType)

	struct DV2_API fieldType : metaObject
	{
		virtual ~fieldType() = default;
		virtual size_t typeId() const override { return staticTypeId<fieldType>(); }
		virtual EFieldType GetFieldType() const = 0;

		TFunction<FString(const FNiFile& file, const FNiField& field, uint32 i)> ToString;
		TFunction<TSharedRef<SWidget>(const FNiFile& file, const FNiField& field, uint32 i)> GenerateSlateWidget;
		TFunction<void()> GenerateMenu;

		void SetSizeString(uint32 bytes, uint8 bits)
		{
			if (bits == 0)
				sizeString = FString::FromInt(bytes);
			else
				sizeString = FString::Printf(TEXT("%d+%d"), bytes, bits);
		}
		FString sizeString;
	};

	struct memberBase : metaObject
	{
		virtual size_t typeId() const override { return staticTypeId<memberBase>(); }
		
		TSharedPtr<fieldType> type;
	};

	struct field : memberBase
	{
		virtual size_t typeId() const override { return staticTypeId<field>(); }
		
		FString defaultValue;
		FString lengthFieldName;
		FString widthFieldName;
		FString vercond;
		FString cond;
		FNiVersion since;
		FNiVersion until;
		FString excludeT;
		FString onlyT;
	};

	struct DV2_API fieldStorage
	{
		TSharedPtr<field> GetField(const FString& fieldName) const { return *fields.FindByPredicate([&](const TSharedPtr<field>& fld) { return fld->name == fieldName; }); }

		int32 FindFieldIndex(const FString& fieldName) const { return fields.IndexOfByPredicate([&](const TSharedPtr<field>& fld) { return fld->name == fieldName; }); }

		TArray<TSharedPtr<field>> fields;
	};

	struct DV2_API basicType : fieldType
	{
		basicType();
		virtual size_t typeId() const override { return staticTypeId<basicType>(); }
		virtual EFieldType GetFieldType() const override { return EFieldType::Basic; }

		bool integral = false;
		bool countable = false;
		uint32 size = 0;
	};

	struct DV2_API enumType : fieldType
	{
		struct option : metaObject
		{
			virtual size_t typeId() const override { return staticTypeId<option>(); }
			
			uint32 value = 0;
		};

		enumType();
		virtual size_t typeId() const override { return staticTypeId<enumType>(); }
		virtual EFieldType GetFieldType() const override { return EFieldType::Enum; }

		const option* FindOption(uint32 value) const;
		bool HasOption(uint32 value) const;

		TSharedPtr<basicType> storage;
		TArray<option> options;
	};

	struct DV2_API bitflagsType : fieldType
	{
		struct option : memberBase
		{
			virtual size_t typeId() const override { return staticTypeId<option>(); }
			
			uint8 bit;
		};

		virtual size_t typeId() const override { return staticTypeId<bitflagsType>(); }
		virtual EFieldType GetFieldType() const override { return EFieldType::BitFlags; }

		TSharedPtr<basicType> storage;
		TArray<TSharedPtr<option>> options;
	};

	struct DV2_API bitfieldType : fieldType
	{
		struct member : memberBase
		{
			virtual size_t typeId() const override { return staticTypeId<member>(); }
			
			uint8 width;
			uint8 pos;
			uint64 mask;
		};

		virtual size_t typeId() const override { return staticTypeId<bitfieldType>(); }
		virtual EFieldType GetFieldType() const override { return EFieldType::BitField; }

		TSharedPtr<basicType> storage;
		TArray<TSharedPtr<member>> members;
	};

	struct DV2_API structType : fieldType, fieldStorage
	{
		virtual size_t typeId() const override { return staticTypeId<structType>(); }
		virtual EFieldType GetFieldType() const override { return EFieldType::Struct; }
		void ProcessFields(const FNiFile& file, const TFunction<void(const TSharedPtr<field>& field)>& fieldHandler, const TFunction<FString(const FString& fieldName)>& fieldTokenHandler);
	
		TSharedPtr<module> module;
	};

	struct DV2_API niobject : metaObject, fieldStorage
	{
		virtual size_t typeId() const override { return staticTypeId<niobject>(); }
		void ReadFrom(const FNiFile& File, FMemoryReader& MemoryReader, TSharedPtr<FNiBlock>& Block);
		void ProcessFields(const FNiFile& file, const TFunction<void(const TSharedPtr<field>& field)>& fieldHandler, const TFunction<FString(const FString& fieldName)>& fieldTokenHandler, TSet<FString>& inheritNames);
	
		bool abstract = false;
		TSharedPtr<niobject> inherit;
		TSharedPtr<module> module;
		FString icon;

		UNiBlockComponentConfigurator* ComponentConfigurator;
		
		TFunction<void(TSharedPtr<FNiBlock>& Block, FMemoryReader& Reader, const FNiFile& File)> CustomRead;
		TFunction<void(FMenuBuilder& MenuBuilder, const TSharedPtr<FNiFile>& File, const TSharedPtr<FNiBlock>& Block)> BuildContextMenu;
	};

	DV2_API TSharedPtr<niobject> GetNiObject(const FString& name);
	DV2_API TSharedPtr<niobject> GetNiObjectChecked(const FString& name);

	DV2_API TMulticastDelegate<void()>& OnReset(); // Called before reset begins
	DV2_API TMulticastDelegate<void()>& OnReload(); // Called after reload finished

	DV2_API void Reset();
	DV2_API void Reload();
}
