# Map Seed Selector

**D2RLoader plugin by MadMike · v0.1.36**

Choose a saved map seed from Offline character selection. Add seeds in-game, select them before starting a game, and keep the choice in the plugin configuration. Hover a row to see its decimal seed.

## Features

- **Off** leaves the game's normal seed behavior in place while keeping the selector visible.
- Named seeds appear by name. Blank or unreadable names fall back to the seed value.
- Add seeds in-game with a name and a decimal or hexadecimal seed. New entries are selected immediately.
- The selector and Add Seed button are positioned together; the label and list direction are configurable.
- The plugin does not edit character save files.

## D2RLoader setting

Keep **Generate new map layouts** turned off for the intended static map-seed behavior. When enabled, D2RLoader rerolls map layouts for new games.

## Configuration

The configuration is `d2rloader/config/map-seed-selector.toml`. The shipped template has no seed entries. The example below adds two; keep all `[[seeds]]` blocks at the end of the file.

```toml
config_version = 1
selected_seed = "character" # The visible Off option.

[ui]
position_x = 800
position_y = 0
text_position = "right"
expand_direction = "below"

[[seeds]]
name = "Meph"
seed = 1056279548

[[seeds]]
name = "Ancient Tunnels"
seed = 0x3FC14219
```

`position_x` and `position_y` place the Add Seed button's upper-left corner in UI pixels, measured from the screen's upper-left. This is the anchor for the button, selector, and label; the selector sits 56 pixels to the right. `text_position` accepts `above`, `below`, `left`, or `right`. `expand_direction` accepts `above` or `below`.

Seed values are unsigned 32-bit integers in decimal or hexadecimal. Names are optional; use unique printable ASCII names for readable entries. `selected_seed` stores the current choice; `"character"` is the stored value for **Off**.

The in-game Add Seed prompt accepts a name and seed. Tab switches fields, Enter adds the seed, and Escape closes the prompt. A successfully saved seed is appended to the end of the TOML file and becomes the selected option immediately.

## Compatibility

The plugin targets D2RLoader plugin ABI 4 and is built with the official PluginSDK 0.4.0. It operates in Offline character selection; the seed override is disabled on the online selection route. Before installing its game hooks, it checks the expected instruction bytes and disables seed injection if the layout is unsupported. It uses the native dropdown callbacks when available and a D2R-styled fallback otherwise.

## Build

Requirements: Windows x64, CMake 3.29 or newer, Visual Studio 2022 or newer with the MSVC C++ toolchain, Windows SDK, and MASM x64 assembler. The first configure fetches the pinned PluginSDK revision.

From a Visual Studio Developer PowerShell in the plugin directory:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target map_seed_selector
```

The DLL is written to `build/Release/d2rl-map-seed-selector.dll`.

## Install

Copy the DLL to D2RLoader's global `d2rloader/plugins/` folder or the active mod's plugin folder. The configuration file is created on first load if it does not exist. Restart D2RLoader after replacing the DLL.

## License

MIT. See [LICENSE](LICENSE).

