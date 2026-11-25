# Hellforge Engine — Complete Guide

From cloning the repo to shipping your own game: building, running, scripting,
UI, saves, tests and the first-game plan. Everything here is written against
the current checkout; file paths are relative to the repository root.

## Quick start

```sh
# 1. Get the source
git clone https://github.com/discoveryos/Hellforge-engine-.git
cd Hellforge-engine-

# 2. Build the plain engine (needs CMake 3.16+, OpenGL + X11 dev headers)
cmake -S neo -B neo/build -DCMAKE_BUILD_TYPE=Release
cmake --build neo/build --target hellforge -j7

# 3. Run it (a window and a working DISPLAY are required)
DISPLAY=:0 ./neo/build/hellforge
```

You land on the **TITLE** screen. Press **Play** to enter the built-in
deterministic arena. There are no `.map` assets or game DLL in this source
release, so this fallback arena is the playable content today.

## Chapters

| # | Chapter | What it covers |
|---|---------|----------------|
| 01 | [Overview & architecture](01-overview.md) | Repo layout, what the engine owns, what the script owns, phases done vs. game phases |
| 02 | [Compiling](02-build.md) | Linux/Windows builds, plain, Jolt, real Python, dedicated/console options, troubleshooting |
| 03 | [Running the engine](03-run.md) | Command line, console, cvars, startup flow, controls, maps |
| 04 | [Scripting (hfscript, English, real Python)](04-scripting.md) | `.hfs` / `.py` / `.rb` / `.eng`, all `arena_*` hosts, AOT/JIT, optional CPython bridge |
| 05 | [Making a game in C++](05-making-a-game-cpp.md) | Extending `HfArena`, consuming input, game DLL API, determinism rules |
| 06 | [UI, input and drawing](06-ui-and-input.md) | Frontend states, widgets, HUD, gameplay input routing, draw-pass gate |
| 07 | [Settings, saves and persistence](07-saves.md) | Per-account console storage, slots, save schema manifest, compatibility |
| 08 | [Tests and the 4-way matrix](08-tests.md) | Harness, `Test_*`, plain/headless × default/Jolt, adding tests |
| 09 | [The first game: twin-stick arena](09-first-game.md) | Game plan, its six phases, what is already mitigated |
| 10 | [Gotchas and troubleshooting](10-gotchas.md) | Non-obvious traps that cost time |
| 11 | [File & symbol reference](11-reference.md) | Where to find each subsystem and key entry points |

## Two meanings of "phase"

- **Engine phases 9–18** are finished (UI, networking, editor, English script,
  console layer, modern GUI, frontend, persistence, host integration). See
  `ROADMAP.md`.
- **Game phases 1–6** are the new plan in
  `docs/twin-stick-arena-design.md` for the first playable game. Only
  pre-phase-1 risk mitigation has landed; gameplay work has not started.

## Language choice in one sentence

Write engine and gameplay systems in **C++**; write strategy/bot logic in
**hfscript** (Python-style `.py`, Ruby-style `.rb` or English `.eng` dialects of
the same small VM); optionally enable `-DHF_PYTHON=ON` for **real Python** with
the standard library. There is no real embedded Ruby and no practical C game
API.

## Conventions

- Shell commands use `sh` syntax and are run from the repository root.
- Engine binaries are **never committed**; `neo/build/` and `neo/build-jolt/`
  are gitignored.
- Any change must leave `ALL PASSED` in all four test matrix runs.
