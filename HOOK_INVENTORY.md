# Hook Inventory — Map Seed Selector

## Plugin metadata

| Field | Value |
|---|---|
| Plugin name | Map Seed Selector |
| Plugin ID | map-seed-selector |
| Release version | 0.1.53 |
| Inventory updated | 2026-11-08 |
| Source revision | Release tag v0.1.53 |
| D2R build(s) audited | Not validated |
| D2RLoader SDK / Plugin ABI | PluginSDK 0.4.0 / Plugin ABI 4 |
| Audit status | Complete |

## Scope and method

Inspected all plugin source under `src/`, `CMakeLists.txt`, `README.md`, and the release configuration template. Enumerated native hook-install calls and operating-system hook-install calls, including the validation guards and cleanup paths. UI and lifecycle callbacks that do not install or replace a native or OS target are excluded.

Numeric addresses below are RVAs. Hook-site behavior and guards were checked against the release source. The inventory records source-level targets; no game runtime was launched for this release.

## Hook sites

| Site ID | Type | Target | Address / identifier | Operation | Guard / validation | Source reference | Notes |
|---|---|---|---|---|---|---|---|
| H-01 | Inline | DRLG seed initialization | RVA 0x326E9E | Installs the selected seed at the native seed-initialization path. | Checks the 129-byte DRLG initializer witness at RVA 0x326E89 before installation. | `src/map_seed_selector.cpp` — `InstallSeedHook` | Required seed override hook. |
| H-02 | Inline | DRLG seed post-write observer | RVA 0x326EB3 | Observes the native seed write. | Checks the same 129-byte witness at RVA 0x326E89 before installation. | `src/map_seed_selector.cpp` — `InstallSeedHook` | Optional observer; the override can remain active if this hook cannot be installed. |
| H-03 | Inline | Game seed ingress at Game+0x12C | RVA 0x48AADB | Intercepts the seed consumer. | Validates the expected instruction bytes at the hook site before installation. | `src/map_seed_selector.cpp` — `InstallGameSeedIngressHook` | Required seed-selection path. |
| H-04 | Inline | Saved-seed copy A | RVA 0x4210CB | Intercepts the saved-seed copy into Game+0x12C. | Checks the 23-byte witness at RVA 0x4210BA and expected hook-site bytes. | `src/map_seed_selector.cpp` — `InstallEarlierSeedPathHooks` | Required saved-seed path. |
| H-05 | Inline | Saved-seed copy B | RVA 0x532443 | Intercepts the saved-seed copy into Game+0x12C. | Checks the 23-byte witness at RVA 0x532432 and expected hook-site bytes. | `src/map_seed_selector.cpp` — `InstallEarlierSeedPathHooks` | Required saved-seed path. |
| H-06 | Inline | Saved-seed copy C | RVA 0x532B74 | Intercepts the saved-seed copy into Game+0x12C. | Checks the 23-byte witness at RVA 0x532B63 and expected hook-site bytes. | `src/map_seed_selector.cpp` — `InstallEarlierSeedPathHooks` | Required saved-seed path. |
| H-07 | Win32 input-message | Low-level keyboard input for Add Seed prompt (`WH_KEYBOARD_LL`) | N/A | Installs a low-level keyboard hook for prompt navigation and text entry. | Active only while the Add Seed prompt is open; the hook is removed when the prompt closes or the plugin unloads. | `src/plugin.cpp` — `AddSeedKeyboardHook` | Prompt input is consumed only while the prompt is active and the game is foreground. |

## Known collision overlaps

| Other plugin | Shared target | Relationship | Evidence / notes |
|---|---|---|---|
| RuffDood Remote Stash | `WH_KEYBOARD_LL` | Possible shared channel | Both sources install a low-level keyboard hook. The seed selector consumes keys only while its Add Seed prompt is active; no shared native RVA is involved. |

## Audit notes

The required native-hook fingerprints fail closed and disable seed selection when expected bytes do not match. The plugin does not enforce a D2R version number. Native hook addresses and expected bytes are unchanged from v0.1.52. No in-game validation was performed for this inventory.
