#include "NiMeta/NiMeta.h"

#include "FastXml.h"
#include "NetImmerse.h"
#include "Containers/Deque.h"
#include "DV2Importer/DV2Settings.h"
#include "NiMeta/NiException.h"
#include "NiMeta/Patch.h"
#include "NiSceneComponents/NiBlockComponentConfigurator.h"

FString ExpandTokens(const FString& str, const TFunction<FString(const FString& token)>& fallbackTokenResolver = nullptr)
{
	struct FTokenInfo
	{
		const TCHAR* Start;
		uint32 SourceLen;
		FString Substitution;
	};

	TArray<FTokenInfo> tokenInfo;
	uint32 newLen = 0;
	const TCHAR* readPtr;
	TCHAR* writePtr;

	bool searchState = false;
	readPtr = *str;

	while (readPtr != *str + str.Len())
	{
		if (*readPtr == '#')
		{
			if (!searchState)
				tokenInfo.Add({readPtr, 0});
			else
			{
				uint32 len = readPtr - tokenInfo.Last().Start + 1;
				tokenInfo.Last().SourceLen = len;

				FString substitutionRaw;
				FString targetKey = FString(len, tokenInfo.Last().Start);

				if (auto tokenValue = NiMeta::TokenMap().Find(targetKey))
					substitutionRaw = *tokenValue;
				else
				{
					if (fallbackTokenResolver.IsSet())
						substitutionRaw = fallbackTokenResolver(targetKey);
					else
						throw NiException(TEXT("Unknown token %s"), *targetKey);
				}

				tokenInfo.Last().Substitution = ExpandTokens(substitutionRaw, fallbackTokenResolver);
				newLen += tokenInfo.Last().Substitution.Len();
			}
			searchState = !searchState;
		}
		else
		{
			if (!searchState)
				newLen++;
		}
		readPtr++;
	}

	FString result;
	result.GetCharArray().SetNumUninitialized(newLen + 1);

	readPtr = *str;
	writePtr = result.GetCharArray().GetData();
	int32 tokenIdx = 0;

	while (readPtr != *str + str.Len())
	{
		if (tokenIdx < tokenInfo.Num() && readPtr == tokenInfo[tokenIdx].Start)
		{
			const FString& Sub = tokenInfo[tokenIdx].Substitution;
			// Copy substitution directly into FString buffer
			FMemory::Memcpy(writePtr, *Sub, Sub.Len() * sizeof(TCHAR));

			writePtr += Sub.Len();
			readPtr += tokenInfo[tokenIdx].SourceLen;
			tokenIdx++;
		}
		else
		{
			*writePtr++ = *readPtr++;
		}
	}

	*writePtr = 0;

	return result;
}

