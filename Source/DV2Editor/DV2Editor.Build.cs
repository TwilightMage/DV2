using UnrealBuildTool;

public class DV2Editor : ModuleRules
{
    public DV2Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "UnrealEd",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "DV2",
                "ToolMenus",
                "InputCore",
                "Projects",
                "GraphEditor",
                "BlueprintGraph",
            }
        );
    }
}