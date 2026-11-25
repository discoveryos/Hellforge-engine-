# Twin-stick Arena — design & implementation plan

**Status:** plan approved; pre-phase-1 risk mitigation landed (see section 7).
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

**Constraints from the current checkout:** no `.map` assets and no game DLL, so
the boot path intentionally falls back to the frontend-owned arena. A real map
becomes possible once `ui_startMap` names one and a game module is installed.

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
arena_enemy_kind(i)      arena_pickup_kind(i)
```

Structured state stays in C++; scripts only issue scalars. This preserves the
engine’s scripting contract and gives a non-GPU way to validate gameplay.

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

1. **Data model** — extend `HfArena` with input commands, enemy kinds, pickups,
   ammo/dodge/combo; add `Step(cmd)`, keep `Step()` auto-pilot; harness checks
   determinism and bounds.
2. **Gameplay rules** — archetype behaviour, wave director, boss, pickups,
   scoring; `Test_Game` extended with scripted 20–40 tick scenarios.
3. **Input plumbing** — frontend input sampler, keybind actions for fire/dodge,
   right-stick aim and trigger fire; no UI focus changes while PLAYING.
4. **Presentation** — arena draw layer, HUD extensions, results summary;
   headless draw is a no-op, so no GPU is required for tests.
5. **Persistence** — v2 save payload with v1 compatibility; slot round-trip and
   replay-after-load checks in `Test_FrontendIntegration`.
6. **Real map path** — add a minimal `.map` and game module, wire `ui_startMap`,
   and verify START → LOADING → PLAYING with the session-owned transition.

## 6. Tests to add

- `Test_Game`: fixed-seed replay produces identical state after N ticks with a
  scripted input sequence; wave 5 spawns a boss; pickups cap; damage/death/win
  transitions; `Step()` auto-pilot still behaves as before.
- `Test_UiFrontend`: fire/dodge keybind actions reach the input sampler; PAUSED
  freezes ticks and queued commands; RESULTS summary values.
- `Test_FrontendIntegration`: v1 and v2 slots both load; v2 round-trip preserves
  new fields; load-then-step diverges identically from a direct replay.
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

## 8. Non-goals for this project

- Networking/co-op (the replication system exists but is not needed here).
- Full content pipeline authoring (glTF/BSP import is available but unused).
- Editor tooling for level layout (phase 6 can consume editor output later).
