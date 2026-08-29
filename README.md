# Automation Forge Hub — the editor plugin

The Automation Forge menu in Unreal Editor. It ships with every engine
install the hub makes and is enabled by default, so every project on that
engine has it without an edit to the `.uproject`.

**Toolbar → Automation Forge**, after Play:

- **Open the hub** — starts the desktop hub, or sends you to the download if
  it is not on this machine.
- **Check for updates now** — reads the public manifest and compares it with
  the Automation Forge plugins the engine found. The button reads
  *Automation Forge (2)* while two of them have a newer release, and a
  notification says so once per session.
- **Preferences** — Editor Preferences › Automation Forge › Hub: check at
  startup, notifications, the stable/nightly channel (the hub has the same
  switch), the manifest URL, and where the hub executable is.
- **Installed** — every Automation Forge plugin with its version. A tick means
  enabled for this project; ticking or unticking edits the `.uproject` and the
  editor applies it on restart.

Nothing installs from here. The hub (or the `forge` command line) does the
downloading, verifying and placing; this plugin tells you when there is
something to do.

## Keys

The shared Keys page lives here - **Tools > Automation Forge > Keys** - and so
does the registry behind it. It was its own plugin, `ForgeKeys`, until
2026-08-29; keys are machine-wide and plugin-dependent, which is what the hub
already is, so a second plugin to hold them was one plugin too many. The two
modules kept their names, so the header, the interface and the tab id are
unchanged for everything that was already using them.

**A plugin that wants a key on that page registers one, and works without it.**
The registry holds no vault code: what a plugin hands over is metadata plus
closures over *its own* store, so the operations stay where the secret does.

```cpp
#include "ForgeKeyRegistry.h"

if (IForgeKeysModule* Keys = IForgeKeysModule::GetOrLoad())   // null when not installed
{
    FForgeKeyProvider Key;
    Key.Id          = TEXT("MyPlugin.Acme");
    Key.DisplayName = LOCTEXT("Acme", "Acme");
    Key.Owner       = LOCTEXT("MyPlugin", "MyPlugin");
    Key.Purpose     = LOCTEXT("Why", "What this unlocks, and what happens without it.");
    Key.HelpUrl     = TEXT("https://acme.example/api-keys");
    Key.bOptional   = false;                       // true when the plugin works without it

    Key.VaultEntryName          = TEXT("MyPlugin/Acme");
    Key.EnvironmentVariableName = TEXT("MYPLUGIN_ACME_KEY");

    // The operations stay with you, over your own store. This plugin holds no vault code.
    Key.IsSet    = []                            { return FMyStore::Has(TEXT("Acme")); };
    Key.Describe = []                            { return FMyStore::DescribeSource(TEXT("Acme")); };
    Key.Store    = [](const FString& Secret)     { return FMyStore::Set(TEXT("Acme"), Secret); };
    Key.Clear    = []                            { return FMyStore::Remove(TEXT("Acme")); };
    Key.Test     = [](FForgeKeyTestResult Done)  { /* one cheap authenticated call */ };

    Keys->Registry().Register(MoveTemp(Key));
}
```

Unregister from `ShutdownModule`, or a hot reload leaves a closure pointing at unloaded code.

Unregister from `ShutdownModule`, or a hot reload leaves a closure pointing at
unloaded code.

**UBT will warn about this, and the warning is wrong.** You will see:

```
Warning: Plugin 'MotionForge' does not list plugin 'AutomationForgeHub' as a dependency,
         but module 'MotionForge' depends on module 'ForgeKeys'.
```

`PrivateIncludePathModuleNames` is a build-time include path, not a link, and UBT reports both the
same way. **Do not silence it by adding the dependency** — that is exactly the change this pattern
exists to prevent, and it would make the plugin unusable on its own. Confirm the truth in the binary
instead:

```powershell
# must print nothing
Select-String -Path UnrealEditor-MotionForge.dll -Pattern "UnrealEditor-ForgeKeys.dll"
```

`bOptional` matters: a key that is genuinely not needed in every mode — a runner on loopback, say —
should say so, or the page reports a working setup as incomplete.

`bOptional` matters: a key that is genuinely not needed in every mode - a runner
on loopback, say - should say so, or the page reports a working setup as
incomplete.

### The same keys, outside the editor

The hub application reads `Config/ForgeMachine.json` from the plugins installed
on this machine and offers the same keys in a drawer of its own — which is worth
having, because a first run currently means opening Unreal to find out that a key
is missing.

It reaches the identical Windows Credential Manager entry through the identical
calls, so there is no second store and nothing to synchronise. The blob is
**UTF-8 and not null terminated**: the plugins write it with `FTCHARToUTF8` and
read it back with `FUTF8ToTCHAR`, sized by the exact byte count. Anything else
touching that row must match, or it stores an entry the editor decodes as
mojibake — the right row, addressed correctly, holding something neither side
can use.

What the hub does not do is **test** a key, and it says so rather than offering a
button that cannot. A test is one authenticated call in the provider's own shape.

## For paid plugins

`FAutomationForgeEntitlement::Check(FriendlyName, FabItemId, FabOfferId, OnAuthorized)`
asks the Epic Games Launcher, through the engine's own PluginWarden, whether
the signed-in account owns the Fab listing, and runs the callback when it does.
Free plugins never call it.
