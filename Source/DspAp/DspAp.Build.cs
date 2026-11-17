// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DspAp : ModuleRules
{
	public DspAp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange
			(new string[] 
			{
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput",
                "AudioExtensions",
                "MetasoundEngine",
                "MetasoundGraphCore",
                "MetasoundFrontend"
            });
	}
}
