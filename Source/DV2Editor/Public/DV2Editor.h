#pragma once

#include "CoreMinimal.h"

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
};
