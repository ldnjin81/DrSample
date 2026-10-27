using UnrealBuildTool;
using System.IO;

public class DrSample : ModuleRules
{
	public DrSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 자신의 폴더를 인클루드 루트로 취급해 헤더를 Source/DrSample 기준
		// 상대경로로 인클루드할 수 있게 한다(예: "Characters/DsCharacter.h").
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
            "AudioMixer",
            "Niagara",
            "AudioModulation",
			"AnimGraphRuntime",
            "EngineCameras",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"UMG",
            "CommonUI",
            "CommonInput",
            "CinematicCamera",
            "Json",
			"ModelViewViewModel",
			"FieldNotification",
			"DrTableRuntime", "DrFramework", "DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
		PrivateDependencyModuleNames.Add("NavigationSystem");
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AnimGraph", "BlueprintGraph", "KismetCompiler" });
            PrivateDependencyModuleNames.Add("AssetRegistry");
		}
	}
}
