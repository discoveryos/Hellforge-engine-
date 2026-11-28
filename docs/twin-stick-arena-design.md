# Twin-stick Arena — design & implementation plan

**Status:** phases 1–6 implemented; follow-up pass added minimal real map/module
path (`ui_startMap` default `maps/arena`, `hfArenaStubGame`, hand-authored
`base/maps/arena.proc`), pure deterministic SFX cues, and presentation-only
visual juice/colorblind palettes (see sections 4.2, 5 and follow-up notes).
**Target:** the first playable game built on the Phase 9–18 engine substrate.
**Definition of done:** start the engine, land on TITLE, press PLAY, steer and
shoot with mouse/keyboard or gamepad, survive escalating waves, pause/resume,
see results, save/load a run, and rewind deterministically from a fixed seed in
the headless harness.

---

## 1. Why this game

`HfArena` is already a deterministic top-down simulation with a player, enemy
waves, bullets, score, health and a 60 Hz fixed step. The frontend already owns
the title/HUD/pause/results screens, keybinds, settings, per-account save slots
and gamepad focus. The remaining work is gameplay, not infrastructure: growing
the sim from a demo into a game, giving it real input and a visible arena, and
extending the save payload as the state grows.

That makes this the shortest path from “engine with a GUI” to “engine with a
game”, and every step is testable headlessly.

## 2. What exists today

| Piece | Location | What it gives us |
|-------|----------|------------------|
| Arena sim | `neo/hflib/game/hfArena.h` | POD state, deterministic LCG, fixed-step `Step()`, 5 waves, player/enemy/bullet caps |
| Script bridge | `neo/script/HfScript_Game.h` | scalar host functions (`arena_reset`, `arena_step`, `arena_attack`, …) |
| Frontend | `neo/hflib/ui/hfUiFrontend.h` | TITLE/MENU/LOADING/PLAYING/PAUSED/RESULTS/SAVES, HUD labels, fixed 60 Hz sim stepping, deterministic action log |
| Save payload | `neo/hflib/game/hfArenaSave.h` | versioned text snapshot + per-account slot I/O |
| Host policy | `neo/framework/hfCommon_menu.cpp` | START → `map`/`devmap` (`ui_startMap`), save/load handlers, engine-owned arena |
| Widgets/input | `hfUiBuilder.h`, `hfUiInput.h`, `hfUiWidget.h` | buttons, sliders, labels, pointer capture, focus traversal, `SE_JOYSTICK` left/right stick events |
| Draw pass | `neo/renderer/hfUiDrawPass.cpp` | pixel-space quads, coverage text, rounded/nine-patch panels |
| Tests | `neo/script/tests/hfstest_features.cpp` | `Test_Game`, `Test_UiFrontend`, `Test_UiPersistence`, `Test_FrontendIntegration` |

**Constraints from the current checkout:** no dmap compiler and no external game
DLL are shipped. The checkout now includes a minimal hand-authored
`base/maps/arena.proc`/`.map` placeholder and a built-in `hfArenaStubGame` so
`ui_startMap` can take the real session map-load path when no game module is
installed. Full shipped content still requires compiled map assets and a real
game module; the engine-owned `HfArenaGame::Active()` remains the deterministic
simulation facade.

## 3. Gameplay design

- **Movement:** WASD / left stick; normalized, capped speed; arena bounds ±120.
- **Aim:** mouse position / right stick; player faces the aim vector.
- **Fire:** LMB / right trigger; cooldown-limited, ammo-consuming bullets.
- **Dodge:** Space / A button; short burst with an i-frame window and cooldown.
- **Enemies:** three archetypes driven by the same enemy slot:
  - *chaser* — current behaviour, contact damage;
  - *spitter* — keeps range, fires slow bullets;
  - *brute* — high HP, slow, heavy contact damage.
- **Wave director:** wave N spawns a deterministic mix from the seeded RNG;
  every fifth wave is a boss (one brute with extra HP plus escorts).
- **Pickups:** health, ammo and score shards dropped by killed enemies; capped
  count and deterministic placement.
- **Scoring:** +10 per chaser, +25 spitter, +50 brute, wave-clear bonus;
  combo multiplier decays over ticks.
- **Run end:** death → RESULTS (loss); wave 5 boss dead → RESULTS (win);
  PLAY AGAIN resets with a new seed.

Keep all tunable values as named constants in `HfArena` so a replay is
bit-identical on the same binary.

## 4. Architecture

### 4.1 Separate commands from state

Add a small POD command struct:

```cpp
struct HfArenaInput {
    float moveX, moveY;   // -1..1, normalized by the sim
    float aimX, aimY;     // direction from player to cursor/stick
    bool  fire;
    bool  dodge;
};
```

