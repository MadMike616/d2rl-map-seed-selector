# Hook Inventory

Plugin release: **0.1.52**  
Inventory date: **2026-11-07**  
Source reviewed: `src/map_seed_selector.cpp`, `src/map_seed_selector.asm`, and `src/plugin.cpp`.

## Native D2R hooks

| Target | Witness checked | Overwrite | Role |
| --- | --- | --- | --- |
| `D2R+0x326E9E` | 129-byte DRLG initializer witness at `D2R+0x326E89`; hook begins 21 bytes into it | 15 bytes | Applies the chosen seed at the native DRLG seed initialization path. Required. |
| `D2R+0x326EB3` | Same DRLG initializer witness; hook begins 42 bytes into it | 15 bytes | Observes the seed after the native DRLG write. Optional; the override can remain active if this observer cannot be installed. |
| `D2R+0x48AADB` | 18-byte seed-ingress witness at the hook address | 18 bytes | Intercepts the `Game+0x12C` seed consumer. Required. |
| `D2R+0x4210CB` | 23-byte saved-seed-copy A witness at `D2R+0x4210BA` | 6 bytes | Intercepts saved seed copy A into `Game+0x12C`. Required. |
| `D2R+0x532443` | 23-byte saved-seed-copy B witness at `D2R+0x532432` | 6 bytes | Intercepts saved seed copy B into `Game+0x12C`. Required. |
| `D2R+0x532B74` | 23-byte saved-seed-copy C witness at `D2R+0x532B63` | 6 bytes | Intercepts saved seed copy C into `Game+0x12C`. Required. |

The plugin checks the expected bytes before installing these inline hooks. A mismatch in a required witness disables seed selection without installing the unqualified hook sites. The code does not enforce a D2R version number. Source comments record reverse-engineering references to D2R 3.2.92777 and matching saved-seed fingerprints in 3.3.93847.

## Operating-system input hook

While the Add Seed prompt is active, `plugin.cpp` installs a `WH_KEYBOARD_LL` low-level keyboard hook with `SetWindowsHookExW`. It handles prompt field navigation and text entry. The hook is removed when the prompt closes or the plugin unloads.

## Change record

No native hook address, witness, or overwrite-size changes were found between the v0.1.36 release source and v0.1.52. The hook targets above were checked against the release source for this inventory. The low-level keyboard hook target is also unchanged.
