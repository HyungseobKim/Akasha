// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
public class Pathfinding : ModuleRules
{
	public Pathfinding(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
    {
	    "AIModule",
      "Core", 
      "CoreUObject", 
      "Engine",
      "GridMap"
    });
		PrivateDependencyModuleNames.AddRange(new string[] {  });
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
	}
}