bool ResolveExpr(const FString& str, const FNiFile& file, const TFunction<FString(const FString& fieldName)>& fieldTokenHandler)
{
	FString expression = ExpandTokens(str, [&](const FString& token) -> FString
	{
		if (token == "#VER#")
			return FString::Printf(TEXT("%d.%d.%d.%d"), file.Version.v1, file.Version.v2, file.Version.v3, file.Version.v4);
		if (token == "#USER#")
			return FString::FromInt(file.UserVersion);
		if (token == "#BSVER#")
			return "0";
		if (fieldTokenHandler.IsSet())
			return fieldTokenHandler(token);
		return "error";
	});

	FRegexPattern PatternFCKU(TEXT("0x\\d+")); // Thanks nif.xml and fuck you
	FRegexPattern PatternInvert(TEXT("!\\s*(true|false)"));
	FRegexPattern PatternOperation(TEXT("([^\\(\\)\\s]+)\\s+(==|!=|<=|>=|<|>|&|\\|)\\s+([^\\(\\)\\s]+)"));
	FRegexPattern PatternBranchAnd(TEXT("([^\\(\\)\\s]+)\\s+&&\\s+([^\\(\\)\\s]+)"));
	FRegexPattern PatternBranchOr(TEXT("([^\\(\\)\\s]+)\\s+\\|\\|\\s+([^\\(\\)\\s]+)"));
	FRegexPattern PatternBrace(TEXT("\\(\\s*([^\\(\\)]*)\\s*\\)"));

	while (true)
	{
#pragma warning(push)
#pragma warning(disable:4456)
		if (FRegexMatcher matcher = FRegexMatcher(PatternFCKU, expression); matcher.FindNext())
		{
			FString expr = matcher.GetCaptureGroup(0);
			uint64 num;
			LexFromString(num, expr);
			FString replacement = FString::FromInt(num);
			expression = expression.Replace(*matcher.GetCaptureGroup(0), *replacement);
		}
		else if (FRegexMatcher matcher = FRegexMatcher(PatternInvert, expression); matcher.FindNext())
		{
			FString expr = matcher.GetCaptureGroup(1);
			FString replacement = expr == "true" ? "false" : "true";
			expression = expression.Replace(*matcher.GetCaptureGroup(0), *replacement);
		}
		else if (FRegexMatcher matcher = FRegexMatcher(PatternOperation, expression); matcher.FindNext())
		{
			FString arg1 = matcher.GetCaptureGroup(1);
			FString op = matcher.GetCaptureGroup(2);
			FString arg2 = matcher.GetCaptureGroup(3);

			FString replacement = "error";

			if (op == "==" || op == "!=" // Generic
				|| op == "<=" || op == ">=" || op == "<" || op == ">" // Numerics
				//|| op == "&" || op == "|" // Flag
			)
			{
				if (op == "==" || op == "!=")
				{
					if (op == "==")
						replacement = arg1 == arg2 ? "true" : "false";
					else if (op == "!=")
						replacement = arg1 != arg2 ? "true" : "false";
				}
				else if (op == "<=" || op == ">=" || op == "<" || op == ">")
				{
					static constexpr uint8 CMP_LT = 0b001;
					static constexpr uint8 CMP_EQ = 0b010;
					static constexpr uint8 CMP_GT = 0b100;
					uint8 state = CMP_EQ;

					const TCHAR
						*chrA = *arg1,
						*chrB = *arg2,
						*endA = chrA + arg1.Len(),
						*endB = chrB + arg2.Len(),
						*lexA = chrA,
						*lexB = chrB;

					while (true)
					{
#define stopA (*chrA == '.' || chrA == endA)
#define stopB (*chrB == '.' || chrB == endB)

						if (!stopA)
							++chrA;

						if (!stopB)
							++chrB;

						if (stopA && stopB)
						{
							uint64 numA, numB;
							LexFromString(numA, lexA);
							LexFromString(numB, lexB);

							if (numA < numB)
							{
								state = CMP_LT;
								break;
							}
							if (numA > numB)
							{
								state = CMP_GT;
								break;
							}

							if (chrA == endA || chrB == endB)
							{
								if (chrA == endA && chrB == endB)
								{
									state = CMP_EQ;
									break;
								}
								if (chrA == endA)
								{
									state = CMP_LT;
									break;
								}
								if (chrB == endB)
								{
									state = CMP_GT;
									break;
								}
							}

							++chrA;
							++chrB;

							lexA = chrA;
							lexB = chrB;
						}

#undef stopA
#undef stopB
					}

					if (op == "<=")
						replacement = state & (CMP_LT | CMP_EQ) ? "true" : "false";
					else if (op == ">=")
						replacement = state & (CMP_GT | CMP_EQ) ? "true" : "false";
					else if (op == "<")
						replacement = state & CMP_LT ? "true" : "false";
					else if (op == ">")
						replacement = state & CMP_GT ? "true" : "false";
				}
			}

			expression = expression.Replace(*matcher.GetCaptureGroup(0), *replacement);
		}
		else if (FRegexMatcher matcher = FRegexMatcher(PatternBranchAnd, expression); matcher.FindNext())
		{
			FString arg1 = matcher.GetCaptureGroup(1);
			FString arg2 = matcher.GetCaptureGroup(2);

			FString replacement = arg1 == "true" && arg2 == "true" ? "true" : "false";

			expression = expression.Replace(*matcher.GetCaptureGroup(0), *replacement);
		}
		else if (FRegexMatcher matcher = FRegexMatcher(PatternBranchOr, expression); matcher.FindNext())
		{
			FString arg1 = matcher.GetCaptureGroup(1);
			FString arg2 = matcher.GetCaptureGroup(2);

			FString replacement = arg1 == "true" || arg2 == "true" ? "true" : "false";

			expression = expression.Replace(*matcher.GetCaptureGroup(0), *replacement);
		}
		else if (FRegexMatcher matcher = FRegexMatcher(PatternBrace, expression); matcher.FindNext())
		{
			FString expr = matcher.GetCaptureGroup(1);
			expression = expression.Replace(*matcher.GetCaptureGroup(0), *expr);
		}
		else
			break;
#pragma warning(pop)
	}

	if (expression != "true" && expression != "false")
		throw NiException(TEXT("Got invalid expression: \"%s\" that expanded to \"%s\""), *str, *expression);

	return expression == "true";
}

#pragma region Data Reading
template <typename T>
T ParseAttrValue(const FString& str)
{
	check(false)
	return T();
}

template <>
FString ParseAttrValue(const FString& str)
{
	return str;
}

template <>
bool ParseAttrValue(const FString& str)
{
	return str.ToLower() == "true";
}

template <>
uint8 ParseAttrValue(const FString& str)
{
	return FCString::Strtoi(*str, nullptr, 0);
}

template <>
uint32 ParseAttrValue(const FString& str)
{
	return FCString::Strtoi64(*str, nullptr, 0);
}

template <>
uint64 ParseAttrValue(const FString& str)
{
	return FCString::Strtoui64(*str, nullptr, 0);
}

template <>
TSharedPtr<NiMeta::module> ParseAttrValue(const FString& str)
{
	return NiMeta::ModuleMap()[str];
}

template <>
TArray<TSharedPtr<NiMeta::module>> ParseAttrValue(const FString& str)
{
	TArray<TSharedPtr<NiMeta::module>> result;
	TArray<FString> bits;
	str.ParseIntoArray(bits, TEXT(" "));

	for (const auto& bit : bits)
	{
		result.Add(NiMeta::ModuleMap()[bit]);
	}

	return result;
}

