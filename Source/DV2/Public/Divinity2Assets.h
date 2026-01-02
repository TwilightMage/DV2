// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Divinity2Assets.generated.h"

struct Dv2File;
struct FDV2AssetTreeEntry;

template <typename... TBits>
FString FixPath(TBits... Bits)
{
	FString Result;
	auto Append = [&](const FString& Bit)
	{
		Result += "/" + Bit;
	};

	(Append(FString(std::forward<TBits>(Bits))), ...);

	FPaths::NormalizeFilename(Result);
	FPaths::CollapseRelativeDirectories(Result);

	while (Result.ReplaceInline(TEXT("//"), TEXT("/")) > 0);

	return Result;
}

/**
 * Wrapper to pass as parameter so nodes would generate proper pin widget (dropdown menu with file selection)
 */
USTRUCT(DisplayName="DV2 Asset Path")
struct FDV2AssetPath
{
	GENERATED_BODY()

	UPROPERTY()
	FString Path;
};

UCLASS(Abstract)
class DV2_API UDivinity2Assets : public UObject
{
	GENERATED_BODY()

public:
	UDivinity2Assets();

	static void ReloadAssets();

	static TSharedPtr<FDV2AssetTreeEntry> GetAssetEntryFromPath(const FString& Path);
	static TSharedPtr<FDV2AssetTreeEntry> GetRootDir();

	static TMulticastDelegate<void()>& OnAssetsReloaded();

private:
	struct FMutexHandle
	{
		FMutexHandle()
		{
			AssetLoadMutex.Lock();
		}

		FMutexHandle(const FMutexHandle&) = delete;
		FMutexHandle(FMutexHandle&&) = default;
		FMutexHandle& operator=(const FMutexHandle&) = delete;
		FMutexHandle& operator=(FMutexHandle&&) = default;

		~FMutexHandle()
		{
			Release();
		}

		void Release()
		{
			if (!bIsReleased)
				AssetLoadMutex.Unlock();
		}

		bool bIsReleased = false;
	};

	inline static TSharedPtr<FDV2AssetTreeEntry> MkDirVirtual(const FString& path);

	inline static TMap<FString, TSharedPtr<Dv2File>> FileMap;
	inline static TSharedPtr<FDV2AssetTreeEntry> RootDir;
	inline static UE::FMutex AssetLoadMutex;
	inline static TSharedRef<UE::Tasks::FCancellationToken> AssetLoadCTS = MakeShared<UE::Tasks::FCancellationToken>();
};
