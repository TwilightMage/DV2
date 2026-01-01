#include "DV2Editor.h"

#include "DV2Browser.h"
#include "DV2EditorCommands.h"
#include "DV2Style.h"
#include "NiMeta/NiMeta.h"
#include "DV2Ghost.h"
#include "Divinity2Assets.h"
#include "FDV2GhostCustomization.h"

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
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder) {
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
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
	PropertyModule.RegisterCustomClassLayout(
		ADV2Ghost::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FDV2GhostCustomization::MakeInstance)
	);
}

void FDV2EditorModule::UnregisterPropertyCustomizations()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(ADV2Ghost::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FDV2EditorModule, DV2Editor)