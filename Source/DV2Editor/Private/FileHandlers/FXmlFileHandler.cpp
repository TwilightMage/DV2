#include "FileHandlers/FXmlFileHandler.h"

#include "DV2AssetTree.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "NetImmerse.h"
#include "TextUtils.h"
#include "NiMeta/CStreamableNode.h"

#if WITH_EDITOR
void FXmlFileHandler::ConfigureMenu(const TSharedPtr<FDV2AssetTreeEntry>& Asset, FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection(NAME_None, MAKE_TEXT("FXmlFileHandler", "XML"));
	MenuBuilder.AddMenuEntry(
		MAKE_TEXT("FXmlFileHandler", "Export as XML"),
		MAKE_TEXT("FXmlFileHandler", "Export CStreamable tree as XML file"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FXmlFileHandler::ExportXml, Asset)));
	MenuBuilder.EndSection();
}

void FXmlFileHandler::ExportXml(TSharedPtr<FDV2AssetTreeEntry> Asset) const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
		return;
	
	TArray<FString> OutFilenames;
	FString FileTypes = TEXT("XML Files (*.xml)|*.xml");

	if (!DesktopPlatform->SaveFileDialog(
		nullptr,
		TEXT("Export XML"),
		TEXT(""),
		Asset->Name,
		FileTypes,
		EFileDialogFlags::None,
		OutFilenames
		))
		return;

	if (OutFilenames.Num() != 1)
		return;
	
	auto File = UNetImmerse::OpenNiFile(Asset, true);
	if (!ensure(File->Blocks.Num() == 1))
		return;

	if (File->Blocks[0]->Error.IsValid())
		return;

	if (!File->Blocks[0]->IsOfType("xml::dom::CStreamableNode"))
		return;

	auto Block = StaticCastSharedPtr<FNiBlock_CStreamableNode>(File->Blocks[0]);
	if (!ensure(Block->CStreamableNodes.Num() > 0))
		return;

	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*OutFilenames[0]);

	uint8 UTF8BOM[] = { 0xEF, 0xBB, 0xBF };
	FileWriter->Serialize(UTF8BOM, 3);
	
	auto RootNode = Block->CStreamableNodes[0];

	const uint32 TabSize = 4;

	auto Print = [&](const FString& Str)
	{
		FTCHARToUTF8 Converter(*Str, Str.Len());
		FileWriter->Serialize((UTF8CHAR*)Converter.Get(), Converter.Length());
	};
	
	TFunction<void(const TSharedPtr<FCStreamableNode>& Node, uint32 Offset)> PrintNode = nullptr;
	PrintNode = [&](const TSharedPtr<FCStreamableNode>& Node, uint32 Offset)
	{
		FString NodeTypeName = FCStreamableNode::GetNodeTypeName(Node->Data.TypeId, "n");
		
		Print(FString().LeftPad(Offset * TabSize));
		Print("<");
		Print(NodeTypeName);

		if (!Node->Data.Name.IsEmpty())
		{
			Print(" name=\"");
			Print(Node->Data.Name);
			Print("\"");
		}

		for (const auto& Property : Node->Data.Properties)
		{
			Print(" ");
			Print(FCStreamableNode::GetPropertyTypeName(Property.Type, "p"));
			Print("=\"");
			Print(Property.Value.ReplaceQuotesWithEscapedQuotes());
			Print("\"");
		}

		if (Node->Children.IsEmpty())
		{
			Print("/>\n");	
		}
		else
		{
			Print(">\n");

			for (const auto& Child : Node->Children)
			{
				PrintNode(Child, Offset + 1);
			}

			Print(FString().LeftPad(Offset * TabSize));
			Print("</");
			Print(NodeTypeName);
			Print(">\n");	
		}
	};

	PrintNode(RootNode, 0);
	FileWriter->Close();
	delete FileWriter;
}
#endif
