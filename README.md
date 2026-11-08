# Map Seed Selector

**D2RLoader plugin by MadMike · v0.1.53**

Choose a saved map seed from Offline character selection, or add one from the Add Seed prompt. The selected seed is applied when the game creates a map.

## Features

- **Off** uses the game's normal per-character map behavior while keeping the selector visible.
- Saved seeds can have names; blank or unreadable names display the seed value.
- Add seeds in-game with a name and a decimal or hexadecimal seed. New entries are saved and selected immediately.
- The question button on the Add Seed prompt opens the last-seed panel. It displays the last gameplay seed in decimal and copies it to the clipboard. Click the question button again to close the panel.
- The selector, label, and Add Seed button share one position anchor. Label placement and dropdown expansion direction are configurable.
- The plugin does not modify character save files.

## D2RLoader setting

Keep **Generate new map layouts** turned off for the intended static map-seed behavior. When enabled, D2RLoader rerolls map layouts for new games.

## Configuration

The configuration is `d2rloader/config/map-seed-selector.toml`. The shipped template has no seed entries. Add seed blocks at the end of the file:

```toml
config_version = 1
selected_seed = "character" # The visible Off option.

[ui]
position_x = 800
position_y = 0
text_position = "right"
expand_direction = "below"

[[seeds]]
name = "My first map"
seed = 123456789

[[seeds]]
name = "Another map"
seed = 0x12345678
```

`position_x` and `position_y` place the Add Seed button's upper-left corner in UI pixels, measured from the screen's upper-left. The selector and label move with it; the selector sits 56 pixels to the right. `text_position` accepts `above`, `below`, `left`, or `right`. `expand_direction` accepts `above` or `below`.

Seed values are unsigned 32-bit integers in decimal or hexadecimal. Names are optional and must be unique printable ASCII. `selected_seed` stores the current choice; `"character"` is the stored value for **Off**.

In the Add Seed prompt, Tab switches between fields, Enter adds the seed, and Escape closes the prompt. A successfully saved seed is appended to the TOML file and selected immediately.

## Compatibility

The plugin targets D2RLoader plugin ABI 4 and uses the official PluginSDK 0.4.0. It operates in Offline character selection; seed overrides are disabled on the online selection route. Before installing game hooks, it checks expected instruction bytes and disables seed injection if the layout does not match. The selector uses native dropdown callbacks when available, with a D2R-styled fallback.

## Build

Requirements: Windows x64, CMake 3.29 or newer, Visual Studio/MSVC, Windows SDK, and MASM x64 assembler. The first configure fetches the pinned PluginSDK revision.

From an x64 Visual Studio Developer Command Prompt in the plugin directory, configure the single-configuration NMake generator explicitly for Release:

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --target map_seed_selector
```

`NMake Makefiles` is a single-configuration generator, so `CMAKE_BUILD_TYPE=Release` selects the Release build. The DLL is written to `build/d2rl-map-seed-selector.dll`.

## Install

Copy `d2rl-map-seed-selector.dll` to D2RLoader's global `d2rloader/plugins/` folder or the active mod's plugin folder. Copy the seedless `map-seed-selector.toml` template to `d2rloader/config/` if the plugin has not created a configuration yet. Restart D2RLoader after replacing the DLL.

## License

MIT. See [LICENSE](LICENSE).
