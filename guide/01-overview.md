# 01 — Overview & architecture

## What this is

Hellforge Power 1 is a cross-platform C++ game engine with an OpenGL renderer,
its own small scripting language, a deterministic sample simulation, a modern
widget-based frontend, a console-style settings/save layer and optional Jolt
physics. This source release intentionally contains **no game data**: the
playable content you see is a built-in arena used to demonstrate the systems.

## Repository map

| Path | Role |
|------|------|
| `neo/framework/` | Engine core: `hfCommon.cpp` (init, event loop, game DLL load), `hfConsole.cpp`, `hfGameModule.h`, `hfCommon_menu.cpp` (host/frontend policy) |
| `neo/sys/` | Platform layer: `linux/hflinux_main.cpp` (`main()`), input, X11/GL init, filesystem, DLL loading |
| `neo/renderer/` | OpenGL backend, `hfUiDrawPass.*` (2D UI + arena overlay), `GLSL_uber_shaders.md` |
| `neo/hflib/game/` | `hfArena.h` (deterministic sim), `hfArenaSave.h` (versioned text snapshots) |
| `neo/hflib/ui/` | ECS-based UI: `hfUiFrontend.h`, `hfUiBuilder.h`, `hfUiInput.h`, `hfUiWidget.h`, `hfUiSystem.h`, themes, keybinds, settings, save slots |
| `neo/hflib/{audio,bv,anim,scene,physics,net,editor,resource,asset,streaming}/` | Subsystems: audio, collision, animation, world/scenes, physics (Jolt or built-in), replication, editor, resources, glTF/BSP import, streaming |
| `neo/script/` | hfscript lexer/parser/VM/AOT/JIT, `HfScript_Game.h` (`arena_*` host functions), Python bridge, Ruby/English dialects, `tests/` |
| `base/script/` | Sample scripts: `arena.eng`, `demo.hfs` |
| `docs/` | Game design plan (`twin-stick-arena-design.md`) |
| `ROADMAP.md` | Engine phase history and validation requirements |
| `neo/build/`, `neo/build-jolt/` | Build outputs (gitignored) |

## Who owns what

The most important design rule in this engine:

> **C++ owns structured state. Scripts own per-tick decisions.**

hfscript values are scalars only (numbers, strings, bools) — no arrays or
objects. So the player, enemies, bullets and physics live in C++ (`HfArena`),
and the script calls scalar host functions such as `arena_hp()` and
`arena_attack()`. This keeps the script tiny, the simulation deterministic and
the whole game testable without a GPU.

```
   sys events ──► hfCommon::ProcessEvent ──► MenuEvent ──► hfUiFrontend
                                                     ├─ UI navigation (menus)
                                                     └─ HandleGameInput (PLAYING)
                                                             │
                                                             ▼
                                                       HfArenaInput
                                                             │
   hfscript .eng ──► arena_* hosts ──► HfArena::Step(cmd) ◄───┘
                                             │
                                             ▼
                                      hfArenaSave ──► console slots
                                             │
                                             ▼
                                    hfUiDrawPass overlay + HUD
```

## Startup flow

`main()` is `neo/sys/linux/hflinux_main.cpp:373`:

1. `HF_Lib::ParseHeadlessCommand(argc, argv)` — scans argv for `--headless`.
2. X11 display open (fails if no `DISPLAY`).
3. `hfCommonLocal::Init()` (`neo/framework/hfCommon.cpp:908`): interfaces,
   command line, script VM, cvars, input, console, filesystem, decls, event
   loop, config files, `InitOpenGL()`, sound, `LoadGameDLL()`, session,
   `CreateMainMenu()` → `InitFrontend()` → `StartMenu(true)`.
4. Frame loop; `FrontendFrame()` advances the UI and the 60 Hz arena sim.

## Engine phases vs. game phases

- `ROADMAP.md` phases **9–18** are engine infrastructure and are complete:
  UI, networking, editor, English scripting + sample game, console porting
  layer, modern GUI I/II, frontend, persistence, frontend/host integration.
- `docs/twin-stick-arena-design.md` phases **1–6** are a *new* game project
  built on that substrate. See [chapter 09](09-first-game.md).

## Determinism contract

Given the same seed and the same sequence of decisions, `HfArena` produces
bit-identical results on the same binary. To preserve it:

- use the built-in integer LCG, not wall-clock or unseeded randomness;
- keep the fixed 60 Hz tick and cap catch-up steps (frontend uses max 8/frame);
- keep state as plain data with fixed-size arrays;
- compare saves field by field, never with `memcmp` (struct tail padding).
