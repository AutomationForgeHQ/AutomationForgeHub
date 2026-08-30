// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Called when a key test finishes. Tests are network calls, so nothing about this is synchronous. */
using FForgeKeyTestResult = TFunction<void(bool /*bOk*/, FText /*Message*/)>;

/**
 * A gated resource this key must *also* be granted access to.
 *
 * **A token is not access, and saying only the first half is how people lose an afternoon.** A valid
 * Hugging Face token whose account has not accepted a model's terms installs perfectly and then fails
 * at the point of use, nowhere near the key page that looked correct.
 *
 * One per consumer that needs one, accumulated by the registry, because a shared key can front two
 * completely different grants: Meta review Llama 3 by hand and take days, NVIDIA's terms are a click.
 */
struct FForgeKeyAccessRequirement
{
	/** What must be granted, and for what: "Llama 3, for Kimodo's text encoder". */
	FText What;

	/** Where to ask. Rendered as a link. */
	FString Url;

	/** True where a human reviews the request, so it is worth starting early rather than at the end. */
	bool bReviewedByHand = false;
};

/**
 * One key a plugin asks for, described by the plugin that owns it.
 *
 * **Pure data and closures, on purpose.** A plugin fills this in and hands it over; it never calls
 * into ForgeKeys for anything else, and ForgeKeys never calls back into the plugin except through
 * the functions below. That is what lets a plugin register *without linking against ForgeKeys at
 * all* — see IForgeKeysModule — so a plugin installed on its own keeps its own key store, its own
 * settings page, and works exactly as if this plugin did not exist.
 *
 * The plugin supplies the operations rather than ForgeKeys implementing them, so there is one vault
 * implementation per plugin and no third copy here.
 */
struct FForgeKeyProvider
{
	/**
	 * Unique across the registry. Conventionally "<Plugin>.<Service>".
	 *
	 * A **shared** key drops the plugin half and is just the service - "Runpod", "HuggingFace" - so
	 * that every plugin wanting it registers the same id and the page shows one row rather than one
	 * per plugin.
	 */
	FName Id;

	/** What to call it: "Uthana", "ElevenLabs". */
	FText DisplayName;

	/** Which plugin wants it. Groups the page. Forced to "General" for a shared key. */
	FText Owner;

	/**
	 * True when this key is an account a *person* has rather than something one plugin owns.
	 *
	 * A Runpod key or a Hugging Face token is the same key whichever plugin asks, so asking twice is
	 * asking the same question twice and leaves two copies to keep in step. A shared key is registered
	 * under a plugin-free id, stored in one family-wide vault entry, and shown once under **General**
	 * with the plugins that use it named beneath.
	 */
	bool bShared = false;

	/**
	 * Which installed plugins consume this key.
	 *
	 * Accumulated by the registry rather than declared by any one plugin: each plugin adds itself as
	 * it registers, so the list describes what is actually installed rather than what somebody
	 * remembered to write down.
	 */
	TArray<FText> Consumers;

	/** One line: what this key unlocks, and what happens without it. */
	FText Purpose;

	/**
	 * True when the plugin works without it — a local runner that needs no token, a provider whose
	 * key only matters in one mode. An unset optional key is a fact, not a problem, and the page
	 * says so rather than showing it like something broken.
	 */
	bool bOptional = false;

	/** Where a person gets one. Rendered as a link; may be empty. */
	FString HelpUrl;

	/**
	 * What this key must additionally be *granted access to*, if anything.
	 *
	 * Accumulated across consumers by the registry rather than owned by one plugin, for the same
	 * reason Consumers is: with two plugins installed, both grants are needed and neither plugin
	 * knows about the other. See FForgeKeyAccessRequirement.
	 */
	TArray<FForgeKeyAccessRequirement> AccessRequirements;

	/** Shown so a person can find the entry themselves. Display only. */
	FString VaultEntryName;
	FString EnvironmentVariableName;

	/** Whether a key is available right now. */
	TFunction<bool()> IsSet;

	/** One line for the UI: configured, and from where. */
	TFunction<FString()> Describe;

	/** Store or replace. Returns false when the platform has nowhere to put it. */
	TFunction<bool(const FString& /*Secret*/)> Store;

	/** Forget the stored key. An environment variable, if set, still applies. */
	TFunction<bool()> Clear;

	/**
	 * Ask the provider whether the key works — one cheap authenticated call.
	 * Optional: a provider without one simply has no Test button.
	 */
	TFunction<void(FForgeKeyTestResult)> Test;

	bool IsUsable() const { return !Id.IsNone() && IsSet && Describe && Store && Clear; }
};

/**
 * Every key the installed plugins ask for, in one list.
 *
 * Reached through the module interface below rather than directly, so that a plugin needs this
 * header and nothing else — no library, no `.uplugin` dependency, no failure to load when ForgeKeys
 * is absent.
 */
class FForgeKeyRegistry
{
public:
	virtual ~FForgeKeyRegistry() = default;

	/** Register at module startup. Registering the same Id twice replaces the first. */
	virtual void Register(FForgeKeyProvider Provider) = 0;

	/** Call from ShutdownModule, or a hot reload leaves a closure pointing at unloaded code. */
	virtual void Unregister(FName Id) = 0;

	/** Every provider, ordered by owning plugin then display name. */
	virtual const TArray<FForgeKeyProvider>& All() const = 0;

	virtual const FForgeKeyProvider* Find(FName Id) const = 0;

	/** Raised when a key is stored or cleared, so an open panel re-reads rather than polls. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnKeysChanged, FName /*Id*/);
	virtual FOnKeysChanged& OnKeysChanged() = 0;
};

/**
 * ForgeKeys' module, and the only way in.
 *
 * **A plugin that wants its key on the shared page does this and nothing else:**
 *
 * ```cpp
 * if (IForgeKeysModule* Keys = IForgeKeysModule::GetIfLoaded())
 * {
 *     Keys->Registry().Register(MoveTemp(Descriptor));
 * }
 * ```
 *
 * Add `PrivateIncludePathModuleNames.Add("ForgeKeys")` to the Build.cs — headers only, no link —
 * and leave the `.uplugin` alone. With ForgeKeys absent the lookup returns null and the plugin
 * carries on with its own settings page, which is the path that must always work.
 */
class IForgeKeysModule : public IModuleInterface
{
public:
	/**
	 * ForgeKeys, loading it if it is installed but has not started yet. Null when it is not
	 * installed at all — which is the ordinary case for a plugin downloaded on its own.
	 *
	 * Loading rather than merely looking up, for the same reason MotionForge's own registry does:
	 * two plugins in the same loading phase start in an order nobody controls, and a lookup would
	 * work on some runs and silently register nothing on others. `ModuleExists` first, so a project
	 * without ForgeKeys gets no warning in its log.
	 */
	static IForgeKeysModule* GetOrLoad()
	{
		if (!FModuleManager::Get().ModuleExists(TEXT("ForgeKeys")))
		{
			return nullptr;
		}
		return FModuleManager::Get().LoadModulePtr<IForgeKeysModule>(TEXT("ForgeKeys"));
	}

	/** Null unless ForgeKeys is already up. For shutdown paths, which must never load anything. */
	static IForgeKeysModule* GetIfLoaded()
	{
		return FModuleManager::GetModulePtr<IForgeKeysModule>(TEXT("ForgeKeys"));
	}

	virtual FForgeKeyRegistry& Registry() = 0;
};