template <>
TSharedPtr<NiMeta::basicType> ParseAttrValue(const FString& str)
{
	return NiMeta::BasicMap()[str];
}

template <>
TSharedPtr<NiMeta::niobject> ParseAttrValue(const FString& str)
{
	return NiMeta::NiObjectMap()[str];
}

template <>
TSharedPtr<NiMeta::version> ParseAttrValue(const FString& str)
{
	return NiMeta::VersionMap()[str];
}

template <>
TArray<TSharedPtr<NiMeta::version>> ParseAttrValue(const FString& str)
{
	TArray<TSharedPtr<NiMeta::version>> result;
	TArray<FString> bits;
	ExpandTokens(str).ParseIntoArray(bits, TEXT(" "));

	for (const auto& bit : bits)
	{
		result.Add(NiMeta::VersionMap()[bit]);
	}

	return result;
}

template <>
TSharedPtr<NiMeta::fieldType> ParseAttrValue(const FString& str)
{
	if (auto type = NiMeta::BasicMap().FindRef(str); type.IsValid())
		return type;
	if (auto type = NiMeta::EnumMap().FindRef(str); type.IsValid())
		return type;
	if (auto type = NiMeta::BitflagsMap().FindRef(str); type.IsValid())
		return type;
	if (auto type = NiMeta::BitfieldMap().FindRef(str); type.IsValid())
		return type;
	if (auto type = NiMeta::StructMap().FindRef(str); type.IsValid())
		return type;

	UE_LOG(LogDV2, Warning, TEXT("Referenced unknown meta type %s"), *str);
	return nullptr;
}

template <>
FNiVersion ParseAttrValue(const FString& str)
{
	return FNiVersion::Parse(str);
}
#pragma endregion

namespace NiMeta
{
#define FAST_MAP_IMPL(ItemType, Name)\
	TMap<FString, ItemType>& Name##Map()\
	{\
		static TMap<FString, ItemType> Map;\
		return Map;\
	}

	FAST_MAP_IMPL(FString, Token)
	FAST_MAP_IMPL(TSharedPtr<version>, Version)
	FAST_MAP_IMPL(TSharedPtr<module>, Module)
	FAST_MAP_IMPL(TSharedPtr<basicType>, Basic)
	FAST_MAP_IMPL(TSharedPtr<enumType>, Enum)
	FAST_MAP_IMPL(TSharedPtr<bitflagsType>, Bitflags)
	FAST_MAP_IMPL(TSharedPtr<bitfieldType>, Bitfield)
	FAST_MAP_IMPL(TSharedPtr<structType>, Struct)
	FAST_MAP_IMPL(TSharedPtr<niobject>, NiObject)

	basicType::basicType()
	{
		ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
		{
			return FString::FromInt(field.NumberAt<uint64>(i));
		};
	}

	enumType::enumType()
	{
		ToString = [this](const FNiFile& file, const FNiField& field, uint32 i) -> FString
		{
			uint64 value = field.NumberAt<uint64>(i);
			if (auto option = FindOption(value))
				return option->name;
			return "<invalid>";
		};
	}

	const enumType::option* enumType::FindOption(uint32 value) const
	{
		for (const auto& option : options)
			if (option.value == value)
				return &option;

		return nullptr;
	}

	bool enumType::HasOption(uint32 value) const
	{
		for (const auto& option : options)
			if (option.value == value)
				return true;

		return false;
	}

	void structType::ProcessFields(const FNiFile& file, const TFunction<void(const TSharedPtr<field>& field)>& fieldHandler, const TFunction<FString(const FString& fieldName)>& fieldTokenHandler)
	{
		for (const auto& field : fields)
		{
			try
			{
				if (field->since.IsValid() && file.Version < field->since)
					continue;
				if (field->until.IsValid() && file.Version > field->until)
					continue;
				if (!field->vercond.IsEmpty() && !ResolveExpr(field->vercond, file, nullptr))
					continue;
				if (!field->cond.IsEmpty() && !ResolveExpr(field->cond, file, fieldTokenHandler))
					continue;

				fieldHandler(field);
			}
			catch (NiException e)
			{
				throw NiException(TEXT("%s -> %hs"), *field->name, e.what());
			}
		}
	}

