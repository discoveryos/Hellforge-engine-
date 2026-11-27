# 03 — Running the engine

## Launching

```sh
DISPLAY=:0 ./neo/build/hellforge          # default engine
DISPLAY=:0 ./neo/build-jolt/hellforge     # Jolt build
```

A working X11 display is required. `K_GRAVE` (the backtick/tilde key) opens
the console when `com_allowConsole` is enabled.

### About `--headless`

`HF_Lib::ParseHeadlessCommand()` (`neo/hflib/hfHeadless.h`) recognizes
`--headless`, `-headless` and `+headless`, case-insensitively, anywhere in
argv. There is **no `r_headless` cvar**.

Important caveat verified in this checkout: `hfCommonLocal::Init()` calls
`renderSystem->InitOpenGL()` unconditionally (`neo/framework/hfCommon.cpp:1024`).
On a machine without X11, GL init fails and `FatalError("Unable to initialize
OpenGL")` terminates the engine. So:

- Use `--headless` for the **script harness** and simulation server, where it
  is fully supported.
- For the full engine, `--headless` alone is not enough on a display-less host.
  See [chapter 08](08-tests.md) for the headless logic tests.

## Command line and configs

`hfCommonLocal::ParseCommandLine()` (`neo/framework/hfCommon.cpp:196`) treats
`+` as a console-line separator. Examples from the code:

```sh
hellforge +set test blah +map test
hellforge set test blah+map test
hellforge set test blah + map test
```

Startup config files: `default.cfg`, `hellforge.cfg` (`CONFIG_FILE`) and
`autoexec.cfg`. `safe` / `cvar_restart` skip normal config loading.

Useful cvars:

| Cvar | Meaning |
|------|---------|
| `ui_startMap` | Map the START action should request; empty means use the built-in arena |
| `ui_startDevMap` | Issue `devmap` instead of `map` (bool, archived) |
| `fs_game`, `fs_game_base`, `fs_basepath`, `fs_savepath` | Mod/search/save paths |
| `com_allowConsole` | Whether the `~` key opens the console |
| `sensitivity`, `s_volume_dB`, `r_fullscreen`, `r_customWidth`, `r_customHeight`, `r_swapInterval` | Settings applied from `hfUiSettings` |

## Startup → gameplay flow

`InitFrontend()` (`neo/framework/hfCommon_menu.cpp`) creates the frontend,
attaches the engine-owned arena (`s_frontendArena`), resets it with seed `1`,
and installs the draw-pass source and gameplay overlay.

Pressing **START**:

- If `ui_startMap` is empty → `BeginGame()` resets the attached arena and
  switches to `PLAYING`.
- If `ui_startMap` names a map → the host buffers `map`/`devmap` for the
  session, switches to `LOADING` and marks the action host-handled.

In this checkout there are **no `.map` assets and no game DLL**, so the
fallback arena path is what you get. It is intentional.

## Controls

Menus (frontend):

| Input | Action |
|-------|--------|
| Enter / Space | Activate focused button |
| Arrow keys / D-pad / TAB | Move focus |
| Mouse | Click buttons, drag sliders |
| Left stick | Focus traversal (with repeat cooldown) |
| Escape / `K_JOY9` | Back / pause |

Gameplay input sampler (`hfUiFrontend::HandleGameInput`, PLAYING only):

| Input | Sampled command |
|-------|-----------------|
| W/A/S/D | `moveX`, `moveY` |
| Left stick axes | `moveX`, `moveY` |
| Mouse position | aim vector |
| Right stick / stick2 keys | aim vector |
| LMB / joystick triggers | `fire` |
| Space | `dodge` |

`K_GRAVE`, `K_ESCAPE` and `K_JOY9` are deliberately left unconsumed so the
console and pause systems still receive them. While PLAYING, the frontend
passes that command to `HfArena::Step(cmd)` on every fixed tick; the bundled
`arena.eng` demo still uses the zero-argument auto-pilot Step. See
[chapter 05](05-making-a-game-cpp.md) for the game-facing API.

## What you see

The frontend has TITLE, MENU, LOADING, PLAYING, PAUSED, RESULTS and SAVES
screens, with HUD labels for score, health and wave. The renderer draws a
centred quad arena overlay (player, enemies, bullets) under the HUD, but only
when a source, an arena and the PLAYING flag are all present. See
[chapter 06](06-ui-and-input.md).
