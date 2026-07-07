# Active Source Volume

An OBS Studio plugin that gives you **hotkeys to raise, lower, and mute a browser
source's volume** — designed for a Stream Deck. It changes one browser source at
a time (the one on your live scene, or one you pin), so your alert, donation, and
chat browsers are left untouched. There's also an optional **DCA mode** to trim
every browser source together.

The volume changes are **relative**: pressing **+** adds a few dB to whatever the
source is currently at, so each source keeps its own level.

## Features

- Raise / lower / mute a browser source from a hotkey or Stream Deck button.
- Automatically follows the top-most browser source on your **live scene**.
- Or **pin a specific browser source** so it's always the one controlled.
- **DCA mode** (optional): one press trims *all* browser sources at once, each
  keeping its relative balance.
- A dock that shows which source is being controlled and its live level.
- Adjustable step size (default ±5 dB).
- Works over **obs-websocket** too, for control surfaces like Bitfocus Companion.

## Requirements

- **OBS Studio 31.0 or newer** (macOS, Windows, or Linux).

## Install

Download the latest release for your platform from the
[**Releases**](../../releases) page.

- **macOS** — open the `.pkg` and follow the installer. It's not signed with an
  Apple Developer ID, so the first time you'll need to **right-click the `.pkg`
  → Open** (or approve it under System Settings → Privacy & Security). It
  installs to `~/Library/Application Support/obs-studio/plugins/`.
- **Windows** — run the installer (or unzip the archive into your OBS plugins
  folder).
- **Linux** — install the `.deb`, or copy the plugin into your OBS plugins
  directory.

Then fully quit and reopen OBS. You'll find a new **Active Source Volume** dock
under the **Docks** menu.

## Set up your hotkeys

1. In OBS, go to **Settings → Hotkeys** and find:
   - **Browser Volume: +**
   - **Browser Volume: -**
   - **Browser Volume: Toggle Mute**
2. Assign a keyboard shortcut to each.
3. In the **Stream Deck** app, add a **System → Hotkey** action to a button and
   record the same shortcut.

Now those buttons control your browser source's volume live.

## The dock

Open **Docks → Active Source Volume**. It shows what's being controlled and its
current level, and has these settings (all saved automatically):

- **Controlled browser sources** — a checklist of your browser sources. Check
  the one(s) you want to control, and the plugin drives whichever *checked*
  source is on the live scene. This is handy when your content browser sits
  below other sources (so "auto" would grab the wrong one), or when you want to
  choose a different browser per scene. Leave everything unchecked to auto-pick
  the top-most browser on each scene. **Clear (auto)** resets to auto, and
  **Refresh list** rescans after you add or rename sources.
- **DCA mode** — when enabled, the hotkeys trim **every** browser source at once
  (each relative to its own level) instead of just one.
- **Hotkey step** — how many dB each press changes the volume (default 5 dB).

## Important: turn on "Control audio via OBS"

For the volume hotkeys to actually affect what viewers hear, each browser source
must have OBS handling its audio:

1. Right-click the browser source → **Properties**.
2. Enable **Control audio via OBS** (near the bottom).

Without this, the browser's audio bypasses OBS's volume control and the hotkeys
won't change anything audible.

## Advanced: control via obs-websocket

If you use Bitfocus Companion or another obs-websocket client, this plugin
registers a vendor named `active-source-volume` with these requests:

| Request | Parameters | Returns |
|---|---|---|
| `NudgeVolume` | `deltaDb` | the controlled source's `sourceName, db, muted` |
| `ToggleMute` | — | the controlled source's state |
| `GetControlled` | — | the controlled source's state |
| `NudgeAll` | `deltaDb` | `count` of browser sources trimmed |
| `ToggleMuteAll` | — | `count` |
| `GetBrowsers` | — | `count` and a list of every browser source |

## Building from source

This plugin uses the standard
[obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) build
system. With CMake 3.28+ and the platform toolchain installed:

```bash
cmake --preset macos        # or: windows-x64 / ubuntu-x86_64
cmake --build --preset macos
```

The build downloads matching OBS and Qt dependencies automatically. On macOS the
result is `build_macos/RelWithDebInfo/active-source-volume.plugin`; copy it into
`~/Library/Application Support/obs-studio/plugins/`.

## License

GPL-2.0-or-later. See [`LICENSE`](LICENSE).
