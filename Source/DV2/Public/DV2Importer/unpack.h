#pragma once

#include "dv2.h"

struct Dv2File;

struct DV2_API FDv2AssetReference
{
	FString name;
	FString dir;
	filedata_t fd;

	bool Read(const Dv2File& dv2File, const TUniquePtr<IFileHandle>& sourceFile, TArray<uint8>& bytes) const;
	bool Read(const Dv2File& dv2File, TArray<uint8>& bytes) const;

	FString GetFilePath() const { return dir + "/" + name; }
};

struct DV2_API Dv2File
{
	FString Path;
	header_t header;
	TArray<FDv2AssetReference> assetReferences;

	void PrintInfo() const;
	void PrintFileEntries() const;
};

bool readDv2(const FString& filePath, Dv2File& dv2File);
bool unpackDv2(const FString& filePath, const FString& dirRoot);
