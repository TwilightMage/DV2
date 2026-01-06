#include "DV2Editor.h"

#include "BlueprintEditorModule.h"
#include "DV2Browser.h"
#include "DV2EditorCommands.h"
#include "DV2Style.h"
#include "Divinity2Assets.h"
#include "FDV2AssetPathCustomization.h"
#include "PinFactory.h"
#include "NiMeta/NiMeta.h"

#define LOCTEXT_NAMESPACE "FDV2EditorModule"

void FDV2EditorModule::StartupModule()
{
	FDV2EditorCommands::Register();

	Commands = MakeShareable(new FUICommandList);
	Commands->MapAction(
		FDV2EditorCommands::Get().OpenDV2Browser,
		FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FDV2Browser::TabName);
		}),
		FCanExecuteAction());
	Commands->MapAction(
		FDV2EditorCommands::Get().ReloadDV2Assets,
		FExecuteAction::CreateLambda([]()
		{
			UDivinity2Assets::ReloadAssets();
		}),
		FCanExecuteAction());
	Commands->MapAction(
		FDV2EditorCommands::Get().ReloadNifMetadata,
		FExecuteAction::CreateLambda([]()
		{
			NiMeta::Reload();
		}),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDV2EditorModule::RegisterMenus));

	PinFactory = MakeShared<FPinFactory>();
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);

	DV2Browser = MakeShared<FDV2Browser>();
	DV2Browser->Init();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FDV2Browser::TabName, FOnSpawnTab::CreateSP(DV2Browser.ToSharedRef(), &FDV2Browser::Open))
	                        .SetDisplayName(NSLOCTEXT("FDV2UEEditorModule", "DV2 Browser", "DV2 Browser"))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);

	NiMeta::Reload();

	FDV2Style::Initialize();

	RegisterPropertyCustomizations();
}

void FDV2EditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);

	FDV2EditorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FDV2Browser::TabName);

	FDV2Style::Shutdown();

	UnregisterPropertyCustomizations();
}

FDV2EditorModule& FDV2EditorModule::Get()
{
	return FModuleManager::GetModuleChecked<FDV2EditorModule>("DV2Editor");
}

void FDV2EditorModule::RegisterMenus()
{
	UToolMenus* ToolMenus = UToolMenus::Get();

	auto AddDV2Section = [this](UToolMenu* InMenu)
	{
		FToolMenuSection& Section = InMenu->AddSection("DV2Section", INVTEXT("Divinity 2"));
		Section.AddSubMenu(
			"DV2Category",
			INVTEXT("Divinity 2"),
			INVTEXT("Divinity 2 tools"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.PushCommandList(Commands.ToSharedRef());
				MenuBuilder.AddMenuEntry(FDV2EditorCommands::Get().OpenDV2Browser);
				MenuBuilder.AddMenuEntry(FDV2EditorCommands::Get().ReloadDV2Assets);
				MenuBuilder.AddMenuEntry(FDV2EditorCommands::Get().ReloadNifMetadata);
				MenuBuilder.PopCommandList();
			})
			);
	};

	UToolMenu* LevelEditorMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu");
	AddDV2Section(LevelEditorMenu);

	UToolMenu* TabMenu = ToolMenus->RegisterMenu(FDV2Browser::MenuName, "MainFrame.MainMenu");
	AddDV2Section(TabMenu);
}

void FDV2EditorModule::RegisterPropertyCustomizations()
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FBlueprintEditorModule& Kismet = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");

	PropertyEditor.RegisterCustomPropertyTypeLayout(
		FStrProperty::StaticClass()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDV2AssetPathCustomization::MakeInstance)
		);

	auto DV2AssetPathBlueprintEditor = FOnGetVariableCustomizationInstance::CreateStatic(&FDV2AssetPathBlueprintEditor::MakeInstance);
	DV2AssetPathBlueprintEditorHandle = DV2AssetPathBlueprintEditor.GetHandle();
	Kismet.RegisterVariableCustomization(
		FStrProperty::StaticClass(),
		DV2AssetPathBlueprintEditor
		);
}

void FDV2EditorModule::UnregisterPropertyCustomizations()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FBlueprintEditorModule& Kismet = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");

		PropertyEditor.UnregisterCustomPropertyTypeLayout(FStrProperty::StaticClass()->GetFName());
		Kismet.UnregisterVariableCustomization(FStrProperty::StaticClass(), DV2AssetPathBlueprintEditorHandle);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDV2EditorModule, DV2Editor)
