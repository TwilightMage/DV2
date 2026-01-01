/*
 * A Divinity 2/Ego Draconis archive handler.
 *
 * December 2019 Vometia a.k.a. Christine Munro
 *
 * unpack.c - does as it says.  Hopefully.
 */

#include "DV2Importer/unpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "DV2.h"
#include "Divinity2Assets.h"
#include "DV2Importer/dv2.h"
#include "DV2Importer/io.h"
#include "DV2Importer/utils.h"
#include "ThirdParty/zlib/1.3/include/zlib.h"

#pragma warning(push)
#pragma warning(disable : 4706)

static unsigned int writeFile(const Dv2File& dv2File, int entryIndex,
                              const TUniquePtr<IFileHandle>& sourceFile, const FString& dirRoot);

bool FDv2AssetReference::Read(const Dv2File& dv2File, const TUniquePtr<IFileHandle>& sourceFile, TArray<uint8>& bytes) const
{
	bool decompress = (bool)dv2File.header.h_ispacked && fd.fd_l != 0;

	int zr;
	TArray<uint8> tempReadBuf;
	uint8* readDst;

	if (decompress)
	{
		tempReadBuf.SetNum(fd.fd_lz);
		readDst = tempReadBuf.GetData();
	}
	else
	{
		bytes.SetNum(fd.fd_lz);
		readDst = bytes.GetData();
	}

	if (!sourceFile->ReadAt(readDst, fd.fd_lz, fd.fd_start))
	{
		UE_LOG(LogDV2, Error, TEXT("%hs: failed to read file data"), __FUNCTION__)
		return false;
	}

	if (decompress)
	{
		z_stream zs;
		zs.zalloc = Z_NULL;
		zs.zfree = Z_NULL;
		zs.opaque = Z_NULL;
		if ((zr = inflateInit(&zs)) != Z_OK)
		{
			UE_LOG(LogDV2, Error, TEXT("%hs: inflateInit failed (zr=%d)"), __FUNCTION__, zr);
			return false;
		}

		/* compressed data in */
		zs.next_in = readDst;
		zs.avail_in = fd.fd_lz;

		/* uncompressed data out */
		bytes.SetNum(fd.fd_l);
		zs.next_out = bytes.GetData();
		zs.avail_out = fd.fd_l;

		if ((zr = inflate(&zs, Z_FULL_FLUSH)) < Z_OK)
		{
			UE_LOG(LogDV2, Error, TEXT("%hs: inflate failed (zr=%d,in=%d,out=%d)"), __FUNCTION__, zr, fd.fd_lz,
			       fd.fd_l);
			return false;
		}
		UE_LOG(LogDV2, Display, TEXT("%hs: inflate succeeded (zr=%d,in=%d,out=%d)"), __FUNCTION__, zr, fd.fd_lz,
		       fd.fd_l);

		if ((zr = inflateEnd(&zs)) != Z_OK)
		{
			UE_LOG(LogDV2, Error, TEXT("%hs: inflateInit failed (zr=%d)"), __FUNCTION__, zr);
			return false;
		}
	}

	return true;
}

bool FDv2AssetReference::Read(const Dv2File& dv2File, TArray<uint8>& bytes) const
{
	TUniquePtr<IFileHandle> fileHandle(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*dv2File.Path));
	if (!fileHandle.IsValid())
	{
		UE_LOG(LogDV2, Error, TEXT("%hs: failed to open \"%s\" for reading"), __FUNCTION__, *dv2File.Path)
		return false;
	}

	return Read(dv2File, fileHandle, bytes);
}

void Dv2File::PrintInfo() const
{
	UE_LOG(LogDV2, Display, TEXT("DV2 Header data:"));
	UE_LOG(LogDV2, Display, TEXT("  DV2 type: %d"), header.h_id);
	UE_LOG(LogDV2, Display, TEXT("  aligned to %d boundaries: %hs"), DV2_BUFSIZ, header.h_isaligned ? "yes" : "no");
	UE_LOG(LogDV2, Display, TEXT("  packed: %hs"), header.h_ispacked ? "yes" : "no");
	UE_LOG(LogDV2, Display, TEXT("  header data-start offset=%u"), header.h_start);
	UE_LOG(LogDV2, Display, TEXT("  header filename length=%u"), header.h_filenames);
	UE_LOG(LogDV2, Display, TEXT("  undocumented int 1: %X"), header.h_unknown1);
	UE_LOG(LogDV2, Display, TEXT("  undocumented int 2: %X"), header.h_unknown2);
}

void Dv2File::PrintFileEntries() const
{
	for (int i = 0; i < assetReferences.Num(); i++)
	{
		UE_LOG(LogDV2, Display, TEXT("%5d %s/%s"), i, *assetReferences[i].dir, *assetReferences[i].name);
	}
}

