# 10 — Gotchas and troubleshooting

Things that are easy to assume incorrectly in this checkout.

## 1. Full-engine `--headless` is not GPU-free on Linux

`--headless` is parsed in `main()`, but `hfCommonLocal::Init()` still calls
`InitOpenGL()`. With no X11 display, GLimp fails three times and
`FatalError("Unable to initialize OpenGL")` ends the process. Use the harness
for headless logic tests; a windowed run needs a valid `DISPLAY`.

## 2. Headless is an argv flag, not a cvar

There is no `r_headless`. Recognized forms: `--headless`, `-headless`,
`+headless` (case-insensitive). `net_headlessServer` belongs to a different
subsystem.

## 3. No `.map` assets and no game DLL

`ui_startMap` names a map that is not present; START therefore falls back to
the built-in arena. `LoadGameDLL()` is behind `__HELLFORGE_DLL__`, which CMake
does not define. This is intentional for phases 1–5 of the game plan.

## 4. `.hfobj` sidecars silently win

`HfScript_LoadFile()` loads `foo.hfobj` instead of `foo.hfs`/`foo.py`/`foo.rb`
when the sidecar exists. Delete or regenerate it after editing a script.

## 5. Offline saves are in-memory

The console services stub is deterministic but not durable across processes
without an installed platform service. Saves work in tests and within a
session.

## 6. Python and Ruby are different kinds of "supported"

- `.py` is an hfscript dialect; **real** Python requires `-DHF_PYTHON=ON`.
- `.rb` is only a syntax tag; there is no embedded Ruby interpreter.

## 7. Jolt and Python are opt-in CMake flags

Both default OFF. The existing Jolt cache also has an empty
`CMAKE_BUILD_TYPE`, so pass `-DCMAKE_BUILD_TYPE=Release` explicitly if you
want optimized Jolt builds.

## 8. aarch64 host

SSE2 flags and `Simd_SSE*.cpp` are skipped on ARM. Precompiled `.hfobj` files
are endian-canonical precisely so they can move between x86_64 and aarch64.

## 9. The input sampler is not wired to the sim yet

`HfArenaInput` and `HandleGameInput()` exist; `HfArena::Step(cmd)` does not.
The arena still runs its autopilot. Don't expect WASD to move the player until
game phase 3.

## 10. Never compare arena saves with `memcmp`

`HfArenaEnemy` (12 bytes) and `HfArenaBullet` (20 bytes) have tail padding.
Use `hfArenaSave::Equals()` and keep every new field in `FieldKeys()`.

## 11. Harness tests cannot use renderer statics

The harness does not link `hfUiDrawPass.cpp`. Put testable policy in pure
functions (`GameplayOverlayGate`) and let the renderer delegate to them.

## 12. Build artifacts must not be committed

`neo/build/`, `neo/build-jolt/`, `*.o`, `*.a`, `*.gch` are gitignored. Verify
with `git status --short` before committing; only source and docs go in.

## 13. Header changes cause long rebuilds

Use `-j7`; `-j4` timed out at 300 s on this host after a widely-included
header change.
