#include "DV2Style.h"

#include "NetImmerse.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

const FSlateBrush* DefaultBrush = FAppStyle::Get().GetBrush("DefaultBrush");

const FString PluginName = TEXT("DV2");

void FDV2Style::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(*PluginName);
		FString ResourcesDir = Plugin->GetBaseDir() / TEXT("Resources");

		StyleInstance->SetContentRoot(ResourcesDir);

		TArray<FString> FileNames;
		FString NifIconsDir = ResourcesDir + "/Icons/NifBlocks";
		IFileManager::Get().FindFiles(FileNames, *NifIconsDir, TEXT("*.png"));

		for (const auto& FileName : FileNames)
		{
			FString IconBasename = FPaths::GetBaseFilename(FileName);
			FString IconName = "NifBlockIcon." + IconBasename;
			StyleInstance->Set(*IconName, new FSlateImageBrush(StyleInstance->RootToContentDir(NifIconsDir + "/" + IconBasename, TEXT(".png")), FVector2D(28.0f, 28.0f)));
		}

		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FDV2Style::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

const ISlateStyle& FDV2Style::Get()
{
	return *StyleInstance;
}

FName FDV2Style::GetStyleSetName()
{
	static FName StyleSetName(TEXT("DV2Style"));
	return StyleSetName;
}

bool FDV2Style::FindNifBlockIcon(const TSharedPtr<NiMeta::niobject>& BlockType, const FSlateBrush*& OutIconBrush)
{
	TSharedPtr<NiMeta::niobject> Ty = BlockType;
	while ((!OutIconBrush || OutIconBrush == DefaultBrush) && Ty.IsValid())
	{
		if (!Ty->icon.IsEmpty())
		{
			OutIconBrush = StyleInstance->GetBrush(*Ty->icon);
			return OutIconBrush != DefaultBrush;
		}

		FString IconName = Ty->name;
		for (TCHAR* chr = IconName.GetCharArray().GetData(); *chr; chr++)
			if (*chr == '\\' ||
				*chr == '/' ||
				*chr == ':' ||
				*chr == '*' ||
				*chr == '?' ||
				*chr == '\"' ||
				*chr == '<' ||
				*chr == '>' ||
				*chr == '|'
			)
				*chr = '_';

		IconName = "NifBlockIcon." + IconName;

		OutIconBrush = StyleInstance->GetOptionalBrush(*IconName, nullptr, DefaultBrush);
		Ty = Ty->inherit;
	}

	return OutIconBrush != DefaultBrush;
}
