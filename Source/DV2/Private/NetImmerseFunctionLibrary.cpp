// Fill out your copyright notice in the Description page of Project Settings.

#include "NetImmerseFunctionLibrary.h"

#include "NetImmerse.h"
#include "NiMeta/CStreamableNode.h"
#include "NiMeta/NiTools.h"

UNiFileHandle* UNetImmerseFunctionLibrary::OpenNiFile(const FString& Path, bool bForceLoad)
{
	auto Result = NewObject<UNiFileHandle>();
	Result->File = UNetImmerse::OpenNiFile(Path, bForceLoad);
	return Result;
}

FString UNetImmerseFunctionLibrary::GetDV2AssetPath(const FString& Path)
{
	return Path;
}

UCStreamableNodeHandle* UNiFileHandle::GetCStreamableNode(int32 BlockIndex)
{
	if (!File.IsValid() || BlockIndex < 0 || BlockIndex >= File->Blocks.Num())
		return nullptr;

	auto Block = StaticCastSharedPtr<FNiBlock_CStreamableNode>(File->Blocks[BlockIndex]);

	if (Block->CStreamableNodes.Num() == 0)
		return nullptr;

	UCStreamableNodeHandle* Result = NewObject<UCStreamableNodeHandle>(this);
	Result->Handle = Block->CStreamableNodes[0];

	return Result;
}

FString UNiFileHandle::GetBlockType(int32 BlockIndex) const
{
	if (!File.IsValid() || BlockIndex < 0 || BlockIndex >= File->Blocks.Num())
		return "";

	return File->Blocks[BlockIndex]->Type->name;
}

FString UNiFileHandle::GetBlockName(int32 BlockIndex) const
{
	if (!File.IsValid() || BlockIndex < 0 || BlockIndex >= File->Blocks.Num())
		return "";

	return File->Blocks[BlockIndex]->GetTitle(*File);
}

void UNiFileHandle::FindBlockByName(const FString& Name, bool bFindAll, int32& OutFirstIndex, TArray<int32>& OutAllIndices, bool& bFoundAny)
{
	OutFirstIndex = -1;
	bFoundAny = false;

	if (!File.IsValid())
		return;
	
	for (int32 i = 0; i < File->Blocks.Num(); i++)
	{
		const auto& Block = File->Blocks[i];
		if (auto NameField = Block->FindField("Name"))
		{
			if (NiTools::ReadString(*File, *NameField) == Name)
			{
				if (OutFirstIndex == -1)
					OutFirstIndex = i;
				OutAllIndices.Add(i);
				bFoundAny = true;
				if (!bFindAll)
					return;
			}
		}
	}
}

void UNiFileHandle::GetFieldSize(int32 BlockIndex, const FString& FieldName, int32& OutSize, bool& bSuccess) const
{
	OutSize = 0;
	bSuccess = false;
	if (!File.IsValid() || BlockIndex < 0 || BlockIndex >= File->Blocks.Num())
		return;

	if (auto Field = File->Blocks[BlockIndex]->FindField(FieldName))
	{
		OutSize = Field->Size;
		bSuccess = true;
	}
}

#pragma region GetField Implementations
FNiField* GetFieldEnsureIndex(const TSharedPtr<FNiFile>& File, int32 BlockIndex, const FString& FieldName, int32 ValueIndex, EGetFieldValueResult& Result)
{
	if (File.IsValid() && BlockIndex >= 0 && BlockIndex < File->Blocks.Num())
	{
		if (auto Field = File->Blocks[BlockIndex]->FindField(FieldName))
		{
			if (ValueIndex >= 0 && (uint32)ValueIndex < Field->Size)
				return Field;
			else
				Result = EGetFieldValueResult::BadValueIndex;
		}
		else
			Result = EGetFieldValueResult::BadFieldName;
	}
	else
		Result = EGetFieldValueResult::BadBlockIndex;
	return nullptr;
}

FNiField* GetFieldGroupEnsureIndex(const TSharedPtr<FNiFile>& File, int32 BlockIndex, const FString& FieldName, int32 ValueIndex, EGetFieldValueResult& Result)
{
	if (auto Field = GetFieldEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result))
	{
		if (Field->IsGroup())
			return Field;

		Result = EGetFieldValueResult::BadType;
		return nullptr;
	}

	return nullptr;
}

#define GetFieldValue_IMPL(Check, Get) \
OutValue = {}; \
EGetFieldValueResult Result; \
if (auto Field = GetFieldEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result)) \
{ \
	if (Check) \
	{ \
		OutValue = Get; \
		return EGetFieldValueResult::Success; \
	} \
	else \
		return EGetFieldValueResult::BadType; \
} \
else \
	return Result;

