# 09 — The first game: twin-stick arena

Full design: `docs/twin-stick-arena-design.md`.

## Goal

Turn the deterministic `HfArena` demo into the first playable game on the
Phase 9–18 substrate: start the engine, land on TITLE, press Play, steer and
shoot with mouse/keyboard or gamepad, survive waves, pause/resume, see results,
save/load a run and replay deterministically in the headless harness.

## Design summary

| Feature | Design |
|---------|--------|
| Movement | WASD / left stick, normalized and capped, ±120 bounds |
| Aim | Mouse position / right stick |
| Fire | LMB / right trigger, cooldown + ammo |
| Dodge | Space / A button, burst with i-frames and cooldown |
| Enemies | chaser (contact), spitter (ranged), brute (high HP); boss every 5th wave |
| Wave director | Deterministic seeded mix; boss = brute + escorts |
| Pickups | Health, ammo, score shards; capped count, deterministic placement |
| Scoring | 10 chaser / 25 spitter / 50 brute, wave bonus, decaying combo |
| End | Death → loss; wave-5 boss dead → win; Play Again resets with new seed |

All tunables stay as named constants in `HfArena` so a replay is bit-identical
on the same binary.

## The six game phases

1. **Data model** — commands, enemy kinds, pickups, ammo/dodge/combo;
   `Step(cmd)` plus legacy auto-pilot `Step()`; determinism tests.
2. **Gameplay rules** — archetypes, wave director, boss, pickups, scoring;
   scripted 20–40 tick scenarios in `Test_Game`.
3. **Input plumbing** — frontend sampler → sim; the sampler already exists,
   the consumption does not.
4. **Presentation** — arena draw layer, HUD extensions, results summary.
5. **Persistence** — v2 save payload with v1 compatibility; replay-after-load.
6. **Real map path** — minimal `.map` + game module, `ui_startMap`, START →
   LOADING → PLAYING through the session-owned transition.

Each phase must keep the 4-way matrix green and end with both binaries rebuilt.

## What has landed already (pre-phase-1 risk mitigation)

These are *enablers*, not gameplay:

- `HfArenaInput` POD in `neo/hflib/game/hfArena.h`.
- `hfUiFrontend::HandleGameInput()` sampling only in PLAYING, with `K_GRAVE`,
  `K_ESCAPE`, `K_JOY9` left unconsumed and a reset on every state transition.
- `hfCommonLocal::MenuEvent()` PLAYING routing.
- `hfUiDrawPass` source + arena + enabled gate; minimal quad arena overlay
  under the HUD; pure `GameplayOverlayGate()` for headless tests.
- `hfArenaSave::FieldKeys()` and `SchemaRoundTrip()` schema guard.
- `Test_RiskMitigations()` covering all three risks.

Verified `ALL PASSED` plain/headless × default/Jolt; both binaries rebuilt.

## What is deliberately out of scope until phase 6

- Real `.map` assets — none exist in this checkout.
- Game DLL — none shipped, and `LoadGameDLL()` is compiled out.
- GPU visual validation — no display/GL on this host; only structural checks.

## Starting phase 1

See [chapter 05](05-making-a-game-cpp.md) for the concrete code changes and
[chapter 07](07-saves.md) for the save checklist. Start with `HfArena`, add a
determinism test, and run the matrix before touching UI or renderer code.
