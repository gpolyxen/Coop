// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MyProject : ModuleRules
{
	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "GameplayTasks", "NavigationSystem", "Niagara", "UMG", "Slate", "SlateCore", "Sockets", "Networking", "OnlineSubsystem", "OnlineSubsystemUtils", "ProceduralMeshComponent" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		if (Target.bBuildEditor)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "FBX");
		}

		DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
	}
}
