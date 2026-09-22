# AutomationForgeHub

Every released version of AutomationForgeHub, newest first. A release publishes **one** section of this
file — the one whose heading matches its tag — as its release notes; for an `open` plugin those
notes are posted to Discord `#releases` automatically. Write for someone who installs the plugin,
not for the commit log.

Headings are `## <x.y.z> — <date>`. Use `Added` / `Changed` / `Fixed` / `Compatibility` /
`Known issues`, only the ones that apply.

## 0.2.4 — 2026-09-22

### Changed
- **On in every project, with nothing to set up.** Installed into an engine, the menu now appears in
  every project on that engine. Until now a project had to name the plugin in its `.uproject` first,
  so the menu you use to switch the other plugins on was itself switched off. To leave it out of one
  project, untick it in that project's Plugins window.
- **Nothing reaches a packaged game.** The keys registry is now built for the editor only, so a game
  packaged from any of those projects, for any platform, carries no part of this plugin.

### Added
- The package includes this changelog.

## 0.2.3 — 2026-09-08
- Packaging fix: the release now carries everything the register allows. `BuildPlugin`'s filter excludes `Config/` and every `public_extra` path, so earlier zips shipped without them.

## 0.2.2 — 2026-09-07
- The Tools menu shows one Automation Forge section, not eight loose tools
- Every plugin now points at kovati.dev
- AutomationForgeHub 0.2.2 release

## 0.2.1 — 2026-08-30
- Keys that belong to a person, not to a plugin
- Emotion detection: the audio decides, and a token is not access
- Versions for the release

## 0.2.0 — 2026-08-29
- Apache-2.0, and a release publishes its source
- Kimodo: a panel for the machine that generates
- Every plugin descriptor agrees with its release tag, and says who made it
- ForgeKeys is not a plugin any more; the hub carries the keys
- The READMEs catch up with a long day

## 0.1.1 — 2026-08-28
- Tools menu, the engine handed to the hub, preferences under Automation Forge

## 0.1.0 — 2026-08-28
- AutomationForgeHub: the menu in the editor
