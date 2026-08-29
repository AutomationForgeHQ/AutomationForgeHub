using UnrealBuildTool;

public class ForgeKeysEditor : ModuleRules
{
	public ForgeKeysEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"ForgeKeys",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"ToolMenus",     // the entry under Tools > Automation Forge
				"UnrealEd",
				"WorkspaceMenuStructure",
			}
			);
	}
}
