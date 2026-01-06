#pragma once

#include "CoreMinimal.h"

class FPinFactory;
class FDV2Browser;

class FDV2EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

	static FDV2EditorModule& Get();

	TSharedPtr<FUICommandList> Commands;
	TSharedPtr<FPinFactory> PinFactory;
	TSharedPtr<FDV2Browser> DV2Browser;
	FDelegateHandle DV2MenuExtenderHandle;

private:
	void RegisterMenus();

	void RegisterPropertyCustomizations();
	void UnregisterPropertyCustomizations();

	FDelegateHandle DV2AssetPathBlueprintEditorHandle;
};
