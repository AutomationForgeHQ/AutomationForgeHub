// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/** The mark, as a Slate brush for the toolbar. */
class FAutomationForgeHubStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static FName GetStyleSetName();

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
