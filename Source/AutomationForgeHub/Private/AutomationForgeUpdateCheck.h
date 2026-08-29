// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"

/** One Automation Forge plugin on this machine, as the check last saw it. */
struct FAutomationForgePluginState
{
	FString Name;
	FString FriendlyName;
	FString Installed;   // the descriptor's VersionName
	FString Latest;      // newest on the chosen channel for this engine, empty if unknown
	bool bUpdate = false;
	bool bEngine = false; // under Engine/Plugins rather than the project
};

/**
 * Reads the public manifest and compares it with the Automation Forge plugins
 * the engine discovered — anything under a Plugins/AutomationForge folder, or
 * named in the manifest. Says something once per session when a newer version
 * exists. Installs nothing.
 */
class FAutomationForgeUpdateCheck
{
public:
	static void Check(bool bNotify);
	static const TArray<FAutomationForgePluginState>& Plugins();
	static int32 UpdateCount();
	static bool HasChecked();
	static bool IsChecking();
	static const FString& LastError();

	/** True when the plugin is ours: it lives in an AutomationForge folder or the manifest names it. */
	static bool IsOurs(const class IPlugin& Plugin);

private:
	static void OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk, bool bNotify);
	static void Rescan();
	static void Notify();

	static TArray<FAutomationForgePluginState> States;
	static TMap<FString, FString> LatestByName;  // from the manifest, for this engine and channel
	static TSet<FString> ManifestNames;
	static bool bChecked;
	static bool bChecking;
	static bool bNotified;
	static FString Error;
};
