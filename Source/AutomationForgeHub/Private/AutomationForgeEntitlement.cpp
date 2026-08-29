// Copyright Blackcode SA. All rights reserved.

#include "AutomationForgeEntitlement.h"
#include "IPluginWardenModule.h"

void FAutomationForgeEntitlement::Check(const FText& PluginFriendlyName, const FString& FabItemId, const FString& FabOfferId, TFunction<void()> Authorized)
{
	IPluginWardenModule::Get().CheckEntitlementForPlugin(
		PluginFriendlyName, FabItemId, FabOfferId,
		FText::GetEmpty(),
		IPluginWardenModule::EUnauthorizedErrorHandling::ShowMessageOpenStore,
		MoveTemp(Authorized));
}
