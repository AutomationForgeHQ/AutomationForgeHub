// Copyright Blackcode SA. All rights reserved.

#include "AutomationForgeUpdateCheck.h"
#include "AutomationForgeHubSettings.h"
#include "AutomationForgeHubToolbar.h"
#include "Dom/JsonObject.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersion.h"
#include "Serialization/JsonSerializer.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "AutomationForgeHub"

TArray<FAutomationForgePluginState> FAutomationForgeUpdateCheck::States;
TMap<FString, FString> FAutomationForgeUpdateCheck::LatestByName;
TSet<FString> FAutomationForgeUpdateCheck::ManifestNames;
bool FAutomationForgeUpdateCheck::bChecked = false;
bool FAutomationForgeUpdateCheck::bChecking = false;
bool FAutomationForgeUpdateCheck::bNotified = false;
FString FAutomationForgeUpdateCheck::Error;

namespace
{
	/** "0.2.0", "0.2", "0.3.0-nightly.20260901" → comparable. A pre-release sorts below its release. */
	struct FForgeVersion
	{
		int32 Parts[3] = { 0, 0, 0 };
		FString Pre;

		static FForgeVersion Parse(const FString& Text)
		{
			FForgeVersion V;
			FString Core = Text;
			if (Core.StartsWith(TEXT("v"))) Core.RightChopInline(1);
			int32 Plus;
			if (Core.FindChar(TEXT('+'), Plus)) Core.LeftInline(Plus);
			int32 Dash;
			if (Core.FindChar(TEXT('-'), Dash)) { V.Pre = Core.Mid(Dash + 1); Core.LeftInline(Dash); }
			TArray<FString> Bits;
			Core.ParseIntoArray(Bits, TEXT("."));
			for (int32 i = 0; i < 3 && i < Bits.Num(); ++i) V.Parts[i] = FCString::Atoi(*Bits[i]);
			return V;
		}

		int32 Compare(const FForgeVersion& O) const
		{
			for (int32 i = 0; i < 3; ++i) if (Parts[i] != O.Parts[i]) return Parts[i] < O.Parts[i] ? -1 : 1;
			if (Pre.IsEmpty()) return O.Pre.IsEmpty() ? 0 : 1;
			if (O.Pre.IsEmpty()) return -1;
			return Pre.Compare(O.Pre);
		}
	};
}

bool FAutomationForgeUpdateCheck::IsOurs(const IPlugin& Plugin)
{
	if (ManifestNames.Contains(Plugin.GetName())) return true;
	FString Dir = Plugin.GetBaseDir();
	FPaths::NormalizeDirectoryName(Dir);
	return Dir.Contains(TEXT("/Plugins/AutomationForge/"));
}

void FAutomationForgeUpdateCheck::Check(bool bNotify)
{
	if (bChecking) return;
	bChecking = true;
	Error.Empty();
	const UAutomationForgeHubSettings* Settings = GetDefault<UAutomationForgeHubSettings>();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Settings->ManifestUrl);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("forge-editor/0.1"));
	Request->SetTimeout(20.f);
	Request->OnProcessRequestComplete().BindStatic(&FAutomationForgeUpdateCheck::OnResponse, bNotify);
	Request->ProcessRequest();
}

