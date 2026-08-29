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

## For paid plugins

`FAutomationForgeEntitlement::Check(FriendlyName, FabItemId, FabOfferId, OnAuthorized)`
asks the Epic Games Launcher, through the engine's own PluginWarden, whether
the signed-in account owns the Fab listing, and runs the callback when it does.
Free plugins never call it.
