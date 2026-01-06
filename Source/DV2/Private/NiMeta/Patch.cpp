#include "NiMeta/Patch.h"

#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "IDesktopPlatform.h"
#include "IImageWrapper.h"
#include "ImageWriteBlueprintLibrary.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "NetImmerse.h"
#include "TextUtils.h"
#include "NiMeta/CStreamableNode.h"

namespace NiMeta
{
	void PatchNifMeta()
	{
		PatchBasic([](basicType& entry)
		{
			entry.SetSizeString(0, 1);

			entry.ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
			{
				auto value = field.NumberAt<uint64>(i);
				return value > 0 ? TEXT("true") : TEXT("false");
			};
		}, "bit");

		PatchBasic([](basicType& entry)
		{
			entry.ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
			{
				auto value = field.ReferenceAt(i);
				auto block = value.Resolve(file);

				return block.IsValid()
					? FString::Printf(TEXT("%s [%d]"), *block->Type->name, value.Index)
					: FString("<none>");
			};
		}, "Ref", "Ptr");

		PatchBasic([](basicType& entry)
		{
			entry.ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
			{
				return FString::Printf(TEXT("%lld"), field.NumberAt<int64>(i));
			};
		}, "int64", "int", "short", "sbyte");

		PatchBasic([](basicType& entry)
		{
			entry.ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
			{
				return FString::Printf(TEXT("%f"), field.NumberAt<float>(i));
			};
		}, "float");

		PatchBasic([](basicType& entry)
		{
			entry.ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
			{
				auto value = field.NumberAt<bool>(i);
				return value ? TEXT("true") : TEXT("false");
			};
		}, "bool");

		PatchStruct([](structType& entry)
		{
			entry.ToString = [](const FNiFile& file, const FNiField& field, uint32 i) -> FString
			{
				const auto& value = *field.GroupAt(i);

				uint32 stringIndex = value.GetField("Index").SingleNumber<uint32>();

				if (stringIndex == (uint32)-1)
					return "";

				if (stringIndex >= (uint32)file.Strings.Num())
					return "[invalid index]";

				return file.Strings[stringIndex];
			};
		}, "string", "FilePath");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				float r = value["r"].SingleNumber<float>();
				float g = value["g"].SingleNumber<float>();
				float b = value["b"].SingleNumber<float>();

				float a;
				if (auto fld = value.FindField("b"))
					a = fld->SingleNumber<float>();
				else
					a = 1.0f;

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("#%s"), *FLinearColor(r, g, b, a).ToFColor(true).ToHex())))
					.ColorAndOpacity(FLinearColor(r, g, b, 1).ToFColor(true));
			};
		}, "Color3", "Color4");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				uint8 r = value["r"].SingleNumber<uint8>();
				uint8 g = value["g"].SingleNumber<uint8>();
				uint8 b = value["b"].SingleNumber<uint8>();

				uint8 a;
				if (auto fld = value.FindField("b"))
					a = fld->SingleNumber<uint8>();
				else
					a = 255;

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("#%s"), *FColor(r, g, b, a).ToHex())))
					.ColorAndOpacity(FColor(r, g, b, 255));
			};
		}, "ByteColor3", "ByteColor4");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				float u = value["u"].SingleNumber<float>();
				float v = value["v"].SingleNumber<float>();

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("U=%f V=%f"), u, v)));
			};
		}, "TexCoord", "HalfTexCoord");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				float x = value["X"].SingleNumber<float>();
				float y = value["Y"].SingleNumber<float>();
				float z = value["Z"].SingleNumber<float>();

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("X=%f Y=%f Z=%f"), x, y, z)));
			};
		}, "Vector3", "HalfVector3", "UshortVector3");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				float x = value["X"].SingleNumber<float>();
				float y = value["Y"].SingleNumber<float>();
				float z = value["Z"].SingleNumber<float>();
				float w = value["W"].SingleNumber<float>();

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("X=%f Y=%f Z=%f W=%f"), x, y, z, w)));
			};
		}, "Vector4");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				double x = value["X"].SingleNumber<double>();
				double y = value["Y"].SingleNumber<double>();
				double z = value["Z"].SingleNumber<double>();
				double w = value["W"].SingleNumber<double>();

				FRotator rotator = FRotator(FQuat(x, y, z, w));

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("P=%f Y=%f R=%f"), rotator.Pitch, rotator.Yaw, rotator.Roll)));
			};
		}, "Quaternion", "hkQuaternion");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				float m11 = value["m11"].SingleNumber<float>();
				float m21 = value["m21"].SingleNumber<float>();
				float m31 = value["m31"].SingleNumber<float>();
				float m12 = value["m12"].SingleNumber<float>();
				float m22 = value["m22"].SingleNumber<float>();
				float m32 = value["m32"].SingleNumber<float>();
				float m13 = value["m13"].SingleNumber<float>();
				float m23 = value["m23"].SingleNumber<float>();
				float m33 = value["m33"].SingleNumber<float>();

				FMatrix matrix = FMatrix(
					FVector(m11, m12, m13),
					FVector(m21, m22, m23),
					FVector(m31, m32, m33),
					FVector::ZeroVector
					);

				FRotator rotator = matrix.Rotator();

				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("P=%f Y=%f R=%f"), rotator.Pitch, rotator.Yaw, rotator.Roll)));
			};
		}, "Matrix33");

		PatchStruct([](structType& entry)
		{
			entry.GenerateSlateWidget = [](const FNiFile& file, const FNiField& field, uint32 i) -> TSharedRef<SWidget>
			{
				const auto& value = *field.GroupAt(i);

				uint16 v1 = value["v1"].SingleNumber<uint16>();
				uint16 v2 = value["v2"].SingleNumber<uint16>();
				uint16 v3 = value["v3"].SingleNumber<uint16>();
				return SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%d %d %d"), v1, v2, v3)));
			};
		}, "Triangle");

		PatchNiObject([](niobject& entry)
		{
			entry.BuildContextMenu = [](FMenuBuilder& MenuBuilder, const TSharedPtr<FNiFile>& File, const TSharedPtr<FNiBlock>& Block)
			{
#if WITH_EDITOR
				MenuBuilder.AddMenuEntry(
					MAKE_TEXT("NiPersistentSrcTextureRendererData", "Open texture"),
					MAKE_TEXT("NiPersistentSrcTextureRendererData", "Open texture in new UE tab"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([File, Block]()
					{
						auto Texture = UNetImmerse::LoadNiTexture(File->Path, Block->BlockIndex, true);
						if (!Texture)
							return;

						UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

						if (AssetEditorSubsystem)
						{
							AssetEditorSubsystem->OpenEditorForAsset(Texture);
						}
					}))
					);

				MenuBuilder.AddMenuEntry(
					MAKE_TEXT("NiPersistentSrcTextureRendererData", "Export texture"),
					MAKE_TEXT("NiPersistentSrcTextureRendererData", "Export texture to disk"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([File, Block]()
					{
						auto Texture = UNetImmerse::LoadNiTexture(File->Path, Block->BlockIndex, true);
						if (!Texture)
							return;

						IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
						if (!DesktopPlatform)
							return;

						TArray<FString> OutFilenames;
						FString FileTypes = TEXT("PNG Image (*.png)|*.png|JPEG Image (*.jpg)|*.jpg|EXR Image (*.exr)|*.exr|BMP Image (*.bmp)|*.bmp");

						if (!DesktopPlatform->SaveFileDialog(
							nullptr,
							TEXT("Export Texture"),
							TEXT(""),
							TEXT("Texture.png"),
							FileTypes,
							EFileDialogFlags::None,
							OutFilenames
							))
							return;

						if (OutFilenames.Num() != 1)
							return;

						FString SelectedExtension = FPaths::GetExtension(OutFilenames[0]);
						const TCHAR* SelectedExtensionChr = *SelectedExtension;

						FImageWriteOptions Options;
						Options.Format = (EDesiredImageFormat)StaticEnum<EDesiredImageFormat>()->GetValueByName(SelectedExtensionChr);
						Options.CompressionQuality = 100;
						Options.bOverwriteFile = true;
						Options.bAsync = true;

						UImageWriteBlueprintLibrary::ExportToDisk(Texture, OutFilenames[0], Options);
					}))
					);
#endif
			};
		}, "NiPersistentSrcTextureRendererData", "NiSourceTexture");

		PatchNiObject([](niobject& entry)
		{
			entry.CustomRead = [](TSharedPtr<FNiBlock>& Block, FMemoryReader& Reader, const FNiFile& File)
			{
				FCStreamableNode::ReadFrom(Block, Reader);
			};
		}, "xml::dom::CStreamableNode");
	}
}
