// Copyright Blackcode SA. All rights reserved.

#include "SForgeKeysPanel.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ForgeKeys"

namespace
{
	FForgeKeyRegistry* GetRegistry()
	{
		IForgeKeysModule* Module = IForgeKeysModule::GetIfLoaded();
		return Module ? &Module->Registry() : nullptr;
	}

	const FLinearColor GoodColour(0.30f, 0.78f, 0.45f);
	const FLinearColor WarnColour(0.95f, 0.65f, 0.20f);
	const FLinearColor QuietColour(0.55f, 0.55f, 0.58f);
}

void SForgeKeysPanel::Construct(const FArguments& InArgs)
{
	// Another surface — a settings page, a console command, an agent — may set a key or load a
	// plugin while this is open. Re-read rather than poll.
	if (FForgeKeyRegistry* Registry = GetRegistry())
	{
		KeysChangedHandle = Registry->OnKeysChanged().AddLambda([this](FName) { Rebuild(); });
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		.Padding(0.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(20.f, 18.f, 20.f, 24.f)
			[
				SAssignNew(Body, SVerticalBox)
			]
		]
	];

	Rebuild();
}

SForgeKeysPanel::~SForgeKeysPanel()
{
	if (FForgeKeyRegistry* Registry = GetRegistry())
	{
		Registry->OnKeysChanged().Remove(KeysChangedHandle);
	}
}

TSharedPtr<SForgeKeysPanel::FRow> SForgeKeysPanel::RowFor(FName Id)
{
	if (TSharedPtr<FRow>* Existing = Rows.Find(Id))
	{
		return *Existing;
	}
	TSharedPtr<FRow> Row = MakeShared<FRow>();
	Row->Id = Id;
	Rows.Add(Id, Row);
	return Row;
}

const FForgeKeyProvider* SForgeKeysPanel::ProviderFor(FName Id) const
{
	FForgeKeyRegistry* Registry = GetRegistry();
	return Registry ? Registry->Find(Id) : nullptr;
}

void SForgeKeysPanel::Rebuild()
{
	if (!Body.IsValid())
	{
		return;
	}
	Body->ClearChildren();
	Body->AddSlot().AutoHeight()[BuildBody()];
}

TSharedRef<SWidget> SForgeKeysPanel::BuildBody()
{
	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);

	Column->AddSlot().AutoHeight()
	[
		SNew(STextBlock)
		.Font(FAppStyle::GetFontStyle("HeadingMedium"))
		.Text(LOCTEXT("Title", "Keys"))
	];

	Column->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 18.f)
	[
		SNew(SBox)
		.MaxDesiredWidth(720.f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("Subtitle",
				"Kept in this computer's credential vault, never in the project — so a key does not "
				"travel with a copied folder or a commit. Each plugin can also set its own key in "
				"Editor Preferences; this is the same store, gathered in one place."))
		]
	];

	FForgeKeyRegistry* Registry = GetRegistry();
	const TArray<FForgeKeyProvider> Providers = Registry ? Registry->All() : TArray<FForgeKeyProvider>();

	if (Providers.Num() == 0)
	{
		Column->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
			.Padding(16.f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(LOCTEXT("Empty",
					"No plugin installed here asks for a key. Install a generator — MotionForge, "
					"SpeechForge — and the keys it wants appear."))
			]
		];
		return Column;
	}

	// One collapsible section per plugin. The registry is already sorted by owner, so a change of
	// owner closes the previous group.
	TArray<FForgeKeyProvider> Group;
	FText GroupOwner;

	auto FlushGroup = [&Column, &Group, &GroupOwner, this]()
	{
		if (Group.Num() > 0)
		{
			Column->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[BuildGroup(GroupOwner, Group)];
			Group.Reset();
		}
	};

	for (const FForgeKeyProvider& Provider : Providers)
	{
		if (!Provider.Owner.EqualTo(GroupOwner))
		{
			FlushGroup();
			GroupOwner = Provider.Owner;
		}
		Group.Add(Provider);
	}
	FlushGroup();

	return Column;
}

TSharedRef<SWidget> SForgeKeysPanel::BuildGroup(const FText& Owner, const TArray<FForgeKeyProvider>& Group)
{
	int32 Set = 0;
	int32 Required = 0;
	for (const FForgeKeyProvider& Provider : Group)
	{
		if (Provider.IsSet()) { ++Set; }
		if (!Provider.bOptional) { ++Required; }
	}

	// "2 of 3" counts what is actually needed; an optional key left unset is not a shortfall.
	const int32 RequiredSet = FMath::Min(Set, Required);
	const FText Summary = Required == 0
		? FText::Format(LOCTEXT("GroupAllOptional", "{0} optional"), Group.Num())
		: FText::Format(LOCTEXT("GroupCount", "{0} of {1} set"), RequiredSet, Required);
	const bool bComplete = Required == 0 || RequiredSet >= Required;

	TSharedRef<SVerticalBox> Cards = SNew(SVerticalBox);
	for (const FForgeKeyProvider& Provider : Group)
	{
		Cards->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[BuildCard(Provider)];
	}

	return SNew(SExpandableArea)
		.InitiallyCollapsed(false)
		.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
		.BodyBorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
		.Padding(FMargin(12.f, 10.f, 12.f, 12.f))
		.HeaderPadding(FMargin(6.f, 6.f))
		.HeaderContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))
				.Text(Owner)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(8.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ColorAndOpacity(bComplete ? FSlateColor(GoodColour) : FSlateColor(QuietColour))
				.Text(Summary)
			]
		]
		.BodyContent()
		[
			Cards
		];
}

