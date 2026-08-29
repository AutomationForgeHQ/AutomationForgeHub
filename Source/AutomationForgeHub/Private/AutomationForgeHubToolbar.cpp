// Copyright Blackcode SA. All rights reserved.

#include "AutomationForgeHubToolbar.h"
#include "AutomationForgeHubSettings.h"
#include "AutomationForgeHubStyle.h"
#include "AutomationForgeUpdateCheck.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformProcess.h"
#include "ISettingsModule.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/IProjectManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "AutomationForgeHub"

TSet<FString> FAutomationForgeHubToolbar::Pending;

void FAutomationForgeHubToolbar::Register()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = Menu->AddSection("AutomationForgeHub", TAttribute<FText>(), FToolMenuInsert("Play", EToolMenuInsertType::After));

	FToolMenuEntry Entry = FToolMenuEntry::InitComboButton(
		"AutomationForgeHub",
		FUIAction(),
		FOnGetContent::CreateStatic(&FAutomationForgeHubToolbar::MakeMenu),
		TAttribute<FText>::CreateStatic(&FAutomationForgeHubToolbar::Label),
		TAttribute<FText>::CreateStatic(&FAutomationForgeHubToolbar::ToolTip),
		FSlateIcon(FAutomationForgeHubStyle::GetStyleSetName(), "AutomationForgeHub.Icon"));
	Entry.StyleNameOverride = "CalloutToolbar";
	Section.AddEntry(Entry);

	// The same menu under Tools, for people who never look at the toolbar.
	UToolMenu* Tools = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& ToolsSection = Tools->FindOrAddSection("AutomationForge", LOCTEXT("ToolsSection", "Automation Forge"));
	ToolsSection.AddSubMenu(
		"AutomationForgeHub",
		LOCTEXT("ToolsLabel", "Automation Forge"),
		LOCTEXT("ToolsTip", "What is installed, what has an update, and the hub."),
		FNewToolMenuChoice(FOnGetContent::CreateStatic(&FAutomationForgeHubToolbar::MakeMenu)),
		false,
		FSlateIcon(FAutomationForgeHubStyle::GetStyleSetName(), "AutomationForgeHub.Icon.Small"));
}

FText FAutomationForgeHubToolbar::Label()
{
	const int32 Updates = FAutomationForgeUpdateCheck::UpdateCount();
	return Updates > 0
		? FText::Format(LOCTEXT("LabelUpdates", "Automation Forge ({0})"), Updates)
		: LOCTEXT("Label", "Automation Forge");
}

FText FAutomationForgeHubToolbar::ToolTip()
{
	const int32 Updates = FAutomationForgeUpdateCheck::UpdateCount();
	if (Updates == 1) return LOCTEXT("TipOne", "Automation Forge — 1 update available. Open the hub to install it.");
	if (Updates > 1) return FText::Format(LOCTEXT("TipMany", "Automation Forge — {0} updates available. Open the hub to install them."), Updates);
	return LOCTEXT("Tip", "Automation Forge — what is installed, and the hub.");
}

