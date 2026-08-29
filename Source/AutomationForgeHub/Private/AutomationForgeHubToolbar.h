// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The combo button after Play. Its menu is the editor's view of the product:
 * every Automation Forge plugin on this machine with its version, a newer
 * version where one exists, a checkbox per plugin for this project, the hub,
 * and the preferences.
 */
class FAutomationForgeHubToolbar
{
public:
	static void Register();

	/** Start the installed hub, or send the person to the download when there is none. */
	static void OpenHub();
	static void OpenSettings();

private:
	static TSharedRef<SWidget> MakeMenu();
	static FText Label();
	static FText ToolTip();
	// By value: delegate payloads are copied in, and CreateStatic wants the parameter type to match.
	static bool IsEnabledForProject(FString Name);
	static void ToggleForProject(FString Name);
	static FString FindHubExecutable();

	/** Plugins toggled this session; the editor's own state does not change until restart. */
	static TSet<FString> Pending;
};