	void niobject::ReadFrom(const FNiFile& File, FMemoryReader& MemoryReader, TSharedPtr<FNiBlock>& Block)
	{
		Block->Error = "Unknown error";

		try
		{
			if (Block->Type->CustomRead.IsSet())
			{
				Block->Type->CustomRead(Block, MemoryReader, File);
			}
			else
			{
				struct ContextEntry
			{
				ContextEntry(FNiBlock* block)
					: FieldStorage(block)
					  , MetaFieldStorage(block->Type.Get())
				{
				}

				ContextEntry(FNiStruct* structObj)
					: FieldStorage(structObj)
					  , MetaFieldStorage(structObj->Type.Get())
				{
				}

				FNiFieldStorage* FieldStorage;
				const fieldStorage* MetaFieldStorage;
			};

			TDeque<ContextEntry> contextStack = {ContextEntry(Block.Get())};

			TFunction<FString(const FString& fieldName)> fieldTokenHandler = [&](const FString& fieldToken) -> FString
			{
				FString fieldName = fieldToken.Mid(1, fieldToken.Len() - 2);
				if (const FNiField* field = contextStack.Last().FieldStorage->FindField(fieldName))
				{
					// Cannot use arrays for evaluaion
					if (field->Size != 1)
						return "error";

					return field->Meta->type->ToString(File, *field, 0);
				}
				else
				{
					for (const auto& defaultField : contextStack.Last().MetaFieldStorage->fields)
					{
						if (defaultField->name == fieldName &&
							(defaultField->since.IsValid() ? File.Version >= defaultField->since : true) &&
							(defaultField->until.IsValid() ? File.Version <= defaultField->until : true))
						{
							if (!defaultField->defaultValue.IsEmpty())
								return defaultField->defaultValue;
							return "error";
						}
					}

					return "error";
				}
			};

			//UE_LOG(LogDV2, Display, TEXT("Field %s with size %d on offset %lld"), *field.name, size, memoryReader.Tell());

			TFunction<bool(const TSharedPtr<fieldType>&, uint32, FMemoryReader&, FNiField&)> fieldConstructor = nullptr;
			TFunction<void(const TSharedPtr<field>&)> fieldHandler = nullptr;

			fieldConstructor = [&](const TSharedPtr<fieldType>& type, uint32 size, FMemoryReader& reader, FNiField& outField) -> bool
			{
				uint64 bytesRemaining = reader.TotalSize() - reader.Tell();

#define VerifySize(ItemSize) \
    	outField.SourceItemSize = ItemSize * size; \
    	if (outField.SourceItemSize > bytesRemaining) \
    		throw NiException(TEXT("Got size overflow: have %d bytes left, but requested %d"), bytesRemaining, outField.SourceItemSize);

				switch (type->GetFieldType())
				{
				case EFieldType::Basic:
				{
					auto ty = StaticCastSharedPtr<basicType>(type);
					if (ty->name == "Ptr" || ty->name == "Ref")
					{
						VerifySize(sizeof(uint32));
						outField.SetType<FNiBlockReference>(size);
					}
					else if (ty->name == "BinaryBlob")
					{
						outField.SetType<uint8>(size);
					}
					else if (ty->size > 0)
					{
						VerifySize(ty->size);
						outField.SetType<uint64>(size);
					}
					else
						throw NiException(TEXT("Malformed type (not ref/ptr, binary blob or number)"));
				}
				break;

				case EFieldType::Enum:
				{
					auto ty = StaticCastSharedPtr<enumType>(type);
					VerifySize(ty->storage->size);
					outField.SetType<uint64>(size);
				}
				break;

				case EFieldType::BitFlags:
				{
					auto ty = StaticCastSharedPtr<bitflagsType>(type);
					VerifySize(ty->storage->size);
					outField.SetType<TSharedPtr<FNiFieldGroup>>(size);
				}
					break;

				case EFieldType::BitField:
				{
					auto ty = StaticCastSharedPtr<bitfieldType>(type);
					VerifySize(ty->storage->size);
					outField.SetType<TSharedPtr<FNiFieldGroup>>(size);
				}
				break;

				case EFieldType::Struct:
				{
					auto ty = StaticCastSharedPtr<structType>(type);
					outField.SetType<TSharedPtr<FNiFieldGroup>>(size);
				}
				break;

				default:
					throw NiException(TEXT("Unsupported base type"));
				}

				switch (type->GetFieldType())
				{
				case EFieldType::Basic:
				{
					auto ty = StaticCastSharedPtr<basicType>(type);

					if (ty->name == "Ptr" || ty->name == "Ref")
					{
						for (uint32 i = 0; i < size; i++)
							reader << outField.PushValue<FNiBlockReference>().Index;
					}
					else if (ty->name == "BinaryBlob")
					{
						auto& arr = outField.GetByteArray();
						arr.SetNum(size);
						reader.ByteOrderSerialize(arr.GetData(), size);
					}
					else
					{
						for (uint32 i = 0; i < size; i++)
							reader.ByteOrderSerialize(&outField.PushValue<uint64>(), ty->size);
					}
				}
				break;

				case EFieldType::Enum:
				{
					auto ty = StaticCastSharedPtr<enumType>(type);
					auto subType = ty->storage;
					uint32 enumSize = subType->size;

					for (uint32 i = 0; i < size; i++)
					{
						uint64& enumValue = outField.PushValue<uint64>();
						reader.ByteOrderSerialize(&enumValue, enumSize);

						if (!ty->HasOption(enumValue))
							throw NiException(TEXT("Provided invalid value %d for enum"), enumValue);	
					}
				}
				break;

				case EFieldType::BitFlags:
				{
					auto ty = StaticCastSharedPtr<bitflagsType>(type);

					for (uint32 i = 0; i < size; i++)
					{
						auto group = StaticCastSharedPtr<FNiBitflags>(outField.PushValue<TSharedPtr<FNiFieldGroup>>(MakeShared<FNiBitflags>()));
						group->Type = ty;

						uint64 bytes = 0;
						reader.ByteOrderSerialize(&bytes, ty->storage->size);

						for (const auto& option : ty->options)
						{
							FNiField optionField;
							optionField.Meta = option;
							optionField.SourceOffset = outField.SourceOffset;
							optionField.SourceSubOffset = option->bit;

							optionField.SetType<uint64>(1);
							optionField.PushValue<uint64>((bytes & ((uint64)1 << option->bit)) >> option->bit);

							group->Fields.Add(optionField);
						}	
					}
				}
					break;

				case EFieldType::BitField:
				{
					auto ty = StaticCastSharedPtr<bitfieldType>(type);

					for (uint32 i = 0; i < size; i++)
					{
						//auto group = StaticCastSharedPtr<FNiBitfield>(outField.PushValue<TSharedPtr<FNiFieldGroup>>(MakeShared<FNiBitfield>()));
						//group->Type = ty;

						uint64 bytes = 0;
						TArray<uint8> memberBytes;
						memberBytes.SetNum(sizeof(bytes));

						reader.ByteOrderSerialize(&bytes, ty->storage->size);

						for (const auto& member : ty->members)
						{
							FNiField memberField;
							memberField.Meta = member;
							memberField.SourceOffset = outField.SourceOffset;
							memberField.SourceSubOffset = member->pos;

							*(uint64*)memberBytes.GetData() = (bytes & member->mask) >> member->pos;
							FMemoryReader memberReader(memberBytes);
							check(fieldConstructor(member->type, 1, memberReader, memberField));

							//group->Fields.Add(memberField);
							contextStack.Last().FieldStorage->Fields.Add(memberField);
						}	
					}

					return false;
				}
				break;

				case EFieldType::Struct:
				{
					auto ty = StaticCastSharedPtr<structType>(type);

					for (uint32 i = 0; i < size; i++)
					{
						auto group = StaticCastSharedPtr<FNiStruct>(outField.PushValue<TSharedPtr<FNiFieldGroup>>(MakeShared<FNiStruct>()));
						group->Type = ty;

						uint32 startPos = reader.Tell();
						contextStack.PushLast(ContextEntry(group.Get()));
						ty->ProcessFields(File, fieldHandler, fieldTokenHandler);
						contextStack.PopLast();
						outField.SourceItemSize = reader.Tell() - startPos;
					}
				}
				break;
				}

				return true;
			};

			fieldHandler = [&](const TSharedPtr<field>& field)
			{
				if (!field->type.IsValid())
					throw NiException(TEXT("Unsupported base type"));

				FNiField newField;
				newField.Meta = field;
				newField.SourceOffset = MemoryReader.Tell();

				uint32 length = 1;
				if (!field->lengthFieldName.IsEmpty())
					length = FCString::IsNumeric(*field->lengthFieldName)
						? FCString::Atoi64(*field->lengthFieldName)
						: contextStack.Last().FieldStorage->GetField(field->lengthFieldName).SingleNumber<uint32>();

				uint32 width = 1;
				if (!field->widthFieldName.IsEmpty())
					width = FCString::IsNumeric(*field->widthFieldName)
						? FCString::Atoi64(*field->widthFieldName)
						: contextStack.Last().FieldStorage->GetField(field->widthFieldName).SingleNumber<uint32>();

				uint32 size = length * width;

				if (fieldConstructor(field->type, size, MemoryReader, newField))
					contextStack.Last().FieldStorage->Fields.Add(newField);
			};

			Block->Fields.Reset();

			TSet<FString> inheritNames;
			ProcessFields(File, fieldHandler, fieldTokenHandler, inheritNames);

			if (MemoryReader.TotalSize() != MemoryReader.Tell())
				Block->Error = FString::Printf(TEXT("Data read is %lld while block size was %lld"), MemoryReader.Tell(), MemoryReader.TotalSize());
			else
				Block->Error = "";
			}
		}
		catch (NiException e)
		{
			Block->Error = e.what();
		}
		catch (...)
		{
		}
	}