TSharedRef<SWidget> FAutomationForgeHubToolbar::MakeMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	const FName AppStyle = FAppStyle::GetAppStyleSetName();

	MenuBuilder.BeginSection("Hub", LOCTEXT("HubSection", "Hub"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenHub", "Open the hub"),
		LOCTEXT("OpenHubTip", "The desktop hub: install sets, update, remove. Downloads it if it is not on this machine."),
		FSlateIcon(FAutomationForgeHubStyle::GetStyleSetName(), "AutomationForgeHub.Icon.Small"),
		FUIAction(FExecuteAction::CreateStatic(&FAutomationForgeHubToolbar::OpenHub)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Check", "Check for updates now"),
		LOCTEXT("CheckTip", "Read the public manifest again and compare it with what is installed."),
		FSlateIcon(AppStyle, "Icons.Refresh"),
		FUIAction(
			FExecuteAction::CreateLambda([]() { FAutomationForgeUpdateCheck::Check(/*bNotify*/ true); }),
			FCanExecuteAction::CreateLambda([]() { return !FAutomationForgeUpdateCheck::IsChecking(); })));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Prefs", "Preferences"),
		LOCTEXT("PrefsTip", "Channel, notifications, where the manifest and the hub are."),
		FSlateIcon(AppStyle, "Icons.Settings"),
		FUIAction(FExecuteAction::CreateStatic(&FAutomationForgeHubToolbar::OpenSettings)));
	MenuBuilder.EndSection();

	const TArray<FAutomationForgePluginState>& Plugins = FAutomationForgeUpdateCheck::Plugins();
	MenuBuilder.BeginSection("Installed", LOCTEXT("InstalledSection", "Installed — ticked plugins are enabled for this project"));
	if (Plugins.Num() == 0)
	{
		MenuBuilder.AddMenuEntry(
			FAutomationForgeUpdateCheck::HasChecked()
				? LOCTEXT("NoneFound", "No Automation Forge plugins found")
				: LOCTEXT("NotChecked", "Not checked yet"),
			FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
	}
	for (const FAutomationForgePluginState& S : Plugins)
	{
		const FString Name = S.Name;
		const FText Label = S.bUpdate
			? FText::Format(LOCTEXT("PluginUpdate", "{0}    {1}  →  {2}"), FText::FromString(S.Name), FText::FromString(S.Installed), FText::FromString(S.Latest))
			: FText::Format(LOCTEXT("Plugin", "{0}    {1}"), FText::FromString(S.Name), FText::FromString(S.Installed));
		const FText Tip = FText::Format(
			LOCTEXT("PluginTip", "{0}\n{1}\n\nTick to enable in this project, untick to disable — the editor applies it on restart. Updates are installed by the hub."),
			FText::FromString(S.FriendlyName), S.bEngine ? LOCTEXT("InEngine", "Installed in the engine") : LOCTEXT("InProject", "Installed in this project"));
		MenuBuilder.AddMenuEntry(
			Label, Tip,
			S.bUpdate ? FSlateIcon(AppStyle, "Icons.Import") : FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FAutomationForgeHubToolbar::ToggleForProject, Name),
				FCanExecuteAction(),
				FIsActionChecked::CreateStatic(&FAutomationForgeHubToolbar::IsEnabledForProject, Name)),
			NAME_None, EUserInterfaceActionType::ToggleButton);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Status");
	FText Status;
	if (FAutomationForgeUpdateCheck::IsChecking()) Status = LOCTEXT("Checking", "Checking…");
	else if (!FAutomationForgeUpdateCheck::LastError().IsEmpty()) Status = FText::Format(LOCTEXT("CheckFailed", "Could not read the manifest ({0})"), FText::FromString(FAutomationForgeUpdateCheck::LastError()));
	else if (!FAutomationForgeUpdateCheck::HasChecked()) Status = LOCTEXT("NotCheckedYet", "Updates not checked yet");
	else if (FAutomationForgeUpdateCheck::UpdateCount() == 0) Status = LOCTEXT("Current", "Everything is current");
	else Status = FText::Format(LOCTEXT("UpdatesReady", "{0} update(s) — open the hub to install"), FAutomationForgeUpdateCheck::UpdateCount());
	MenuBuilder.AddMenuEntry(Status, FText::GetEmpty(), FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FAutomationForgeHubToolbar::OpenHub),
			FCanExecuteAction::CreateLambda([]() { return FAutomationForgeUpdateCheck::UpdateCount() > 0; })));
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

bool FAutomationForgeHubToolbar::IsEnabledForProject(FString Name)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(Name);
	const bool bNow = Plugin.IsValid() && Plugin->IsEnabled();
	return Pending.Contains(Name) ? !bNow : bNow;
}

void FAutomationForgeHubToolbar::ToggleForProject(FString Name)
{
	const bool bEnable = !IsEnabledForProject(Name);
	FText Fail;
	if (!IProjectManager::Get().SetPluginEnabled(Name, bEnable, Fail) || !IProjectManager::Get().SaveCurrentProjectToDisk(Fail))
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("ToggleFailed", "Could not change {0}: {1}"), FText::FromString(Name), Fail));
		Info.ExpireDuration = 6.f;
		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		return;
	}
	if (Pending.Contains(Name)) Pending.Remove(Name); else Pending.Add(Name);

	FNotificationInfo Info(FText::Format(
		bEnable ? LOCTEXT("Enabled", "{0} enabled for this project. Restart the editor to load it.")
		        : LOCTEXT("Disabled", "{0} disabled for this project. Restart the editor to unload it."),
		FText::FromString(Name)));
	Info.ExpireDuration = 6.f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

FString FAutomationForgeHubToolbar::FindHubExecutable()
{
	const UAutomationForgeHubSettings* Settings = GetDefault<UAutomationForgeHubSettings>();
	if (!Settings->HubExecutable.FilePath.IsEmpty() && FPaths::FileExists(Settings->HubExecutable.FilePath))
	{
		return Settings->HubExecutable.FilePath;
	}
#if PLATFORM_WINDOWS
	const FString LocalAppData = FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
	const FString Installed = LocalAppData / TEXT("Programs/Automation Forge/AutomationForgeHub.exe");
	if (FPaths::FileExists(Installed)) return Installed;
#endif
	return FString();
}

void FAutomationForgeHubToolbar::OpenHub()
{
	const FString Exe = FindHubExecutable();
	if (Exe.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*GetDefault<UAutomationForgeHubSettings>()->DownloadUrl, nullptr, nullptr);
		return;
	}
	// Tell the hub which engine this editor runs from, so it opens on that one.
	// A running hub raises its window when a second copy starts; a first copy just starts.
	const FString EngineRoot = FPaths::ConvertRelativePathToFull(FPaths::RootDir());
	const FString Args = FString::Printf(TEXT("--engine \"%s\""), *EngineRoot.TrimChar(TEXT('/')));
	FPlatformProcess::CreateProc(*Exe, *Args, /*bLaunchDetached*/ true, /*bLaunchHidden*/ false, /*bLaunchReallyHidden*/ false, nullptr, 0, nullptr, nullptr);
}

void FAutomationForgeHubToolbar::OpenSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer("Editor", "Automation Forge", "Hub");
	}
}

#undef LOCTEXT_NAMESPACE
