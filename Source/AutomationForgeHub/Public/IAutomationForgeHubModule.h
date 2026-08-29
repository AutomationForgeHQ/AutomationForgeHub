// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * How another plugin reaches the hub without depending on it.
 *
 * Same contract as `IForgeKeysModule`, and for the same reason: the hub is a convenience, never a
 * requirement. A plugin installed on its own from Fab or GitHub has no hub, and everything it does
 * must still work - so a caller includes this header, adds `PrivateIncludePathModuleNames`, and
 * writes `if (IAutomationForgeHubModule* Hub = IAutomationForgeHubModule::GetOrLoad())`. No link,
 * nothing in the `.uplugin`, and the null case is the normal case rather than an error.
 *
 * What lives here is only what another plugin has a reason to ask for. The toolbar, the update
 * check and the entitlement bridge stay private - they are the hub's own business.
 */
class IAutomationForgeHubModule : public IModuleInterface
{
public:

	/** Null when the hub plugin is not installed, which is not a fault. */
	static IAutomationForgeHubModule* GetOrLoad()
	{
		if (!FModuleManager::Get().ModuleExists(TEXT("AutomationForgeHub")))
		{
			return nullptr;
		}
		return FModuleManager::Get().LoadModulePtr<IAutomationForgeHubModule>(TEXT("AutomationForgeHub"));
	}

	/** As above, but never loads. For teardown, and for anything on a hot path. */
	static IAutomationForgeHubModule* GetIfLoaded()
	{
		return FModuleManager::GetModulePtr<IAutomationForgeHubModule>(TEXT("AutomationForgeHub"));
	}

	/**
	 * Bring the desktop hub up, on the engine this editor runs from.
	 *
	 * Falls back to the download page when the hub application is not on this machine - the plugin
	 * being installed and the application being installed are different things, and a button that
	 * did nothing in that case would be the worse answer.
	 */
	virtual void OpenHub() = 0;

	/**
	 * True when the hub **application** is installed on this machine.
	 *
	 * Distinct from this module existing. Ask before telling somebody the hub can do a thing for
	 * them, because otherwise the advice sends them somewhere that is not there yet.
	 */
	virtual bool IsHubApplicationInstalled() const = 0;
};
