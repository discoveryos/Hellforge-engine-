# 05 — Making a game in C++

There are two ways to put a game in this engine. Today, path A is the working
one; path B is the intended shipping mechanism once you have a map and a
compiled game module.

## Path A — extend the built-in arena (recommended now)

`neo/hflib/game/hfArena.h` is a header-only, allocation-free simulation. It is
the best place to prototype because the same code runs in the engine and in
the headless harness.

### 1. State and constants

```cpp
struct HfArenaEnemy { float x, y; int hp; };
struct HfArenaBullet { float x, y, vx, vy; int life; };

struct HfArena {
    enum { MAX_ENEMIES = 32, MAX_BULLETS = 64, TOTAL_WAVES = 5,
           TICK_LIMIT = 4000, PLAYER_HITPOINTS = 100, PLAYER_HIT_DAMAGE = 10 };
    float playerX, playerY;
    int   hitPoints, score, wave, ticks;
    bool  gameOver, won;
    int   enemyCount, bulletCount, fireCooldown, intent;
    HfArenaEnemy  enemies[ MAX_ENEMIES ];
    HfArenaBullet bullets[ MAX_BULLETS ];
    unsigned int  rng;
    // methods...
};
```

Add new gameplay state as plain fields with fixed-size arrays and named
constants. Keep it POD-like: no pointers, no virtuals, no OS calls.

### 2. Consume the sampled input

`HfArenaInput` already exists:

```cpp
struct HfArenaInput {
    float moveX, moveY;   // -1..1
    float aimX, aimY;     // direction to target
    bool  fire;
    bool  dodge;
    void  Clear();
};
```

The frontend already samples it (`hfUiFrontend::HandleGameInput`) and the host
routes it in `hfCommonLocal::MenuEvent()` while PLAYING. **What is missing is
the sim side:** `HfArena::Step(const HfArenaInput&)` is not implemented yet.
The existing zero-argument `Step()` is the attack/retreat autopilot used by the
script sample and tests.

Implement it like this:

```cpp
void Step( const HfArenaInput &cmd ) {
    if ( gameOver ) { return; }
    // 1. apply movement (normalize, clamp to ±120 arena bounds)
    // 2. apply dodge if cmd.dodge && dodgeCooldown == 0
    // 3. spawn bullet if cmd.fire && fireCooldown == 0 && ammo > 0
    // 4. integrate enemies/bullets, resolve contact damage
    // 5. wave director, pickups, scoring, tick counters
    // 6. set gameOver/won
    ticks++;
}
```

Keep `Step()` (no arguments) as a thin wrapper around a deterministic
auto-pilot command so old tests and `arena.eng` keep working:

```cpp
void Step() {
    HfArenaInput autoCmd;
    // derive intent from nearest enemy, same as today
    Step( autoCmd );
}
```

Then have the frontend pass `GetGameInput()` to `Step(cmd)` on each fixed tick
instead of calling `Step()` — that is game phase 3 in the plan.

### 3. Expose new decisions to hfscript

Add scalar hosts in `neo/script/HfScript_Game.h`, following
`HfScript_RegisterArenaHosts()`:

```
arena_set_move(x, y)   arena_set_aim(x, y)
arena_set_fire(flag)   arena_set_dodge(flag)
arena_enemy_kind(i)    arena_pickup_kind(i)
```

No arrays or objects cross the script boundary.

### 4. Extend the save payload

See [chapter 07](07-saves.md). Every new field must appear in **five** places:

```
Capture → Serialize → Parse → Restore → Equals → FieldKeys → tests
```

Bump `HF_ARENA_SAVE_VERSION` to 2 and keep `Parse()` accepting version 1 with
defaults for new fields. Never use `memcmp` in `Equals()`:
`HfArenaEnemy` (12 bytes) and `HfArenaBullet` (20 bytes) have tail padding,
which makes byte-wise comparison produce false mismatches.

### 5. UI and presentation

- HUD currently has three slots: `HF_UI_HUD_SCORE`, `HEALTH`, `WAVE` in
  `hfUiFrontend.h`. Add `AMMO`, `DODGE`, `COMBO` as needed and mirror values in
  `SyncHud()`.
- The results screen reads `arena->won`; add score/kills/wave summary there.
- The quad overlay lives in `DrawArenaOverlay()` in
  `neo/renderer/hfUiDrawPass.cpp`. Map your world bounds into its viewport
  square and add quads for pickups/bosses. Headless runs never call it.

### 6. Tests

Add deterministic scenarios to `Test_Game` and `Test_FrontendIntegration` in
`neo/script/tests/hfstest_features.cpp`, then run the 4-way matrix
([chapter 08](08-tests.md)).

## Path B — external game DLL

`neo/framework/hfGameModule.h` defines the public game module API:

- `class hfGame` — pure virtual interface the game implements.
- `gameImport_t` — engine services passed in.
- `gameExport_t` — contains `version` and `hfGame *game`.
- `GAME_API_VERSION = 1`.
- `extern "C"` typedef `GetGameAPI_t`: `gameExport_t * ( * )( gameImport_t * )`.

Your module exports:

```cpp
extern "C" gameExport_t * GetGameAPI( gameImport_t *import ) {
    static gameExport_t ex;
    ex.version = GAME_API_VERSION;
    ex.game = new MyGame( import );
    return &ex;
}
```

`hfCommonLocal::LoadGameDLL()` (`neo/framework/hfCommon.cpp:791`) looks up
`GetGameAPI`, fills `gameImport_t`, and rejects a version mismatch with
`FatalError("wrong game DLL API version")`. On Linux the filename pattern is
`libgame<CPUSTRING>.so` (`hfSysLocal::DLL_GetFileName()`).

**Current limitations:** `LoadGameDLL()` sits inside
`#ifdef __HELLFORGE_DLL__`, which CMake does not define, and no game DLL is
shipped. Treat path B as the target for game phase 6, not a working workflow
today.

## Determinism checklist

- Same seed + same commands ⇒ same result on the same binary.
- Integer LCG only; no `rand()`, wall clock or uninitialized memory.
- Fixed step; cap catch-up; no frame-rate-dependent gameplay math.
- Stable array ordering; compaction preserves order.
- Field-wise save equality; versioned text payload.
- Test replay-after-load: load a slot, step N ticks, compare with a direct
  replay of the same seed and commands.
