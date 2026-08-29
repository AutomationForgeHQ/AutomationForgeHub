// Copyright Blackcode SA. All rights reserved.

#include "AutomationForgeHubStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"

TSharedPtr<FSlateStyleSet> FAutomationForgeHubStyle::StyleSet;

FName FAutomationForgeHubStyle::GetStyleSetName()
{
	static const FName Name(TEXT("AutomationForgeHubStyle"));
	return Name;
}

void FAutomationForgeHubStyle::Initialize()
{
	if (StyleSet.IsValid())
	{
		return;
	}
	StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AutomationForgeHub"));
	StyleSet->SetContentRoot((Plugin.IsValid() ? Plugin->GetBaseDir() : FPaths::EnginePluginsDir()) / TEXT("Resources"));

	StyleSet->Set("AutomationForgeHub.Icon", new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Icon40"), TEXT(".png")), FVector2D(20.f, 20.f)));
	StyleSet->Set("AutomationForgeHub.Icon.Small", new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Icon20"), TEXT(".png")), FVector2D(16.f, 16.f)));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FAutomationForgeHubStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}
}
