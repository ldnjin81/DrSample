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
			"DrTableRuntime", "DrFramework", "DrUIKit", "DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
		PrivateDependencyModuleNames.Add("NavigationSystem");

		// DrUIKit 스타일 플러그인(토큰만 있는 것)은 코드 모듈이 없어 스스로 파일을 패키지에 넣지 못한다.
		// 실행 중에 읽는 Tokens·Images·Glyphs 폴더를 샘플 모듈이 대신 넣는다(DrUIGlass는 자기 Build.cs가 넣는다).
		string UISample = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "Plugins", "DrUIKit", "UISample"));
		if (Directory.Exists(UISample))
		{
			foreach (string Group in Directory.GetDirectories(UISample))
			foreach (string Plugin in Directory.GetDirectories(Group))
			foreach (string Folder in new string[] { "Tokens", "Images", "Glyphs" })
			{
				if (Directory.Exists(Path.Combine(Plugin, Folder))) RuntimeDependencies.Add(Path.Combine(Plugin, Folder, "..."));
			}
		}
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AnimGraph", "BlueprintGraph", "KismetCompiler", "RenderCore", "RHI" });
            PrivateDependencyModuleNames.Add("AssetRegistry");
		}
	}
}
