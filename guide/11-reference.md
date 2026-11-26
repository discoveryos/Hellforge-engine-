# 11 — File & symbol reference

## Entry points

| What | Where |
|------|-------|
| `main()` | `neo/sys/linux/hflinux_main.cpp:373` |
| Engine init | `hfCommonLocal::Init()` — `neo/framework/hfCommon.cpp:908` |
| Event routing | `hfCommonLocal::ProcessEvent()` — `neo/framework/hfCommon.cpp:1459` |
| Frontend/host policy | `hfCommonLocal::MenuEvent()` — `neo/framework/hfCommon_menu.cpp:330` |
| Per-frame frontend | `hfCommonLocal::FrontendFrame()` — `neo/framework/hfCommon_menu.cpp` |
| Frontend install | `hfCommonLocal::InitFrontend()` — `neo/framework/hfCommon_menu.cpp` |
| Game DLL load | `hfCommonLocal::LoadGameDLL()` — `neo/framework/hfCommon.cpp:791` |
| Script init | `HfScript_Init()` — `neo/script/HfScript_Engine.cpp` |
| Script load | `HfScript_LoadFile()` — `neo/script/HfScript_Engine.cpp:383` |
| Harness main | `neo/script/tests/hfsharness.cpp` |
| Feature suite | `HfTest_FeaturesMain()` — `neo/script/tests/hfstest_features.cpp` |
| Draw pass | `hfUiDrawPass::Draw` / `DrawInstalled` — `neo/renderer/hfUiDrawPass.cpp` |

## Key types

| Type | File | Notes |
|------|------|-------|
| `HfArena`, `HfArenaEnemy`, `HfArenaBullet`, `HfArenaInput` | `neo/hflib/game/hfArena.h` | Header-only deterministic sim + command POD |
| `hfArenaSave` | `neo/hflib/game/hfArenaSave.h` | Versioned text snapshot, `FieldKeys`, `SchemaRoundTrip` |
| `hfUiFrontend` | `neo/hflib/ui/hfUiFrontend.h` | States, widgets, HUD, input sampler |
| `hfUiBuilder` | `neo/hflib/ui/hfUiBuilder.h` | AddLabel/AddButton helpers |
| `hfUiInput`, `hfUiWidget`, `hfUiSystem` | `neo/hflib/ui/` | Pointer/focus, widgets, ECS draw list |
| `hfUiTheme`, `hfUiKeybinds`, `hfUiSettings`, `hfUiSaveSlots` | `neo/hflib/ui/` | Presentation and persistence helpers |
| `hfconsole::hfConsoleServices` | `neo/hflib/console/hfConsoleServices.h` | Account-scoped slots |
| `hfGame`, `gameImport_t`, `gameExport_t` | `neo/framework/hfGameModule.h` | `GAME_API_VERSION = 1` |
| `HfScriptVM` | `neo/script/HfScript.h` | Scalar-only VM |

## Documentation in the repo

| Document | Contents |
|----------|----------|
| `README.txt` | License, original build steps, game-data note |
| `ROADMAP.md` | Engine phases 1–18 and validation requirements |
| `docs/twin-stick-arena-design.md` | First-game design and six game phases |
| `neo/renderer/GLSL_uber_shaders.md` | Renderer/UI shader-notes and Phase 18/overlay documentation |
| This guide | Practical how-to, from build to game |
| `guide/12-hfscript.md` | Dedicated hfscript language/VM deep dive |
| `guide/13-wsl-alpine-debian.md` | Alpine/musl and Debian/Ubuntu WSL build notes |

## Glossary

- **hfscript** — the engine's scalar-only scripting language and VM.
- **Dialect** — syntax flavour (`.hfs`, `.py`, `.rb`, `.eng`) over the same VM.
- **Host function** — a C++ function callable from hfscript; values are scalars.
- **Frontend** — the ECS-based menu/HUD system, not the renderer.
- **Draw pass** — the 2D overlay that draws UI quads and the arena layer.
- **Slot** — a named, account-scoped storage record (`settings`, `save1..8`).
- **Autopilot** — the existing zero-argument `HfArena::Step()` decision policy.
- **Matrix** — 4 harness runs: plain/headless × default/Jolt.

## Command cheat sheet

```sh
# Configure + build
cmake -S neo -B neo/build -DCMAKE_BUILD_TYPE=Release
cmake --build neo/build --target hellforge -j7
cmake -S neo -B neo/build-jolt -DHF_ENABLE_JOLT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build neo/build-jolt --target hellforge -j7
cmake -S neo -B neo/build-py -DHF_PYTHON=ON -DCMAKE_BUILD_TYPE=Release

# Run (requires X11)
DISPLAY=:0 ./neo/build/hellforge

# Tests (this workspace)
HF_NOBENCH=1 sh /tmp/opencode/build_harness2.sh
HF_NOBENCH=1 sh /tmp/opencode/build_harness_jolt3.sh
sh /tmp/opencode/run_matrix.sh

# Before every commit
git status --short
```
