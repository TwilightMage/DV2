#include "DV2EditorCommands.h"

#define LOCTEXT_NAMESPACE "FDV2EditorCommands"

void FDV2EditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenDV2Browser, "DV2 Browser", "Bring up DV2 Browser", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ReloadDV2Assets, "Reload DV2 Assets", "Reload all divinity 2 assets", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ReloadNifMetadata, "Reload NIF Metadata", "Reload all metadata from provided xml files", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ReloadCStreamableAliases, "Reload CStreamable Aliases", "Reload aliases from ini file", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE