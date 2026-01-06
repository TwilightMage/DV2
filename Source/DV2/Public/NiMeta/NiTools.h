#pragma once
#include "NetImmerse.h"

struct FNiFile;
struct FNiField;

namespace NiTools
{
	struct FFieldQuery
	{
		enum FieldType
		{
			FT_Integer,
			FT_Float,
			FT_String,
		};

		FFieldQuery() = default;

		FFieldQuery(const FString& InFieldName, uint32 InValueIndex, FieldType InExpectedType)
			: FieldName(InFieldName)
			  , ValueIndex(InValueIndex)
			  , ExpectedType(InExpectedType)
		{
		}

		template<typename T>
		void SetValue(T Val)
		{
			ReturnedValue.Emplace<T>(Val);
			bSuccess = true;
		}

		FString FieldName;
		uint32 ValueIndex;
		FieldType ExpectedType;
		TVariant<int64, float, FString> ReturnedValue;
		bool bSuccess = false;
	};

	DV2_API FString ReadString(const FNiFile& File, const FNiField& Field, uint32 Index = 0);
	DV2_API FVector ReadVector3(const FNiField& Field, uint32 Index = 0);
	DV2_API FColor ReadColorFloat(const FNiField& Field, uint32 Index = 0);
	DV2_API FVector2D ReadVector2(const FNiField& Field, uint32 Index = 0);
	DV2_API FVector2D ReadUV(const FNiField& Field, uint32 Index = 0);
	DV2_API FRotator ReadRotator(const FNiField& Field, uint32 Index = 0);

	template<typename T>
	bool ReadNumberSafe(T& Result, const FNiField& Field, uint32 Index = 0)
	{
		if (Field.Meta->type->isType<NiMeta::basicType>())
		{
			auto basicType = StaticCastSharedPtr<NiMeta::basicType>(Field.Meta->type);
			if (basicType->integral)
			{
				Result = (T)Field.NumberAt<int64>(Index);
				return true;
			}
			if (basicType == NiMeta::BasicMap().FindRef("float"))
			{
				Result = (T)Field.NumberAt<float>(Index);
				return true;
			}
		}
		else if (Field.Meta->type->isType<NiMeta::enumType>())
		{
			Result = (T)Field.NumberAt<int64>(Index);
			return true;
		}
		
		Result = 0;
		return false;
	}

	DV2_API bool ReadStringSafe(FString& Result, const FNiFile& File, const FNiField& Field, uint32 Index = 0);
	DV2_API bool ReadStructSafe(TArray<FFieldQuery>& Query, const FNiFile& File, const FNiField& Field, uint32 Index = 0);
}
