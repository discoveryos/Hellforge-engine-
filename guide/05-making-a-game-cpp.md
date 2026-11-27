# 05 — Making a game in C++

There are two ways to put a game in this engine. Path A is working today;
path B is the shipping mechanism once you have a map and a compiled game
module. Both use the same simulation API.

## Path A — drive the built-in arena (recommended now)

`neo/hflib/game/hfArena.h` is a header-only, allocation-free simulation. It is
the best place to prototype because the same code runs in the engine and in
the headless harness. For anything beyond a prototype, include
`neo/hflib/game/hfArenaGame.h` and call the facade instead of wiring engine
internals yourself:

```cpp
#include "../../hflib/game/hfArenaGame.h"

HfArenaGame::Start( seed );                       // reset + clear command

HfArenaInput cmd;
cmd.moveX = 1.0f;                                  // WASD / left stick
cmd.aimX = 1.0f; cmd.aimY = 0.0f;                  // mouse / right stick
cmd.fire = true;                                   // LMB / trigger
cmd.dodge = false;                                 // Space / A button

HfArenaGame::Run( cmd, 60 );                       // 60 fixed gameplay ticks
HfArena &arena = HfArenaGame::Active();            // read state for HUD/render
```

`Run()` applies the same command repeatedly (clearing one-shot dodge after
the first step). A game that owns its own state can instantiate `HfArena`
directly and call `Reset(seed)` / `Step(cmd)`; nothing in the API requires the
facade's singleton.

### 1. State and constants

The sim owns fixed-size arrays and named constants: `MAX_ENEMIES`,
`MAX_BULLETS`, `MAX_PICKUPS`, `TOTAL_WAVES`, `MAX_AMMO`, `DODGE_COOLDOWN`,
`COMBO_TIMEOUT` and friends. Structured state includes enemy kind and y,
bullet owner, pickups, facing, combo, kills and wave-delay. Keep it POD-like:
no pointers, no virtuals, no OS calls.

### 2. Commands are separate from auto-pilot

`HfArena::Step(const HfArenaInput&)` is the gameplay entry point. The frontend
samples `HfArenaInput` only while PLAYING and passes it to `Step(cmd)` on each
fixed 60 Hz tick. `hfUiFrontend::HandleGameInput()` consumes WASD/left stick,
mouse/right stick, LMB/trigger and Space, but leaves `K_GRAVE`, `K_ESCAPE` and
`K_JOY9` to the host.

The zero-argument `Step()` is still the deterministic attack/retreat
auto-pilot used by `base/script/arena.eng` and the older tests. It is kept
separate on purpose so a gameplay tweak cannot silently change a shipped
script replay.

### 3. Expose decisions to hfscript

The scalar hosts in `neo/script/HfScript_Game.h` include:

```
arena_set_move(x, y)   arena_set_aim(x, y)
arena_set_fire(flag)   arena_set_dodge(flag)
arena_step_cmd()       arena_clear_command()
arena_enemy_kind(i)    arena_pickup_kind(i)
arena_ammo()           arena_combo()   arena_kills()
```

No arrays or objects cross the script boundary. The hosts operate on
`HfArenaGame::Active()` and `HfArenaGame::Command()`, so C++ game code and an
hfscript bot share one engine instance.

### 4. Extend the save payload

See [chapter 07](07-saves.md). Every new field must appear in the manifest,
serializer, parser, restore path, equality check and tests:

```
FieldKeys → Capture → Serialize → Parse → Restore → Equals → tests
```

`hfArenaSave` is version 2 and still parses version 1 with defaults for the
new fields. Never use `memcmp` in `Equals()`: `HfArenaEnemy`, `HfArenaBullet`
and `HfArenaPickup` carry tail padding, so byte-wise comparison produces false
mismatches.

### 5. UI and presentation

- HUD slots are `HF_UI_HUD_SCORE`, `HEALTH`, `WAVE`, `AMMO`, `DODGE`, `COMBO`
  and `KILLS`; `SyncHud()` mirrors arena fields every frame.
- RESULTS adds `GetResultSummaryLabel()` with wave, kills, best combo and ammo.
- The quad overlay in `DrawArenaOverlay()` (`hfUiDrawPass.cpp`) maps ±120 world
  units into a centred viewport square and colours enemy/pickup/bullet kinds,
  plus facing and dodge cues. Headless runs never call it.

### 6. Tests

`Test_TwinStickArena()` in `neo/script/tests/hfstest_features.cpp` covers
command determinism, archetypes, boss wave, pickups, scoring, dodge/death/win,
frontend routing, HUD/results text, v1/v2 saves and `HfArenaGame`. Run the 4-way
matrix afterwards ([chapter 08](08-tests.md)).

## Path B — game module + map

`neo/framework/hfGameModule.h` is the public DLL interface. A shipped build
sets `ui_startMap` to a real map and installs a game module; the module can use
`HfArenaGame` or its own `HfArena` instance. This checkout has no `.map` assets
and no game DLL, so START uses the engine-owned facade arena instead. Keep
session commands (`map`, `devmap`, `victory`) in `gameReturn_t`; do not bake
map names into the game code.
