// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CityLiveRP : ModuleRules
{
	public CityLiveRP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CityLiveRP",
			"CityLiveRP/Variant_Platforming",
			"CityLiveRP/Variant_Platforming/Animation",
			"CityLiveRP/Variant_Combat",
			"CityLiveRP/Variant_Combat/AI",
			"CityLiveRP/Variant_Combat/Animation",
			"CityLiveRP/Variant_Combat/Gameplay",
			"CityLiveRP/Variant_Combat/Interfaces",
			"CityLiveRP/Variant_Combat/UI",
			"CityLiveRP/Variant_SideScrolling",
			"CityLiveRP/Variant_SideScrolling/AI",
			"CityLiveRP/Variant_SideScrolling/Gameplay",
			"CityLiveRP/Variant_SideScrolling/Interfaces",
			"CityLiveRP/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
