// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SnowSimulation : ModuleRules
{
	public SnowSimulation(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "Landscape", "RHI" });

		PrivateDependencyModuleNames.AddRange(new string[] { "WorldClimData", "SnowSimulation" });
        // DynamicallyLoadedModuleNames.AddRange(new string[] { "UObjectPlugin" });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");
        // if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        // {
        //		if (UEBuildConfiguration.bCompileSteamOSS == true)
        //		{
        //			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
        //		}
        // }
    }
}
