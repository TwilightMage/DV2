#pragma once

struct FDv2AssetReference;
struct Dv2File;

struct DV2_API FDV2AssetTreeEntry
{
	FText GetTitle() const
	{
		return FText::FromString(Name);
	}

	bool IsFile() const
	{
		return File.IsValid() && AssetIndex >= 0;
	}

	bool ExportToDisk(FString* outFilePath = nullptr, bool overwrite = false, bool open = false) const;
	bool ExportToDiskRecursive(bool overwrite = false) const;

	const FDv2AssetReference& GetAssetReference() const;
	FString GetPath() const;

	FString Name;
	TSortedMap<FString, TSharedPtr<FDV2AssetTreeEntry>> Children;
	TWeakPtr<FDV2AssetTreeEntry> Parent;
	TSharedPtr<Dv2File> File;
	int32 AssetIndex = -1;
};
