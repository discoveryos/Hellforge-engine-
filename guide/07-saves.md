# 07 — Settings, saves and persistence

## Console storage layer

Phase 13 added a console-style storage service modeled on PS5/Xbox/Switch
account storage. It lives in `neo/hflib/console/hfConsoleServices.h` and is
accessed through `hfconsole::HfConsole_GetServices()`.

API used by the UI:

```
WriteSlot(userId, name, data)   ReadSlot(userId, name, ...)
DeleteSlot(userId, name)        SlotSize(userId, name)
```

Slots are **per account**, not per machine. If no platform service is
installed, `HfConsole_GetServices()` returns a deterministic in-memory stub —
perfect for tests, but note that **offline saves do not persist across
processes** unless a real platform service is installed.

## UI slots

- Settings slot: `"settings"`, managed by `hfUiSettings`.
- Game slots: `save1` … `save8`, managed by `hfUiSaveSlots`.

Settings keys (`hfUiSettings.h`):

```
music_volume, sfx_volume, mouse_sensitivity, width, height,
quality, fullscreen, vsync
```

`ApplyToCVars()` maps them to `s_volume_dB`, `sensitivity`, `r_fullscreen`,
`r_customWidth`, `r_customHeight`, `r_vidMode = -1`, `opt_applyPreset` and
`r_swapInterval`. Options edits are written once per changed value, not every
idle frame.

## The arena save payload

`neo/hflib/game/hfArenaSave.h` is a header-only, allocation-free struct:

```
HF_ARENA_SAVE_VERSION = 1
HF_ARENA_SAVE_TEXT_MAX = 4096
```

Methods:

| Method | Purpose |
|--------|---------|
| `Clear()` | Zero the snapshot |
| `Capture(arena)` | Copy all mutable arena state into the snapshot |
| `Restore(arena)` | Write the snapshot back into a sim |
| `Equals(other)` | Field-wise comparison |
| `Serialize(text, max)` | Write versioned text record |
| `Parse(text)` | Read record; returns false on malformed input |
| `FieldKeys(count)` | Manifest of on-disk keys |
| `SchemaRoundTrip()` | Serialize → check every key → parse → `Equals` |
| `SaveToSlot` / `LoadFromSlot` | Console storage helpers |

Text format: one `key = value` line per field; floats are stored as their
32-bit bit patterns in hex so the round trip is exact on a given binary while
remaining readable and diffable. Version 1 records are still parsed by newer
code, with unknown keys ignored and missing new fields defaulted.

The 16 declared keys:

```
version, player_x, player_y, hit_points, score, wave, ticks, game_over,
won, enemy_count, bullet_count, fire_cooldown, intent, rng, enemies, bullets
```

## Why equality is field-wise

`HfArenaEnemy` is 12 bytes and `HfArenaBullet` is 20 bytes in this build. They
contain **tail padding**, which is not initialized predictably. A `memcmp` of
two logically equal snapshots reports differences (this actually broke a test:
`SNAP memcmp e=-1 b=0 sizes 12/20`). `Equals()` therefore compares every field
explicitly, and the harness mutates each field to prove it notices.

## Adding a new saved field (v2 checklist)

When the game adds ammo, dodge cooldown, pickups, combo, enemy `kind`, etc.:

1. Add the field to the sim (`hfArena.h`) and the snapshot (`hfArenaSave.h`).
2. Update `Clear()`, `Capture()` and `Restore()`.
3. Add it to `Serialize()` and `Parse()` with a version-1 default.
4. Add it to `Equals()` — field by field.
5. Add its key to `FieldKeys()` — the manifest that tests walk.
6. Bump `HF_ARENA_SAVE_VERSION` to 2.
7. Add a v1 fixture test and a v2 round-trip test; run the 4-way matrix.

`SchemaRoundTrip()` plus the field-mutation loop in `Test_RiskMitigations()`
catch an omission in steps 3–5 automatically.

## Save/load UI flow

`hfUiFrontend` actions `SAVE_SLOT_LOAD`, `SAVE_SLOT_WRITE` and
`SAVE_SLOT_DELETE` flow through the host handlers in `hfCommon_menu.cpp`; the
UI remembers the last slot and shows status. The host owns policy (which slot,
when to switch screens); the UI owns presentation; `hfArenaSave` owns the
payload format. Keep those three roles separate.

## Headless testing

Everything above uses only plain data and the console interface, so
`Test_UiPersistence` and `Test_FrontendIntegration` run in the harness with no
window or GL context. That is why the payload must stay free of pointers and
host globals.
