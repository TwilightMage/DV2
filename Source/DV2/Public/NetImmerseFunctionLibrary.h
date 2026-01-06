// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NetImmerseFunctionLibrary.generated.h"

struct FNiFile;
class UNiFileHandle;
class UCStreamableNodeHandle;

UCLASS()
class DV2_API UNetImmerseFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Divinity 2|Net Immerse")
	static UNiFileHandle* OpenNiFile(UPARAM(meta=(DV2AssetPath)) const FString& Path, bool bForceLoad);

	UFUNCTION(BlueprintPure, DisplayName="Get DV2 Asset Path", Category="Divinity 2|Net Immerse")
	static FString GetDV2AssetPath(UPARAM(meta=(DV2Assetpath="dir, file")) const FString& Path);
};

UENUM()
enum class EGetFieldValueResult : uint8
{
	BadBlockIndex,
	BadFieldName,
	BadValueIndex,
	BadType,
	Success,
};

UCLASS(DisplayName="NI File")
class UNiFileHandle : public UObject
{
	GENERATED_BODY()

public:
	TSharedPtr<FNiFile> File;

	UFUNCTION(BlueprintCallable, DisplayName="Get CStreamableNode", Category="Divinity 2|Net Immerse")
	UCStreamableNodeHandle* GetCStreamableNode(int32 BlockIndex);

	UFUNCTION(BlueprintPure, Category="Divinity 2|Net Immerse")
	FString GetBlockType(int32 BlockIndex) const;

	UFUNCTION(BlueprintPure, Category="Divinity 2|Net Immerse")
	FString GetBlockName(int32 BlockIndex) const;

	UFUNCTION(BlueprintCallable, Category="Divinity 2|Net Immerse")
	void FindBlockByName(const FString& Name, bool bFindAll, int32& OutFirstIndex, TArray<int32>& OutAllIndices, bool& bFoundAny);

	UFUNCTION(BlueprintPure, Category="Divinity 2|Net Immerse")
	void GetFieldSize(int32 BlockIndex, const FString& FieldName, int32& OutSize, bool& bSuccess) const;

#pragma region GetField Declarations
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Bool(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, bool& OutValue);
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Int32(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, int32& OutValue);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Int64(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, int64& OutValue);
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Float(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, float& OutValue);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_String(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FString& OutValue);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Name(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FName& OutValue);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Text(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FText& OutValue);
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Vector(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FVector& OutValue);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Vector2D(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FVector2D& OutValue);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	EGetFieldValueResult GetFieldValue_Rotator(int32 BlockIndex, const FString& FieldName, int32 ValueIndex, FRotator& OutValue);
#pragma endregion
};