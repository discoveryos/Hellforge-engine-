# 06 — UI, input and drawing

## Frontend states

`hfUiFrontend` (`neo/hflib/ui/hfUiFrontend.h`) owns the screen flow:

```
BOOT → TITLE ⇄ MENU ⇄ OPTIONS / KEY BINDINGS / SAVES
                ↓ START
             LOADING → PLAYING ⇄ PAUSED → RESULTS → TITLE
```

States: `BOOT`, `TITLE`, `MENU`, `LOADING`, `PLAYING`, `PAUSED`, `RESULTS`,
`SAVES`.

Actions emitted by widgets: `START`, `OPTIONS`, `BACK`, `PAUSE`, `RESUME`,
`RESTART`, `MAIN_MENU`, `QUIT`, `SAVES`, `SAVE_SLOT_LOAD`, `SAVE_SLOT_WRITE`,
`SAVE_SLOT_DELETE`.

`SetState()` clears the ECS registry, rebuilds the screen, applies the theme,
syncs HUD values and publishes a `ui.state` event. Consequently, **all entity
handles are invalid across a state change** — never cache them.

## Building widgets

The builder helpers are in `neo/hflib/ui/hfUiBuilder.h`:

```cpp
HF_Ecs::Entity label = hfUiBuilder::AddLabel( root, x, y, w, h,
                                              "SCORE 0", 1.0f, align, r, g, b );
HF_Ecs::Entity button = hfUiBuilder::AddButton( root, x, y, w, h,
                                                "PLAY", HF_UI_ACTION_START );
```

`Bind(entity, action)` wires clicks to actions; `FindBoundAction(entity)`
returns the action for a clicked entity. `hfUiInput` handles pointer capture,
focus traversal and `SE_JOYSTICK` left-stick navigation with a frame-counted
repeat cooldown, reset on each state change.

HUD slots today: `HF_UI_HUD_SCORE`, `HF_UI_HUD_HEALTH`, `HF_UI_HUD_WAVE`
(`HF_UI_HUD_COUNT` is the count). To add a slot, extend the enum, the arrays
and `SyncHud()`.

## Input: two paths that never collide

`hfCommonLocal::MenuEvent()` (`neo/framework/hfCommon_menu.cpp:330`):

- In menu states (TITLE, MENU, LOADING, PAUSED, RESULTS): the frontend gets
  every event via `HandleSysEvent()`.
- In PLAYING: `HandleGameInput()` samples gameplay first, then `K_ESCAPE` /
  `K_JOY9` are offered to `HandleSysEvent()` for pause. Everything else flows
  to the console/game.

`hfUiFrontend::HandleGameInput()`:

- returns `false` immediately unless the state is PLAYING;
- samples W/A/S/D and left stick into `moveX/moveY`;
- samples mouse position, right stick and stick2 keys into a normalized aim
  vector;
- samples LMB and both triggers into `fire`, Space into `dodge`;
- explicitly returns `false` for `K_GRAVE`, `K_ESCAPE`, `K_JOY9` so console
  and pause keep priority;
- resets on **every** state transition via `ResetGameInput()`, so no command
  leaks across screens.

The frontend hands `GetGameInput()` to `HfArena::Step(cmd)` on each fixed
60 Hz tick while PLAYING. PAUSED freezes the sim and clears the command;
see [chapter 05](05-making-a-game-cpp.md) for the game-facing API.

## Drawing

`hfUiDrawPass` (`neo/renderer/hfUiDrawPass.h/.cpp`) draws pixel-space quads,
coverage text and rounded/nine-patch panels. It has two entry points:

- `Draw(registry, uiSystem)` — immediate, used by tests and custom hosts.
- `DrawInstalled()` — called by the OpenGL backend once per frame after the
  3D views; uses the installed source.

Install a source with:

```cpp
hfUiDrawPass::SetSource( frontend->RegistryPtr(), frontend->SystemPtr() );
hfUiDrawPass::ClearSource();   // also clears arena + enabled flag
```

`InitFrontend()` also installs the live arena once:

```cpp
hfUiDrawPass::SetGameplayOverlay( frontend->GetArena() );
```

and `FrontendFrame()` derives the flag from the frontend state every frame:

```cpp
hfUiDrawPass::SetGameplayOverlayEnabled(
    frontend->GetState() == hfUiFrontend::HF_UI_FRONTEND_PLAYING );
```

The gate is:

```cpp
static bool GameplayOverlayGate( bool enabled, bool hasArena, bool hasSource ) {
    return enabled && hasArena && hasSource;
}
static bool ShouldDrawGameplayOverlay();   // delegates to the pure gate + statics
```

`DrawArenaOverlay()` maps the ±120 world box into a centred viewport square
and emits quads for bounds, player, enemies and bullets **before** the HUD, so
the HUD stays on top. No textures or map assets are needed, and `DrawInstalled()`
is a no-op in headless mode or without a source.

Why the split? The headless harness deliberately does not link the renderer
translation unit, so tests call the pure `GameplayOverlayGate()` directly.
Copy this pattern for any renderer policy you want to test headlessly.

## Theme, keybinds and settings

- `hfUiTheme` — `fontScale`, `focus`, `focusWidth`, `cornerRadius`,
  `borderWidth`; applied in place so repeated `Apply()` calls do not compound
  scaling.
- `hfUiKeybinds` — named actions, hard conflict rejection, click-to-capture UI.
- `hfUiSettings` — per-account settings saved to slot `"settings"`; applied to
  cvars live. See [chapter 07](07-saves.md).
