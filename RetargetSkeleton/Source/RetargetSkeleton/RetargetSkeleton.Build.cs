// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RetargetSkeleton : ModuleRules
{
	public RetargetSkeleton(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {

			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",

				// ... add private dependencies that you statically link with here ...	
				"AssetTools",
                "EditorStyle",
				"AppFramework",
                "Persona",
                "AnimationEditor",
                "Kismet",
                "ContentBrowser",
				"AnimationModifiers",
				"DesktopPlatform",
				"Json","JsonUtilities",
				"AnimationBlueprintLibrary",
				"ContentBrowserData",
				"AssetTools",
				"AnimGraph",
				"Persona",
				"IKRig",
				"PropertyEditor",
				"EditorScriptingUtilities"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
