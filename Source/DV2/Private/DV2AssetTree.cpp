#include "DV2AssetTree.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <shellapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "DV2.h"
#include "DV2Importer/DV2Settings.h"
#include "DV2Importer/unpack.h"

bool FDV2AssetTreeEntry::ExportToDisk(FString* outFilePath, bool overwrite, bool open) const
{
	if (!IsFile())
		return false;

	const auto& ref = GetAssetReference();

	FString dirPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(GetDefault<UDV2Settings>()->ExportLocation.Path, ref.dir));
	FString filePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(GetDefault<UDV2Settings>()->ExportLocation.Path, ref.dir, ref.name));

	if (outFilePath)
		*outFilePath = filePath;

	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*filePath) && !overwrite)
		return true;

	TArray<uint8> bytes;
	if (!ref.Read(*File, bytes))
		return false;

	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*dirPath);

	TUniquePtr<IFileHandle> outFileHandle(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*filePath));
	if (!outFileHandle.IsValid())
	{
		UE_LOG(LogDV2, Error, TEXT("Couldn't open %s for writing"), *filePath);
		return false;
	}

	outFileHandle->Write(bytes.GetData(), bytes.Num());
	outFileHandle->Flush();

	if (open)
		ShellExecute(0, L"open", *filePath, 0, 0, SW_SHOW);

	return true;
}

bool FDV2AssetTreeEntry::ExportToDiskRecursive(bool overwrite) const
{
	if (IsFile())
	{
		if (!ExportToDisk(nullptr, overwrite))
			return false;
	}
	else
	{
		for (const auto& child : Children)
		{
			if (!child.Value->ExportToDiskRecursive(overwrite))
				return false;
		}
	}

	return true;
}

const FDv2AssetReference& FDV2AssetTreeEntry::GetAssetReference() const
{
	return File->assetReferences[AssetIndex];
}
