using UnrealBuildTool;

public class AutomationForgeHub : ModuleRules
{
	public AutomationForgeHub(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings", // the Editor Preferences page
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"ToolMenus",        // the combo button after Play
				"Projects",         // IPluginManager, IProjectManager — what is installed, enable/disable
				"HTTP",             // the manifest
				"Json",
				"Settings",         // ISettingsModule::ShowViewer
				"PluginWarden",     // the Fab entitlement check, for the paid plugins to call
			}
			);
	}
}
