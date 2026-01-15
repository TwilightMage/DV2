#pragma once
#include "NiVersion.h"
#include "Containers/Deque.h"

class FNiSceneBlockHandler;
class FNiFieldStorage;
class FNiFieldGroup;
class FNiBlock;
struct FNiField;
struct FNiFile;

namespace NiMeta
{
	struct BlockReadContext;
	struct version;
	struct module;
	struct basicType;
	struct enumType;
	struct bitflagsType;
	struct bitfieldType;
	struct structType;
	struct niobject;

	struct templatedStructInstance;

#define FAST_MAP(ItemType, Name)\
	TMap<FString, ItemType>& Name##Map();
	FAST_MAP(FString, Token)
	FAST_MAP(TSharedPtr<version>, Version)
	FAST_MAP(TSharedPtr<module>, Module)
	FAST_MAP(TSharedPtr<basicType>, Basic)
	FAST_MAP(TSharedPtr<enumType>, Enum)
	FAST_MAP(TSharedPtr<bitflagsType>, Bitflags)
	FAST_MAP(TSharedPtr<bitfieldType>, Bitfield)
	FAST_MAP(TSharedPtr<structType>, Struct)
	FAST_MAP(TSharedPtr<niobject>, NiObject)

	FAST_MAP(TSharedPtr<templatedStructInstance>, TemplatedStructInstance)

	struct DV2_API metaObject : TSharedFromThis<metaObject>
	{
		virtual ~metaObject() = default;

		virtual size_t typeId() const = 0;

		template <typename T>
		static size_t staticTypeId()
		{
			static size_t id = staticTypeIdHelper(__FUNCSIG__);
			return id;
		}

		template <typename T>
		bool isType() const
		{
			return typeId() == staticTypeId<T>();
		}

		void openSource() const;

		FString name;
		FString description;

		int32 SourceFilePathIndex = -1;
		int32 SourceLine = -1;

		inline static TArray<FString> FilePaths;

	private:
		static size_t staticTypeIdHelper(const char* str);
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

	struct DV2_API fieldType : metaObject
	{
		virtual ~fieldType() = default;
		virtual size_t typeId() const override { return staticTypeId<fieldType>(); }
		
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) = 0;

		TFunction<FString(const FNiFile& file, const FNiField& field, uint32 i)> ToString;
		TFunction<TSharedRef<SWidget>(const FNiFile& file, const FNiField& field, uint32 i)> GenerateSlateWidget;

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

		virtual bool isAvailable(const FNiFile& file) const = 0;
		virtual FString getDefaultValue() const = 0;
		virtual bool isMemberArray() const { return false; }

		TSharedPtr<fieldType> type;
	};

	struct field : memberBase
	{
		virtual size_t typeId() const override { return staticTypeId<field>(); }

		virtual bool isAvailable(const FNiFile& file) const override;
		virtual FString getDefaultValue() const override { return defaultValue; }
		virtual bool isMemberArray() const override { return !lengthFieldName.IsEmpty() || !widthFieldName.IsEmpty(); }

		FString templateType;
		FString Arg;
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

	struct DV2_API memberStorage
	{
		virtual ~memberStorage() = default;

		TSharedPtr<memberBase> GetMember(const FString& memberName) const
		{
			for (int32 i = 0; i < NumMembers(); i++)
			{
				auto Member = GetMemberAt(i);
				if (Member->name == memberName)
					return Member;
			}
			return nullptr;
		}

		int32 FindMemberIndex(const FString& memberName) const
		{
			for (int32 i = 0; i < NumMembers(); i++)
			{
				auto Member = GetMemberAt(i);
				if (Member->name == memberName)
					return i;
			}
			return INDEX_NONE;
		}

		virtual int32 NumMembers() const = 0;
		virtual TSharedPtr<memberBase> GetMemberAt(uint32 Index) const = 0;
	};

	struct DV2_API basicType : fieldType
	{
		basicType();
		virtual size_t typeId() const override { return staticTypeId<basicType>(); }
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) override;

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
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) override;

		const option* FindOption(uint32 value) const;
		bool HasOption(uint32 value) const;

