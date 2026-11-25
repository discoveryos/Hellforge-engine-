# 02 — Compiling

## Requirements

- CMake **3.16+**
- C++17 compiler (`g++`/clang on Linux, Visual Studio 2019+ on Windows)
- OpenGL development headers; X11 development headers on Linux
- Optional: `python3-dev` for the real CPython bridge
- Optional: bundled Jolt source in `neo/external/jolt` for the Jolt backend

The verified host is **aarch64 Linux**. CMake detects ARM and skips the SSE2
flag and the `Simd_SSE*.cpp` sources automatically; on x86 they are enabled.
CMake default build type is `Release` (`-O2 -fno-omit-frame-pointer
-ffast-math` for non-MSVC).

## Plain engine (verified)

```sh
cmake -S neo -B neo/build -DCMAKE_BUILD_TYPE=Release
cmake --build neo/build --target hellforge -j7
```

Output: `neo/build/hellforge`.

`-j7` is deliberate: a cold rebuild after a widely-included header change
timed out at `-j4` on this host. Incremental rebuilds are fast.

## Jolt physics build (verified)

```sh
cmake -S neo -B neo/build-jolt -DHF_ENABLE_JOLT=ON
cmake --build neo/build-jolt --target hellforge -j7
```

Output: `neo/build-jolt/hellforge`. Jolt defines
`JPH_CROSS_PLATFORM_DETERMINISTIC`, and the engine gets
`HF_PHYSICS_HAS_JOLT=1`. Note the existing Jolt cache has an empty
`CMAKE_BUILD_TYPE`, so add `-DCMAKE_BUILD_TYPE=Release` if you want optimized
builds:

```sh
cmake -S neo -B neo/build-jolt -DHF_ENABLE_JOLT=ON -DCMAKE_BUILD_TYPE=Release
```

## Real Python bridge (optional, OFF by default)

```sh
cmake -S neo -B neo/build-py -DCMAKE_BUILD_TYPE=Release -DHF_PYTHON=ON
cmake --build neo/build-py --target hellforge -j7
```

`HF_PYTHON=ON` finds Python3 development files, defines `HF_PYTHON` and links
`libpython`. Without it, `neo/script/HfScript_PyBridge.h` compiles no-op stubs.
Both currently verified builds (`neo/build`, `neo/build-jolt`) have it **OFF**.
See [chapter 04](04-scripting.md#real-python-optional).

## Other CMake options

From `neo/CMakeLists.txt`:

| Option | Default | Effect |
|--------|---------|--------|
| `BUILD_DEDICATED` | OFF | Dedicated server build |
| `HF_CONSOLE_SUPPORT` | OFF | Defines `HF_CONSOLE` |
| `HF_ENABLE_JOLT` | OFF | Build/link Jolt physics |
| `HF_PYTHON` | OFF | Embedded CPython host functions |

Targets: `hflib`, `hellforge`, `hfassetlib`, `assettool`, and `jolt` when
enabled.

## Windows

```sh
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Legacy root-level path

The original `README.txt` documents an older layout:
`mkdir build && cd build; cmake ..; make`, producing `build/hellforge`.
A root `build/` directory still exists here, but the actively verified builds
use `neo/` as the CMake source directory.

## Never commit build artifacts

`.gitignore` excludes `neo/build/`, `neo/build-jolt/`, `*.o`, `*.a`, `*.gch`.
Commit source and docs only. Check before committing:

```sh
git status --short
```

## Troubleshooting

- **"Could NOT find OpenGL/X11"** — install the dev packages, not just the
  runtime libraries (`libgl1-mesa-dev`, `libx11-dev` on Debian/Ubuntu).
- **Link errors mentioning `HfArenaInput` or new headers** — stale objects;
  do a clean rebuild of the affected build directory.
- **`Unable to initialize OpenGL` at runtime** — that is a *run* problem,
  not a build problem: no X11 display. See [chapter 03](03-run.md).
- **Harness link fails with `undefined reference to hfUiDrawPass::s_arena`** —
  the headless harness does not link the renderer unit. Keep testable policy
  in pure functions (see the `GameplayOverlayGate` pattern in
  [chapter 08](08-tests.md)).
