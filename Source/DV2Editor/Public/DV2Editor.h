#pragma once

#include "CoreMinimal.h"

struct FDV2AssetTreeEntry;
class FAssetContextMenuGenerator;
class FFileHandlerBase;
class FPinFactory;
class FDV2Browser;

class FDV2EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

	static FDV2EditorModule& Get();

	TSharedPtr<FFileHandlerBase> GetFileHandler(const FString& Extension) const;

	void RegisterAssetContextMenuGenerator(const TSharedPtr<FAssetContextMenuGenerator>& Generator) { AssetContextMenuGenerators.Add(Generator); }
	void UnregisterAssetContextMenuGenerator(const TSharedPtr<FAssetContextMenuGenerator>& Generator) { AssetContextMenuGenerators.Remove(Generator); }
	void GenerateContextmenuForAsset(FMenuBuilder& MenuBuilder, const TSharedPtr<FDV2AssetTreeEntry>& Asset);

	TSharedPtr<FUICommandList> Commands;
	TSharedPtr<FPinFactory> PinFactory;
	TSharedPtr<FDV2Browser> DV2Browser;
	FDelegateHandle DV2MenuExtenderHandle;

private:
	void RegisterMenus();

	void RegisterPropertyCustomizations();
	void UnregisterPropertyCustomizations();

	void RegisterFileHandlers();

	FDelegateHandle DV2AssetPathBlueprintEditorHandle;

	TMap<FString, TSharedPtr<FFileHandlerBase>> FileHandlers;
	TArray<TSharedPtr<FAssetContextMenuGenerator>> AssetContextMenuGenerators;
};