#define GetNumberFieldValue_IMPL() \
OutValue = {}; \
EGetFieldValueResult Result; \
if (auto Field = GetFieldEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result)) \
{ \
	if (Field->IsNumeric()) \
	{ \
		return NiTools::ReadNumberSafe(OutValue, *Field, ValueIndex) \
			? EGetFieldValueResult::Success \
			: EGetFieldValueResult::BadType; \
	} \
	else \
		return EGetFieldValueResult::BadType; \
} \
else \
	return Result;

#define GetStringFieldValue_IMPL(Constructor) \
OutValue = {}; \
EGetFieldValueResult Result; \
if (auto Field = GetFieldEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result)) \
{ \
	if (Field->IsNumeric()) \
	{ \
		FString Str; \
		if (NiTools::ReadStringSafe(Str, *File, *Field, ValueIndex)) \
		{ \
			OutValue = Constructor(Str); \
			return EGetFieldValueResult::Success; \
		} \
		return EGetFieldValueResult::BadType; \
	} \
	else \
		return EGetFieldValueResult::BadType; \
} \
else \
	return Result;

EGetFieldValueResult UNiFileHandle::GetFieldValue_Bool(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, bool& OutValue)
{
	GetNumberFieldValue_IMPL();
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Int32(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, int32& OutValue)
{
	GetNumberFieldValue_IMPL();
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Int64(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, int64& OutValue)
{
	GetNumberFieldValue_IMPL();
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Float(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, float& OutValue)
{
	GetNumberFieldValue_IMPL();
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_String(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FString& OutValue)
{
	GetStringFieldValue_IMPL(FString)
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Name(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FName& OutValue)
{
	GetStringFieldValue_IMPL(*)
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Text(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FText& OutValue)
{
	GetStringFieldValue_IMPL(FText::FromString)
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Vector(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FVector& OutValue)
{
	OutValue = {};
	EGetFieldValueResult Result;
	if (auto Field = GetFieldGroupEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result))
	{
		TArray<NiTools::FFieldQuery> Query = {
			NiTools::FFieldQuery("x", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("y", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("z", 0, NiTools::FFieldQuery::FT_Float),
		};

		if (!NiTools::ReadStructSafe(Query, *File, *Field, 0))
			return EGetFieldValueResult::BadType;

		OutValue.X = Query[0].ReturnedValue.Get<float>();
		OutValue.Y = Query[1].ReturnedValue.Get<float>();
		OutValue.Z = Query[2].ReturnedValue.Get<float>();

		return EGetFieldValueResult::Success;
	}
	else
		return Result;
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Vector2D(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FVector2D& OutValue)
{
	OutValue = {};
	EGetFieldValueResult Result;
	if (auto Field = GetFieldGroupEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result))
	{
		TArray<NiTools::FFieldQuery> Query = {
			NiTools::FFieldQuery("x", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("y", 0, NiTools::FFieldQuery::FT_Float),
		};

		if (!NiTools::ReadStructSafe(Query, *File, *Field, 0))
			return EGetFieldValueResult::BadType;

		OutValue.X = Query[0].ReturnedValue.Get<float>();
		OutValue.Y = Query[1].ReturnedValue.Get<float>();

		return EGetFieldValueResult::Success;
	}
	else
		return Result;
}

EGetFieldValueResult UNiFileHandle::GetFieldValue_Rotator(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FRotator& OutValue)
{
	OutValue = {};
	EGetFieldValueResult Result;
	if (auto Field = GetFieldGroupEnsureIndex(File, BlockIndex, FieldName, ValueIndex, Result))
	{
		TArray<NiTools::FFieldQuery> Query = {
			NiTools::FFieldQuery("m11", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m12", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m13", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m21", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m22", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m23", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m31", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m32", 0, NiTools::FFieldQuery::FT_Float),
			NiTools::FFieldQuery("m33", 0, NiTools::FFieldQuery::FT_Float),
		};

		if (!NiTools::ReadStructSafe(Query, *File, *Field, 0))
			return EGetFieldValueResult::BadType;

		FMatrix matrix = FMatrix(
			FVector(Query[0].ReturnedValue.Get<float>(), Query[1].ReturnedValue.Get<float>(), Query[2].ReturnedValue.Get<float>()),
			FVector(Query[3].ReturnedValue.Get<float>(), Query[4].ReturnedValue.Get<float>(), Query[5].ReturnedValue.Get<float>()),
			FVector(Query[6].ReturnedValue.Get<float>(), Query[7].ReturnedValue.Get<float>(), Query[8].ReturnedValue.Get<float>()),
			FVector::ZeroVector
			);

		OutValue = matrix.Rotator();

		return EGetFieldValueResult::Success;
	}
	else
		return Result;
}
#pragma endregion