	void niobject::ProcessFields(const FNiFile& file, const TFunction<void(const TSharedPtr<field>& field)>& fieldHandler, const TFunction<FString(const FString& fieldName)>& fieldTokenHandler, TSet<FString>& inheritNames)
	{
		if (inherit)
		{
			inheritNames.Add(name);
			inherit->ProcessFields(file, fieldHandler, fieldTokenHandler, inheritNames);
		}

		for (const auto& field : fields)
		{
			try
			{
				if (field->since.IsValid() && file.Version < field->since)
					continue;
				if (field->until.IsValid() && file.Version > field->until)
					continue;
				if (!field->vercond.IsEmpty() && !ResolveExpr(field->vercond, file, nullptr))
					continue;
				if (!field->cond.IsEmpty() && !ResolveExpr(field->cond, file, fieldTokenHandler))
					continue;
				if (!field->excludeT.IsEmpty() && inheritNames.Contains(field->excludeT))
					continue;
				if (!field->onlyT.IsEmpty() && !inheritNames.Contains(field->onlyT))
					continue;

				fieldHandler(field);
			}
			catch (NiException e)
			{
				throw NiException(TEXT("%s -> %hs"), *field->name, e.what());
			}
		}
	}

	TSharedPtr<niobject> GetNiObject(const FString& name)
	{
		auto entry = NiObjectMap().FindOrAdd(name);
		if (!entry.IsValid())
		{
			UE_LOG(LogDV2, Warning, TEXT("Created moc NIF type %s"), *name)
			entry = MakeShared<niobject>();
			entry->name = name;
		}

		return entry;
	}

