// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DV2 : ModuleRules
{
	public DV2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Landscape",
				"ProceduralMeshComponent",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"DeveloperSettings",
				"XmlParser",
			}
		);

		if (Target.Type == TargetRules.TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"ImageWriteQueue",
					"ImageWrapper",
					"Projects"
				}
			);
		}

		AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");
	}
}