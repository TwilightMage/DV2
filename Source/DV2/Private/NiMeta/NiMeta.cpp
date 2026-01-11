#include "NiMeta/NiMeta.h"

#include "FastXml.h"
#include "NetImmerse.h"
#include "Containers/Deque.h"
#include "DV2Importer/DV2Settings.h"
#include "NiMeta/NiException.h"
#include "NiMeta/Patch.h"
#include "NiMeta/XmlParser.h"
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
						throw MakeNiExceptionA("Unknown token %s", *targetKey);
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

FString ResolveExpr(const FString& str, const FNiFile& file, const TFunction<FString(const FString& tokenName)>& tokenResolver)
{
	FString expression = ExpandTokens(str, [&](const FString& token) -> FString
	{
		if (token == "#VER#")
			return FString::Printf(TEXT("%d.%d.%d.%d"), file.Version.v1, file.Version.v2, file.Version.v3, file.Version.v4);
		if (token == "#USER#")
			return FString::FromInt(file.UserVersion);
		if (token == "#BSVER#")
			return "0";
		if (tokenResolver.IsSet())
			return tokenResolver(token);
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

	if (expression.Contains("error"))
		throw MakeNiExceptionA("Got invalid expression: \"%s\" that expanded to \"%s\"", *str, *expression);

	return expression;
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

TSharedPtr<NiMeta::fieldType> FieldTypeByName(const FString& str)
{
	if (str.IsEmpty())
		return nullptr;
	if (str == "#T#")
		return NiMeta::TemplateTypePlaceholder;
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
	return nullptr;
}

template <>
TSharedPtr<NiMeta::fieldType> ParseAttrValue(const FString& str)
{
	if (auto type = FieldTypeByName(str); type.IsValid())
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

	FAST_MAP_IMPL(TSharedPtr<templatedStructInstance>, TemplatedStructInstance)

	static TMap<FName, size_t> MetaTypeIdMap;
	static size_t MetaTypeIdGenerator = 0;
	size_t metaObject::staticTypeIdHelper(const char* str)
	{
		if (auto Id = MetaTypeIdMap.Find(str))
			return *Id;
		else
			return MetaTypeIdMap.Add(str, ++MetaTypeIdGenerator);
	}

	bool field::isAvailable(const FNiFile& file) const
	{
		return (since.IsValid() ? file.Version >= since : true) &&
			(until.IsValid() ? file.Version <= until : true);
	}

	basicType::basicType()
	{
		ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
		{
			return FString::Printf(TEXT("%llu"), field.NumberAt<uint64>(i));
		};
	}

#define VerifySize(ItemSize) \
    	field.SourceItemSize = (ItemSize) * arraySize; \
    	if (field.SourceItemSize > (reader.TotalSize() - reader.Tell())) \
    		throw MakeNiExceptionA("Got size overflow: have %d bytes left, but requested %d", (reader.TotalSize() - reader.Tell()), field.SourceItemSize) \
    			.AddSource("Block Meta", ctx.Block->Type) \
    			.AddSource("Field Meta", field.Meta);

	bool basicType::writeField(FNiField& field, uint32 arraySize, FMemoryReader& reader, BlockReadContext& ctx)
	{
		if (name == "Ptr" || name == "Ref")
		{
			VerifySize(sizeof(uint32));
			field.SetType<FNiBlockReference>(arraySize);
			for (uint32 i = 0; i < arraySize; i++)
				reader << field.PushValue<FNiBlockReference>().Index;
		}
		else if (name == "BinaryBlob")
		{
			field.SetType<uint8>(arraySize);
			auto& arr = field.GetByteArray();
			arr.SetNum(arraySize);
			reader.ByteOrderSerialize(arr.GetData(), arraySize);
		}
		else if (size > 0)
		{
			VerifySize(size);
			field.SetType<uint64>(arraySize);
			for (uint32 i = 0; i < arraySize; i++)
				reader.ByteOrderSerialize(&field.PushValue<uint64>(), size);
		}
		else
			throw MakeNiException("Malformed type (not ref/ptr, binary blob or number)")
				.AddSource("Block Meta", ctx.Block->Type)
				.AddSource("Field Meta", field.Meta);

		return true;
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

	bool enumType::writeField(FNiField& field, uint32 arraySize, FMemoryReader& reader, BlockReadContext& ctx)
	{
		VerifySize(storage->size);
		field.SetType<uint64>(arraySize);

		for (uint32 i = 0; i < arraySize; i++)
		{
			uint64& enumValue = field.PushValue<uint64>();
			reader.ByteOrderSerialize(&enumValue, storage->size);

			if (!HasOption(enumValue))
				throw MakeNiExceptionA("Provided invalid value %d for enum", enumValue)
					.AddSource("Block Meta", ctx.Block->Type)
					.AddSource("Field Meta", field.Meta);
		}

		return true;
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

	bool bitflagsType::writeField(FNiField& field, uint32 arraySize, FMemoryReader& reader, BlockReadContext& ctx)
	{
		VerifySize(storage->size);
		field.SetType<TSharedPtr<FNiFieldGroup>>(arraySize);

		for (uint32 i = 0; i < arraySize; i++)
		{
			auto group = field.PushValue<TSharedPtr<FNiFieldGroup>>(MakeShared<FNiFieldGroup>());
			group->Type = StaticCastSharedPtr<bitflagsType>(AsShared().ToSharedPtr());

			uint64 bytes = 0;
			reader.ByteOrderSerialize(&bytes, storage->size);

			for (const auto& option : options)
			{
				FNiField optionField;
				optionField.Meta = option;
				optionField.SourceOffset = field.SourceOffset;
				optionField.SourceSubOffset = option->bit;

				optionField.SetType<uint64>(1);
				optionField.PushValue<uint64>((bytes & ((uint64)1 << option->bit)) >> option->bit);

				group->Fields.Add(optionField);
			}
		}

		return true;
	}

	bool bitfieldType::writeField(FNiField& field, uint32 arraySize, FMemoryReader& reader, BlockReadContext& ctx)
	{
		VerifySize(storage->size);
		field.SetType<TSharedPtr<FNiFieldGroup>>(arraySize);

		for (uint32 i = 0; i < arraySize; i++)
		{
			uint64 bytes = 0;
			TArray<uint8> memberBytes;
			memberBytes.SetNum(sizeof(bytes));

			reader.ByteOrderSerialize(&bytes, storage->size);

			for (const auto& member : members)
			{
				FNiField memberField;
				memberField.Meta = member;
				memberField.SourceOffset = field.SourceOffset;
				memberField.SourceSubOffset = member->pos;

				*(uint64*)memberBytes.GetData() = (bytes & member->mask) >> member->pos;
				FMemoryReader memberReader(memberBytes);
				check(member->type->writeField(memberField, 1, memberReader, ctx));

				ctx.ContextStack.Last().FieldStorage->Fields.Add(memberField);
			}
		}

		return false;
	}

	bool structType::writeField(FNiField& field, uint32 arraySize, FMemoryReader& reader, BlockReadContext& ctx)
	{
		field.SetType<TSharedPtr<FNiFieldGroup>>(arraySize);

		for (uint32 i = 0; i < arraySize; i++)
		{
			auto group = field.PushValue<TSharedPtr<FNiFieldGroup>>(MakeShared<FNiFieldGroup>());
			group->Type = StaticCastSharedPtr<structType>(AsShared().ToSharedPtr());

			uint32 startPos = reader.Tell();
			ctx.ContextStack.PushLast(BlockReadContext::ContextEntry(group.Get()));
			ProcessFields(reader, ctx);
			ctx.ContextStack.PopLast();
			field.SourceItemSize = reader.Tell() - startPos;
		}

		return true;
	}

	void structType::ProcessFields(FMemoryReader& reader, BlockReadContext& ctx)
	{
		for (const auto& field : fields)
		{
			try
			{
				if (field->since.IsValid() && ctx.File.Version < field->since)
					continue;
				if (field->until.IsValid() && ctx.File.Version > field->until)
					continue;
				if (!field->vercond.IsEmpty() && ResolveExpr(field->vercond, ctx.File, nullptr) != "true")
					continue;
				if (!field->cond.IsEmpty() && ResolveExpr(field->cond, ctx.File, [&](const FString& tokenName)
				{
					return ctx.ResolveFieldToken(tokenName);
				}) != "true")
					continue;

				ctx.HandleField(reader, field);
			}
			catch (NiException e)
			{
				throw NiException(e, field->name);
			}
		}
	}

	BlockReadContext::ContextEntry::ContextEntry(FNiBlock* block)
		: FieldStorage(block)
		  , MetaFieldStorage(block->Type.Get())
	{
	}

	BlockReadContext::ContextEntry::ContextEntry(FNiFieldGroup* group)
		: FieldStorage(group)
		  , MetaFieldStorage(group->Type.Get())
	{
	}

	void BlockReadContext::HandleField(FMemoryReader& reader, const TSharedPtr<field>& field)
	{
		if (!field->type.IsValid())
			throw MakeNiException("Unsupported base type")
				.AddSource("Block Meta", Block->Type)
				.AddSource("Field Meta", field);

		FNiField newField;
		newField.Meta = field;
		newField.SourceOffset = reader.Tell();

		uint32 length = 1;
		if (!field->lengthFieldName.IsEmpty())
			length = FCString::IsNumeric(*field->lengthFieldName)
				? FCString::Atoi64(*field->lengthFieldName)
				: ContextStack.Last().FieldStorage->GetField(field->lengthFieldName).SingleNumber<uint32>();

		uint32 width = 1;
		if (!field->widthFieldName.IsEmpty())
			width = FCString::IsNumeric(*field->widthFieldName)
				? FCString::Atoi64(*field->widthFieldName)
				: ContextStack.Last().FieldStorage->GetField(field->widthFieldName).SingleNumber<uint32>();

		uint32 size = length * width;

		if (field->type->writeField(newField, size, reader, *this))
			ContextStack.Last().FieldStorage->Fields.Add(newField);
	}

	FString BlockReadContext::ResolveFieldToken(const FString& fieldToken)
	{
		FString fieldName = fieldToken.Mid(1, fieldToken.Len() - 2);
		if (const FNiField* field = ContextStack.Last().FieldStorage->FindField(fieldName))
		{
			// Cannot use arrays for evaluaion
			if (field->Size != 1)
				return "error";

			return field->Meta->type->ToString(File, *field, 0);
		}
		else
		{
			for (int32 i = 0; i < ContextStack.Last().MetaFieldStorage->NumMembers(); i++)
			{
				auto defaultField = ContextStack.Last().MetaFieldStorage->GetMemberAt(i);
				if (defaultField->name == fieldName && defaultField->isAvailable(File))
				{
					FString defaultValue = defaultField->getDefaultValue();
					if (!defaultValue.IsEmpty())
						return defaultValue;
					return "error";
				}
			}

			return "error";
		}
	}

	void BlockReadContext::Read(FMemoryReader& reader)
	{
		Block->SetError("Unknown error");

		ContextStack.PushLast(ContextEntry(Block.Get()));

		try
		{
			Block->Fields.Reset();

			if (Block->Type->CustomRead.IsSet())
			{
				Block->Type->CustomRead(Block, reader, File);
			}
			else
			{
				TSet<FString> inheritNames;
				Block->Type->ProcessFields(reader, *this, inheritNames);

				if (reader.TotalSize() != reader.Tell())
					Block->SetErrorManual(FString::Printf(TEXT("Data read is %lld while block size was %lld"), reader.Tell(), reader.TotalSize()));
				else
					Block->ClearError();
			}
		}
		catch (NiException e)
		{
			Block->SetError(e);
		}
		catch (...)
		{
		}
	}

	void niobject::ProcessFields(FMemoryReader& reader, BlockReadContext& ctx, TSet<FString>& inheritNames)
	{
		if (inherit)
		{
			inheritNames.Add(name);
			inherit->ProcessFields(reader, ctx, inheritNames);
		}

		for (const auto& field : fields)
		{
			try
			{
				if (field->since.IsValid() && ctx.File.Version < field->since)
					continue;
				if (field->until.IsValid() && ctx.File.Version > field->until)
					continue;
				if (!field->vercond.IsEmpty() && ResolveExpr(field->vercond, ctx.File, nullptr) != "true")
					continue;
				if (!field->cond.IsEmpty() && ResolveExpr(field->cond, ctx.File, [&](const FString& tokenName)
				{
					return ctx.ResolveFieldToken(tokenName);
				}) != "true")
					continue;
				if (!field->excludeT.IsEmpty() && inheritNames.Contains(field->excludeT))
					continue;
				if (!field->onlyT.IsEmpty() && !inheritNames.Contains(field->onlyT))
					continue;

				ctx.HandleField(reader, field);
			}
			catch (NiException e)
			{
				throw NiException(e, field->name);
			}
		}
	}

	bool templatedStructInstance::writeField(FNiField& field, uint32 arraySize, FMemoryReader& reader, BlockReadContext& ctx)
	{
		field.SetType<TSharedPtr<FNiFieldGroup>>(arraySize);

		for (uint32 i = 0; i < arraySize; i++)
		{
			auto group = field.PushValue<TSharedPtr<FNiFieldGroup>>(MakeShared<FNiFieldGroup>());
			group->Type = StaticCastSharedPtr<templatedStructInstance>(AsShared().ToSharedPtr());

			uint32 startPos = reader.Tell();
			ctx.ContextStack.PushLast(BlockReadContext::ContextEntry(group.Get()));
			ProcessFields(reader, ctx, templateArg);
			ctx.ContextStack.PopLast();
			field.SourceItemSize = reader.Tell() - startPos;
		}

		return true;
	}

	void templatedStructInstance::ProcessFields(FMemoryReader& reader, BlockReadContext& ctx, const FString& arg)
	{
		for (const auto& field : baseStruct->fields)
		{
			try
			{
				auto actualField = field->type == TemplateTypePlaceholder
					? templateSpecifiedFields[field->name]
					: field;

				FString argResolved = arg;
				if (!arg.IsEmpty())
				{
					argResolved = ResolveExpr(actualField->cond, ctx.File, [&](const FString& tokenName)
					{
						return ctx.ResolveFieldToken(tokenName);
					});
				}

				if (actualField->since.IsValid() && ctx.File.Version < actualField->since)
					continue;
				if (actualField->until.IsValid() && ctx.File.Version > actualField->until)
					continue;
				if (!actualField->vercond.IsEmpty() && ResolveExpr(actualField->vercond, ctx.File, nullptr) != "true")
					continue;
				if (!actualField->cond.IsEmpty() && ResolveExpr(actualField->cond, ctx.File, [&](const FString& tokenName)
				{
					if (tokenName == "#ARG#")
						return argResolved;
					return ctx.ResolveFieldToken(tokenName);
				}) != "true")
					continue;

				ctx.HandleField(reader, actualField);
			}
			catch (NiException e)
			{
				throw NiException(e, field->name);
			}
		}
	}

	TSharedPtr<fieldType> ResolveTemplatedType(const TSharedPtr<fieldType>& BaseType, const TSharedPtr<fieldType>& TemplateType, const FString& TemplateArg)
	{
		if (!BaseType.IsValid())
			return nullptr;

		if (BaseType->isType<structType>())
		{
			if (!TemplateType.IsValid() || TemplateType == TemplateTypePlaceholder)
				return BaseType;

			FString TemplatedInstanceName = FString::Printf(TEXT("%s`%s"), *BaseType->name, *TemplateType->name);
			auto templatedInstance = TemplatedStructInstanceMap().FindOrAdd(TemplatedInstanceName);
			if (!templatedInstance.IsValid())
			{
				templatedInstance = MakeShared<templatedStructInstance>();
				templatedInstance->name = TemplatedInstanceName;
				templatedInstance->baseStruct = StaticCastSharedPtr<structType>(BaseType);
				templatedInstance->templateType = TemplateType;
				templatedInstance->templateArg = TemplateArg;

				for (const auto& baseField : templatedInstance->baseStruct->fields)
				{
					TSharedPtr<field> specifiedField;
					if (baseField->type == TemplateTypePlaceholder)
					{
						specifiedField = MakeShared<field>(*baseField);
						specifiedField->type = TemplateType;
					}
					else if (baseField->templateType == "#T#")
					{
						specifiedField = MakeShared<field>(*baseField);
						specifiedField->type = ResolveTemplatedType(specifiedField->type, TemplateType, baseField->templateArg);
					}

					if (!specifiedField.IsValid())
						continue;

					templatedInstance->ToString = BaseType->ToString;
					templatedInstance->GenerateSlateWidget = BaseType->GenerateSlateWidget;

					templatedInstance->templateSpecifiedFields.Add(baseField->name, specifiedField);
				}
			}

			return templatedInstance;
		}
		return BaseType;
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

		metaObject::FilePaths.Reset();

		TokenMap().Reset();
		VersionMap().Reset();
		ModuleMap().Reset();
		BasicMap().Reset();
		EnumMap().Reset();
		BitflagsMap().Reset();
		BitfieldMap().Reset();
		StructMap().Reset();
		NiObjectMap().Reset();

		TemplatedStructInstanceMap().Reset();

		UE_LOG(LogDV2, Display, TEXT("NIF metadata has been reset"));
	}

#pragma region Metadata Loading
	class FNifMetadataParser : public FXmlParser
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
			int32 Line;
		};

		virtual void OnComment(const FString& Comment, uint32 Line) override
		{

		}

		virtual void OnTagOpen(const FString& TagName, uint32 Line) override
		{
			auto node = MakeShared<XmlNode>();
			node->Name = TagName;
			node->Line = Line;

			if (XmlParseStack.Num() != 0)
				XmlParseStack.Last()->Children.Add(node);
			else
				XmlParseRoot = node;

			XmlParseStack.PushLast(node);
		}

		virtual void OnText(const FString& Text) override
		{
			if (XmlParseStack.IsEmpty())
				return;

			XmlParseStack.Last()->Content += "\n" + Text;
		}

		virtual void OnAttribute(const FString& Name, const FString& Value) override
		{
			XmlParseStack.Last()->Attributes.Add(Name, Value);
		}

		virtual void OnTagClose() override
		{
			XmlParseStack.PopLast();
		}

		TSharedPtr<XmlNode> XmlParseRoot;
		TDeque<TSharedPtr<XmlNode>> XmlParseStack;
	};

	void LoadNifMetaFile(const FString& filePath, TArray<TSharedPtr<field>>& OutTemplatedFields)
	{
		FString FullPath = filePath;

		FString XmlParseError;
		uint32 XmlParseLine;
		FNifMetadataParser XmlParser;

		if (!XmlParser.Parse(*FullPath, XmlParseError, XmlParseLine))
		{
			UE_LOG(LogDV2, Error, TEXT("Failed to parse nif metadata: %s on line %d"), *XmlParseError, XmlParseLine);
			return;
		}

		if (!XmlParser.XmlParseRoot.IsValid())
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

		for (const auto& child : XmlParser.XmlParseRoot->Children)
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

		auto CleanDescription = [](const FString& Description)
		{
			TArray<FString> Lines;
			Description.ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num(); i++)
			{
				Lines[i].TrimStartAndEndInline();
				if (Lines[i].IsEmpty())
					Lines.RemoveAt(i--);
			}
			return FString::Join(Lines, TEXT("\n"));
		};

#define WriteNiMetaAttr(Obj, Name) Obj->Name = node->GetAttribute<decltype(Obj->Name)>(#Name);
#define WriteNiMetaAttrN(Obj, Name1, Name2) Obj->Name1 = node->GetAttribute<decltype(Obj->Name1)>(#Name2);
#define WriteNiMetaAttrO(Obj, Name) Obj.Name = node->GetAttribute<decltype(Obj.Name)>(#Name);
#define WriteNiMetaAttrON(Obj, Name1, Name2) Obj.Name1 = node->GetAttribute<decltype(Obj.Name1)>(#Name2);
#define WriteNiMetaDescription(Obj) Obj->description = CleanDescription(node->Content);
#define WriteNiMetaDescriptionO(Obj) Obj.description = CleanDescription(node->Content);

#pragma warning(push)
#pragma warning(disable:4456)

		for (const auto& node : versionNodes)
		{
			auto& version = VersionMap().Add(node->Attributes["id"], MakeShared<NiMeta::version>());
			WriteNiMetaAttrN(version, name, id);
			WriteNiMetaAttr(version, num);
			WriteNiMetaAttr(version, user);
			WriteNiMetaDescription(version);
			version->SourceFilePathIndex = metaObject::FilePaths.Num();
			version->SourceLine = node->Line;

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
			module->SourceFilePathIndex = metaObject::FilePaths.Num();
			module->SourceLine = node->Line;
		}

		for (const auto& node : basicNodes)
		{
			auto& basicType = BasicMap().Add(node->Attributes["name"], MakeShared<NiMeta::basicType>());
			WriteNiMetaAttr(basicType, name);
			WriteNiMetaAttr(basicType, integral);
			WriteNiMetaAttr(basicType, countable);
			WriteNiMetaAttr(basicType, size);
			WriteNiMetaDescription(basicType);
			basicType->SourceFilePathIndex = metaObject::FilePaths.Num();
			basicType->SourceLine = node->Line;

			basicType->SetSizeString(basicType->size, 0);
		}
		auto bitTy = BasicMap().FindRef("bit");

		for (const auto& node : enumNodes)
		{
			auto& enumType = EnumMap().Add(node->Attributes["name"], MakeShared<NiMeta::enumType>());
			WriteNiMetaAttr(enumType, name);
			WriteNiMetaAttr(enumType, storage);
			WriteNiMetaDescription(enumType);
			enumType->SourceFilePathIndex = metaObject::FilePaths.Num();
			enumType->SourceLine = node->Line;

			enumType->SetSizeString(enumType->storage->size, 0);

			for (const auto& node : node->Children)
			{
				auto& option = enumType->options.Add_GetRef(enumType::option());
				WriteNiMetaAttrO(option, value);
				WriteNiMetaAttrO(option, name);
				WriteNiMetaDescriptionO(option);
				option.SourceFilePathIndex = metaObject::FilePaths.Num();
				option.SourceLine = node->Line;
			}
		}

		for (const auto& node : bitflagsNodes)
		{
			auto& bitflagType = BitflagsMap().Add(node->Attributes["name"], MakeShared<NiMeta::bitflagsType>());
			WriteNiMetaAttr(bitflagType, name);
			WriteNiMetaAttr(bitflagType, storage);
			WriteNiMetaDescription(bitflagType);
			bitflagType->SourceFilePathIndex = metaObject::FilePaths.Num();
			bitflagType->SourceLine = node->Line;

			bitflagType->SetSizeString(bitflagType->storage->size, 0);

			for (const auto& node : node->Children)
			{
				auto& option = bitflagType->options.Add_GetRef(MakeShared<NiMeta::bitflagsType::option>());
				WriteNiMetaAttr(option, name);
				WriteNiMetaAttr(option, bit);
				WriteNiMetaDescription(option);
				option->SourceFilePathIndex = metaObject::FilePaths.Num();
				option->SourceLine = node->Line;

				option->type = bitTy;
			}
		}

		for (const auto& node : bitfieldNodes)
		{
			auto& bitfieldType = BitfieldMap().Add(node->Attributes["name"], MakeShared<NiMeta::bitfieldType>());
			WriteNiMetaAttr(bitfieldType, name);
			WriteNiMetaAttr(bitfieldType, storage);
			WriteNiMetaDescription(bitfieldType);
			bitfieldType->SourceFilePathIndex = metaObject::FilePaths.Num();
			bitfieldType->SourceLine = node->Line;

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
				member->SourceFilePathIndex = metaObject::FilePaths.Num();
				member->SourceLine = node->Line;
			}
		}

		for (const auto& node : structNodes)
		{
			auto& structType = StructMap().Add(node->Attributes["name"], MakeShared<NiMeta::structType>());
			WriteNiMetaAttr(structType, name);
			WriteNiMetaAttr(structType, module);
			WriteNiMetaDescription(structType);
			structType->SourceFilePathIndex = metaObject::FilePaths.Num();
			structType->SourceLine = node->Line;

			for (const auto& node : node->Children)
			{
				auto& field = structType->fields.Add_GetRef(MakeShared<NiMeta::field>());
				WriteNiMetaAttr(field, name);
				WriteNiMetaAttr(field, type);
				WriteNiMetaAttrN(field, templateType, template);
				WriteNiMetaAttrN(field, templateArg, arg);
				WriteNiMetaAttrN(field, defaultValue, default);
				WriteNiMetaAttrN(field, lengthFieldName, length);
				WriteNiMetaAttrN(field, widthFieldName, width);
				WriteNiMetaAttr(field, vercond);
				WriteNiMetaAttr(field, cond);
				WriteNiMetaAttr(field, since);
				WriteNiMetaAttr(field, until);
				WriteNiMetaDescription(field);
				field->SourceFilePathIndex = metaObject::FilePaths.Num();
				field->SourceLine = node->Line;

				if (!field->templateType.IsEmpty() && field->templateType != "#T#" && field->type->name != "Ref" && field->type->name != "Ptr")
					OutTemplatedFields.Add(field);
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
			niobject->SourceFilePathIndex = metaObject::FilePaths.Num();
			niobject->SourceLine = node->Line;

			for (const auto& node : node->Children)
			{
				auto& field = niobject->fields.Add_GetRef(MakeShared<NiMeta::field>());
				WriteNiMetaAttr(field, name);
				WriteNiMetaAttr(field, type);
				WriteNiMetaAttrN(field, templateType, template);
				WriteNiMetaAttrN(field, templateArg, arg);
				WriteNiMetaAttrN(field, defaultValue, default);
				WriteNiMetaAttrN(field, lengthFieldName, length);
				WriteNiMetaAttrN(field, widthFieldName, width);
				WriteNiMetaAttr(field, vercond);
				WriteNiMetaAttr(field, cond);
				WriteNiMetaAttr(field, since);
				WriteNiMetaAttr(field, until);
				WriteNiMetaAttr(field, excludeT);
				WriteNiMetaAttr(field, onlyT);
				WriteNiMetaDescription(field);
				field->SourceFilePathIndex = metaObject::FilePaths.Num();
				field->SourceLine = node->Line;

				if (!field->templateType.IsEmpty() && field->templateType != "#T#" && field->type->name != "Ref" && field->type->name != "Ptr")
					OutTemplatedFields.Add(field);
			}
		}

#pragma warning(pop)

#undef WriteNiMetaAttr
#undef WriteNiMetaAttrN
#undef WriteNiMetaAttrO
#undef WriteNiMetaAttrON
#undef WriteNiMetaDescription
#undef WriteNiMetaDescriptionO

		metaObject::FilePaths.Add(filePath);

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

		TArray<TSharedPtr<field>> TemplatedFields;
		for (const auto& path : GetDefault<UDV2Settings>()->NifMetaFiles)
		{
			LoadNifMetaFile(FPaths::ConvertRelativePathToFull(path.FilePath), TemplatedFields);
		}

		PatchNifMeta();

		for (const auto& Field : TemplatedFields)
		{
			Field->type = ResolveTemplatedType(Field->type, FieldTypeByName(Field->templateType), Field->templateArg);
		}

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
