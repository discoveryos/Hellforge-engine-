# 08 — Tests and the 4-way matrix

## The requirement

After **every** change, all of these must be true:

- `ALL PASSED` in the script harness
- plain **and** `--headless`
- default **and** Jolt builds

That is four runs: `default/plain`, `default/headless`, `Jolt/plain`,
`Jolt/headless`. Every phase in `ROADMAP.md` records this.

## Harness layout

`neo/script/tests/`:

| File | Role |
|------|------|
| `hfsharness.cpp` | `main()`, host registration, watchdog, prints `ALL PASSED` or `FAILURES PRESENT` |
| `hfstest_render.cpp`, `hfstest_optimize.cpp` | Render and optimizer checks |
| `hfstest_features.cpp` | The big feature suite, `HfTest_FeaturesMain()` |

`HfTest_FeaturesMain()` calls, in order: `Test_ECS`, `Test_AssetStore`,
`Test_Cluster`, `Test_Stream`, `Test_Audio`, `Test_Collide`,
`Test_AudioSpatial`, `Test_Scene`, `Test_Lights`, `Test_Resources`,
`Test_Serialization`, `Test_Headless`, `Test_SimServer`, `Test_AudioServer`,
`Test_Ui`, `Test_Net`, `Test_Editor`, `Test_Physics`, `Test_PhysicsBackend`,
`Test_Materials`, `Test_Import`, `Test_hfStr`, `Test_BSP`, `Test_Zip`,
`Test_Animation`, `Test_Game`, `Test_Console`, `Test_UiWidgets`,
`Test_UiFrontend`, `Test_UiPersistence`, `Test_FrontendIntegration`,
`Test_RiskMitigations`.

## Building the harnesses in this workspace

The harness is compiled against the `libhflib.a` already produced in each
build directory, plus the script translation units and test files. The
workspace uses incremental scripts in `/tmp/opencode/` (ephemeral, not in the
repo):

- `build_harness2.sh` → `/tmp/opencode/hfstest_new` (plain)
- `build_harness_jolt3.sh` → `/tmp/opencode/hfstest_jolt` (Jolt; adds
  `-DHF_PHYSICS_HAS_JOLT=1` and links `libjolt.a`)
- `run_matrix.sh` → runs all four combinations

To rebuild both harnesses:

```sh
HF_NOBENCH=1 sh /tmp/opencode/build_harness2.sh
HF_NOBENCH=1 sh /tmp/opencode/build_harness_jolt3.sh
```

They compile each translation unit into its own mtime-cached object under
`/tmp/opencode/objs*` and link with `-Wl,--export-dynamic`. If you clone on a
fresh machine, recreate these scripts from the pattern above: the source list
is `hfsharness.cpp`, `hfstest_render.cpp`, `hfstest_optimize.cpp`,
`hfstest_features.cpp`, `HfScript_Lexer.cpp`, `HfScript_Parser.cpp`,
`HfScript_VM.cpp`, `HfScript_AOT.cpp`, `HfScript_JIT.cpp`, `HfScript_Ruby.cpp`,
linked against `libhflib.a`, `-ldl -lpthread -lm` (plus `libjolt.a` for Jolt).

## Running the matrix

`run_matrix.sh` is simply:

```sh
cd /home/kali/hellforge-engine
for bin in hfstest_new hfstest_jolt; do
  for mode in plain headless; do
    if [ "$mode" = "plain" ]; then arg=""; else arg="--headless"; fi
    /tmp/opencode/$bin $arg
  done
done
```

Expected output for each of the four runs:

```
phase 8 console: ok
features logic: ok (0 failures)
ALL PASSED
```

A failing run prints `FAILURES PRESENT` and individual `FAIL` lines. Never
commit with a red matrix.

## Adding a test

1. Declare `static void Test_MyFeature();` near the other prototypes in
   `hfstest_features.cpp`.
2. Implement it using `FEAT_ASSERT(cond)` and `FEAT_NEAR(a, b, epsilon)`.
3. Call it from `HfTest_FeaturesMain()`.
4. Use the `int fc = g_featFails;` pattern and print
   `phase N my feature: ok/FAIL` so failures are easy to locate.
5. Rebuild both harnesses, run the matrix, then rebuild both engine binaries.

## Headless-safe test rules

- Never call GL or renderer statics from a harness test. The harness does not
  link `hfUiDrawPass.cpp`; use a pure policy function instead:

  ```cpp
  static bool GameplayOverlayGate( bool enabled, bool hasArena, bool hasSource ) {
      return enabled && hasArena && hasSource;
  }
  ```

  The renderer's `ShouldDrawGameplayOverlay()` delegates to it, so the truth
  table is testable without a GL context.
- `Test_RiskMitigations()` is the worked example: input sampling, pure draw
  gate, schema manifest, per-field equality and legacy-record parsing.
- Keep sim tests deterministic: fixed seed, fixed command sequence.
- Compare snapshots field-wise, never `memcmp`.
- Use `base/script/arena.eng` relative to the engine root for script tests.

## Rebuilding the engine binaries after code changes

```sh
cmake --build neo/build --target hellforge -j7
cmake --build neo/build-jolt --target hellforge -j7
```

Header changes can trigger long rebuilds; `-j7` is recommended on this host.
Afterwards, confirm `git status --short` shows only source/doc changes.
