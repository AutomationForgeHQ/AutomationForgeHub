// Copyright Blackcode SA. All rights reserved.

#include "ForgeKeys.h"

#include "ForgeKeyRegistry.h"

DEFINE_LOG_CATEGORY(LogForgeKeys);

namespace
{
	/** The registry, kept behind the module so callers need only the header. */
	class FForgeKeyRegistryImpl final : public FForgeKeyRegistry
	{
	public:
		virtual void Register(FForgeKeyProvider Provider) override
		{
			if (!Provider.IsUsable())
			{
				UE_LOG(LogForgeKeys, Warning,
					TEXT("Refusing a key provider without an id and the four operations."));
				return;
			}

			const FName Id = Provider.Id;
			Providers.RemoveAll([Id](const FForgeKeyProvider& Existing) { return Existing.Id == Id; });
			Providers.Add(MoveTemp(Provider));
			Sort();

			UE_LOG(LogForgeKeys, Log, TEXT("Key provider '%s' registered."), *Id.ToString());

			// An open panel is showing a list that just changed. A plugin can register or unload at
			// any time, so the page follows the installed set rather than a snapshot of it.
			KeysChanged.Broadcast(Id);
		}

		virtual void Unregister(FName Id) override
		{
			if (Providers.RemoveAll([Id](const FForgeKeyProvider& Existing) { return Existing.Id == Id; }) > 0)
			{
				UE_LOG(LogForgeKeys, Log, TEXT("Key provider '%s' unregistered."), *Id.ToString());
				KeysChanged.Broadcast(Id);
			}
		}

		virtual const TArray<FForgeKeyProvider>& All() const override { return Providers; }

		virtual const FForgeKeyProvider* Find(FName Id) const override
		{
			return Providers.FindByPredicate([Id](const FForgeKeyProvider& P) { return P.Id == Id; });
		}

		virtual FOnKeysChanged& OnKeysChanged() override { return KeysChanged; }

	private:
		void Sort()
		{
			Providers.Sort([](const FForgeKeyProvider& A, const FForgeKeyProvider& B)
			{
				const FString OwnerA = A.Owner.ToString();
				const FString OwnerB = B.Owner.ToString();
				return OwnerA == OwnerB
					? A.DisplayName.ToString() < B.DisplayName.ToString()
					: OwnerA < OwnerB;
			});
		}

		TArray<FForgeKeyProvider> Providers;
		FOnKeysChanged KeysChanged;
	};

	class FForgeKeysModule final : public IForgeKeysModule
	{
	public:
		virtual FForgeKeyRegistry& Registry() override { return RegistryImpl; }

	private:
		FForgeKeyRegistryImpl RegistryImpl;
	};
}

IMPLEMENT_MODULE(FForgeKeysModule, ForgeKeys)
