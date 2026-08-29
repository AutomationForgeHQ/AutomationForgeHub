// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The Fab bridge. A paid plugin asks, at startup or before its first use,
 * whether the signed-in Epic account owns it; the answer comes from the Epic
 * Games Launcher through the engine's own PluginWarden. On success the
 * callback runs; otherwise the engine shows its message with a link to the
 * store. Free plugins never call this.
 *
 * The item and offer ids are the ones Fab assigns to a listing.
 */
class AUTOMATIONFORGEHUB_API FAutomationForgeEntitlement
{
public:
	static void Check(const FText& PluginFriendlyName, const FString& FabItemId, const FString& FabOfferId, TFunction<void()> Authorized);
};
