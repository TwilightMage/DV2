#include "AssetContextMenuGenerator.h"

#include "DV2.h"
#include "DV2AssetTree.h"
#include "DesktopPlatformModule.h"
#include "Divinity2Assets.h"
#include "IDesktopPlatform.h"
#include "IImageWrapperModule.h"
#include "NetImmerse.h"
#include "TextUtils.h"
#include "NiMeta/CStreamableNode.h"

#define LOCTEXT_NAMESPACE "FDefaultAssetContextMenuGenerator"

void FDefaultAssetContextMenuGenerator::GenerateMenu(FMenuBuilder& MenuBuilder, const TSharedPtr<FDV2AssetTreeEntry>& Asset)
{
	FString Extension = FPaths::GetExtension(Asset->Name).ToLower();

	if (Extension == "nif")
	{
		MenuBuilder.BeginSection(NAME_None, MAKE_LOCTEXT("NIF"));
		MenuBuilder.AddMenuEntry(
			MAKE_LOCTEXT("Export as model"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSPLambda(this, [Asset]()
			{
				UNetImmerse::ExportSceneWithWizard(Asset->GetPath(), 0);
			})));
		MenuBuilder.EndSection();
	}
	else if (Extension == "item")
	{
		MenuBuilder.BeginSection(NAME_None, MAKE_LOCTEXT("ITEM"));
		MenuBuilder.AddMenuEntry(
		MAKE_LOCTEXT("Export as model"),
		FText::GetEmpty(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSPLambda(this, [Asset]()
		{
			auto NiFile = UNetImmerse::OpenNiFile(Asset, true);
			if (!NiFile.IsValid())
				return;

			if (NiFile->Blocks.Num() == 0 || !NiFile->Blocks[0]->IsOfType("CStreamableAssetData"))
				return;
			
			uint32 RootBlockIndex = NiFile->Blocks[0]->ReferenceAt("Root").Index;
			UNetImmerse::ExportSceneWithWizard(Asset->GetPath(), RootBlockIndex);
		})));
		MenuBuilder.EndSection();
	}
	else if (Extension == "xml")
	{
		MenuBuilder.BeginSection(NAME_None, MAKE_LOCTEXT("XML"));
		MenuBuilder.AddMenuEntry(
			MAKE_LOCTEXT("Export as XML"),
			MAKE_LOCTEXT("Export CStreamable tree as XML file"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FDefaultAssetContextMenuGenerator::ExportXml, Asset)));
		MenuBuilder.EndSection();
	}
	else
	{
		MenuBuilder.BeginSection(NAME_None, MAKE_LOCTEXT("Level Directory"));

		auto VegetationMapPath = Asset->GetPath() / "Vegetation";
		if (UDivinity2Assets::DirExists(VegetationMapPath))
		{
			MenuBuilder.AddMenuEntry(
				MAKE_LOCTEXT("Preview vegetation map"),
				MAKE_LOCTEXT("Reconstruct vegetation map from Vegetation folder"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FDefaultAssetContextMenuGenerator::PreviewVegetationMap, Asset)));	
		}
		
		MenuBuilder.EndSection();
	}
}

void FDefaultAssetContextMenuGenerator::ExportXml(TSharedPtr<FDV2AssetTreeEntry> Asset) const
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

void FDefaultAssetContextMenuGenerator::PreviewVegetationMap(TSharedPtr<FDV2AssetTreeEntry> Asset) const
{
	auto VegetationMapPath = Asset->GetPath() / "Vegetation";

	FIntRect MapBox(FIntPoint(0, 0), FIntPoint(0, 0));

	TArray<TTuple<FIntPoint, FString>> Tiles;

	auto VegetationDir = UDivinity2Assets::GetAssetEntryFromPath(VegetationMapPath);
	for (const auto& VegetationEntry : VegetationDir->Children)
	{
		if (FPaths::GetExtension(VegetationEntry.Key) != "tga")
			continue;
		
		TArray<FString> EntryNameParts;
		FPaths::GetBaseFilename(VegetationEntry.Key).ParseIntoArray(EntryNameParts, TEXT("_"));

		if (EntryNameParts.Num() != 3)
			continue;

		FIntPoint Pos;
		LexFromString(Pos.X, *EntryNameParts[1]);
		LexFromString(Pos.Y, *EntryNameParts[2]);

		MapBox.Min = MapBox.Min.ComponentMin(Pos);
		MapBox.Max = MapBox.Max.ComponentMax(Pos);

		Tiles.Add({Pos, VegetationMapPath / VegetationEntry.Key});
	}

	FIntPoint GridSize(MapBox.Width() + 1, MapBox.Height() + 1);
	FIntPoint TextureSize(GridSize * 32);

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(TextureSize.X * TextureSize.Y);

	TArray<bool> GrassTypeFlags;
	GrassTypeFlags.SetNumZeroed(256);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	for (const auto& Tile : Tiles)
	{
		TArray<uint8> Bytes;
		UDivinity2Assets::GetAssetEntryFromPath(Tile.Value)->Read(Bytes);
		auto Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::TGA, *FString::Printf(TEXT("Vegetation tile loader %d %d"), Tile.Key.X, Tile.Key.Y));
		Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num());

		TArray<uint8> TilePixelBytes;
		Wrapper->GetRaw(ERGBFormat::BGRA, 8, TilePixelBytes);

		if (!ensure(TilePixelBytes.Num() == 32 * 32 * 4))
			continue;

		for (int32 i = 0; i < TilePixelBytes.Num(); i += 4)
		{
			FColor& LocalPixel = *(FColor*)&TilePixelBytes[i];

			GrassTypeFlags[LocalPixel.R] = true;

			int32 LocalPixelX = (i / 4) % 32;
			int32 LocalPixelY = (i / 4) / 32;

			int32 GlobalPixelX = (Tile.Key.X - MapBox.Min.X) * 32 + LocalPixelX;
			int32 GlobalPixelY = (Tile.Key.Y - MapBox.Min.Y) * 32 + LocalPixelY;

			GlobalPixelY = TextureSize.Y - GlobalPixelY - 1;

			int32 GlobalPixelIndex = TextureSize.X * GlobalPixelY + GlobalPixelX;
			Pixels[GlobalPixelIndex] = LocalPixel;
		}
	}

	TConstArrayView64<uint8> PixelsView((uint8*)Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize.X, TextureSize.Y, PF_B8G8R8A8, NAME_None, PixelsView);
	Texture->Filter = TF_Nearest;
	Texture->UpdateResource();

	TArray<uint8> GrassTypeBytes;
	TArray<FString> GrassTypeBytesString;
	for (int32 i = 0; i < GrassTypeFlags.Num(); i++)
		if (GrassTypeFlags[i])
		{
			GrassTypeBytes.Add(i);
			GrassTypeBytesString.Add(FString::FromInt(i));
		}

	UE_LOG(LogDV2, Display, TEXT("Grass types detected on texture %s: %s"), *Texture->GetName(), *FString::Join(GrassTypeBytesString, TEXT(", ")));
	
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	if (AssetEditorSubsystem)
		AssetEditorSubsystem->OpenEditorForAsset(Texture, EAssetTypeActivationOpenedMethod::View);
}
