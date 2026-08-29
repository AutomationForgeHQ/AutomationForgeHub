// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "AutomationForgeHubSettings.generated.h"

UENUM()
enum class EAutomationForgeChannel : uint8
{
	/** Tagged releases only. */
	Stable,
	/** Whatever is newest, nightly builds included. */
	Nightly,
};

/**
 * Editor Preferences > Automation Forge > Hub. Per user, because the channel
 * and the notifications are a person's choice, not a project's. The other
 * plugins keep their project settings under the same "Automation Forge"
 * category on the Project side.
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Hub"))
class AUTOMATIONFORGEHUB_API UAutomationForgeHubSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAutomationForgeHubSettings();

	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	virtual FName GetCategoryName() const override { return TEXT("Automation Forge"); }

	/** Look at the manifest when the editor starts, and mark the toolbar button when something is newer. */
	UPROPERTY(config, EditAnywhere, Category = "Updates")
	bool bCheckForUpdatesOnStartup = true;

	/** Show a notification in the corner, once per session, when updates exist. */
	UPROPERTY(config, EditAnywhere, Category = "Updates")
	bool bNotify = true;

	/** Stable sees tagged releases only; Nightly sees whichever is newest. The hub has the same switch. */
	UPROPERTY(config, EditAnywhere, Category = "Updates")
	EAutomationForgeChannel Channel = EAutomationForgeChannel::Stable;

	/** Where the manifest of published releases lives. Leave alone unless you mirror it. */
	UPROPERTY(config, EditAnywhere, Category = "Sources", AdvancedDisplay)
	FString ManifestUrl = TEXT("https://raw.githubusercontent.com/AutomationForgeHQ/automation-forge/main/manifest.json");

	/** The hub executable, if it is not where the installer puts it. */
	UPROPERTY(config, EditAnywhere, Category = "Hub", meta = (FilePathFilter = "exe"))
	FFilePath HubExecutable;

	/** Where to send people who do not have the hub yet. */
	UPROPERTY(config, EditAnywhere, Category = "Hub", AdvancedDisplay)
	FString DownloadUrl = TEXT("https://github.com/AutomationForgeHQ/automation-forge/releases/latest");
};
