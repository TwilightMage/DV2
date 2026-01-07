#pragma once

class FDV2EditorCommands : public TCommands<FDV2EditorCommands>
{
public:
	FDV2EditorCommands()
		: TCommands<FDV2EditorCommands>(TEXT("DV2Editor"), NSLOCTEXT("Contexts", "DV2Editor", "DV2Editor Plugin"),
		                                NAME_None, FCoreStyle::Get().GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> OpenDV2Browser;
	TSharedPtr<FUICommandInfo> ReloadDV2Assets;
	TSharedPtr<FUICommandInfo> ReloadNifMetadata;
	TSharedPtr<FUICommandInfo> ReloadCStreamableAliases;
};
