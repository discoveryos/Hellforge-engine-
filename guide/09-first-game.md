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

## What has landed

All six phases are implemented in the engine:

- `HfArena::Step(cmd)` with archetypes, boss waves, pickups, ammo/dodge/combo,
  plus the separate legacy `Step()` auto-pilot.
- `hfUiFrontend` command routing and the extended HUD/RESULTS summary.
- `hfUiDrawPass` quad overlay for enemy/pickup/bullet kinds, facing and dodge.
- `hfArenaSave` v2 with v1-compatible parsing and replay-after-load tests.
- `HfArenaGame` plus scalar `arena_*` hosts so a future game can reuse the
  engine implementation instead of rewriting it.

`Test_TwinStickArena()` and `Test_RiskMitigations()` cover the headless path.
Verified `ALL PASSED` plain/headless × default/Jolt; both binaries rebuilt.

## What remains content work

- Real `.map` assets — none exist in this checkout.
- Game DLL — none shipped; `ui_startMap` is the hook for one.
- GPU visual validation — no display/GL on this host; only structural checks.

See [chapter 05](05-making-a-game-cpp.md) for the game-facing API and
[chapter 07](07-saves.md) for the save schema.
