// Copyright Blackcode SA. All rights reserved.

#include "AutomationForgeHubSettings.h"
#include "AutomationForgeHubStyle.h"
#include "AutomationForgeHubToolbar.h"
#include "AutomationForgeUpdateCheck.h"
#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "AutomationForgeHub"

UAutomationForgeHubSettings::UAutomationForgeHubSettings()
{
	CategoryName = TEXT("Automation Forge");
	SectionName = TEXT("Hub");
}

/**
 * The editor half of the hub. Registers the toolbar button once ToolMenus is
 * up, and — a few seconds after the editor is — asks the manifest whether
 * anything on this machine has a newer release.
 */
class FAutomationForgeHubModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FAutomationForgeHubStyle::Initialize();
		ToolMenusHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FAutomationForgeHubToolbar::Register));

		if (GetDefault<UAutomationForgeHubSettings>()->bCheckForUpdatesOnStartup)
		{
			// Let the editor finish coming up before a network call and a notification.
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				FAutomationForgeUpdateCheck::Check(/*bNotify*/ true);
				return false;
			}), 8.f);
		}
	}

	virtual void ShutdownModule() override
	{
		if (TickerHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		if (UToolMenus* Menus = UToolMenus::TryGet())
		{
			UToolMenus::UnRegisterStartupCallback(ToolMenusHandle);
			Menus->UnregisterOwner(this);
		}
		FAutomationForgeHubStyle::Shutdown();
	}

private:
	FDelegateHandle ToolMenusHandle;
	FTSTicker::FDelegateHandle TickerHandle;
};

IMPLEMENT_MODULE(FAutomationForgeHubModule, AutomationForgeHub)

#undef LOCTEXT_NAMESPACE
