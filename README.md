# Active Source Volume

An OBS Studio plugin that gives you **one Stream Deck button (or dial) that
controls the volume of the browser source on the scene currently live on
program output** — and follows it automatically as scenes change.

Only **browser sources** are considered. Mics, media, images, nested scenes and
everything else in a scene are ignored by design.

Built on the official [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
(OBS 31.1.1), so building is one preset command per platform and produces a
properly bundled, ad-hoc-signed plugin.

---

## Get a pre-built plugin with no local toolchain (GitHub Actions)

If you don't want to install Xcode at all, let GitHub build the `.plugin` for you
and just download it. The CI in `.github/workflows/` builds macOS (Universal),
Windows, and Linux and uploads the results as run artifacts. No Apple Developer
account or secrets are needed — builds fall back to ad-hoc signing automatically.

1. Create a **new, public** GitHub repo (public = free macOS runner minutes).
2. Push this project to it:
   ```bash
   cd active-source-volume
   git init && git add . && git commit -m "Active Source Volume plugin"
   git branch -M main
   git remote add origin https://github.com/<you>/active-source-volume.git
   git push -u origin main
   ```
3. On GitHub → **Actions** tab → if prompted, enable workflows. Then open the
   **Dispatch** workflow → **Run workflow** → job **build** → Run.
   (This path produces the raw `.plugin` for drag-and-drop. A plain push to
   `main` instead produces a `.pkg` installer.)
4. When the run finishes (~10–15 min), open it and download the artifact
   **`active-source-volume-1.0.0-macos-universal-<hash>`** (a `.zip`).
5. Unzip it → inside is `active-source-volume-1.0.0-macos-universal.tar.xz` →
   extract that → you get **`active-source-volume.plugin`**.
6. Drag `active-source-volume.plugin` into
   `~/Library/Application Support/obs-studio/plugins/`, then clear quarantine and
   restart OBS:
   ```bash
   xattr -dr com.apple.quarantine \
     ~/Library/Application\ Support/obs-studio/plugins/active-source-volume.plugin
   ```

> This CI run is the first *real* compile of the Qt UI code. If the macOS job
> fails, open its log — the error is almost always a one- or two-line fix. The
> `-ci` presets have warnings-as-errors disabled here so a cosmetic warning
> won't block your artifact.

---

## macOS quick start (build + deploy for testing)

**Prerequisites**
- **Xcode** (full app from the App Store, not just Command Line Tools — the
  build uses the Xcode generator). After installing:
  ```bash
  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
  sudo xcodebuild -license accept
  ```
- **CMake 3.28+** and **Ninja**:
  ```bash
  brew install cmake ninja
  ```
  (No need to install Qt or OBS — the build downloads matching prebuilt OBS
  libs and Qt6 automatically, which avoids the Homebrew-Qt version-mismatch
  crash.)

**Build** (from the repo root)
```bash
cmake --preset macos
cmake --build --preset macos
```
This produces a Universal (arm64 + x86_64) bundle at:
```
build_macos/RelWithDebInfo/active-source-volume.plugin
```

> Faster arm64-only build on your M2 (optional): add
> `-DCMAKE_OSX_ARCHITECTURES=arm64` to the first command.

**Install** (copy the bundle into your user plugin folder)
```bash
mkdir -p ~/Library/Application\ Support/obs-studio/plugins
cp -R build_macos/RelWithDebInfo/active-source-volume.plugin \
   ~/Library/Application\ Support/obs-studio/plugins/
```
Restart OBS.

> Alternative: `cmake --install build_macos --config RelWithDebInfo` installs to
> the same folder (it also runs the packaging step; the manual copy above is the
> simplest path for testing).

**Troubleshooting — `ld: framework 'AGL' not found`:** newer macOS SDKs
(macOS 15 / 26) removed the legacy AGL framework that the pinned libobs still
lists as a link dependency. This repo's `CMakeLists.txt` already strips AGL from
the inherited link interface, so a fresh build is fine. (The plugin never uses
AGL.)

**Confirm it loaded** — OBS → **Help → Log Files → View Current Log**, look for:
```
[active-source-volume] Loading version 1.0.0
```
Then the dock is under **Docks → Active Source Volume** and the hotkeys are in
**Settings → Hotkeys**.

**Troubleshooting — `set_target_properties called with incorrect number of
arguments` at configure:** this is the upstream template leaving the build
number empty on a fresh local (non-CI) build. This repo already patches
`cmake/common/buildnumber.cmake` to default it to `1`. If you're on an older
copy, either re-download this repo or just pass the number explicitly:
```bash
cmake --preset macos -DPLUGIN_BUILD_NUMBER=1
cmake --build --preset macos
```

**Gatekeeper note:** the bundle is ad-hoc signed (no Apple Developer account
needed). If macOS ever quarantines it, clear the flag:
```bash
xattr -dr com.apple.quarantine \
  ~/Library/Application\ Support/obs-studio/plugins/active-source-volume.plugin
```

---

## Other platforms

Same pattern, different preset:
```bash
# Windows (PowerShell, needs VS 2022 + CMake)
cmake --preset windows-x64
cmake --build --preset windows-x64

# Linux (needs OBS + Qt6 dev packages)
cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64
```

CI workflows for all three ship in `.github/workflows/` (inherited from the
template) if you want automated signed builds later.

---

## How it works

```
program scene change ─▶ SceneTracker ─▶ finds the active browser source ─▶ ActiveBrowserController
   (transition start)                                                          │
                          ┌─────────────────────────────────────────────────────┤
                          ▼                                                       ▼
               OBS hotkeys (+/- dB, mute)                        obs-websocket vendor requests
               → Stream Deck "Hotkey" action                     → Companion dial (absolute + feedback)
```

**Which browser is "active":** the top-most **visible** source of type
`browser_source` in the live scene (groups are searched; nested scenes are not).

**dB control:** hotkeys nudge volume by a fixed dB step (default ±5 dB).

### Transition safety (no jump / no drop)

OBS applies a transition's audio crossfade as a multiplier **on top of** each
source's base volume — it never rewrites the source volume itself. So the plugin
(re)resolves the active browser at **transition start** and, with **carry-level**
on (default), writes the level then, so the incoming browser fades in already at
the right level. It never writes volume at transition-stop, which is what would
otherwise cause the "loud during the wipe, then drops when it lands" artifact.

**Carry-level** (toggle in the dock): on = the level follows you across scenes as
one continuous channel. off = each browser keeps its own level (still
artifact-free).

## Usage

1. **Bind hotkeys** — OBS → Settings → Hotkeys: *Active Browser: Volume +*,
   *Volume -*, *Toggle Mute*. Point a Stream Deck **System → Hotkey** action at
   each keystroke.
2. **Dock** — OBS → Docks → Active Source Volume shows the browser being
   controlled and its live level, plus the dB step and carry-level toggle.

### Dial via obs-websocket

Vendor `active-source-volume`:

| Request | Params | Response |
|---|---|---|
| `GetActiveBrowser` | — | `sourceName, db, muted, hasActiveBrowser` |
| `NudgeVolume` | `deltaDb` | current state |
| `SetVolume` | `db` | current state |
| `SetMute` | `muted` | current state |
| `ToggleMute` | — | current state |

Emits `ActiveBrowserChanged { sourceName, db }` on every switch.

## Verifying the three behaviours

1. **Auto-select** — open the dock, switch scenes; "Active browser" flips to the
   live scene's browser immediately.
2. **Transition-safe** — set a slow (1 s) fade, switch between two scenes that
   each have a browser; the incoming audio should fade in at the intended level
   with no jump during the wipe and no drop after.
3. **±dB hotkeys** — press +/- and watch the dock "Level" move by the step.

## Important runtime setting

Enable **"Control audio via OBS"** in each browser source's Properties, or its
audio isn't routed through the source-volume fader and the hotkeys won't be
audible.

## Config file

`~/Library/Application Support/obs-studio/plugins/active-source-volume/config.json`
```json
{ "nudge_step_db": 5.0, "carry_level": true }
```

## Notes / limitations

- Detects source type `browser_source`; top-most visible wins if a scene has
  several.
- "Active" = program (live) output. For Studio Mode preview instead, swap
  `obs_frontend_get_current_scene()` for `obs_frontend_get_current_preview_scene()`
  in `src/scene-tracker.cpp`.
- The `author`, `website`, `email`, and macOS `bundleId` in `buildspec.json` are
  placeholders — edit before publishing.

## License

GPL-2.0-or-later (matches libobs). See `LICENSE`.