void FAutomationForgeUpdateCheck::OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk, bool bNotify)
{
	bChecking = false;
	if (!bOk || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		Error = Response.IsValid() ? FString::Printf(TEXT("HTTP %d"), Response->GetResponseCode()) : TEXT("no response");
		Rescan();
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		Error = TEXT("the manifest did not parse");
		Rescan();
		return;
	}

	const FEngineVersion& Ver = FEngineVersion::Current();
	const FString Engine = FString::Printf(TEXT("%d.%d"), Ver.GetMajor(), Ver.GetMinor());
	const bool bNightly = GetDefault<UAutomationForgeHubSettings>()->Channel == EAutomationForgeChannel::Nightly;

	LatestByName.Empty();
	ManifestNames.Empty();
	const TArray<TSharedPtr<FJsonValue>>* Plugins = nullptr;
	if (Root->TryGetArrayField(TEXT("plugins"), Plugins))
	{
		for (const TSharedPtr<FJsonValue>& PluginValue : *Plugins)
		{
			const TSharedPtr<FJsonObject> PluginObject = PluginValue->AsObject();
			if (!PluginObject.IsValid()) continue;
			const FString Id = PluginObject->GetStringField(TEXT("id"));
			ManifestNames.Add(Id);

			const TArray<TSharedPtr<FJsonValue>>* Versions = nullptr;
			if (!PluginObject->TryGetArrayField(TEXT("versions"), Versions)) continue;
			FString Best, BestReleased;
			for (const TSharedPtr<FJsonValue>& VersionValue : *Versions)
			{
				const TSharedPtr<FJsonObject> V = VersionValue->AsObject();
				if (!V.IsValid() || V->GetStringField(TEXT("engine")) != Engine) continue;
				const FString Channel = V->HasField(TEXT("channel")) ? V->GetStringField(TEXT("channel")) : TEXT("stable");
				if (!bNightly && Channel != TEXT("stable")) continue;
				const FString Released = V->HasField(TEXT("releasedAt")) ? V->GetStringField(TEXT("releasedAt")) : FString();
				if (Best.IsEmpty() || Released > BestReleased)
				{
					Best = V->GetStringField(TEXT("version"));
					BestReleased = Released;
				}
			}
			if (!Best.IsEmpty()) LatestByName.Add(Id, Best);
		}
	}
	bChecked = true;
	Rescan();
	if (bNotify) Notify();
}

void FAutomationForgeUpdateCheck::Rescan()
{
	States.Empty();
	const FString EngineDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetDiscoveredPlugins())
	{
		if (!IsOurs(*Plugin)) continue;
		FAutomationForgePluginState S;
		S.Name = Plugin->GetName();
		S.FriendlyName = Plugin->GetFriendlyName();
		S.Installed = Plugin->GetDescriptor().VersionName;
		S.bEngine = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir()).StartsWith(EngineDir);
		if (const FString* Latest = LatestByName.Find(S.Name))
		{
			S.Latest = *Latest;
			S.bUpdate = FForgeVersion::Parse(*Latest).Compare(FForgeVersion::Parse(S.Installed)) > 0;
		}
		States.Add(MoveTemp(S));
	}
	States.Sort([](const FAutomationForgePluginState& A, const FAutomationForgePluginState& B) { return A.Name < B.Name; });
	if (UToolMenus* Menus = UToolMenus::TryGet()) Menus->RefreshAllWidgets();
}

void FAutomationForgeUpdateCheck::Notify()
{
	const int32 Count = UpdateCount();
	if (Count == 0 || bNotified || !GetDefault<UAutomationForgeHubSettings>()->bNotify) return;
	bNotified = true;

	FString Names;
	for (const FAutomationForgePluginState& S : States)
	{
		if (!S.bUpdate) continue;
		if (!Names.IsEmpty()) Names += TEXT(", ");
		Names += FString::Printf(TEXT("%s %s"), *S.Name, *S.Latest);
	}
	FNotificationInfo Info(Count == 1
		? LOCTEXT("OneUpdate", "Automation Forge: 1 update available")
		: FText::Format(LOCTEXT("Updates", "Automation Forge: {0} updates available"), Count));
	Info.SubText = FText::FromString(Names);
	Info.ExpireDuration = 10.f;
	Info.bFireAndForget = true;
	Info.bUseSuccessFailIcons = false;
	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("OpenHub", "Open the hub"), LOCTEXT("OpenHubTip", "The hub installs updates; nothing changes from here."),
		FSimpleDelegate::CreateStatic(&FAutomationForgeHubToolbar::OpenHub), SNotificationItem::CS_None));
	FSlateNotificationManager::Get().AddNotification(Info);
}

const TArray<FAutomationForgePluginState>& FAutomationForgeUpdateCheck::Plugins() { return States; }

int32 FAutomationForgeUpdateCheck::UpdateCount()
{
	int32 N = 0;
	for (const FAutomationForgePluginState& S : States) if (S.bUpdate) ++N;
	return N;
}

bool FAutomationForgeUpdateCheck::HasChecked() { return bChecked; }
bool FAutomationForgeUpdateCheck::IsChecking() { return bChecking; }
const FString& FAutomationForgeUpdateCheck::LastError() { return Error; }

#undef LOCTEXT_NAMESPACE
