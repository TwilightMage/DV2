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
                "DV2",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "InputCore",
                "Projects",
                "GraphEditor",
                "BlueprintGraph",
                "ToolWidgets",
                "ProceduralMeshComponent",
                "ImageWrapper",
            }
        );
    }
}