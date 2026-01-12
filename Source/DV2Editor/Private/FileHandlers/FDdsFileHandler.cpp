#include "FileHandlers/FDdsFileHandler.h"

#include "DV2AssetTree.h"
#include "Slate/SNifView.h"

TSharedPtr<SWidget> FDdsFileHandler::CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset)
{
	TArray<uint8> Bytes;
	if (!Asset->Read(Bytes))
		return nullptr;

	if (Bytes.Num() > 0 && Bytes[0] == 0x44 && Bytes[1] == 0x44 && Bytes[2] == 0x53) // DDS image magic
	{
		return nullptr;
	}
	else
	{
		return SNew(SNifView, Asset);
	}
}
