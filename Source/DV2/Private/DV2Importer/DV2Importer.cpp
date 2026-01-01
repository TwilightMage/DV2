#include "DV2Importer/DV2Importer.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "DV2Importer/DV2Settings.h"
#include "DV2Importer/unpack.h"

bool UDV2Importer::Unpack(const FString& dv2, const FString& dir)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*dv2))
		return false;
	
	return unpackDv2(TCHAR_TO_UTF8(*dv2), TCHAR_TO_UTF8(*dir)) == 0;
}

bool UDV2Importer::UnpackWithWizard()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> inFiles;
		if (DesktopPlatform->OpenFileDialog(nullptr, TEXT("Select .dv2 files"), GetDefault<UDV2Settings>()->Divinity2AssetLocation.Path, TEXT(""), TEXT("DV2 Files|*.dv2"), EFileDialogFlags::Multiple, inFiles))
		{
			for (const FString& file : inFiles)
			{
				if (!Unpack(file, GetDefault<UDV2Settings>()->ExportLocation.Path))
				{
					return false;
				}
			}

			return true;
		}
	}

	return false;
}