TSharedRef<SWidget> SForgeKeysPanel::BuildCard(const FForgeKeyProvider& Provider)
{
	const FName Id = Provider.Id;
	TSharedPtr<FRow> Row = RowFor(Id);
	const bool bIsSet = Provider.IsSet();

	// A dot rather than a sentence: the state is scannable down a column, and the sentence that
	// explains it sits underneath where it does not compete.
	const FLinearColor DotColour = bIsSet ? GoodColour : (Provider.bOptional ? QuietColour : WarnColour);
	const FText StateText = bIsSet
		? FText::FromString(Provider.Describe())
		: (Provider.bOptional
			? LOCTEXT("OptionalUnset", "Optional — not set")
			: LOCTEXT("Missing", "Not set"));

	TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);

	// Name, state.
	Card->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(DotColour))
			.Text(FText::FromString(TEXT("\x2022")))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("NormalFontBold"))
			.Text(Provider.DisplayName)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(bIsSet ? FSlateColor(GoodColour) : FSlateColor::UseSubduedForeground())
			.Text(StateText)
		]
	];

	// Who actually uses it. Only meaningful for a shared key, where the answer is not "the plugin this
	// row is filed under" - because it is filed under General precisely so it is not asked for twice.
	if (Provider.bShared && Provider.Consumers.Num() > 0)
	{
		TArray<FString> Names;
		for (const FText& Consumer : Provider.Consumers)
		{
			Names.Add(Consumer.ToString());
		}

		Card->AddSlot().AutoHeight().Padding(16.f, 2.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(FText::Format(
				NSLOCTEXT("ForgeKeys", "UsedBy", "Used by {0}"),
				FText::FromString(FString::Join(Names, TEXT(", ")))))
		];
	}

	if (!Provider.Purpose.IsEmpty())
	{
		Card->AddSlot().AutoHeight().Padding(16.f, 4.f, 0.f, 0.f)
		[
			SNew(SBox)
			.MaxDesiredWidth(640.f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(Provider.Purpose)
			]
		];
	}

	// The half a key page usually leaves out. Entering a token is one step; being allowed to download
	// what it unlocks is another, and it fails much later and much less clearly.
	if (Provider.AccessRequirements.Num() > 0)
	{
		Card->AddSlot().AutoHeight().Padding(16.f, 6.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
			.ColorAndOpacity(FSlateColor(WarnColour))
			.Text(LOCTEXT("AccessAlso", "A token is not access. You must also be granted:"))
		];

		for (const FForgeKeyAccessRequirement& Requirement : Provider.AccessRequirements)
		{
			const FString Url = Requirement.Url;

			TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("SmallFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(FText::Format(LOCTEXT("AccessItem", "\x2022  {0}"), Requirement.What))
				];

			if (!Url.IsEmpty())
			{
				Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SHyperlink)
					.Text(Requirement.bReviewedByHand
						? LOCTEXT("RequestAccess", "Request access")
						: LOCTEXT("AcceptTerms", "Accept the terms"))
					.OnNavigate_Lambda([Url]() { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); })
				];
			}

			if (Requirement.bReviewedByHand)
			{
				Line->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("SmallFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(LOCTEXT("ByHand", "(reviewed by a human - start it early)"))
				];
			}

			Card->AddSlot().AutoHeight().Padding(24.f, 2.f, 0.f, 0.f)[Line];
		}
	}

	// Paste, save, clear, test.
	TSharedRef<SHorizontalBox> Controls = SNew(SHorizontalBox);

	Controls->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
	[
		SNew(SBox)
		.MaxDesiredWidth(420.f)
		[
			SNew(SEditableTextBox)
			.IsPassword(true)
			.HintText(bIsSet
				? LOCTEXT("HintReplace", "Paste a new key to replace the stored one")
				: LOCTEXT("HintSet", "Paste the key"))
			.OnTextChanged_Lambda([Row](const FText& NewText) { Row->Entry = NewText.ToString(); })
			.OnTextCommitted_Lambda([this, Id, Row](const FText& NewText, ETextCommit::Type CommitType)
			{
				Row->Entry = NewText.ToString();
				if (CommitType == ETextCommit::OnEnter)
				{
					OnSave(Id);
				}
			})
		]
	];

	Controls->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
	[
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
		.Text(LOCTEXT("Save", "Save"))
		.ToolTipText(LOCTEXT("SaveTip", "Store this key in the credential vault. The field clears."))
		.IsEnabled_Lambda([Row]() { return !Row->Entry.IsEmpty(); })
		.OnClicked(this, &SForgeKeysPanel::OnSave, Id)
	];

	Controls->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Clear", "Clear"))
		.ToolTipText(LOCTEXT("ClearTip", "Forget the stored key. An environment variable, if set, still applies."))
		.IsEnabled(bIsSet)
		.OnClicked(this, &SForgeKeysPanel::OnClear, Id)
	];

	if (Provider.Test)
	{
		Controls->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Test", "Test"))
			.ToolTipText(LOCTEXT("TestTip", "One cheap authenticated call, to prove the key works."))
			.IsEnabled_Lambda([Row, bIsSet]() { return bIsSet && !Row->bTesting; })
			.OnClicked(this, &SForgeKeysPanel::OnTest, Id)
		];
	}

	Card->AddSlot().AutoHeight().Padding(16.f, 10.f, 0.f, 0.f)[Controls];

	// The last test's answer.
	Card->AddSlot().AutoHeight().Padding(16.f, 6.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.ColorAndOpacity_Lambda([Row]()
		{
			return Row->bResultOk ? FSlateColor(GoodColour) : FSlateColor(WarnColour);
		})
		.Visibility_Lambda([Row]() { return Row->Result.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
		.Text_Lambda([Row]() { return Row->Result; })
	];

	// Where it lives, and where to get one. Quiet, but never hidden: somebody has to be able to find
	// the entry themselves, or set the variable on a build machine.
	TSharedRef<SHorizontalBox> Footer = SNew(SHorizontalBox);

	Footer->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.ColorAndOpacity(FSlateColor(QuietColour))
		.Text(FText::Format(
			LOCTEXT("Provenance", "{0}  ·  or set {1}"),
			FText::FromString(Provider.VaultEntryName),
			FText::FromString(Provider.EnvironmentVariableName)))
	];

	if (!Provider.HelpUrl.IsEmpty())
	{
		const FString Url = Provider.HelpUrl;
		Footer->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)
		[
			SNew(SHyperlink)
			.Text(LOCTEXT("Where", "Where do I get one?"))
			.OnNavigate_Lambda([Url]() { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); })
		];
	}

	Card->AddSlot().AutoHeight().Padding(16.f, 8.f, 0.f, 0.f)[Footer];

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
		.Padding(FMargin(14.f, 12.f))
		[
			Card
		];
}