- `HfArena::Step( const HfArenaInput &cmd )` becomes the gameplay entry point.
- Keep the existing zero-argument `Step()` as “auto-pilot” (current nearest-enemy
  behaviour) so the harness and old tests stay valid.
- Input is *sampled* by the host/frontend once per rendered frame and consumed
  by the fixed-step sim; it is never stored in the save payload.

### 4.2 Keep hfscript as the strategy layer

Expose the new commands as scalar host functions so the English dialect can
drive a demo bot and tests can replay decisions:

```
arena_set_move(x, y)     arena_set_aim(x, y)
arena_set_fire(flag)     arena_set_dodge(flag)
arena_step_cmd()         arena_clear_command()
arena_enemy_kind(i)      arena_pickup_kind(i)
arena_ammo()             arena_combo()        arena_kills()
```

C++ game code can skip hfscript entirely and include
`neo/hflib/game/hfArenaGame.h`:

```cpp
HfArenaGame::Start( seed );
HfArenaGame::Run( command, steps );
HfArena &arena = HfArenaGame::Active();
```

The facade owns the engine's default arena and command buffer, so a future game
module can drive the same simulation without touching the frontend, save slots
or renderer. Structured state stays in C++; scripts only issue scalars. This
preserves the engine’s scripting contract and gives a non-GPU way to validate
gameplay.

### 4.3 Save payload versioning

`hfArenaSave` grows with the sim (new fields, enemy `kind`, pickups, ammo,
dodge cooldown, combo). Bump to version 2, keep `Parse()` accepting version 1
and defaulting the new fields, so existing slots still load. Determinism checks
must include a v1 fixture.

### 4.4 Rendering

Two options, in order of preference:

1. **Draw pass overlay first:** add an `ArenaLayer` to `hfUiDrawPass` that maps
   world coordinates (±120) into a centred viewport and emits coloured quads
   for player, enemies, bullets and pickups. No assets, works immediately.
2. **Scene/renderer path later:** once `.map` content exists, move the arena into
   an `hfWorld` with mesh renderers and camera, driven by `hfRenderCullSystem`.

Option 1 is the milestone target; option 2 is a follow-up, not a prerequisite.

### 4.5 Frontend changes

- Add HUD slots for ammo, dodge cooldown, combo and a wave banner.
- Route mouse move/buttons and `SE_JOYSTICK` right-stick/trigger to an input
  sampler while PLAYING (today the frontend only handles UI navigation).
- RESULTS already reads `arena->won`; add score/kills/wave summary.
- PAUSED already freezes the sim; verify dodge/fire state survives resume.

## 5. Implementation phases

Each phase keeps the 4-way matrix green (`ALL PASSED`, plain/headless ×
default/Jolt) and ends with both engine binaries rebuilt.

1. **Data model ✅** — `HfArena` now has enemy/pickup/bullet kinds, facing,
   ammo, dodge, combo, kills and wave-director fields; `Step(cmd)` is the
   gameplay entry point and `Step()` remains the legacy auto-pilot.
2. **Gameplay rules ✅** — chaser/spitter/brute/boss behaviour, seeded wave
   mixing, boss escorts, ranged enemy bullets, pickups, combo scoring, wave
   clear bonus and win/loss transitions; `Test_TwinStickArena()` covers 20–40
   tick scripted scenarios.
3. **Input plumbing ✅** — the frontend feeds its sampled `HfArenaInput` to
   `Step(cmd)`; WASD/left stick, mouse/right stick, LMB/trigger fire and
   Space dodge are routed only in PLAYING, with reserved keys left free.
4. **Presentation ✅** — `hfUiDrawPass` colours archetypes/pickups/bullets and
   draws facing/dodge cues; HUD adds ammo, dodge, combo and kills, and RESULTS
   adds a run summary. Headless draw remains a no-op.
5. **Persistence ✅** — `hfArenaSave` is version 2 with v1-compatible parsing;
   `FieldKeys()`/`Equals()` cover every new field and replay-after-load is
   asserted.
6. **Game-facing API ✅** — the six phases live in the engine; a future game
   uses `HfArenaGame` and the scalar `arena_*` hosts instead of rewriting
   engine internals. With no `.map` assets or game DLL in this checkout, START
   still uses the engine-owned facade arena; `ui_startMap` remains the hook for
   a shipped map/module.

## 6. Tests to add

- `Test_Game`: legacy `.eng` auto-pilot still terminates deterministically.
- `Test_TwinStickArena`: command determinism and bounds; archetypes, boss wave,
  spitter bullets, pickups cap, scoring, death/i-frame/win transitions;
  frontend command routing; HUD and RESULTS summary text; v2 save round-trip,
  v1 slot compatibility, replay after load; `HfArenaGame` and hfscript command
  hosts share one engine instance.