	TSharedPtr<niobject> GetNiObjectChecked(const FString& name)
	{
		auto entry = NiObjectMap().FindRef(name);
		check(entry.IsValid())

		return entry;
	}

	TMulticastDelegate<void()>& OnReset()
	{
		static TMulticastDelegate<void()> Event;
		return Event;
	}

	TMulticastDelegate<void()>& OnReload()
	{
		static TMulticastDelegate<void()> Event;
		return Event;
	}

	void Reset()
	{
		OnReset().Broadcast();
		
		TokenMap().Reset();
		VersionMap().Reset();
		ModuleMap().Reset();
		BasicMap().Reset();
		EnumMap().Reset();
		BitflagsMap().Reset();
		BitfieldMap().Reset();
		StructMap().Reset();
		NiObjectMap().Reset();

		UE_LOG(LogDV2, Display, TEXT("NIF metadata has been reset"));
	}

#pragma region Metadata Loading
	class FNifMetadataParser : public IFastXmlCallback
	{
	public:
		struct XmlNode
		{
			template <typename T>
			T GetAttribute(const FString& attrName, T defaultValue = T())
			{
				for (const auto& attr : Attributes)
				{
					if (attr.Key == attrName)
						return ParseAttrValue<T>(attr.Value);
				}
				return defaultValue;
			}

			FString Name;
			FString Content;
			TSortedMap<FString, FString> Attributes;
			TArray<TSharedPtr<XmlNode>> Children;
		};

		virtual bool ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber) override
		{
			auto node = MakeShared<XmlNode>();
			node->Name = ElementName;
			node->Content = ElementData;

			if (XmlParseStack.Num() != 0)
				XmlParseStack.Last()->Children.Add(node);
			else
				XmlParseRoot = node;

			XmlParseStack.PushLast(node);

			return true;
		}

