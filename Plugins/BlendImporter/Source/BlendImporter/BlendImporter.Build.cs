// Copyright 2022 nuclearfriend

using UnrealBuildTool;

public class BlendImporter : ModuleRules
{
	public BlendImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"SlateCore",
				"Slate",
				"UnrealEd",
				"Projects",
				"DesktopPlatform",
				"EditorStyle",
				"MessageLog"
				
				// ... add other public dependencies that you statically link with here ...
			}
		);
		
		// For SWarningOrErrorBox
		if (Target.Version.MajorVersion >= 5)
        {
            PrivateDependencyModuleNames.Add("ToolWidgets");
        }        

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",

				// ... add private dependencies that you statically link with here ...	
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
