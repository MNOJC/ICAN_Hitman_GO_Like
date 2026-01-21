// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GraphToolEditor : ModuleRules
{
	public GraphToolEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine", 
			"Blutility",
			"HitmanGO",
			"UMG",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",        
			"Slate",
			"SlateCore",
			"EditorSubsystem",
			"UMGEditor",  
			"LevelEditor",
			"EditorScriptingUtilities",
		});
	}
}

