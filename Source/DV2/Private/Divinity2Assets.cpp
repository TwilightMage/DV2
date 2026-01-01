// Fill out your copyright notice in the Description page of Project Settings.


#include "Divinity2Assets.h"

#include "DV2AssetTree.h"
#include "DV2Importer/DV2Settings.h"
#include "DV2Importer/unpack.h"

UDivinity2Assets::UDivinity2Assets()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ReloadAssets();
	}
}

void TraverseDir(const FString& path, TFunction<bool(const FString& path, bool isDir)> fileDelegate)
{
	IFileManager::Get().IterateDirectory(*path, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
	{
		if (!fileDelegate(FilenameOrDirectory, bIsDirectory))
			return false;

		if (bIsDirectory)
		{
			TraverseDir(FilenameOrDirectory, fileDelegate);
		}

		return true;
	});
}

void UDivinity2Assets::ReloadAssets()
{
	AssetLoadCTS->Cancel();
	AssetLoadCTS = MakeShared<UE::Tasks::FCancellationToken>();

	auto token = AssetLoadCTS;
	Async(EAsyncExecution::ThreadPool, [token]()
	{
		AssetLoadMutex.Lock();

		FileMap.Reset();

		if (!token->IsCanceled())
		{
			TraverseDir(GetDefault<UDV2Settings>()->Divinity2AssetLocation.Path, [&](const FString& path, bool isDir)
			{
				if (!isDir)
				{
					TSharedPtr<Dv2File>& file = FileMap.FindOrAdd(path);
					check(!file.IsValid());

					file = MakeShared<Dv2File>();

					if (readDv2(path, *file))
					{
						int i = 0;
						for (const auto& ref : file->assetReferences)
						{
							auto e = MakeShared<FDV2AssetTreeEntry>();
							e->File = file;
							e->AssetIndex = i++;
							e->Name = ref.name;
							MkDirVirtual(ref.dir)->Children.Add(ref.name, e);
						}
					}
				}

				return !token->IsCanceled();
			});

			AsyncTask(ENamedThreads::GameThread, []()
			{
				OnAssetsReloaded().Broadcast();
			});
		}

		AssetLoadMutex.Unlock();
	});
}

TSharedPtr<FDV2AssetTreeEntry> UDivinity2Assets::GetAssetEntryFromPath(const FString& Path)
{
	FString ValidPath = FixPath(Path);

	TArray<FString> Bits;
	ValidPath.ParseIntoArray(Bits, TEXT("/"));

	TSharedPtr<FDV2AssetTreeEntry> Cur = RootDir;
	for (const auto& Bit : Bits)
	{
		const auto& Next = Cur->Children.FindRef(Bit);
		if (!Next.IsValid())
			return nullptr;

		Cur = Next;
	}

	return Cur;
}

TSharedPtr<FDV2AssetTreeEntry> UDivinity2Assets::GetRootDir()
{
	return RootDir;
}

TMulticastDelegate<void()>& UDivinity2Assets::OnAssetsReloaded()
{
	static TMulticastDelegate<void()> Event;
	return Event;
}

TSharedPtr<FDV2AssetTreeEntry> UDivinity2Assets::MkDirVirtual(const FString& path)
{
	FString validPath = FixPath(path);

	if (!RootDir.IsValid())
	{
		RootDir = MakeShared<FDV2AssetTreeEntry>();
	}

	TArray<FString> directories;
	validPath.ParseIntoArray(directories, TEXT("/"));

	TSharedPtr<FDV2AssetTreeEntry> cur = RootDir;
	for (const auto& directory : directories)
	{
		auto& next = cur->Children.FindOrAdd(directory);
		if (!next.IsValid())
		{
			next = MakeShared<FDV2AssetTreeEntry>();
			next->Name = directory;
		}
		cur = next;
	}

	return cur;
}
