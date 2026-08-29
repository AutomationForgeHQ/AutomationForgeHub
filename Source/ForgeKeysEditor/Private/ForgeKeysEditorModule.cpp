// Copyright Blackcode SA. All rights reserved.

#include "SForgeKeysPanel.h"

#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "ForgeKeys"

namespace ForgeKeysEditor
{
	static const FName KeysTabName("ForgeKeys");
}

/**
 * The editor half: one panel, and one way to open it.
 *
 * It contributes nothing to any plugin's own settings page — those keep working on their own, so a
 * plugin installed alone still has a way to set its key. This is the aggregate view.
 */
class FForgeKeysEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(ForgeKeysEditor::KeysTabName,
				FOnSpawnTab::CreateStatic(&FForgeKeysEditorModule::SpawnTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Automation Forge Keys"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Every key the installed Automation Forge plugins ask for."))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Lock"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

		ToolMenusHandle = UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateStatic(&FForgeKeysEditorModule::RegisterMenu));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus* Menus = UToolMenus::TryGet())
		{
			UToolMenus::UnRegisterStartupCallback(ToolMenusHandle);
			Menus->UnregisterOwner(this);
		}
		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ForgeKeysEditor::KeysTabName);
		}
	}

private:
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SForgeKeysPanel)
			];
	}

	static void RegisterMenu()
	{
		UToolMenu* Tools = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		// The same section name the hub plugin uses, so the family groups together whichever of the
		// two is installed.
		FToolMenuSection& Section = Tools->FindOrAddSection("AutomationForge",
			LOCTEXT("Section", "Automation Forge"));

		Section.AddMenuEntry(
			"ForgeKeys",
			LOCTEXT("MenuLabel", "Keys"),
			LOCTEXT("MenuTip", "Set, replace and test the API keys the installed plugins ask for."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Lock"),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(ForgeKeysEditor::KeysTabName);
			})));
	}

	FDelegateHandle ToolMenusHandle;
};

IMPLEMENT_MODULE(FForgeKeysEditorModule, ForgeKeysEditor)

#undef LOCTEXT_NAMESPACE
