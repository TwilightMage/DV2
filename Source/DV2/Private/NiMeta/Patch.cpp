#include "NiMeta/Patch.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "IDesktopPlatform.h"
#include "IImageWrapper.h"
#include "ImageUtils.h"
#include "ImageWriteBlueprintLibrary.h"
#include "ImageWriteQueue.h"
#include "ImageWriteTask.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "NetImmerse.h"
#include "TextUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
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
						auto Texture = UNetImmerse::LoadNiTexture(File->Path, Block->BlockIndex);
						if (!Texture)
							return;

						UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

						if (AssetEditorSubsystem)
							AssetEditorSubsystem->OpenEditorForAsset(Texture, EAssetTypeActivationOpenedMethod::View);
					}))
					);

				MenuBuilder.AddMenuEntry(
					MAKE_TEXT("NiPersistentSrcTextureRendererData", "Export texture"),
					MAKE_TEXT("NiPersistentSrcTextureRendererData", "Export texture to disk"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([File, Block]()
					{
						auto Texture = UNetImmerse::LoadNiTexture(File->Path, Block->BlockIndex);
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
							Block->BlockIndex == 0
								? FPaths::ChangeExtension(FPaths::GetPathLeaf(File->Path), "png")
								: TEXT("Texture.png"),
							FileTypes,
							EFileDialogFlags::None,
							OutFilenames
							))
							return;

						if (OutFilenames.Num() != 1)
							return;

						FString SelectedExtension = FPaths::GetExtension(OutFilenames[0]).ToLower();
						EImageFormat ImageFormat;
						if (SelectedExtension == "png")
							ImageFormat = EImageFormat::PNG;
						else if (SelectedExtension == "jpg")
							ImageFormat = EImageFormat::JPEG;
						else if (SelectedExtension == "exr")
							ImageFormat = EImageFormat::EXR;
						else if (SelectedExtension == "bmp")
							ImageFormat = EImageFormat::BMP;

						FIntPoint ImageSize = {Texture->GetSizeX(), Texture->GetSizeY()};
						TArray64<FColor> ImageData;

						if (Texture->GetPixelFormat() == EPixelFormat::PF_R8 ||
							Texture->GetPixelFormat() == EPixelFormat::PF_R8G8 ||
							Texture->GetPixelFormat() == EPixelFormat::PF_R8G8B8 ||
							Texture->GetPixelFormat() == EPixelFormat::PF_R8G8B8A8)
						{
							ImageData.SetNumZeroed(ImageSize.X * ImageSize.Y);
							auto TextureDataRaw = (const uint8*)Texture->GetPlatformData()->Mips[0].BulkData.LockReadOnly();
							if (Texture->GetPixelFormat() == EPixelFormat::PF_R8)
								for (int32 i = 0; i < ImageSize.X * ImageSize.Y; i++)
									ImageData[i] = FColor(TextureDataRaw[i], 0, 0, 255);
							else if (Texture->GetPixelFormat() == EPixelFormat::PF_R8G8)
								for (int32 i = 0; i < ImageSize.X * ImageSize.Y; i++)
									ImageData[i] = FColor(TextureDataRaw[i * 2 + 0], TextureDataRaw[i * 2 + 1], 0, 255);
							else if (Texture->GetPixelFormat() == EPixelFormat::PF_R8G8B8)
								for (int32 i = 0; i < ImageSize.X * ImageSize.Y; i++)
									ImageData[i] = FColor(TextureDataRaw[i * 3 + 0], TextureDataRaw[i * 3 + 1], TextureDataRaw[i * 3 + 2], 255);
							else if (Texture->GetPixelFormat() == EPixelFormat::PF_R8G8B8)
								for (int32 i = 0; i < ImageSize.X * ImageSize.Y; i++)
									ImageData[i] = FColor(TextureDataRaw[i * 4 + 0], TextureDataRaw[i * 4 + 1], TextureDataRaw[i * 4 + 2], TextureDataRaw[i * 4 + 3]);
							Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
						}
						else if (Texture->GetPixelFormat() == EPixelFormat::PF_DXT1 ||
							Texture->GetPixelFormat() == EPixelFormat::PF_DXT3 ||
							Texture->GetPixelFormat() == EPixelFormat::PF_DXT5)
						{
							UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(GWorld);
							RT->InitCustomFormat(ImageSize.X, ImageSize.Y, PF_B8G8R8A8, false);
							RT->SRGB = true;
							RT->MipGenSettings = TMGS_NoMipmaps;
							RT->UpdateResource();

							FCanvas Canvas(RT->GameThread_GetRenderTargetResource(), nullptr, FGameTime::CreateUndilated(0.0f, 0.0f), GWorld->GetFeatureLevel());
							Canvas.Clear(FLinearColor::Black);
							FCanvasTileItem TileItem(FVector2D(0, 0), Texture->GetResource(), FVector2D(ImageSize.X, ImageSize.Y), FLinearColor::White);
							TileItem.BlendMode = SE_BLEND_Opaque;
							Canvas.DrawItem(TileItem);
							Canvas.Flush_GameThread();

							TArray<FColor> OutPixels;
							FTextureRenderTargetResource* RTResource = RT->GameThread_GetRenderTargetResource();
							FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
							ReadPixelFlags.SetLinearToGamma(false);
							RTResource->ReadPixels(OutPixels, ReadPixelFlags);

							ImageData.Append(OutPixels);

							if (ImageData.Num() != ImageSize.X * ImageSize.Y)
								return;
						}

						auto Task = MakeUnique<FImageWriteTask>();
						Task->bOverwriteFile = true;
						Task->Filename = OutFilenames[0];
						Task->Format = ImageFormat;
						Task->CompressionQuality = 100;
						Task->PixelData = MakeUnique<TImagePixelData<FColor>>(ImageSize, MoveTemp(ImageData));

						IImageWriteQueueModule& WriteQueueModule = FModuleManager::Get().LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue");
						IImageWriteQueue& ImageWriteQueue = WriteQueueModule.GetWriteQueue();
						ImageWriteQueue.Enqueue(MoveTemp(Task));
					}))
					);
#endif
			};
		}, "NiPersistentSrcTextureRendererData", "NiSourceTexture");

		PatchNiObject([](niobject& entry)
		{
			entry.BuildContextMenu = [](FMenuBuilder& MenuBuilder, const TSharedPtr<FNiFile>& File, const TSharedPtr<FNiBlock>& Block)
			{
				MenuBuilder.AddMenuEntry(
						MAKE_TEXT("NiAVObject", "Export as model"),
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([File, Block]()
						{
							UNetImmerse::ExportSceneWithWizard(File->Path, Block->BlockIndex);
						}))
						);	
			};
		}, "NiAVObject");

		PatchNiObject([](niobject& entry)
		{
			entry.CustomRead = [](TSharedPtr<FNiBlock>& Block, FMemoryReader& Reader, const FNiFile& File)
			{
				FCStreamableNode::ReadFrom(Block, Reader);
			};
		}, "xml::dom::CStreamableNode");
	}
}