		virtual bool ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue) override
		{
			XmlParseStack.Last()->Attributes.Add(AttributeName, AttributeValue);
			return true;
		}

		virtual bool ProcessClose(const TCHAR* ElementName) override
		{
			XmlParseStack.PopLast();
			return true;
		}

		virtual bool ProcessXmlDeclaration(const TCHAR* ElementData, int32 XmlFileLineNumber) override { return true; }
		virtual bool ProcessComment(const TCHAR* Comment) override { return true; }

		TSharedPtr<XmlNode> XmlParseRoot;
		TDeque<TSharedPtr<XmlNode>> XmlParseStack;
	};

	void LoadNifMetaFile(const FString& filePath)
	{
		FString FullPath = filePath;

		FString xmlString;
		if (!FFileHelper::LoadFileToString(xmlString, *FullPath))
		{
			UE_LOG(LogDV2, Error, TEXT("Failed to read nif meta file %s"), *filePath);
			return;
		}

		FText parseErrorMessage;
		int32 parseErrorLineNumber;
		FNifMetadataParser xmlParseHandler;

		if (!FFastXml::ParseXmlFile(
			&xmlParseHandler,
			nullptr,
			xmlString.GetCharArray().GetData(),
			GWarn,
			false,
			false,
			parseErrorMessage,
			parseErrorLineNumber
			))
		{
			UE_LOG(LogDV2, Error, TEXT("Failed to parse nif metadata: %s on line %d"), *parseErrorMessage.ToString(), parseErrorLineNumber);
			return;
		}

		if (!xmlParseHandler.XmlParseRoot.IsValid())
		{
			UE_LOG(LogDV2, Error, TEXT("Invalid nif metadata"));
			return;
		}

		uint32 TokenNum = TokenMap().Num();
		uint32 VersionNum = VersionMap().Num();
		uint32 ModuleNum = ModuleMap().Num();
		uint32 BasicNum = BasicMap().Num();
		uint32 EnumNum = EnumMap().Num();
		uint32 BitfieldNum = BitfieldMap().Num();
		uint32 BitflagsNum = BitflagsMap().Num();
		uint32 StructNum = StructMap().Num();
		uint32 NiObjectNum = NiObjectMap().Num();

		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> tokenNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> versionNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> moduleNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> basicNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> enumNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> structNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> bitfieldNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> bitflagsNodes;
		TArray<TSharedPtr<FNifMetadataParser::XmlNode>> niobjectNodes;

		for (const auto& child : xmlParseHandler.XmlParseRoot->Children)
		{
			if (child->Name == "token")
				tokenNodes.Add(child);
			else if (child->Name == "version")
				versionNodes.Add(child);
			else if (child->Name == "module")
				moduleNodes.Add(child);
			else if (child->Name == "basic")
				basicNodes.Add(child);
			else if (child->Name == "enum")
				enumNodes.Add(child);
			else if (child->Name == "bitfield")
				bitfieldNodes.Add(child);
			else if (child->Name == "bitflags")
				bitflagsNodes.Add(child);
			else if (child->Name == "struct")
				structNodes.Add(child);
			else if (child->Name == "niobject")
				niobjectNodes.Add(child);
		}

		for (auto node : tokenNodes)
		{
			for (auto child : node->Children)
			{
				TokenMap().Add(child->Attributes["token"], child->Attributes["string"]
				                                           .Replace(TEXT("&lt;"), TEXT("<"))
				                                           .Replace(TEXT("&gt;"), TEXT(">"))
				                                           .Replace(TEXT("&amp;"), TEXT("&"))
					);
			}
		}

#define WriteNiMetaAttr(Obj, Name) Obj->Name = node->GetAttribute<decltype(Obj->Name)>(#Name);
#define WriteNiMetaAttrN(Obj, Name1, Name2) Obj->Name1 = node->GetAttribute<decltype(Obj->Name1)>(#Name2);
#define WriteNiMetaAttrO(Obj, Name) Obj.Name = node->GetAttribute<decltype(Obj.Name)>(#Name);
#define WriteNiMetaAttrON(Obj, Name1, Name2) Obj.Name1 = node->GetAttribute<decltype(Obj.Name1)>(#Name2);
#define WriteNiMetaDescription(Obj) Obj->description = node->Content;
#define WriteNiMetaDescriptionO(Obj) Obj.description = node->Content;

#pragma warning(push)
#pragma warning(disable:4456)

		for (const auto& node : versionNodes)
		{
			auto& version = VersionMap().Add(node->Attributes["id"], MakeShared<NiMeta::version>());
			WriteNiMetaAttrN(version, name, id);
			WriteNiMetaAttr(version, num);
			WriteNiMetaAttr(version, user);
			WriteNiMetaDescription(version);

			version->ver = FNiVersion::Parse(version->num);
		}

		for (const auto& node : moduleNodes)
		{
			auto& module = ModuleMap().Add(node->Attributes["name"], MakeShared<NiMeta::module>());
			WriteNiMetaAttr(module, name);
			WriteNiMetaAttr(module, priority);
			WriteNiMetaAttr(module, depends);
			WriteNiMetaAttr(module, custom);
			WriteNiMetaDescription(module);
		}

		for (const auto& node : basicNodes)
		{
			auto& basicType = BasicMap().Add(node->Attributes["name"], MakeShared<NiMeta::basicType>());
			WriteNiMetaAttr(basicType, name);
			WriteNiMetaAttr(basicType, integral);
			WriteNiMetaAttr(basicType, countable);
			WriteNiMetaAttr(basicType, size);
			WriteNiMetaDescription(basicType);
			basicType->SetSizeString(basicType->size, 0);
		}
		auto bitTy = BasicMap().FindRef("bit");

		for (const auto& node : enumNodes)
		{
			auto& enumType = EnumMap().Add(node->Attributes["name"], MakeShared<NiMeta::enumType>());
			WriteNiMetaAttr(enumType, name);
			WriteNiMetaAttr(enumType, storage);
			WriteNiMetaDescription(enumType);
			enumType->SetSizeString(enumType->storage->size, 0);

			for (const auto& node : node->Children)
			{
				auto& option = enumType->options.Add_GetRef(enumType::option());
				WriteNiMetaAttrO(option, value);
				WriteNiMetaAttrO(option, name);
				WriteNiMetaDescriptionO(option);
			}
		}

		for (const auto& node : bitflagsNodes)
		{
			auto& bitflagType = BitflagsMap().Add(node->Attributes["name"], MakeShared<NiMeta::bitflagsType>());
			WriteNiMetaAttr(bitflagType, name);
			WriteNiMetaAttr(bitflagType, storage);
			WriteNiMetaDescription(bitflagType);
			bitflagType->SetSizeString(bitflagType->storage->size, 0);

			for (const auto& node : node->Children)
			{
				auto& option = bitflagType->options.Add_GetRef(MakeShared<NiMeta::bitflagsType::option>());
				WriteNiMetaAttr(option, name);
				WriteNiMetaAttr(option, bit);
				WriteNiMetaDescription(option);

				option->type = bitTy;
			}
		}

		for (const auto& node : bitfieldNodes)
		{
			auto& bitfieldType = BitfieldMap().Add(node->Attributes["name"], MakeShared<NiMeta::bitfieldType>());
			WriteNiMetaAttr(bitfieldType, name);
			WriteNiMetaAttr(bitfieldType, storage);
			WriteNiMetaDescription(bitfieldType);
			bitfieldType->SetSizeString(bitfieldType->storage->size, 0);

			for (const auto& node : node->Children)
			{
				auto& member = bitfieldType->members.Add_GetRef(MakeShared<NiMeta::bitfieldType::member>());
				WriteNiMetaAttr(member, name);
				WriteNiMetaAttr(member, type);
				WriteNiMetaAttr(member, width);
				WriteNiMetaAttr(member, pos);
				WriteNiMetaAttr(member, mask);
				WriteNiMetaDescription(member);
			}
		}

		for (const auto& node : structNodes)
		{
			auto& structType = StructMap().Add(node->Attributes["name"], MakeShared<NiMeta::structType>());
			WriteNiMetaAttr(structType, name);
			WriteNiMetaAttr(structType, module);
			WriteNiMetaDescription(structType);

			for (const auto& node : node->Children)
			{
				auto& field = structType->fields.Add_GetRef(MakeShared<NiMeta::field>());
				WriteNiMetaAttr(field, name);
				WriteNiMetaAttrN(field, defaultValue, default);
				WriteNiMetaAttr(field, type);
				WriteNiMetaAttrN(field, lengthFieldName, length);
				WriteNiMetaAttrN(field, widthFieldName, width);
				WriteNiMetaAttr(field, vercond);
				WriteNiMetaAttr(field, cond);
				WriteNiMetaAttr(field, since);
				WriteNiMetaAttr(field, until);
				WriteNiMetaDescription(field);
			}
		}

		for (const auto& node : niobjectNodes)
		{
			auto& niobject = NiObjectMap().Add(node->Attributes["name"], MakeShared<NiMeta::niobject>());
			WriteNiMetaAttr(niobject, name);
			WriteNiMetaAttr(niobject, abstract);
			WriteNiMetaAttr(niobject, inherit);
			WriteNiMetaAttr(niobject, module);
			WriteNiMetaAttr(niobject, icon);
			WriteNiMetaDescription(niobject);

			for (const auto& node : node->Children)
			{
				auto& field = niobject->fields.Add_GetRef(MakeShared<NiMeta::field>());
				WriteNiMetaAttr(field, name);
				WriteNiMetaAttrN(field, defaultValue, default);
				WriteNiMetaAttr(field, type);
				WriteNiMetaAttrN(field, lengthFieldName, length);
				WriteNiMetaAttrN(field, widthFieldName, width);
				WriteNiMetaAttr(field, vercond);
				WriteNiMetaAttr(field, cond);
				WriteNiMetaAttr(field, since);
				WriteNiMetaAttr(field, until);
				WriteNiMetaAttr(field, excludeT);
				WriteNiMetaAttr(field, onlyT);
				WriteNiMetaDescription(field);
			}
		}

#pragma warning(pop)

#undef WriteNiMetaAttr
#undef WriteNiMetaAttrN
#undef WriteNiMetaAttrO
#undef WriteNiMetaAttrON
#undef WriteNiMetaDescription
#undef WriteNiMetaDescriptionO

		uint32 TokenNumAdded = TokenMap().Num() - TokenNum;
		uint32 VersionNumAdded = VersionMap().Num() - VersionNum;
		uint32 ModuleNumAdded = ModuleMap().Num() - ModuleNum;
		uint32 BasicNumAdded = BasicMap().Num() - BasicNum;
		uint32 EnumNumAdded = EnumMap().Num() - EnumNum;
		uint32 BitfieldNumAdded = BitfieldMap().Num() - BitfieldNum;
		uint32 BitflagsNumAdded = BitflagsMap().Num() - BitflagsNum;
		uint32 StructNumAdded = StructMap().Num() - StructNum;
		uint32 NiObjectNumAdded = NiObjectMap().Num() - NiObjectNum;

		UE_LOG(LogDV2, Display, TEXT(
			       "Loaded NIF metadata:\n"
			       "%d tokens\n"
			       "%d versions\n"
			       "%d modules\n"
			       "%d basic types\n"
			       "%d enum types\n"
			       "%d bitfield types\n"
			       "%d bitflags types\n"
			       "%d struct types\n"
			       "%d niobject types\n"
			       "from file %s"),
		       TokenNumAdded,
		       VersionNumAdded,
		       ModuleNumAdded,
		       BasicNumAdded,
		       EnumNumAdded,
		       BitfieldNumAdded,
		       BitflagsNumAdded,
		       StructNumAdded,
		       NiObjectNumAdded,
		       *filePath);
	}
#pragma endregion

	void Reload()
	{
		Reset();

		for (const auto& path : GetDefault<UDV2Settings>()->NifMetaFiles)
		{
			LoadNifMetaFile(FPaths::ConvertRelativePathToFull(path.FilePath));
		}

		PatchNifMeta();

		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UNiBlockComponentConfigurator::StaticClass()))
			{
				auto CDO = It->GetDefaultObject<UNiBlockComponentConfigurator>();
				if (auto BlockType = CDO->GetTargetBlockType(); BlockType.IsValid())
				{
					BlockType->ComponentConfigurator = It->GetDefaultObject<UNiBlockComponentConfigurator>();
				}
			}
		}

		OnReload().Broadcast();
	}
}
