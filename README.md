# Map Seed Selector

**Version 0.1.36** is the D2RLoader plugin by MadMike. It adds a D2R-styled seed selector to Offline character selection, shows `Seed:` followed by the decimal seed on hover, and supplies the selected seed through guarded game hooks. A hook at `D2R+0x48AADB` updates `Game+0x12C` as the map-generation path reads it, before the existing DRLG initializer hook. The plugin does not write character save files directly. Seed entries come from `d2rloader/config/map-seed-selector.toml`. Closed selector/list trees are trimmed to eight after a short unload cooldown. The Add Seed prompt uses centered rendered fields and a private keyboard hook so Tab, Enter, and Escape behave inside the prompt even on loaders that omit the SDK input callbacks.

Add a `[[seeds]]` entry for each map:

```toml
config_version = 1
selected_seed = "My map"

[ui]
# Screen coordinates of the Add Seed button's upper-left corner, in UI pixels.
# The selector and "Map Seed" text move with this anchor, 56 pixels to its right.
position_x = 800
position_y = 0
# Place the "Map Seed" label above, below, left, or right of the selector.
text_position = "right"
expand_direction = "below"
[[seeds]]
name = "My map"
seed = 0x12345678

[[seeds]]
name = "Another map"
seed = 305419896
```

Names are optional. Printable ASCII names should be unique; blank or non-ASCII names display the seed value instead and use a seed-based `selected_seed` key. Seeds are unsigned 32-bit values in decimal or hexadecimal. The dropdown remembers its selection in `selected_seed`; `"character"` is the stored compatibility value for `Off`.
Set `text_position` to `"above"`, `"below"`, `"left"`, or `"right"` to place the `Map Seed` label relative to the selector. Set `position_x` and `position_y` to the screen coordinates of the Add Seed button's upper-left corner, in UI pixels. This button anchors the whole group; the selector and label move with it, with the selector 56 pixels to its right at the same Y. The screen's upper-left is `(0, 0)`; X increases to the right and Y increases downward. Coordinates may range from 0 to 16384. Set `expand_direction` to `"above"` or `"below"` to choose where the option rows open. When using `"above"`, place the button anchor far enough down that the complete list remains on screen.

The temporary **Add Seed** icon button appears on the Offline character-select screen. Its supplied artwork is scaled to 56 by 56 UI pixels, the same height as the collapsed selector. The `position_x` and `position_y` values above position the button and anchor the selector and label as one group. The prompt accepts a name and an unsigned 32-bit seed in decimal or `0x` hexadecimal, with centered text that expands around the middle as you type. Press Tab to cycle between the fields, Escape to close the prompt, or Enter in either field to add the seed. Choosing **Add Seed** appends a `[[seeds]]` block to the end of the plugin TOML and closes the prompt after a successful save. Reopen it to add another seed. The selector switches to a fresh pre-registered panel layout after saving, so the new choice appears immediately without restarting D2RLoader. If the panel switch fails, the prompt reports that a restart is needed. The button remains available when no seed entries are configured.

The Add Seed prompt uses the four-state ModalButton atlas supplied in `resources/modal-button-source.png`, with its half-size counterpart in `resources/modal-button-lowend-source.png`. The Add Seed and Cancel controls use separate 265-by-70 UI-pixel hitboxes with even spacing. Its prompt panels rotate through a bounded pool so centered text can redraw while typing; closed panels are unloaded after the same cooldown policy. Their SpA1 resources can be regenerated with `tools/build_modal_button_sprites.ps1`.

## D2RLoader setting

For the intended static map-seed behavior, keep **Generate new map layouts** turned off in D2RLoader. When enabled, Loader rerolls map layouts for each new game.

## Safety and compatibility

The plugin checks an 18-byte runtime fingerprint at `D2R+0x48AADB` before installing the early `Game+0x12C` hook. That hook records the field's current value, writes the selected seed there, and then lets the original code read it. The plugin also checks a 129-byte runtime fingerprint for the DRLG seed initializer; it replaces the initializer's `{seed, 0x29A}` state before the game's normal LCG step. If either required fingerprint does not match, the picker stays unavailable. The log records both the early field write and the later DRLG seed value. In-game validation is still required to confirm map persistence across difficulty changes.

The override is active only after entering the Offline character-selection flow and is disabled for the online character-selection route. It does not rewrite character saves. When WidgetService exposes the dropdown callback functions, the picker uses D2R's native `DropdownListWidget`. Older services, including the 64-byte table reported by D2RLoader 1.3.1-beta, use a popup built from the game's `dropdownbutton` and `dropdowntile` artwork. After data tables load, the fallback validates the game's archive-function signatures, reads the six source sprites, and registers composite sprites with full-width normal, hover, pressed, and disabled states. An unsupported function or sprite layout disables the fallback instead of using unchecked addresses or malformed art. The popup closes when a choice is made, when Escape is pressed, or when the player clicks outside it. This fallback uses the loader's existing panel and widget services and does not require a loader update. Selecting `Off` keeps the picker panel visible while passing the DRLG seed initializer through unchanged.

## Build

Requirements:

- Windows x64
- CMake 3.29 or newer
- Visual Studio 2022 or newer with the MSVC C++ toolchain, Windows SDK, and MASM x64 assembler
- Network access during the first CMake configure to fetch the pinned D2RLoader PluginSDK revision

From a Visual Studio Developer PowerShell in this directory:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target map_seed_selector
```

The DLL is written to `build/Release/d2rl-map-seed-selector.dll`. The build pins the official PluginSDK to commit `03a5af9e0feb3f95f95e148e330e01f6635950df` (SDK 0.4.0). The plugin ABI remains version 4. It checks the WidgetService size before using optional dropdown functions and falls back to the panel list when dropdown callbacks are absent. The Add Seed prompt uses a plugin-owned keyboard hook when the loader omits its later SDK text-editing and key-listener callbacks.

## Install and verify

Copy the DLL to D2RLoader's global `d2rloader/plugins/` folder or the active mod's plugin folder. The config is created on first load. Seeds added through the in-game prompt should appear in the dropdown immediately. Check that the selector persists its selection and changes the generated map after starting a new game. Also check that it stays unavailable in online character selection and closes when leaving Offline selection.