bool readDv2(const FString& filePath, Dv2File& dv2File)
{
	dv2File = Dv2File();
	dv2File.Path = filePath;
	filedata_t filedata;

	sizeddata_t headerversion[] = {
		SD_ENTRY(dv2File.header.h_id),
		SD_END
	};
	sizeddata_t headerv4[] = {
		SD_ENTRY(dv2File.header.h_isaligned),
		SD_ENTRY(dv2File.header.h_ispacked),
		SD_ENTRY(dv2File.header.h_start),
		SD_ENTRY(dv2File.header.h_filenames),
		SD_END
	};
	sizeddata_t headerv5[] = {
		SD_ENTRY(dv2File.header.h_unknown1),
		SD_ENTRY(dv2File.header.h_unknown2),
		SD_ENTRY(dv2File.header.h_isaligned),
		SD_ENTRY(dv2File.header.h_ispacked),
		SD_ENTRY(dv2File.header.h_start),
		SD_ENTRY(dv2File.header.h_filenames),
		SD_END
	};
	sizeddata_t filedatabits[] = {
		SD_ENTRY(filedata.fd_start),
		SD_ENTRY(filedata.fd_lz),
		SD_ENTRY(filedata.fd_l),
		SD_END
	};

	TUniquePtr<IFileHandle> fileHandle(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*filePath));
	if (!fileHandle.IsValid())
	{
		UE_LOG(LogDV2, Error, TEXT("failed to open \"%s\" for reading"), *filePath)
		return false;
	}

	getsizeddata(fileHandle, headerversion);
	if (dv2File.header.h_id == 4)
		getsizeddata(fileHandle, headerv4);
	else if (dv2File.header.h_id == 5)
		getsizeddata(fileHandle, headerv5);
	else
	{
		UE_LOG(LogDV2, Error, TEXT("%s is not a recognised DV2 format"), *filePath)
		return false;
	}

	unsigned int filenamedata = 0;
	for (filenamedata = 0; (unsigned int)filenamedata < dv2File.header.h_filenames;)
	{
		FString fileName;
		filenamedata += getBytesString(fileHandle, fileName);
		fileName = FixPath(fileName);
		dv2File.assetReferences.Add({.name = FPaths::GetPathLeaf(fileName), .dir = FPaths::GetPath(fileName)});
	}

	if (filenamedata != dv2File.header.h_filenames)
		UE_LOG(LogDV2, Warning, TEXT("incorrect file data size (%d; should be %d)"), filenamedata,
	       dv2File.header.h_filenames);

	unsigned int nentries;
	getBytesRaw(fileHandle, sizeof(nentries), &nentries);

	if (dv2File.assetReferences.Num() != nentries)
	{
		UE_LOG(LogDV2, Error, TEXT("file count %d is wrong (should be %d)"), dv2File.assetReferences.Num(), nentries)
		return false;
	}

	UE_LOG(LogDV2, Display, TEXT("%d file(s) found in %s"), dv2File.assetReferences.Num(), *filePath);

	for (int i = 0; i < dv2File.assetReferences.Num(); i++)
	{
		getsizeddata(fileHandle, filedatabits);
		dv2File.assetReferences[i].fd.fd_start = filedata.fd_start + dv2File.header.h_start;
		dv2File.assetReferences[i].fd.fd_lz = filedata.fd_lz;
		dv2File.assetReferences[i].fd.fd_l = filedata.fd_l;
	}

	return true;
}

bool unpackDv2(const FString& filePath, const FString& dirRoot)
{
	Dv2File dv2File;
	readDv2(filePath, dv2File);

	TUniquePtr<IFileHandle> fileHandle(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*filePath));
	if (!fileHandle.IsValid())
	{
		UE_LOG(LogDV2, Error, TEXT("failed to open \"%s\" for reading"), *filePath)
		return false;
	}

	for (int i = 0; i < dv2File.assetReferences.Num(); i++)
	{
		const auto& entry = dv2File.assetReferences[i];

		UE_LOG(LogDV2, Display, TEXT("%5d %s/%s (st=%d,sz=%d,os=%d)"),
		       i + 1,
		       *entry.dir,
		       *entry.name,
		       entry.fd.fd_start,
		       entry.fd.fd_lz,
		       entry.fd.fd_l);

		auto expectBytes = entry.fd.fd_l
			? entry.fd.fd_l
			: entry.fd.fd_lz;

		auto gotBytes = writeFile(dv2File, i, fileHandle, dirRoot);

		if (gotBytes != expectBytes)
		{
			UE_LOG(LogDV2, Error, TEXT("%s/%s is the wrong size: expected %d, got %d"),
			       *entry.dir,
			       *entry.name,
			       expectBytes,
			       gotBytes)
			return false;
		}
	}
	return true;
}

/*
 * Decompress data, then write file
 */

static unsigned int writeFile(const Dv2File& dv2File, int entryIndex, const TUniquePtr<IFileHandle>& sourceFile,
                              const FString& dirRoot)
{
	const FDv2AssetReference& entry = dv2File.assetReferences[entryIndex];

	FString entryPath = FPaths::Combine(dirRoot, entry.dir, entry.name);
	FString entryDir = FPaths::Combine(dirRoot, entry.dir);

	do_mkdir(TCHAR_TO_UTF8(*entryDir));

	TUniquePtr<IFileHandle> outFileHandle(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*entryPath));
	if (!outFileHandle.IsValid())
	{
		UE_LOG(LogDV2, Error, TEXT("%hs: couldn't open %s for writing"), __FUNCTION__, *entryPath);
		return -1;
	}

	TArray<uint8> fileBuf;
	if (!entry.Read(dv2File, sourceFile, fileBuf))
		return -1;

	outFileHandle->Write(fileBuf.GetData(), fileBuf.Num());
	outFileHandle->Flush();

	UE_LOG(LogDV2, Display, TEXT("%s created, %d byte(s) written"), *entryPath, fileBuf.Num());
	return fileBuf.Num();
}

/* vim:set syntax=c nospell tabstop=8: */

#pragma warning(pop)
