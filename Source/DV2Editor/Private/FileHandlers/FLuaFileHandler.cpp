#include "FileHandlers/FLuaFileHandler.h"

#include "DV2AssetTree.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

TSharedPtr<SWidget> FLuaFileHandler::CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset)
{
	TArray<uint8> bytes;
	if (Asset->Read(bytes))
	{
		return SNew(SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.Text(FText::FromString(UTF8_TO_TCHAR(bytes.GetData())));
	}

	return nullptr;
}