FReply SForgeKeysPanel::OnSave(FName Id)
{
	const FForgeKeyProvider* Provider = ProviderFor(Id);
	TSharedPtr<FRow> Row = RowFor(Id);
	if (!Provider || Row->Entry.IsEmpty())
	{
		return FReply::Handled();
	}

	const bool bStored = Provider->Store(Row->Entry.TrimStartAndEnd());

	// Never leave a key sitting in a widget: it invites a screenshot, and it is stored now anyway.
	Row->Entry.Reset();
	Row->Result = bStored
		? LOCTEXT("Saved", "Saved.")
		: FText::Format(LOCTEXT("SaveFailed", "Could not store it. Set {0} instead."),
			FText::FromString(Provider->EnvironmentVariableName));
	Row->bResultOk = bStored;

	Rebuild();
	return FReply::Handled();
}

FReply SForgeKeysPanel::OnClear(FName Id)
{
	if (const FForgeKeyProvider* Provider = ProviderFor(Id))
	{
		TSharedPtr<FRow> Row = RowFor(Id);
		Provider->Clear();
		Row->Result = FText::GetEmpty();
		Row->bResultOk = false;
		Rebuild();
	}
	return FReply::Handled();
}

FReply SForgeKeysPanel::OnTest(FName Id)
{
	const FForgeKeyProvider* Provider = ProviderFor(Id);
	TSharedPtr<FRow> Row = RowFor(Id);
	if (!Provider || !Provider->Test || Row->bTesting)
	{
		return FReply::Handled();
	}

	Row->bTesting = true;
	Row->bResultOk = false;
	Row->Result = LOCTEXT("Testing", "Testing…");

	// The panel may be closed before the call returns; hold a weak reference and check it.
	TWeakPtr<SForgeKeysPanel> WeakSelf = StaticCastSharedRef<SForgeKeysPanel>(AsShared());
	Provider->Test([WeakSelf, Row](bool bOk, FText Message)
	{
		Row->bTesting = false;
		Row->bResultOk = bOk;
		Row->Result = Message.IsEmpty()
			? (bOk ? LOCTEXT("TestOk", "The key works.") : LOCTEXT("TestFailed", "The provider refused it."))
			: Message;

		if (TSharedPtr<SForgeKeysPanel> Self = WeakSelf.Pin())
		{
			Self->Rebuild();
		}
	});

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
