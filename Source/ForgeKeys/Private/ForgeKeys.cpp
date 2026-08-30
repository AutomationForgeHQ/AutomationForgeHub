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

			// A shared key is one key however many plugins ask for it, so a second registration is a
			// second *consumer* rather than a replacement. Replacing would be wrong twice over: the
			// page would lose the fact that two plugins use it, and whichever plugin happened to
			// register last would own operations that both are relying on.
			if (Provider.bShared)
			{
				Provider.Owner = NSLOCTEXT("ForgeKeys", "GeneralOwner", "General");

				if (FForgeKeyProvider* Existing = Providers.FindByPredicate(
						[Id](const FForgeKeyProvider& Candidate) { return Candidate.Id == Id; }))
				{
					for (const FText& Consumer : Provider.Consumers)
					{
						const bool bAlreadyNamed = Existing->Consumers.ContainsByPredicate(
							[&Consumer](const FText& Named) { return Named.EqualTo(Consumer); });

						if (!bAlreadyNamed)
						{
							Existing->Consumers.Add(Consumer);
						}
					}

					// Every consumer's grants, not just the first registrant's. One Hugging Face token
					// can front two unrelated approvals - Meta's, reviewed by hand, and NVIDIA's, a
					// click - and a page showing one of them sends people to the wrong form.
					for (const FForgeKeyAccessRequirement& Requirement : Provider.AccessRequirements)
					{
						const bool bAlreadyListed = Existing->AccessRequirements.ContainsByPredicate(
							[&Requirement](const FForgeKeyAccessRequirement& Listed)
							{
								return Listed.Url == Requirement.Url;
							});

						if (!bAlreadyListed)
						{
							Existing->AccessRequirements.Add(Requirement);
						}
					}

					// Required wins. A key that one plugin can live without and another cannot is a key
					// this machine needs, and showing it as optional would be telling half the truth to
					// whichever half of the user is about to be stuck.
					Existing->bOptional = Existing->bOptional && Provider.bOptional;

					UE_LOG(LogForgeKeys, Log,
						TEXT("Shared key '%s' is now used by %d plugin(s)."),
						*Id.ToString(), Existing->Consumers.Num());

					KeysChanged.Broadcast(Id);
					return;
				}
			}

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
				// General first, whatever it would sort as alphabetically. The keys that belong to a
				// person rather than to a plugin are the ones worth meeting first, and they are the
				// ones a second plugin will otherwise ask for again.
				if (A.bShared != B.bShared)
				{
					return A.bShared;
				}

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
