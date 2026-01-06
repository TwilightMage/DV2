using UnrealBuildTool;

public class DV2UncookedOnly : ModuleRules
{
    public DV2UncookedOnly(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
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
                "BlueprintGraph",
                "KismetCompiler",
                "UnrealEd",
                "ToolMenus",
            }
        );
    }
}