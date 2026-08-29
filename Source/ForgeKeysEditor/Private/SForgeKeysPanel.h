// Copyright Blackcode SA. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ForgeKeyRegistry.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Every key the installed plugins ask for, in one place.
 *
 * Reads the registry, never a hardcoded list, so a plugin that is not installed contributes nothing
 * and a plugin added later appears without this panel knowing about it. Sets and clears through the
 * same store the plugins' own settings pages use — this is a convenience over that store, never the
 * only way to set a key.
 */
class SForgeKeysPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SForgeKeysPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SForgeKeysPanel() override;

private:
	/** One provider's row, holding the transient state that belongs to the widget rather than the store. */
	struct FRow
	{
		FName Id;
		FString Entry;        // what is typed, never stored here beyond the click
		FText Result;         // the last test's answer
		bool bTesting = false;
		bool bResultOk = false;
	};

	TSharedRef<SWidget> BuildBody();
	TSharedRef<SWidget> BuildGroup(const FText& Owner, const TArray<FForgeKeyProvider>& Group);
	TSharedRef<SWidget> BuildCard(const FForgeKeyProvider& Provider);
	void Rebuild();

	TSharedPtr<FRow> RowFor(FName Id);
	const FForgeKeyProvider* ProviderFor(FName Id) const;

	FReply OnSave(FName Id);
	FReply OnClear(FName Id);
	FReply OnTest(FName Id);

	TSharedPtr<SVerticalBox> Body;
	TMap<FName, TSharedPtr<FRow>> Rows;
	FDelegateHandle KeysChangedHandle;
};