		TSharedPtr<basicType> storage;
		TArray<option> options;
	};

	struct DV2_API bitflagsType : fieldType, memberStorage
	{
		struct option : memberBase
		{
			virtual size_t typeId() const override { return staticTypeId<option>(); }

			virtual bool isAvailable(const FNiFile& file) const override { return true; }
			virtual FString getDefaultValue() const override { return ""; }

			uint8 bit;
		};

		virtual size_t typeId() const override { return staticTypeId<bitflagsType>(); }
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) override;

		virtual int32 NumMembers() const override { return options.Num(); }
		virtual TSharedPtr<memberBase> GetMemberAt(uint32 Index) const override { return options[Index]; }

		TSharedPtr<basicType> storage;
		TArray<TSharedPtr<option>> options;
	};

	struct DV2_API bitfieldType : fieldType, memberStorage
	{
		struct member : memberBase
		{
			virtual size_t typeId() const override { return staticTypeId<member>(); }

			virtual bool isAvailable(const FNiFile& file) const override { return true; }
			virtual FString getDefaultValue() const override { return ""; }

			uint8 width;
			uint8 pos;
			uint64 mask;
		};

		virtual size_t typeId() const override { return staticTypeId<bitfieldType>(); }
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) override;

		virtual int32 NumMembers() const override { return members.Num(); }
		virtual TSharedPtr<memberBase> GetMemberAt(uint32 Index) const override { return members[Index]; }

		TSharedPtr<basicType> storage;
		TArray<TSharedPtr<member>> members;
	};

	struct DV2_API structType : fieldType, memberStorage
	{
		virtual size_t typeId() const override { return staticTypeId<structType>(); }
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) override;
		void ProcessFields(FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg);

		virtual int32 NumMembers() const override { return fields.Num(); }
		virtual TSharedPtr<memberBase> GetMemberAt(uint32 Index) const override { return fields[Index]; }

		TArray<TSharedPtr<field>> fields;
		TSharedPtr<module> module;
	};

	struct DV2_API BlockReadContext
	{
		struct ContextEntry
		{
			ContextEntry(FNiBlock* block);
			ContextEntry(FNiFieldGroup* group);

			FNiFieldStorage* FieldStorage;
			const memberStorage* MetaFieldStorage;
		};

		BlockReadContext(const FNiFile& InFile, const TSharedPtr<FNiBlock>& InBlock)
			: File(InFile)
			  , Block(InBlock)
		{
		}

		void HandleField(FMemoryReader& Reader, const TSharedPtr<field>& Field, const FString& ResolvedArg);
		FString ResolveFieldToken(const FString& fieldToken);
		void Read(FMemoryReader& reader);

		const FNiFile& File;
		TSharedPtr<FNiBlock> Block;
		TDeque<ContextEntry> ContextStack;
	};

	struct DV2_API niobject : metaObject, memberStorage
	{
		virtual size_t typeId() const override { return staticTypeId<niobject>(); }
		void ProcessFields(FMemoryReader& reader, BlockReadContext& ctx, TSet<FString>& inheritNames);

		virtual int32 NumMembers() const override { return fields.Num(); }
		virtual TSharedPtr<memberBase> GetMemberAt(uint32 Index) const override { return fields[Index]; }

		bool abstract = false;
		TSharedPtr<niobject> inherit;
		TSharedPtr<module> module;
		FString icon;
		TArray<TSharedPtr<field>> fields;

		TSharedPtr<FNiSceneBlockHandler> SceneHandler;

		TFunction<void(TSharedPtr<FNiBlock>& Block, FMemoryReader& Reader, const FNiFile& File)> CustomRead;
		TFunction<void(FMenuBuilder& MenuBuilder, const TSharedPtr<FNiFile>& File, const TSharedPtr<FNiBlock>& Block)> BuildContextMenu;
	};

	struct DV2_API templatedStructInstance : fieldType, memberStorage
	{
		virtual size_t typeId() const override { return staticTypeId<templatedStructInstance>(); }
		virtual bool writeField(FNiField& Field, uint32 ArraySize, FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg) override;
		void ProcessFields(FMemoryReader& Reader, BlockReadContext& Ctx, const FString& ResolvedArg);

		virtual int32 NumMembers() const override { return baseStruct->NumMembers(); }
		virtual TSharedPtr<memberBase> GetMemberAt(uint32 Index) const override { return baseStruct->GetMemberAt(Index); }

		TSharedPtr<structType> baseStruct;
		TSharedPtr<fieldType> templateType;

		TMap<FString, TSharedPtr<field>> templateSpecifiedFields;
	};

	DV2_API TSharedPtr<fieldType> ResolveTemplatedType(const TSharedPtr<fieldType>& BaseType, const TSharedPtr<fieldType>& TemplateType);

	inline static TSharedPtr<basicType> TemplateTypePlaceholder = []()
	{
		auto Value = MakeShared<basicType>();
		Value->name = "Template Placeholder";
		return Value;
	}();

	DV2_API TSharedPtr<niobject> GetNiObject(const FString& name);
	DV2_API TSharedPtr<niobject> GetNiObjectChecked(const FString& name);
	DV2_API TSharedPtr<niobject> FindNiObject(const FString& name);

	DV2_API TMulticastDelegate<void()>& OnReset(); // Called before reset begins
	DV2_API TMulticastDelegate<void()>& OnReload(); // Called after reload finished

	DV2_API void Reset();
	DV2_API void Reload();
}