- `Test_FrontendIntegration`: v2 slot text/round-trip and field-wise equality.
- Matrix requirement unchanged: no FAIL lines in any of the four runs.

## 7. Risks / open questions

### Mitigated before phase 1

- **Input routing — MITIGATED.** `HfArenaInput` is now a plain command POD in
  `neo/hflib/game/hfArena.h`. `hfUiFrontend::HandleGameInput()` samples
  WASD/mouse/sticks only while PLAYING and explicitly leaves `K_GRAVE`,
  `K_ESCAPE` and `K_JOY9` unconsumed. `hfCommonLocal::MenuEvent()` calls it on
  the PLAYING path, so console and pause keep priority. Commands reset on every
  state transition; no command crosses a screen boundary. Covered by
  `Test_RiskMitigations()` (key, stick, mouse, fire/dodge, reserved keys,
  pause reset).
- **Draw-pass ownership — MITIGATED.** `hfUiDrawPass` owns an explicit
  `(source, arena, enabled)` triple. The host installs the live arena once and
  derives `enabled` from the frontend state each frame in `FrontendFrame()`;
  `ShouldDrawGameplayOverlay()` delegates to the pure `GameplayOverlayGate()`
  so the policy is pinned headlessly without linking GL. The minimal quad
  overlay draws under the HUD and is a no-op without a source. The pure gate's
  full truth table is asserted in `Test_RiskMitigations()`.
- **Save growth — MITIGATED for today's schema.** `hfArenaSave::FieldKeys()`
  is the manifest of the on-disk fields and `SchemaRoundTrip()` verifies every
  declared key is serialized and restores to an equal snapshot. The harness
  mutates each field and requires `Equals()` to notice. Field-wise equality
  remains mandatory: `memcmp` is wrong because `HfArenaEnemy`/`HfArenaBullet`
  have tail padding. A v1 record with an unknown key still parses with
  defaults. **Ongoing discipline:** when phase 5 adds fields, update
  `Serialize`, `Parse`, `Restore`, `Equals` *and* `FieldKeys()` together.

### Accepted constraints

- **Scope:** a real `.map` and game DLL are out of scope until phase 6; the
  fallback arena is the deliverable for phases 1–5.
- **No GPU here:** visuals can only be validated structurally on this host;
  runtime checks need a machine with OpenGL. The draw gate is testable now;
  visual composition is not.

### Verification

`Test_RiskMitigations()` joins the harness suite: `ALL PASSED`, plain and
`--headless`, default and Jolt (4/4 matrix). Both engine binaries rebuilt.

## 8. Follow-up: real map/module path, SFX, and visual juice

- **Real map/module path (minimal):** `ui_startMap` defaults to `maps/arena`.
  When START is pressed and no `hfGame` is installed, the host installs a
  minimal built-in `hfArenaStubGame` (`neo/framework/hfArenaStubGame.h`) that
  implements the required `hfGame` surface and keeps the arena running through
  the normal session command/map-load path. `base/maps/arena.proc` is a
  hand-authored minimal `mapProcFile003` placeholder; `base/maps/arena.map`
  documents the map name. This is a prototype path, not a replacement for a
  dmap-compiled shipped map or a full external game DLL.
- **Gameplay SFX:** `neo/hflib/audio/hfArenaSfx.h/.cpp` provides pure,
  deterministic cue detection (`HF_DetectArenaSfx`) and procedural PCM
  generation (`HF_GenerateArenaSfxPcm`) for fire/hit/pickup/dodge/boss
  warning/wave clear/death. The module has no miniaudio/device dependency and
  is testable in the harness.
- **Visual juice:** `HfArena` owns presentation-only `fx` state (events,
  particles, shake, damage flash). FX is emitted at gameplay transitions in
  `Step()`/`Step(cmd)`, ticks deterministically, does not consume `rng`, and is
  not serialized; `hfArenaSave::Restore()` resets it. `hfUiDrawPass` renders
  quad-based muzzle flashes, hit sparks, pickups, dodge/wave pulses, boss
  telegraphs, particles, screen shake, and damage vignette behind the existing
  headless/source/arena gate.
- **Colorblind palettes:** `hfUiTheme` adds `palette_mode` (default/colorblind),
  serialized/parsed with the theme. `hfUiSettings` persists `colorblind_mode`
  through the per-account console slot, `hfUiFrontend` exposes a COLORBLIND
  option toggle, and `hfUiDrawPass` uses the palette for arena overlay colors.

## 9. Non-goals for this project

- Networking/co-op (the replication system exists but is not needed here).
- Full content pipeline authoring (glTF/BSP import is available but unused).
- Editor tooling for level layout (phase 6 can consume editor output later).
- Full dmap compiler, external game DLL packaging, or authored audio assets for
  the arena prototype.
