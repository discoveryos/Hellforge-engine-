# Native GLSL material shaders & GPU post-processing

This document covers the two hand-written GPU paths added to the renderer:

1. **Uber material shaders** (`hfUberGLSL`) — native GLSL replacement for the
   runtime Cg->GLSL conversion of the common material builtins, selected
   per-stage with a feature mask like id Tech 6's explicit program permutations.
2. **Compute post-processing** (`hfOpenGLPostprocess`) — HDR tone mapping
   (ACES filmic) plus a separable gaussian bloom, all computed in a single
   GL 4.3 compute program.

Both paths are opt-out and fall back to the original engine code, so they are
safe to validate side by side on a real GPU.

---

## 1. Uber material shaders

### Files
- `renderer/hfUberGLSL.h` / `hfUberGLSL.cpp`
- Interception points: `renderer/hfRenderProgs.cpp`
  (`LoadVertexShader` / `LoadFragmentShader`), `hfRenderProgs.h`
  (`nativeGLSL`, `useUniformArray`), `hfRenderProgs_GLSL.cpp`
  (`CommitUniforms`, `LoadGLSLProgram`), and the ambient stage loop in
  `renderer/hftr_backend_draw.cpp` (`HF_UberGLSL_SetStageFeatures`).

### cvars
| cvar | default | description |
|------|---------|-------------|
| `r_useNativeGLSL` | 1 | use the native uber programs instead of the converted Cg programs. Setting to `0` restores the original converted-material path. |

### PBR interaction lighting (`HF_UBER_PBR`, bit 8)
`#version 140` vertex/fragment sources. Regular and skinned variants both get
view-space position in `vofi_TexCoord1` when `F_LIGHTED && F_PBR`. The
fragment evaluates a Cook-Torrance GGX BRDF on top of the classic
hemisphere light, using extra `rpUserN` vectors:

| uniform | contents |
|---------|----------|
| `rpUser1` | `vec4(roughness, metallic, ao, emissiveScale)` |
| `rpUser2` | `vec4(dofStrength, dofNear, dofFar, dofFocusDist)` (reserved for the compute DoF) |
| `rpUser3` | `vec4(ssrScale, ssrSteps, ssrMaxDist, ssaoStrength)` (reserved for SSR/AO) |
| `rpUser4` | `vec4(ambientR, ambientG, ambientB, ambientScale)` |

Diffuse is multiplied by `(1-metallic)`; specular is `D*G*F/(4*NdotV*NdotL)`
with Schlick Fresnel and Smith-Schlick geometry; ambient and emissive terms
are additive. Controlled by `r_usePBR` (default 1) plus `r_pbrRoughness`
(0.55), `r_pbrMetallic` (0.0), `r_pbrAO` (1.0), `r_pbrEmissiveScale` (1.0),
`r_pbrAmbientR/G/B` (0.55/0.55/0.6) and `r_pbrAmbientScale` (1.0). The
feature bit is only added when `r_usePBR` is on.

### Pure logic sharing (`hfUberLogic.h/.cpp`)
The feature-bit math, coverage list and default masks live in a header-only
`HF_Uber` namespace (plus a non-header sibling for the engine link). The CPU
test harness compiles the same files, so `hfstest_render.cpp` validates the
GLSL `mod(floor(x))` decode, per-program masks and the stage→features mapping
offline (no GPU): 30 checks, `render logic: all ok`.

## 4. GL capability detection (`hfGLCaps`)
`hfGLCaps::Init()` parses the driver `GL_VERSION` / `GLSL_VERSION` strings,
assigns a flag set (`HF_GL_CAP_GLSL_120/140/330/400/430`, `COMPUTE`, `UBO`,
`SSBO`, `FBO`, `BLIT_FRAMEBUFFER`, `TEXTURE_FLOAT`, `INSTANCING`) and scans
`GL_EXTENSIONS` for `ARB_compute_shader`/`ARB_uniform_buffer_object`/etc. so
older-core drivers can still unlock newer features. Users of the flags:
- uber native compile needs `HF_GL_CAP_GLSL_140` (skinned additionally
  `HF_GL_CAP_UBO`), else graceful fallback to the converted program;
- compute post needs `HF_GL_CAP_COMPUTE`;
- the FPS optimizer picks its starting quality level from the parsed versions.

The pure version-parsing/flags math is inline in `hfGLCaps.inl` so the harness
verifies `ParseVersionString` and `FlagsForVersion` without linking the GL
wrapper (`hfstest_render.cpp`: ~24 checks).

## 5. Adaptive FPS optimizer (`hfFpsOptimizer`)
Toolbox for sustaining a target FPS on both old and new GPUs. Driven once per
frame from `RB_ExecuteBackEndCommands` with the GPU timer query.

### cvars
| cvar | default | description |
|------|---------|-------------|
| `opt_enable` | 1 | master switch |
| `opt_targetFps` | 0 | target FPS (0 = 60) |
| `opt_aggression` | 0.5 | reaction strength 0..1 |
| `opt_stepUpFrames` | 8 | below-budget frames before stepping quality up |
| `opt_minQuality` / `opt_maxQuality` | 0 / 9 | allowed quality window |
| `opt_applyPreset` | -1 | `-1` = adaptive, `0..9` = lock a quality level |
| `opt_show` | 0 | 1 = per-change prints, 2 = per-frame stats line |
| `opt_warmup` | 1 | pre-compile all uber + compute shaders at init |
| `opt_level` | 4 (ROM) | read-only current quality level (console overlay) |

### Quality ladder (10 levels)
`0 UltraPerf` (no AA/shadows/spec/bump, aniso off, sharp LOD) up to
`9 Nightmare` (msaa 8, sRGB, bloom+SSAO+SSR+DoF, aniso 16, LOD 0). The optimizer
writes the level's cvar values (`r_multiSamples`, `r_skipShadows`,
`r_skipSpecular`, `r_skipBump`, `r_useSRGB`, `r_bloom`, `r_ssao`, `r_ssr`,
`r_depthOfField`, `r_maxAnisotropicFiltering`, `r_useTrilinearFiltering`,
`r_lodBias`) through `cvarSystem->Find()`.

### Resolution axis
The optimizer reuses `hfResolutionScale`: it writes `rs_dropMilliseconds`,
`rs_raiseMilliseconds`, `rs_dropFraction`, `rs_raiseFraction`,
`rs_raiseFrames` and `rs_enable` from the FPS target and aggression each
frame. On a quality step it calls `ResetToFullResolution()` so both axes do
not over-correct.

### Startup
`R_InitOpenGL` runs `hfGLCaps::Init()`, then
`fpsOptimizer.ApplyCapabilityDefaults()` (picks level from GL caps:
GL 4.3+compute → Ultra, GL 3.3 → High, GL 2.x → Performance) and
`fpsOptimizer.Warmup()` (force-compiles all 11 covered uber programs + compute
shader, eliminating first-frame shader-compile hitches).

### Pure logic sharing
The decision math (`TargetFrameMs`, `StepDirection`, `OverBudgetRatio`,
`HeadroomRatio`, `QualityLevelForCaps`, the quality table) lives in
`hfOptimize.h`; the harness verifies it offline (`hfstest_optimize.cpp`:
`optimizer logic: all ok`). Diagnostics: `opt_printStats` console command and
a `Q:<label>` line in the `com_showFPS` overlay reading `opt_level`.

### Shared feature mask
The feature mask is passed as `RENDERPARM_USER` (`rpUser0.x`). Bits:

| bit | constant | meaning |
|-----|----------|---------|
| 0   | `HF_UBER_DIFFUSE` | modulate the base color by `samp0` |
| 1   | `HF_UBER_BUMP` | rebuild the normal from `samp1` in tangent space |
| 2   | `HF_UBER_SPECULAR` | add `samp2` specular |
| 3   | `HF_UBER_VERTEXCOLOR` | multiply by `in_Color * rpVertexColorModulate` |
| 4   | `HF_UBER_ALPHATEST` | `discard` when `base.a < rpAlphaTest.x` |
| 5   | `HF_UBER_TEXGEN` | use projected screen-space UVs (`gl_Position.xy/w`) |
| 6   | `HF_UBER_DIFFUSEMOD` | base color from `rpDiffuseModifier` (interactions) instead of `rpColor` |
| 7   | `HF_UBER_LIGHTED` | compute hemisphere lighting / bump / specular |
| 8   | `HF_UBER_PBR` | physically-based GGX Cook-Torrance lighting branch (needs `F_LIGHTED`) |

The default mask per builtin is set in `HF_UberGLSL_DefaultFeaturesFor`; the
mask is *then refined per material stage* via `HF_UberGLSL_SetStageFeatures`
to match each stage's actual texture / vertex color / alpha test / texgen.

### Covered builtins
`gui`, `color`, `simpleshade`, `texture`, `texture_color`, `texture_color_skinned`,
`texture_color_texgen`, `interaction`, `interaction_skinned`,
`interactionAmbient`, `interactionAmbient_skinned`.

`environment` / `bumpyEnvironment` are intentionally left on the legacy
converter: they need cube-map texgen and view-vector reflection math that the
uber program does not model.

### GPU skinning
The `_skinned` variants use a `#version 140` vertex program. The joint matrices
are supplied through the `matrices_ubo` uniform block (std140) already bound
for the `BUILTIN_*_SKINNED` draw path:

```glsl
layout( std140 ) uniform matrices_ubo {
	vec4 uboJointRows[ 256 * 3 ];   // 12 KB, within the guaranteed UBO minimum
};
```

Each `hfJointMat` is a 3x4 row-major matrix, so one joint occupies three
consecutive `vec4` rows. Joint indices are packed in `in_Color`
(`UNSIGNED_BYTE` normalized; reconstruct with `int(in_Color.x * 255.0)`),
weights in `in_Color2` (0..1). Up to 4 weighted joints per vertex.

### How to validate (runtime)
1. Start the game on a machine with a real GPU and OpenGL 4.3+ driver.
2. Run on a GPU with debug output if available; watch the console for
   `HF_UberGLSL: compiled native vertex/fragment program <name>` for each
   covered builtin load.
3. `r_useNativeGLSL 0` then `r_useNativeGLSL 1` in the same scene; the two
   should be visually identical. Differences to look for:
   - alpha-testing materials: no over-draw on cut-outs
   - interactions: lighting/stage direction should match the converted program
   - skinned models (`texture_color_skinned`, `interaction_skinned`):
     geometry should follow the skeleton — a broken UBO/format would show
     clearly.
4. If a covered builtin fails to compile, the renderer logs the GLSL info log
   and falls back to the converted program for that builtin (see
   `LoadVertexShader` / `LoadFragmentShader` in `hfRenderProgs.cpp`), so the
   game keeps running.

## 2. Compute post-processing

### Files
- `renderer/hfOpenGLPostprocess.h` / `.cpp`
- Hook: `RenderView_` / `RB_PostProcess` in `renderer/hftr_backend_draw.cpp`.

### cvars
| cvar | default | description |
|------|---------|-------------|
| `r_computePostprocess` | 1 | enable the compute path (vs. the legacy quad path) |
| `r_postProcessExposure` | 1.0 | exposure before ACES tone mapping |
| `r_postProcessGamma` | 2.2 | output gamma |
| `r_bloom` | 1 | enable the bloom effect stack |
| `r_bloomBrightThreshold` | 1.0 | luminance threshold for the bright pass |
| `r_bloomIntensity` | 0.6 | strength of the bloom added back |

### Pipeline (single compute program, dispatch sequence)
- **pass 1** — bright extraction + 4x4 downsample of the HDR image to
  quarter-resolution into `bloomA` (RGBA16F).
- **pass 2** — horizontal gaussian blur `bloomA -> bloomB`.
- **pass 3** — vertical gaussian blur `bloomB -> bloomA`.
- **pass 4** — ACES tone map (`u_exposure`), add `r_bloomIntensity * bloomA`,
  gamma, write the LDR result.

Image bindings: 0 = HDR source (read), 1 = LDR result (write),
2/3 = `bloomA`/`bloomB` (read-write ping-pong). The GL 4.3 entry points are
resolved with `dlsym` and the original FBO/tone-map-quad path is used as a
fallback when the driver lacks them.

### How to validate (runtime)
1. Requires OpenGL 4.3. Watch for
   `HF_OpenGLPostprocess: compute HDR post-processing enabled`.
2. Toggle `r_computePostprocess` in-game: the frame should switch between the
   legacy quad tonemap and the compute ACES/bloom version without artifacts.
3. Bloom controls: with `r_bloom 1`, bright lights/extreme highlights should
   glow; raise/lower `r_bloomBrightThreshold` and `r_bloomIntensity` to see the
   effect. Set `r_bloom 0` to confirm the frame matches the no-bloom path.
4. If the dispatch fails, the console reports the compile/link log and the
   renderer continues on the legacy path (`r_computePostprocess` fallback).

## 6. Audio engine (miniaudio backend)

### Files
- `hflib/audio/hfAudioData.h` — pure audio primitives: sample meta, distance
  attenuation models (`hfAttenuationGain`: linear / inverse-clamped / smoothstep
  tangled), stereo pan (`hfPanGains`), generation-tagged sound keys
  (`hfSoundKey`), fixed-slot sound bank (`hfSoundBank`).
- `hflib/audio/hfAudioSpatial.h` — positional layer: `hfSoundListener`
  (position + orthonormal forward/right basis), `hfSpatialEmitter` (per-emitter
  play/volume/pitch/spatialized/minDistance/maxDistance/attenuationModel fields,
  mirroring SynapseEngine's `AudioSourceComponent`), `hfSpatialEvaluate()`
  returning per-left/right mixer gains and stereo pan.
- `sound/hfMiniAudio.cpp` / `.h` — miniaudio wrapper (`HF_Audio::hfMiniAudioEngine`):
  device init/shutdown, `LoadSound` / `PlaySound` / `StopSound` /
  `SetListener` / `SetEmitter`, per-frame `Update()` driving the positional
  layer through the pure `hfAudioSpatial` API.
- `sound/hfAudioManager.cpp` / `.h` — asset-level manager bridging the
  sound bank + mini audio engine; file loaders (`hfMiniAudioLoader`).
- `sound/hfAudioEngineInit.cpp` / `.h` — wired into `R_InitOpenGL()`; registers
  console commands: `audioInit`, `audioPlay`, `audioStop`, `audioVolume`,
  `audioPitch`, `audioListener`, `audioEmitter`, `audioStatus`.
- **Harness coverage**: `Test_Audio` (attenuation + pan + bank + sample info)
  and `Test_AudioSpatial` (listener/emitter evaluation: zero-distance mono,
  directional pan, back-centre, non-spatialized mono, muted emitter) in
  `hfstest_features.cpp`; fully headless, no OS/miniaudio dependency.

### Pure logic sharing
`hfAttenuationGain`, `hfPanGains`, `hfSoundBank` alloc/free/generation and
the full `hfSpatialEvaluate` path are all inline / header-only, so the offline
harness exercises the exact code path the runtime `Update()` drives through.
No `hfLib::Printf` in the harness (stubs NULL → segfault); `printf` is used.

## 7. Collision query & spatial mesh module (`hfCollide`)

### Files
- `hflib/bv/hfCollide.h` — header-only; registered in `hfLib.h` after
  `bv/hfBounds.h`. Provides:
  - **`hfFrustum`** — 6 inward-facing `hfPlane` half-spaces;
    `FromViewProjMatrix` (Gribb-Hartmann for row-major `hfMat4`),
    `ContainsPoint`, `hfFrustum_FromCorners`.
  - **`hfIntersection`** enum — `HF_I_OUTSIDE` / `HF_I_INTERSECT` /
    `HF_I_INSIDE`, mirroring SynapseEngine's `IntersectionType`.
  - **`hfCollide` namespace** — inline testers, fresh implementations of
    the `CollisionTester` pattern: `TestSphereSphere[Type]`, `TestSphereBounds`
    / `TestSphereBoundsType` (per-axis containment), `TestSphereAabb`,
    `TestSphereFrustum[Type]`, `TestBoundsFrustum[Type]`, `IsInFrustum`
    / `IsInFrustumType` (sphere precheck + exact bounds accept), `TestConeSphere`
    (spot clip: range, half-angle cosine/sine), `SphereScreenSize`,
    `CalculateLodFromScreenSize`, `ClosestPointOnTriangle` (Barycentric),
    `RayTriangle` (Moller-Trumbore).
  - **`hfMeshCollider`** — indexed triangle soup (vertices + indices + bounds),
    with `ClosestPoint`, `RayTrace` (closest-hit Moller-Trumbore), and
    `SphereTrace` (Kasper Fauerby swept-sphere-triangle: inflated-face plane
    test + edge capsule + close enough). Pattern mirrors SynapseEngine's
    `MeshColliderSystem` baking `physicsVertices`/`physicsIndicesPerLod` into
    the physics engine's mesh body (`IPhysicsEngine::CreateMeshBody`) —
    here answered headlessly so the harness can exercise it.
- **Harness coverage**: `Test_Collide` (sphere/sphere, sphere/AABB, sphere/frustum
  6-sided, cone/sphere spot, ray-triangle hit/miss, `hfMeshCollider` floor
  ray-trace + swept-sphere drop in `hfstest_features.cpp`); compile/run-time
  verified on headless aarch64.
- `renderer/hfLightCluster.h` — existing forward cluster can adopt the new
  `hfCollide::TestConeSphere` for spot-light frustum clipping.

## 8. Scene & world core (general-purpose game layer)

Headless-friendly scene foundation, the first step toward a general-purpose
engine. Mirrors the SynapseEngine layout (`Component/Core`, `System/Core`,
`Scene/Hierarchy`) using Hellforge's ECS.

- `hflib/hfEcs.h` — component stores upgraded from a fixed 256-slot array to a
  growable heap store (`EnsureCapacity`), so scenes are no longer capped.
- `hflib/scene/hfTransformComponent.h` — local T/R/S + `worldMatrix` (written by
  the transform system); `LocalMatrix()` builds the TRS matrix from the
  native `hfQuat`/`hfMat4`; `TransformPoint/Direction`, `WorldRight/Up/Forward`.
- `hflib/scene/hfHierarchyComponent.h` — hierarchy marker;
  `hfTagComponent.h` — entity name; `hfCameraComponent.h` — fov/aspect/near/far,
  those matrices, `ComputeProjection` (ndc depth [-1,1], camera -Z forward).
- `hflib/scene/hfSystem.h` — system base (name, priority, `OnRegister`,
  `Update`); systems talk only to the `HF_Ecs::Registry`, never the world, so
  they stay header-only and reusable.
- `hflib/scene/hfHierarchySystem.h` — owns parent/child linkage indexed by
  entity (survives component-store repacking); `SetParent` (with ancestry
  cycle check), `PreDestroy` (adopts children out), `Validate` (repairs stale
  parent links), `GetChildren`/`IsDescendant`.
- `hflib/scene/hfTransformSystem.h` — computes world matrices parent-first via
  a DFS from every root (priority 10, after hierarchy 0).
- `hflib/scene/hfCameraSystem.h` — refreshes `viewMatrix` (inverse world) +
  `projectionMatrix`; static `ProjectToClip` for pure-math projection (priority 20).
- `hflib/scene/hfWorld.h` — one world = one scene: owns the registry + built-in
  core systems (hierarchy/transforms/cameras, auto-registered), an ordered
  system pipeline (`RegisterSystem`/`FindSystem`/`Update(dt)`), entity helpers
  (`CreateEntity[tag]`, `DestroyEntity`, `FindEntityByTag`, `SetParent`,
  `GetChildren`).
- **Harness coverage**: `Test_Scene` in `hfstest_features.cpp` — child world
  position under parent translate/rotate/scale, children/tag queries, cycle
  prevention, parent-destroy adoption, camera NDC projection, and a 600-entity
  grow test past the old 256-slot cap. `Scene ok`, `features logic` green.

Note the headless math convention discovered: `hfQuat * vec` evaluates
`q⁻¹ v q`, i.e. the visual rotation is the **negation** of the quaternion's
axis (a `hfQuat(0,0,-0.7071,0.7071)` is a visual +90° about +Z).

## 9. Phase 2 — lights, GPU-agnostic culling, resources, scene files

Layer sitting between the world core and the GL backend. Everything here is
CPU/headless-testable; GL/Vulkan front-ends consume the results.

- `scene/hfLightComponent.h` — point/spot/directional parameters (color,
  intensity, range, cone half-angles, shadow flags, `enabled`).
- `scene/hfLightSystem.h` — per-frame world-space resolve: each light's
  position + world direction (`-Z` forward, camera convention) into a
  `hfWorldLight`; `priority = intensity` is the cluster sort key feeding
  `HF_Cluster::AssignLights`. Runs after transforms (priority 15).
- `scene/hfMeshRendererComponent.h` — mesh/material ids, model-space
  `localBounds`, shadow + visibility flags.
- `scene/hfRenderCullSystem.h` — the active camera's
  `projection*view` → `hfFrustum`; per mesh renderer expands `localBounds` to
  a world AABB (8-corner `TransformBounds`) and tests it with
  `hfCollide::TestBoundsFrustum`, emitting an ordered `hfDrawEntry` list
  (entity, mesh/material ids, world matrix, bounds, camera distance). Runs
  after cameras (priority 30).
- `resource/hfResourceManager.h` — typed resource registry over a validated
  `HF_Asset::PackedStore`: `Create/Find/Extract` by name, ref counts, and
  `Bump/IsDirty/AckDirty` generation markers for hot reload. Rejects malformed
  stores on `LoadStore`.
- `scene/hfSceneSerializer.h` — text scene files (one directive per line,
  `#` comments): writer dumps every transform entity + tag/parent/camera/
  light/mesh; reader rebuilds the world with two-pass parent binding. Small
  enough to ride inside an asset pack.
- **Harness coverage** (`hfstest_features.cpp`): `Test_Lights` (rotated spot
  resolves `direction = -Z` → `+Y` after visual +90° about X; disabling a
  light drops it from the resolve), `Test_Resources` (pack→manager, extract,
  material parse, ref counting, hot-reload markers, bad-store rejection),
  `Test_Serialization` (write→read round trip with hierarchy, transforms,
  camera fov/near/far, light type/intensity/shadow, mesh ids + bounds).
  `Lights/Resources/Serialization ok`, `features logic` green.

## 10. Phase 3 — rigid-body physics (modern-console conventions)

Built in the direction of the newer engine generation (UE/Unity/Godot-shaped):
fixed-timestep dynamics behind a variable-dt accumulator, impulse solving with
Baumgarte-style penetration recovery, triggers with enter/stay/exit semantics,
sleeping, and a SAP broadphase — not a fixed-step toy.

- `physics/hfRigidBodyComponent.h` — body type (static/dynamic/kinematic),
  collider (sphere radius / box half-extents), material block
  (`restitution`, `friction`), `gravityScale`, trigger flag, collision group
  **as a bit** (`layer = 1u` default) + `mask`, sleeping state, and
  `RefreshMass()` (static/kinematic are infinite-mass).
- `physics/hfBroadphase.h` — X-axis sort-and-sweep: insertion-sorted mins.x,
  sweep break on `B[0].x > A[1].x`, pairs are self-deduplicating (emitted
  i<j), so no pair-set hashing. Plate/skin in a BVH or uniform grid behind the
  same interface (`Add` -> `Build` -> `GetPair(i)`).
- `physics/hfPhysicsSystem.h` — `hfSystem` at priority 25, after transforms.
  Fixed step `1/60s`, accumulated with `MAX_STEPS_PER_UPDATE = 4` clamp.
  Pipeline per step: collect bodies (top-level only) -> gravity-Euler integrate
  -> SAP -> SAT narrowphase (sphere/sphere; sphere/OBB with nearest-face exit
  for centers inside the box; OBB/OBB on 15 axes) -> sequential impulse +
  friction -> penetration correction (slop 0.005, factor 0.8, never relifts
  past contact -> resting bodies stay stable and can sleep) -> transform write
  -> sleep bookkeeping -> trigger diff (enter/stay/exit) from the persisted
  overlap set.
- Collision groups gate after the broadphase: a pair resolves only if
  `(A.mask & B.layer)` and `(B.mask & A.layer)` are both nonzero (so layer 0
  is "belongs to no group" and is excluded by any mask). Sleeps at
  lin/ang 0.02 over 1 s, woken by contact with a mover.
- One frame of render lag is standard and documented (physics writes
  transforms; the render cull sees last frame's states).
- **Harness coverage** (`hfstest_features.cpp`) `Test_Physics`:
  (a) broadphase unit (far boxes -> 0 pairs, overlapping -> exact 1 pair);
  (b) falling sphere rests on a static box floor top (z=0) at z≈0.5 and
  sleeps with zeroed velocity, gravityScale-0 body hovers; (c) equal-mass
  elastic head-on swap (`e=1`, gravity 0) reverses the x velocities at ≈unity
  magnitude; (d) triggers: mask-exclusion first (groups gated -> no enter, 0
  active overlaps), then enter within 120 steps, active overlap pair
  reported, then teleport-out -> exit event and overlap release.
  `Physics ok`, `features logic` green.

A production-grade back-end (e.g. Jolt) can later replace this system's Step
narrowphase/solver while keeping `hfRigidBodyComponent` and the world-pipeline
contract.

## 11. Phase 4 — PBR material + rendering server (Godot-RenderingServer split)

The GPU-neutral half of the render path, validated headlessly before any GL
front-end runs on this host (no GPU here).

- `hflib/render/hfMaterial.h` — `hfPbrMaterial`: baseColor (RGBA linear),
  metallic/roughness/ao/emissiveScale matching the uber shader `rpUser1`
  lane (`roughness, metallic, ao, emissiveScale`), emissiveColor, ambient
  color+scale matching `rpUser4`, albedo/normal texture resource ids, and
  flags (noShadow, transparent, twoSided, polygonOffset). `BuildMaterial`
  assembles one from a `.mtr` `mtrDef_t` (stage 0 = albedo, stage 1+
  `vertextexgen normalmap` = normal, `blendfunc blend/alpha` -> transparent,
  `noshadows` -> flag). Fog (`hfFogParams`: none/linear/exp2) and ACES
  filmic tone mapping (`ACESFilmic`/`ACESTonemap`) are pure functions.
- `hflib/render/hfRenderingServer.h` — Godot-style server: RID-handled
  textures/materials (`CreateTexture`/`CreateMaterial`), a per-frame
  `BuildFrame` that resolves the cull `hfDrawEntry` list + `hfEnvironment`
  into `hfDrawCommand` — a struct whose vec4 lanes ARE the uber shader
  uniforms 1:1 (memcpy-able to a UBO) plus per-command fog factor, texture
  RIDs and transform. A CPU reference rasterizer (`RasterReference`, 64x64
  scanline triangle with diffuse + simple ambient, fog blend then ACES)
  proves the lighting/tonemap pipeline offline.
- `hfEnvironment` carries frame-wide exposure, ambient color/scale and fog;
  draw commands carry per-instance material resolution.
- **Harness coverage** (`hfstest_features.cpp`) `Test_Materials`:
  (a) `.mtr` -> material assembly (albedo, transparent, noshadows, normal
  stage, malformed-def rejection); (b) ACES monotonicity + saturation and
  linear/exp2 fog factors; (c) `BuildFrame` resolves `rpUser1/rpUser4`
  lanes, texture RIDs and fog factors from distance; (d) CPU raster: lit
  center bright, background corner exact, back-lit surface reduced to
  ambient, fogged surface collapses into the fog color.
  `Materials ok`, `features logic` green.

## Notes
- Neither path can be executed on this build host (headless aarch64, no GL
  context), so validation is compile-time only here; run the above checks on a
  machine with a GPU.
- Both subsystems degrade gracefully: a failed compile/link or missing driver
  entry points always restore the original engine behavior.
## 12. Phase 5 — import & content pipeline (glTF subset -> packed store + CSG)

Like every other Phase, this side is **fully headless**: importer + JSON
parser + base64 + asset-store packing are compiled and exercised offline,
so nothing is held hostage by the missing-GPU fail path.

- `hflib/asset/hfJson.h` — a dependency-free RFC-8259 JSON parser
  (`HF_Json::Parse/Free`, `Value` tree with `Find/Get/Count`, num/str/
  arr/obj/bool accessors) used by both the glTF importer and the Phase-9
  editor tooling. Stored with real-buffer copies (no pointer aliasing),
  validated by round-tripping a nested doc and a trailing-garbage case.
- `hflib/asset/hfGltf.h` — a glTF 2.0 subset importer (`HF_Gltf::ImportGltf` +
  `ImportGltfBin`): buffers (data-URI base64, little/big-endian swap-aware
  since `Swap_Init()` may be lazily installed), bufferViews, accessors
  (POSITION/NORMAL/TEXCOORD_0 + indices), PBR materials, node hierarchy
  with translation/rotation/scale -> mesh-local transforms. Assembles the
  offline content items exactly the way libhflib's builder (`hfGltf::BuildItems`)
  does: one `AssetSource` for the relinked mesh blob and one for the
  resolved `.mtr`, packed through `hfPackedStore` and verified with
  `Pack`/`Validate`/`Extract`.
- `hflib/scene/hfCsgComponent.h` + `hfCsgSystem.h` — CSG brushes as scene
  entities: `.mtr`-agnostic `hfCsgComponent` (box/sphere/quad) tessellated
  by a system into a triangle soup with local bounds (12 tris for a box,
  20 for a sphere, 2 for a quad) fed into the render-cull path like a normal
  mesh.
- **Harness coverage** (`Test_Import`): (a) JSON parse of arrays/objects/
  escaped strings + trailing-data rejection; (b) base64 buffer decode of a
  triangle glTF: POSITION recurts into 3 verts, indices+material resolve,
  node translation moves the mesh to (2,0,0), bounds center ≈ (0.5,0.5,0);
  (c) BuildItems + PackedStore round-trip extracts a matching mesh blob and
  `.mtr`; (d) CSG box/sphere/quad tessellation asserts tri counts and bounds.
  `Import ok` -> `ALL PASSED` (0 failures).

## 13. Q3 `.bsp` v46 loader + `.pk3`/`.zip` filesystem

The two halves of the Quake III content path: geometry comes out of a BSP,
textures/models/sounds out of zip archives mounted into the search path. Both
are pure file-format work, so both are validated offline against synthetic
fixtures built in-process.

### `hflib/hfBsp.h` — IBSP v46 reader

- **Header and lumps.** `IBSP` ident + version 46 (checked explicitly, so a
  future layout is rejected instead of misread), then 17 `fileofs/filelen`
  entries. Every lump is bounds-checked against the file before it is used,
  and a lump whose length is not a whole number of records is refused rather
  than truncated. Errors are returned as `HF_BSP_ERR_*` with a readable
  string; a rejected file leaves the object zeroed.
- **Traversal.** `dnode_t` / `dleaf_t` / `dsurface_t` in their real 9/12/12-int
  layouts, with `planenum` resolved back to a plane. Child indices are stored
  as `-(leaf+1)`, so `GetChild()` hands back a node or a leaf correctly instead
  of treating a negative index as a raw pointer.
- **Brushes.** Sides are re-derived from the plane list (Q3 stores only
  `planeNum`/`contents` per side), which is what makes winding, point
  containment and bounds possible. `ContainsPoint()` and `ComputeBounds()`
  are exact for axis-aligned and rotated brushes; `BuildMesh()` fans the
  projected corners around the centroid for a usable collision/render proxy.
- **Entities.** The entity lump is parsed as key/value pairs with quoting
  tracked, so `\"` survives, and in idTech's own exact layout: no brace
  before the first entity, `}\n{\n` between entities, one `}\n` at the end.
- **Harness coverage** (`Test_BSP`): builds a complete synthetic map in memory
  (box brush, planar quad, node/leaf/surface lumps, two entities) and checks
  (a) bad ident, bad version, truncated header, out-of-file lump and
  non-multiple lump size are each rejected with the right error; (b) a good
  file returns `HF_BSP_OK` and every lump count matches; (c) nodes, leafs,
  surfaces and models carry the values that were written; (d) brush plane
  count, point containment and exact bounds; (e) entity count plus the
  multi-line and escaped-quote values. `BSP: ok`.

### `.zip`/`.pk3` archives

- **Discovery and validation.** `baseq3` and any `fs_basepath`/game directory
  is scanned for `*.pk3` and `*.zip`, in sorted order, and each candidate has
  to parse as a zip end-of-central-directory record before it is mounted. A
  file that is not an archive is skipped with a warning instead of aborting
  initialization, so one stray `.zip` in a download folder cannot break the
  whole search path.
- **Mounting and lookup.** Mounted archives take part in the normal search
  path, so a lookup walks the real directories and the archives in priority
  order and returns the first hit. Path matching is case-insensitive and
  treats `/` and `\` as the same separator (`hfStr::IcmpPrefixPath`), which is
  what mount-time `sound/vo/` style prefix tests need.
- **Reads.** `hfFile_InZip` exposes a normal `hfFile` to the rest of the
  engine: sequential `Read`, `Seek`, `Tell`, `Length` and a compressed-data
  cursor shared through the bundled minizip-compatible inflate code in
  `framework/hfUnzip.cpp`. `OpenFileReadMemory` slurps an archive member
  through the same path, so asset loaders do not care whether their bytes
  came from disk or from a `.pk3`.
- **Harness coverage** (`Test_Zip`): the member data path is checked against
  a real third party deflate stream (python's zlib, lifted out of a zip file,
  so it is dynamic Huffman rather than a friendly stored block): it inflates to
  the expected payload and stops there, its CRC32 matches the value the
  archive recorded, a truncated member reports an error instead of a partial
  success, and a corrupted payload no longer matches the CRC. A round trip
  through the bundled deflate is checked as well.
- **Known gap.** Mounting itself (`AddGameDirectory` scanning a game
  directory, archive validation, search-path priority) is not reachable from
  the headless harness: `hfFileSystemLocal` is defined in a `.cpp`, and
  linking it drags in the session, command, render and sound systems. That path
  is verified by inspection against the idTech3 layout rather than by an
  executed test, and is the first thing to cover when the engine can be linked
  headless.

## 14. Phase 7 — Jolt physics backend (vendored, opt-in)

`hfPhysicsSystem` used to mean exactly one thing: the built-in impulse solver in
`hflib/physics/hfPhysicsSystem.h`. It still compiles and still runs, but it is
no longer the only solver behind that interface.

### What was added

- `external/jolt` — the MIT Jolt Physics library, vendored unmodified from
  Godot's `thirdparty/jolt_physics` (Godot is MIT; SynapseEngine's AGPL
  Jolt modifications were deliberately not taken). Only the licence and the
  upstream sources are in the tree.
- `hflib/physics/hfJoltConfig.h` — pins `JPH_CROSS_PLATFORM_DETERMINISTIC`,
  `JPH_OBJECT_LAYER_BITS=16`, `JPH_ENABLE_ASSERTS=0`, `JPH_PROFILE_ENABLED=0`
  and `JPH_DEBUG_RENDERER=0` *before* any Jolt header is pulled in. Jolt folds
  these into `JPH_VERSION_ID` and aborts inside `RegisterTypes()` when a library
  and its client disagree, so the adapter has to state them itself rather than
  trust every build system to pass the same set.
- `hflib/physics/hfPhysicsLimits.h` — `HF_MAX_PHYSICS_BODIES`,
  `HF_MAX_OVERLAPS`, `HF_MAX_OVERLAP_EVENTS` and `hfOverlapEvent`, which both
  the built-in solver and the Jolt adapter size their buffers from.
- `hflib/physics/hfJoltBackend.h` — the adapter itself, behind
  `-DHF_PHYSICS_HAS_JOLT=1`.
- `hfPhysicsSystem` grew `BackendType { BACKEND_BUILTIN, BACKEND_JOLT }`,
  `SetBackend`, `GetBackend`, `GetBackendName`, `IsBackendAvailable` and
  `PreferredBackend`. With Jolt out of the build, `IsBackendAvailable` is false
  and `SetBackend(BACKEND_JOLT)` quietly falls back, so game code can ask for
  Jolt unconditionally.

### How the engine's semantics are mapped

| engine | Jolt |
| --- | --- |
| `HF_BT_STATIC` / `DYNAMIC` / `KINEMATIC` | `EMotionType::Static` / `Dynamic` / `Kinematic` |
| `layer` + `mask` | `ObjectLayer` + an `ObjectLayerPairFilter` that re-evaluates the engine's own `(maskA & layerB) && (maskB & layerA)` rule |
| `isTrigger` | `BodyCreationSettings::mIsSensor` |
| `radius` / `halfExtents` | `SphereShape` / `BoxShape` |
| `gravityScale` | `mGravityFactor` |
| `offset` | body position is the centre of mass, entity origin is `com - rot * offset` |
| `restitution`, `friction` | per body, and the engine's "a pair uses the larger of the two materials" rule applied in `OnContactAdded` / `OnContactPersisted` |
| `asleep` | Jolt owns sleeping; the component mirrors `IsActive()` and is zeroed on sleep |
| enter / stay / exit | `OnContactAdded` / `OnContactPersisted` / `OnContactRemoved`, buffered and de-duplicated into the same `hfOverlapEvent` stream the built-in solver produces |

Transforms, velocities and angular velocities sync both ways every step, so a
game cannot tell from the outside which solver is running.

Jolt is stepped on `JobSystemSingleThreaded` on purpose. The harness requires
determinism, and an island solver allowed to use every core would make results
depend on the thread schedule. The object layer to broadphase layer mapping
puts layer 0 in the moving broadphase layer and the rest in the static one,
which costs broadphase accuracy, not correctness: `ObjectVsBroadPhaseLayerFilterTable`
is built once from the pair filter and Jolt re-checks the real layer pair in the
narrowphase.

### Build

CMake, off by default so a plain build is unchanged:

```
cmake -DHF_ENABLE_JOLT=ON ..
make hellforge -j$(nproc)
```

The flag compiles all of `external/jolt/Jolt`, links it as `jolt`, and puts
`HF_PHYSICS_HAS_JOLT=1` on `hflib` as a PUBLIC definition so every translation
unit — including the ones the harness compiles on the fly — agrees with the
library. Hand-rolled builds need `-DHF_PHYSICS_HAS_JOLT=1 -Iexternal/jolt` and
the five Jolt macros above.

### Harness coverage (`Test_PhysicsBackend`)

New, and it runs every check against **both** solvers when Jolt is in the build
and against the fallback alone when it is not:

- **Stack stability.** A static floor plus a three box stack, stepped 600
  times. Jolt must leave every box at the height it was placed at (±0.2), with
  near-zero velocity. The built-in solver is deliberately *not* asked to hold a
  stack: it is a single pass impulse solver with no position correction and no
  contact caching, so it sinks through a stack, which is the entire reason this
  backend exists. It is still held to the contract it can meet — nothing
  explodes, nothing leaves the world, nothing becomes a NaN.
- **No creep.** A body that has come to rest must not drift over a further 400
  steps (±1e-3), checked on both backends.
- **Determinism.** The same scene, built and stepped again, must land on
  bit-identical positions, velocities and sleep flags.
- **Triggers.** A sphere driven through a static trigger must report `enter`
  then `exit`, and the trigger must not change the sphere's velocity.
- `Test_Physics` itself is untouched and still passes under both backends.

```
Physics ok
PhysicsBackend ok (builtin + jolt)
```

### One integration trap worth knowing

Jolt includes `<cfloat>`/`<cmath>`, which define `FLT_EPSILON` and `INFINITY` as
macros. `hflib/math/hfMath.h` deliberately `#undef`s both and declares its own
constants with exactly those names, so a translation unit that reaches Jolt
through the adapter breaks on its own math (`expected unqualified-id before
numeric constant` in `hfMath.cpp`, `hfMatrix.cpp`, `hfMatX.cpp`). The adapter
takes both macros back out after its Jolt includes, which restores the engine
invariant that they are undefined in every translation unit. Any future vendored
library that pulls in `<cfloat>` needs the same treatment.

### Known gaps

- Only spheres and boxes are mapped. Convex hulls, meshes, cylinders and
  character controllers are not implemented, because
  `hfRigidBodyComponent` cannot express them yet.
- No serialisation of Jolt bodies: a saved scene reloads through the built-in
  solver's state, not a Jolt snapshot.
- The `layer`/`mask` mapping is per *layer*, so two bodies that share a layer
  but declare different masks cannot both be honoured; Jolt's
  `ObjectLayerPairFilter` only ever sees two layers. The engine's own rule is
  reproduced exactly as long as masks agree within a layer, which is what the
  game code does today.
- The `layer`/`mask` pair test is re-evaluated by Jolt's narrowphase, but the
  broadphase layer mapping itself is fixed at `PhysicsSystem::Init` time; a body
  that changes motion type keeps its original broadphase layer until it is
  recreated. Harmless for the harness, worth revisiting before shipping.

## 15. Phase 8 — headless mode + the threaded simulation server

Two things shipped together, because the second one is only useful if the first
one exists: a `--headless` switch that means "no window, no GPU, full
simulation", and a simulation server that advances the world on a worker thread
and publishes a snapshot for whatever front end is attached.

### What was added

- `hflib/hfHeadless.h` — the switch, and the process-wide flag behind it.
- `hflib/server/hfSimulationServer.h` — the server, in the Godot `Servers`
  sense: it owns the per-frame advance of an `hfWorld`, runs it on a worker
  thread, and publishes an immutable snapshot. It does not own the world.
- `sys/linux/hflinux_main.cpp` — reads the switch **before** it opens the X11
  display and skips the display entirely when headless.
- `framework/hfCommon.cpp::ParseCommandLine` — lifts the switch out of `argv`
  before the `+` arguments are tokenized into console lines, so `--headless`
  never becomes a "command not found".
- `script/tests/hfstest_features.cpp` — `Test_Headless` and `Test_SimServer`.
- Two fixes in the threading layer that Phase 8 forced into the open, detailed
  below. Both were latent before because nothing ever waited on anything.

### Why the flag lives in hflib

It has to be answered before any subsystem exists: `main()` needs it before
`XOpenDisplay`, and the server needs it before it decides whether a front end
is involved at all. That rules out a cvar (needs the command system), a
component (needs an entity), and anything in `framework/` (the harness does not
link `framework/`). So it is a function-local static in an inline function in
`hflib`, which every translation unit already reaches through `hfprecompiled.h`
→ `hfLib.h`, and which costs nothing when unused.

Recognised anywhere in `argv`, case insensitive: `--headless`, `-headless`,
`+headless`. `headless` without a prefix and `--headless1` are not switches.

```c
HF_Lib::ParseHeadlessCommand( argc, argv );    // -> true when headless
HF_Lib::IsHeadless();                          // -> asks anyone, anytime
```

In `hflinux_main.cpp` the display open is now conditional, and `DefaultScreen()`
is guarded with it, because `DefaultScreen()` dereferences the display pointer.
Everything downstream already treats `x_display == NULL` as "there is no front
end": `GLimp_Init` returns false, input and `Sys_GenerateEvents` check the
pointer first. Headless is therefore not a new code path so much as the
existing null-display path finally being reachable on purpose.

### The split: world in, snapshot out

```
hfSimConfig cfg;                     // dt = 1/60, threaded = true
hfSimulationServer sim( &world, cfg );
sim.SetSeed( 0xC0FFEE );
sim.AddInput( { step: 30, entity: ball, impulse: (2,0,0) } );
sim.Start();
for ( int frame = 0; ; frame++ ) {
    sim.Kick();                      // worker starts step N
    hfSimSnapshot snap;
    sim.CopySnapshot( &snap );       // step N-1, already done
    ... render / audio / network consume snap ...
    sim.WaitForStep();               // let step N finish
}
sim.Stop();
```

Without `Start()` every `Kick()` runs inline on the calling thread, which is
what makes the determinism check meaningful rather than circular.

- **`hfSimSnapshot`** carries `sequence`, `step`, `simTime`, `seed`,
  `inputsApplied`, `bodyCount`, a body array and a checksum. The body is
  world-space position, local rotation, velocity and sleep flag — no pointers,
  no handles into the world, no reference to anything the worker might free.
- **The checksum** is FNV-1a over every byte before it and is recomputed from
  the copy the consumer holds, so "byte-identical" is a checked property and
  not a claim about a memcmp somebody forgot to write.
- **`CopySnapshot` copies**, under a short mutex that covers only the
  assignment. There is deliberately no API that hands out a pointer into the
  published buffer: a pointer creates an ordering problem between the reader
  and the worker's next publish, and a copy does not.
- **RID handles** mirror `HF_Render::hfRid` — `hfSimRid` plus a static
  `Register`/`Resolve`/`Count`, with the slot pool in a function-local static so
  the header stays header-only (the script JIT compiles hflib headers mid-air,
  and an out-of-line static would break that). Freed slots are reused, so a
  session that builds and tears down servers never exhausts the pool.
- **Only the worker touches the world while it is stepping.** The ECS systems
  are unchanged and know nothing about threads; thread-agnostic here means the
  world has exactly one writer at a time, which is enforced by ownership rather
  than by locks around every component.

### Determinism contract, and what the harness asserts

A step is a pure function of the world, the seed and the input log. Nothing in
the server reads a clock.

`Test_SimServer` builds a scene whose initial layout is derived from the seed
(a static floor, a ball dropped from a seeded position, a seeded peg), then:

- runs it twice inline and twice on the worker: all four checksums equal;
- **threaded checksum == inline checksum**, and `IsStarted()` is asserted so the
  comparison cannot pass vacuously with no worker in the build;
- a different seed changes both the checksum and the ball's landed position
  (the seed has to move the *world*, not just a field in the struct);
- dropping the input log changes both again;
- the kick/read/wait overlap pattern, reading a snapshot while the next step is
  already running, ends at `sequence == step == 120` with
  `checksum == ComputeChecksum()`;
- an inline (`threaded = false`) server never starts a thread and still steps.

```
Headless ok (started with --headless)
SimServer ok (120 steps, checksum 3911319015, worker thread)
features logic: ok (0 failures)
ALL PASSED
```

The checksum is identical with and without `--headless`: the flag changes what
is presented to, never what is simulated.

### The blocking fix underneath: Win32 events on Linux

`hfSysThread`'s handshake is written against Win32 events — a state bit with
`Wait`, `Raise`, `Clear`, a timeout in milliseconds, and a distinction between
auto-reset (one wake, consumed by the wait) and manual-reset (stays observed).
The Linux implementation had a bare `pthread_cond_t` and a `Sys_SignalWait`
that **always returned false without waiting**:

```c
if ( timeout == -1 ) { return false; }   // "block forever - not supported"
return false;                            // any timeout
```

So `WaitForThread()` returned immediately, `StartWorkerThread()`'s readiness
wait was a no-op, and the worker spun on `signalMoreWorkToDo.Wait()` in a busy
loop. Nothing had noticed because no code in the harness or the engine had
ever actually started a worker.

A condition variable alone cannot be an event: it has no state to clear,
nothing to poll, and it forgets every broadcast that arrives with no waiter.
`hfsys_threading.h` now defines

```c
struct hfLinuxSignal { pthread_cond_t cond; pthread_mutex_t mutex;
                       bool signaled; bool manualReset; };
typedef hfLinuxSignal signalHandle_t;
```

and the five `Sys_Signal*` functions implement the Win32 semantics: `timeout`
`-1` blocks, `0` polls, otherwise milliseconds; a successful wait consumes the
signal when the handle is auto-reset; `Clear` is a real operation. `signalWorkerDone`
is manual-reset (a stop stays observed by every later waiter) and
`signalMoreWorkToDo` is auto-reset (one wake per piece of work), which is what
`hfThread.cpp` is written against.

Only `hfThread.cpp` and `hflinux_glimp.cpp` reference `signalHandle_t`, and
both go through the `Sys_*` functions, so the type change is contained.

### The shutdown race that only appears once waits are real

With waits actually blocking, `hfSysThread::StopThread()` had a lost-wakeup
window: it read `isRunning` outside `signalMutex`, then cleared
`signalWorkerDone`. The worker's final act was to raise `signalWorkerDone` and
*then* clear `isRunning`. If the raise landed between the check and the clear,
the raise was erased and `WaitForThread()` waited on a signal nobody would ever
send again — the destructor of a server, after an ordinary successful run.

Both sides now do it under `signalMutex` (`hflib/hfThread.cpp`): the worker
publishes `isRunning = false` and the final raise as one locked step, and
`StopThread`'s check happens inside its own critical section. Whichever order
they meet in, the raise is either observed by a waiter that has not cleared yet,
or the stop returns early without clearing. It cannot be lost.

### Harness invocation

```
./hfstest                 # as before
./hfstest --headless      # same tests, headless flag on for the whole process
```

`main()` parses the switch before `hfLib::Init()`. The harness also treats
`argv[1]` as a script to run, so a switch is now skipped when picking that
argument — previously `--headless` was opened as a file, reported
`cannot open --headless` and counted a failure in an otherwise green run.

A watchdog thread is installed in the harness: if a run is still alive at
600 s it sends `SIGUSR1` to the main thread, whose handler prints a native
backtrace to stderr. Normal runs finish in 35–55 s, so only a genuine wedge
trips it. (A first attempt used `usleep()` for the delay; a large `usleep()`
value never returned under this runtime, which silently disarmed the watchdog,
so it uses `sleep()`.)

### Known gaps

- **Render and audio are not servers yet.** `hfRenderingServer` already owns
  RID handles for its own GPU-side objects, but nothing feeds it from a
  snapshot, and `hflib/audio/` is data plus spatialisation with no server and no
  handles. The simulation half of the phase is done; the presentation halves are
  not.
- **The engine front end is not wired to the snapshot.** `hfCommonLocal::Frame`
  still drives the world directly. Only the harness exercises the server.
- **One step of latency, not a scheduler.** `Kick`/`WaitForStep` overlaps one
  step; there is no fixed-rate thread that runs ahead of the consumer, so a slow
  consumer still gates the next kick.
- **The input log is thin.** `hfSimInput` is one impulse per (step, entity) —
  no button or analog state, no persistence, no replay from disk.
- **The snapshot has no previous state**, so a front end cannot interpolate
  between two of them yet.
- **A rare wedge remains open.** Roughly one run in twenty of
  `Test_SimServer` has been observed to hang (the harness watchdog above is how
  it was caught); twenty-plus consecutive runs pass. Root cause not yet pinned
  down. Reported rather than papered over.

## 16. Phase 8 (cont.) — the audio server

Section 15 shipped the simulation half of Phase 8 and listed the presentation
halves as a known gap: "`hflib/audio/` is data plus spatialisation with no
server and no handles." This closes the audio part. Rendering is still a gap.

### What was added

- `hflib/audio/hfAudioServer.h` — an `HF_Audio` server in the same shape as
  `hfSimulationServer`: it owns a worker thread, advances a mix on that thread,
  and publishes an immutable snapshot. It does **not** own the sound data or the
  emitters; they are registered into it and referenced by RID.
- `Test_AudioServer` in `script/tests/hfstest_features.cpp`.

`hfLib.h` now pulls in `audio/hfAudioData.h`, `audio/hfAudioSpatial.h`, and
`audio/hfAudioServer.h`, so the header is reachable from the same precompiled
path as everything else. (`hfAudioData.h` had been including `"hfStr.h"` with a
path that only worked when `-I hflib` was on the command line; the JIT compiles
its generated translation unit with a single `-I`, so that include is now the
correct `"../hfStr.h"`.)

### Handles, not pointers

The server hands out `hfAudioRid` — an index/generation pair — for samples and
emitters, exactly as `hfRid` works in the render server. The front end never
touches an emitter that the server owns; it holds a handle and asks the server
to move it. This is what lets the mix run on a thread while the game thread only
ever reads a copy-out snapshot.

```
hfAudioConfig cfg;                   // threaded = true
hfAudioServer audio( cfg );

hfAudioRid s = audio.CreateSample( nameHash, seconds );   // deduped by hash
hfSpatialEmitter e; e.play = true; e.spatialized = true;
hfAudioRid r = audio.CreateEmitter( e );
audio.SetEmitterPosition( r, pos );

audio.Kick();                        // start the worker on this step
audio.WaitForStep();                 // join its publication
hfAudioSnapshot mix; mix.Zero();
audio.CopySnapshot( &mix );          // voiceCount, voices[], checksum
```

`MixFrame` is the single publish point: it evaluates every live emitter against
the listener with `hfSpatialEvaluate`, writes voices in slot order, computes an
FNV-1a checksum over the whole snapshot, then bumps `sequence` under the lock.
The checksum is what the harness compares between an inline run and a threaded
run to prove the mix is deterministic regardless of who computed it.

### What the harness pins down

`Test_AudioServer` covers the parts that can silently drift:

- RID lifecycle: create, resolve, count, slot reuse after teardown, and
  `IsHeadless` agreeing with the process flag.
- Sample dedup: two `CreateSample` calls with the same hash collapse to one
  slot, and `DurationSec` round-trips.
- Spatial mix: an emitter at the listener is `gain 1.0` with `left == right`; an
  emitter off to one side pans with the far side quieter; an emitter past
  `maxDistance` is not audible; `volume` scales both channels.
- Determinism: `inlineA == inlineB == threadA == threadB`, all four trading the
  same checksum.
- The front-end contract: `Kick` / `WaitForStep` / `CopySnapshot` overlap
  correctly, `checksum == ComputeChecksum()`, and `voiceCount` matches the live
  set.
- Hand-off from the simulation: a body moved by an `hfSimulationServer`, read
  back and pushed into the audio server as an emitter position, arrives in the
  mix at that position.

### Known gaps

- **The engine front end is not wired to the audio snapshot.** Exactly as with
  the simulation server, only the harness drives it. `hfCommonLocal::Frame`
  still talks to the world directly.
- **No mixing, no DSP.** Voices are spatialised gains, not resampled or summed
  into a bus. The snapshot is the contract; the mixer is future work.
- **Rendering is still not a server.** This section only closes the audio half
  of the "presentation" gap in section 15.

## 17. Phase 9 — UI: canvas, layout, and a CPU text rasterizer

Phase 9 is the Control/Canvas layer: anchors, containers, clipping, scrolling,
hit-testing, and text laid out on the CPU so it can be verified without a GPU.

### What was added

- `hflib/ui/hfCanvasComponent.h` — geometry (`hfUiRect`, `hfUiMargin`,
  `hfUiAnchor`) and the `hfUiCanvasComponent` / `hfUiLabelComponent` data.
- `hflib/ui/hfBitmapFont.h` — a fixed-cell 5x7 bitmap font: measurement,
  greedy word wrap, and a coverage-buffer rasterizer. No font file, no GPU.
- `hflib/ui/hfUiSystem.h` — `hfUiSystem`, a registry-only `hfSystem` that
  resolves the component graph every layout pass.
- `Test_Ui` in `script/tests/hfstest_features.cpp`.

### The layout model

A control is an `hfUiCanvasComponent` with a parent (or `ENTITY_NULL` for a
root). Its rect is resolved from a `hfUiAnchor`, which is the Unity-style
pair of `anchorMin`/`anchorMax` in parent-relative units plus `offsetMin` /
`offsetMax` in pixels:

- `anchorMin == anchorMax` → fixed size (`offsetMax - offsetMin`), positioned at
  the anchor point.
- `anchorMin != anchorMax` → stretched; the offsets are insets from the two
  anchored corners. `HF_UI_ANCHOR_FULL_RECT` is the special case that fills the
  parent.

Roots anchor in the viewport, so `SetViewport( w, h )` is all the window state
the system needs. A container (`HF_UI_LAYOUT_ROW` / `COLUMN` / `GRID`) ignores
its children's anchors and places them in flow order instead — sorted by the
component's `order`, ties broken by entity so layout is deterministic — honouring
`padding` and `spacing`. A control that sets `clipChildren` clips its subtree to
its own rect, and `scrollX` / `scrollY` offset every child (anchored or
container-placed) by a fixed amount. Hit-testing walks the draw order
(`zIndex`, then depth, then entity) from the top down and returns the first
visible, interactive control whose *clipped* rect contains the point.

### Text on the CPU

`hfBitmapFont` is deliberately small: advance 6px, line height 9px, a 5x7
bitmap per glyph, lowercase folded to uppercase and an unknown glyph drawn as an
open box, so metrics never depend on a complete glyph table. Measurement and
wrapping count advances, not pixels:

- `MeasureText` → width of the longest `\n`-separated line.
- `WrapText` → greedy word wrap; a word wider than `maxWidth` overflows its
  line rather than being split.
- `Rasterize` → stamps glyph bitmaps into a caller-owned `hfUiCoverage` buffer.

This is the "text via CPU rasterizer first" the roadmap calls for: it is
headless-testable today, and a real glyph atlas can replace `Glyph()` later
without moving the metrics that layout depends on.

### What the harness pins down

`Test_Ui` builds one registry, drives a single `Update`, and asserts:

- Anchors: a full-rect root fills the viewport, a fixed panel lands exactly at
  its offsets, and a centred panel is centred.
- Containers: a row of three cells with padding and spacing produces the
  expected x positions.
- Clipping: a child that overhangs its clipping parent reports a visible rect
  that is the intersection, not its full rect.
- Scrolling: a scrolled column raises its children by the scroll amount.
- Hit-testing: overlapping controls resolve to the one on top (`zIndex`), an
  uncovered point hits the backdrop, and an empty point hits nothing.
- Text: `MeasureText` and `WrapText` return the expected widths and line
  counts, `Rasterize` writes coverage, and the system fills `lineCount` /
  `textWidth` on a wrapped and a scaled label.

### Known gaps

- **Nothing renders the controls.** `hfUiSystem` produces rects and a draw
  order; no renderer consumes them yet, so the UI is not visible on screen.
- **The font is a placeholder for shape only.** Metrics and wrapping are final;
  the glyph bitmaps are a starter set.
- **No input routing.** `HitTest` says *what* was hit; there is no event
  dispatch, focus, or widget behaviour (buttons, sliders, text fields).
- **No layout cache.** Every `Update` re-gathers and re-sorts the whole graph;
  fine for the harness, worth a dirty-flag pass before it drives a real HUD.

## 18. Phase 10 — Networking: replication over the deterministic sim

Phase 10 moves the 60 Hz fixed-step sim across a link. Because the sim is
deterministic, this is a *copy* problem rather than a reconciliation problem:
there is no second integration to correct, only the same numbers to transmit.
A peer that holds the same seed and the same input log reaches the same state
on its own, which is what `Test_Net` proves end to end.

### What was added

- `hflib/net/hfNetComponent.h` — the data. `hfNetRid` replication handles,
  `hfNetComponent` (ownership, the previous transform used for deltas, the
  last replicated step), `hfNetInputEvent`, `hfNetPeerStats` (acks and
  duplicate/drop counters), plus `HF_NET_MAX_PACKET`, `HF_NET_MAX_PEERS` and
  the reliable/unreliable channel ids.
- `hflib/net/hfReplicationSystem.h` — transport, channel, serializers, lag
  compensation, and the scene system, in that order.
- `Test_Net` in `script/tests/hfstest_features.cpp`.

### Transport behind an interface

`hfNetTransport` is three virtuals — `Send( from, to, channel, data, len )`,
`Receive( peer, out, maxOut )`, `Reset()` — and everything above it is written
against that interface, so swapping in a real UDP socket changes one class.
`hfNetLoopbackTransport` is the in-process implementation: a fixed ring queue
per peer, ENet-style per-sender-per-channel sequence numbers, and two fault
injection switches (`SetDuplicate`, `SetReorder`) so the channel's rejection
rules can be tested without a network.

`hfNetChannel` is the sequence validation half of a reliable channel: a
256-wide window keyed by `sequence % 256` that rejects a packet already seen
(`duplicatesDropped`) and a packet older than the window (`tooOld`), while
accepting out-of-order arrivals *inside* the window. Sequence 0 is never
valid, so a zeroed or unsent packet can never be mistaken for data.

### Serialization

All wire formats are `hfBitMsg` streams — bit-packed, byte-order safe, and
alignment-safe — framed as:

```
magic 0x48464E31 ('HFN1') | msg type | packet sequence | ... payload
```

`hfNetReplicator` owns four of them:

- `WriteSim` / `ReadSim` carry a whole `HF_Server::hfSimSnapshot` — step,
  sequence, `simTime`, seed, `inputsApplied`, `bodyCount`, then every body's
  entity, position, rotation, velocity and sleep flag.
- `WriteSimDelta` / `ReadSimDelta` are the same packet with a `changed` bit
  per body against a baseline. Unchanged bodies cost one bit; a mismatched
  `bodyCount` silently degrades to a full write. The decoder starts from the
  baseline and patches, so the receiver must hand in the state it is
  updating.
- `WriteInputs` / `ReadInputs` forward the input log (client → server). The
  count is clamped to the caller's buffer, so a short read truncates rather
  than overruns.

Two rules the code depends on. First, the packet sequence is **always read**,
whether or not the caller wants it back — skipping the read to "save" the
value desynchronizes every field after it. Second, a `hfBitMsg` built over a
non-const `byte *` calls `InitWrite` (write state, `curSize == 0`), so every
read comes back as `-1`; decoding must go through the `const byte *`
constructor, which calls `InitRead`. Both mistakes are silent: the first
produces plausible-looking garbage, the second produces all-ones.

### Lag compensation

`hfNetLagCompensator` is a bounded ring of `(step, hfSimSnapshot)` entries.
`Record` overwrites on a repeated step and evicts the oldest once full;
`Sample( step )` returns that exact step or `NULL` — never an approximation,
because interpolating between two snapshots would resolve a shot against a
world that never existed. `hfNetPeerStats::lastAckedStep` is the value fed to
it: the step the client actually saw, which is the step the authority rewinds
to when resolving what that client reported.

### The scene system

`hfReplicationSystem` is a `HF_Scene::hfSystem` (priority 95) with one role
per peer. As the **authority** it walks `hfNetComponent`, pulls each entity's
`hfTransformComponent`, writes `changed` per entity and, when changed, its
position and rotation — recording the previous transform as the baseline for
the next delta. As a **remote** it drains its own queue, validates the
sequence through the channel, resolves entities by `netId`, and applies the
transform directly to the component fields (bypassing `SetRotation`, which
normalizes and would perturb the bits). `Acknowledge( step )` is monotonic;
`Update()` is just `SendState` or `ReceiveState` on the right side.

### What the harness pins down

- **Snapshot round trip** is byte identical, checked with a second
  `ComputeChecksum()` over the decoded struct rather than field-by-field
  comparison, and a corrupted magic is rejected.
- **Delta < full** for a one-body change, and the delta decodes back to
  exactly the state it was written from.
- **Input forwarding** round-trips all four inputs, and a short buffer
  truncates to the caller's capacity.
- **Loopback and channel**: delivery, out-of-range and over-sized refusal,
  injected duplicates dropped, injected reordering accepted as valid
  out-of-order arrivals, late-in-window packets accepted, packets older than
  the window and sequence 0 rejected.
- **Lag ring**: exact-step lookup, replace-on-repeat, ring overwrite, and
  `NULL` for a step that was never recorded.
- **The determinism proof**: an authority runs a logged input stream, ships
  the log *and* its snapshot; three clients, one at a time, decode both,
  replay the log into a fresh sim built from the same seed, and arrive at a
  `ComputeChecksum()` identical to the authority's.
- **Transform replication**: three entities replicate authority → remote,
  an idle packet is smaller than a full one, a one-entity change is smaller
  than the full packet and only moves that entity, a replayed sequence is
  dropped by the channel, acks advance monotonically, and the same path runs
  through `Update()`.

### Known gaps

- **No socket.** `hfNetTransport` is an interface with only the loopback
  implementation; UDP, connection setup, MTU discovery and fragmentation are
  all unimplemented. `HF_NET_MAX_PACKET` (8 KB) is smaller than a full
  1024-body snapshot, so a large world would need fragmentation.
- **No reliability on top of the channel.** `hfNetChannel` validates
  sequences; it does not retransmit, and there is no ordered-delivery queue
  for the reliable channel — the caller drops out-of-window packets instead
  of asking for them again.
- **No peer lifecycle.** There is no connect/handshake, no disconnect, no
  timeout, and no `HF_NET_MAX_PEERS` enforcement in the interface itself
  (the loopback refuses out-of-range peers, nothing else checks).
- **Lag compensation is a hook, not a rewind.** The ring records and samples
  correctly, but nothing yet replays a shot against a sampled snapshot, and
  nothing feeds `lastAckedStep` into it automatically.
- **No input transport system.** Input forwarding is exercised as a
  serializer; there is no `hfSystem` that polls a local input queue and sends
  it, and no server-side queue that applies forwarded inputs by step.

## 19. Phase 11 — Editor: reflection, scene tree, and a round-trippable document

Phase 11 is the first code written *for the person in front of the screen*
rather than for the frame. The roadmap asks for a Godot-class scene editor —
gizmos, an inspector, play-in-editor, an import view — and the honest version
of that starts at the only layer all of those share: a document that can load
the text scene format, expose every component as editable fields, and write it
back without changing anything it did not touch.

The whole phase therefore turns on one sentence from the roadmap: *open → edit
→ save → reload keeps semantics, and the round-trip diff of the text scene file
is empty*. Everything here exists to make that sentence true and testable
without a window, and the gaps at the bottom are exactly the parts that need
one.

### What was added

- `hflib/editor/hfEditorReflection.h` — the property model: value kinds,
  per-field descriptors, component classes with typed thunks, and a small
  registry keyed by name and by `HF_Ecs` type key.
- `hflib/editor/hfEditorTree.h` — the outline: entities flattened into a dense,
  depth-annotated, deterministically ordered node array, plus an indented text
  dump for diffing.
- `hflib/editor/hfEditorDocument.h` — the document: open/edit/save, selection,
  the inspector sheet, and structural edits (create, delete, reparent, add
  component, rename).
- an extension to `hflib/scene/hfSceneSerializer.h` so the fields the
  inspector can edit are also fields the file can carry.
- `Test_Editor` in `script/tests/hfstest_features.cpp`.

### Reflection without RTTI or a code generator

There is no reflection in C++, and the editor does not get to use RTTI: the
types it must describe are template-instantiated ECS component stores, so
`typeid` would name the wrong thing anyway. Instead each component class is
described once by hand:

```
name     display name ("Transform")
typeKey  ComponentStore<T>::TypeKeyAddr()   — the address the registry already
                                                uses to identify the store
get/add/has  three thunks over Registry so the editor never spells T
props    the field table: name, value kind, byte offset, group, enum names
size     sizeof(T), so every offset can be bounds-checked
```

A field is then `base + offset`, and one `ReadValue`/`WriteValue` pair knows
every value kind: `FLOAT`, `INT`, `BOOL`, `STRING` (an `hfStr` at the offset),
`VEC3`, `QUAT` (three and four contiguous floats) and `ENUM` (an int named by
the class's own string table). Five builtins are registered —
Transform, Tag, Camera, Light, MeshRenderer — and registration is idempotent
so it can be called from a constructor without a static-initialization-order
trap. `TypeKeyAddr()` is public, which is what lets the editor key its table
the same way the registry keys its stores, and is why `hfEcs.h` did not have to
change.

The registry deliberately does **not** enumerate the stores a `Registry`
holds — that list is protected — so the inspector is driven by the hardcoded
known-component list rather than by "whatever is attached". That is the same
trade Godot makes with its class database, and it keeps the inspector showing
only fields the rest of the engine actually owns.

### The tree

An editor browses a hierarchy; the registry holds a flat set of entities and
the hierarchy component holds parent/child links. `hfEditorTree::Build`
flattens both into one array:

- the entity set is `Each<hfTransformComponent>`, the same set
  `hfSceneWriter` serialises, so the tree and the saved file describe the same
  scene;
- children keep creation order, so one scene yields one deterministic tree;
- an entity whose parent is not in that set is a root — again matching what
  the writer does.

Each node carries `entity`, parent index, first-child and next-sibling indices,
depth and cached tag name, so an outline view can walk firstChild/nextSibling
without touching the registry. `DumpText` writes the tree as an indented name
list; two builds of one scene dump identical text, which is the cheapest way to
regress-test ordering.

Every structural edit resolves through `FindByEntity` / `FindByTag` into a node
index and then rebuilds, so the tree is never stale relative to the world.

### The document

`hfEditorDocument` owns one `hfWorld` and one tree, and is the only thing that
mutates either. Open clears the world, reads the text, and rebuilds the tree;
save writes it back. Edit comes in two shapes:

- **Inspector edits** go through the sheet — for the selected entity, a flat
  list of `(component class, property)` pairs, in registration order, for every
  registered component that entity actually has. A UI binds row *i* to a widget
  by `hfEditorProperty::type`; the document never learns what the widget is.
  `SetPropertyValue` writes through the offset and marks the document dirty.
- **Structural edits** — `CreateEntity`, `DeleteEntity`, `Reparent`,
  `AddComponent`, `SetName` — each bump the edit count and rebuild. `Reparent`
  refuses self-parenting and walks the proposed parent's ancestors so a
  hierarchy can never be hung off its own descendant. `SetName` creates the tag
  component when the entity has none, so an untagged entity can be given a name
  from the sidebar.

Adding a component is guarded by `!Has<T>` and is therefore idempotent.
*Removing* one is refused outright: `hfEcs` has no per-type removal, so the
editor can add a component but not take one back off, and returning `false` is
better than reporting success and saving the component anyway.

### Making the file carry what the inspector edits

The inspector can edit `Light.Enabled` and `MeshRenderer.{Casts Shadow,
Receives Shadow, Visible}`, and the camera's clear alpha — none of which the
text format used to write. A save that silently drops an edit is worse than no
edit at all, so the format grew those fields:

```
camera 75 1.7777 0.1 1000 0.05 0.05 0.08 1
light point 1 1 1 1 10 20 30 1 1
mesh 13 7 -1 -1 -1 1 1 1 "box" 1 1 1
```

The reader gained the matching reads. The format stays positional and gets its
next value by skipping whitespace, so a field that is written by one side and
not read by the other silently eats the following directive's token; both sides
moved together here, and there are no pre-existing `.scene` files — the
serializer is used only by the harness — so nothing outside this change had to
be migrated.

The scene still round-trips through *three* consistent edits now: Transform
position, `Tag.Name` rename and a reparent all survive the reload, alongside the
new light and mesh flags, and the second save is byte-identical to the first.

### What the harness pins down

`Test_Editor` runs without a window and drives the same API a UI would:

- **Reflection sanity**: five classes registered (registering twice changes
  nothing), name and type-key lookups are self-consistent, and every property
  offset is non-negative and lies wholly inside its component's `sizeof`,
  including the string and vector fields.
- **Value access**: `ReadAt`/`WriteAt` round-trip a `VEC3` at an offset with no
  component involved, and the enum metadata is the `point`/`spot`/`directional`
  table the dropdown needs.
- **Tree**: a three-entity hierarchy flattens to one root of two children at
  depth 1, `FindByTag` and `FindByEntity` agree, an untagged entity shows as
  untagged and can be named later, and two text dumps of one tree are identical.
- **Inspector**: select an entity via the tree, edit its `Transform.Position`,
  rename it through `Tag.Name` (the tree follows immediately), flip a light's
  `Type` and `Enabled` and a mesh's `Visible` through the sheet, and confirm an
  out-of-range enum value is refused.
- **Structural**: add a light, add a mesh (idempotent), be told `RemoveComponent`
  is refused, reparent, reject a cycle, reject self-parenting, and delete a
  created entity.
- **The contract**: after all of that, `Save` → reload → `Save` is
  byte-identical, and the reloaded world has the same parent link, the same
  `5, 0, -2` position, the same `directional` + disabled light and the same
  hidden mesh — so the diff is empty *and* the semantics survived.

### Known gaps

- **No GUI.** There is no window, no widget toolkit binding and no
  `editor/main.cpp`; the document, tree and reflection model are the whole
  implementation, and `Test_Editor` is the only caller.
- **Gizmos, play-in-editor and the import view are not built.** The three
  Godot features the roadmap names beyond the inspector all need the renderer
  or a running sim loop. The glTF importer and CSG layer that an import view
  would sit on exist from Phase 5, but nothing connects them to an editor.
- **No undo.** Every edit is applied in place; there is no command object, no
  history ring, and no redo. `IsDirty`/`GetEditCount` are the only edit
  bookkeeping.
- **Selection is a node index, not a stable handle.** The index is refreshed
  on every structural change, so it is valid between edits but is not meant to
  outlive one.
- **Deletion is not recursive and not user-facing.** `DeleteEntity` removes one
  entity; children are left pointing at a destroyed parent and become roots on
  the next build, which is a defensible default but not a designed policy.
- **The text format is positional.** Adding a field means changing the writer
  and the reader together; there is no key/version header to make an old file
  load into a new reader.

## 20. Phase 6 — Animation: rest pose, skinning, blend weights, state machine

Phase 6 is the last phase that was scaffolded but never finished. The first
pass left a skeleton (bone names + parents), a clip with rotation and
translation tracks, and a system that sampled one clip straight onto the
working pose. That is a keyframe player, not an animation system: there was no
bind pose, so skinning could not be expressed, no blend, no events actually
fired, and no state graph. This section is the completion — the parts the
roadmap lists that the scaffolding did not yet carry.

The through-line is the inverse-bind identity. A skeleton has a *rest* (bind)
pose and a *working* pose. When the working pose equals the rest pose the skin
transform `world · restWorld⁻¹` must be the identity for every bone. Every
piece below is arranged so that this one property is exact — because it is
also the property the harness checks first, and the one a bug in a transpose
or a quaternion conjugate silently violates.

### What was added

- `hflib/anim/hfSkeletonComponent.h` — rest pose, composed world pose, world
  matrices, inverse-bind `skinMatrix`, `ResetToRest`, `ComposeWorld`,
  `ComputeSkinMatrices`, and a `TransformVertex` CPU reference.
- `hflib/anim/hfAnimationClip.h` — `EventsInRange`, the range scan the system
  drives once per frame.
- `hflib/anim/hfAnimationSystem.h` — per-slot weights and weighted blending,
  plus an event callback dispatched as the playhead crosses event times.
- `hflib/anim/hfAnimationStateMachine.h` — states, parameters, and guarded
  transitions returning the clip to play.
- `Test_Animation` extended to cover all of the above.

### Forward kinematics and inverse-bind skinning

Bones are added parent-first, so index order *is* evaluation order and no
recursion or visited set is needed. One pass composes the chain:

```
worldRot[b] = worldRot[parent] * localRot[b]
worldPos[b] = worldPos[parent] + worldRot[parent] * localPos[b]
worldMat[b] = mat4( worldRot[b].ToMat3(), worldPos[b] )
```

The rest world matrices come from the same pass over `restPose`. The skin
matrix is then the textbook pair

```
skinMat[b] = worldMat[b] * inverse( restWorldMat[b] )
```

At rest `worldMat == restWorldMat`, so the product is exactly the identity —
not approximately, because it is a matrix times its own inverse. When the pose
moves, a bind-space vertex `v` deforms as `skinMat[b] · v`, and `TransformVertex`
exposes precisely that so the test can compare it against a hand-computed
world-space position. The engine's `hfQuat * hfVec3` rotates opposite to the
naive right-hand rule about a given axis (its operator is the conjugate
sandwich `q⁻¹ v q`); the test asserts against that convention rather than
assuming the textbook one.

### Blend weights

Each slot carries a weight. `Advance` samples every playing slot, then folds
the samples together sequentially: the rotation with `Slerp`, the translation
with a lerp, each at `weight / (accumulated + weight)`. A single slot at
weight 1 reproduces the raw clip exactly; two at `0.5/0.5` meet at the
midpoint; a zero-weight slot is dropped without even being sampled. This is
the blend-tree core the state machine feeds — the machine decides *which*
clips and weights, the system only combines them.

### Event tracks

`EventsInRange(lo, hi]` is a plain scan; the wrap arithmetic lives in the
system, which walks `(prev, next]` one lap at a time so an event is delivered
exactly once per crossing even across a loop boundary. The callback is
`void (*)(const char *name, float t, void *user)`, so the test can count
events and record the last name without any allocation.

### The state machine

`hfAnimationStateMachine` holds up to eight states (name → clip), eight named
parameters (bool, float, trigger), and sixteen transitions. `Update` takes the
first transition leaving the current state whose guard holds — `from == -1`
means "from any state" — consumes a trigger if that is what fired, and returns
the new state's clip index. It never touches the blender: the caller hands the
returned clip to `hfAnimationSystem::Play`. Keeping the graph free of the
sampling code is what makes it testable on its own, and the test drives it
through a bool transition, an up and a down float transition, and a
fire-once-consume trigger.

### Validation

`Test_Animation` now checks: rest-pose skin matrices are the identity; a
rotated parent carries its child to the expected world position; a bind-space
vertex follows the animated transform; two clips at half weight blend to the
midpoint and degenerate correctly at weight 0 and 1; events fire once each as
the playhead crosses them, including across a loop; and the state machine
walks idle → walk → run → walk → idle and fires a trigger that is consumed on
use. All pass in the default and Jolt harnesses, plain and `--headless`.

### Known gaps

- **No world-space scale in bone poses.** `hfBonePose` is rotation +
  translation, so every bone is rigid and the skin matrix is a rigid
  transform. Non-uniform bone scale would need a third channel and a 4x4
  inverse that does not assume orthonormality.
- **No skinning on the GPU yet.** The uber-shader doc has carried a skinning
  stub since Phase 2; `skinMatrix` is the data such a shader would consume,
  but no shader reads it and no vertex buffer binds to it. The CPU reference
  is the only consumer.
- **Blending is sequential, not a normalized pose-space average.** Order
  matters once three or more slots overlap; the fold is deterministic and
  collapses correctly for one or two slots, but it is not a general n-way
  blend tree. There is no additive or mask layer.
- **Transitions are instantaneous.** There is no cross-fade duration; a state
  change swaps clips on the next `Play`, so the caller gets a hard cut unless
  it blends the old and new clips itself via the weighted slots.
- **The state machine is not serialized.** States, parameters and transitions
  are built in code; none of them are written to the text scene format.

## 21. Phase 12 — An English scripting dialect and a sample game

Sections 15–20 closed out the engine subsystems. This section is about the
*author-facing* half: making the built-in scripting language `hfscript`
readable as plain English, and proving the whole loop with a small game whose
logic is a script.

### Why a dialect, not a new language

`hfscript` already compiles a Python-flavoured subset (and, via the `.rb`
front end, a Ruby-flavoured one) down to a stack bytecode. The language did
not need new semantics to read like English — it needed a different *surface*.
So the English dialect is a **token-level rewrite**: the source is lexed with
the existing lexer, the token stream is lowered onto the canonical tokens, and
the normal parser/compiler takes over. Nothing about the VM, the bytecode, or
the host-function ABI changes. This is the same trick Godot uses when it keeps
one `GDScript` tokenizer but offers several literals/keywords, scaled down.

The dialect is selected the same way the Ruby front end always was: by source
name. A `.rb` file sets `ruby = true`; a `.eng` file sets `english = true` and
`ruby = true`, so the Ruby-style optional block delimiters (optional colons,
optional trailing `end`) apply and indentation alone closes a block. A caller
that only passes a name through — the engine's `HfScript_CompileFile` path —
gets the dialect for free.

### The rewrite (`script/HfScript_English.h`)

`HfScript_TranslateEnglish` walks the token list with a small peephole over
its lookahead and emits a rewritten list. It is deliberately conservative: a
word that is not a known English keyword passes through unchanged, so ordinary
identifiers survive. The mapping is:

| English | Canonical |
| --- | --- |
| `set x to E` | `x = E` |
| `change`/`increase`/`decrease x by E` | `x +=`/`-= E` |
| `show E` | `print(E)` |
| `repeat E times` | `for __repeat in range(E)` |
| `define f with a and b` | `def f(a, b)` |
| `give back` | `return` |
| `otherwise` | `else` |
| `else if` | `elif` |
| `do nothing` | `pass` |
| `stop` / `skip` | `break` / `continue` |
| `is greater than` / `is less than` | `>` / `<` |
| `is at least` / `is at most` | `>=` / `<=` |
| `is equal to` / `equals` / `is` | `==` |
| `is not equal to` / `is not` | `!=` |
| `plus` / `minus` / `times` | `+` / `-` / `*` |
| `multiplied by` / `divided by` | `*` / `/` |
| `modulo` | `%` |
| `then` | *(dropped)* |

The words are reserved in `.eng` sources, so an English program cannot name a
variable `times` or `show`. That is the deliberate cost of the surface read.

### The `else` bug the dialect exposed

Adding `otherwise` surfaced a latent bug in `ParseIf`. The compiler emitted
the else body correctly, but the false-jump patch ran unconditionally at the
end of the construct and used the *next* statement (which is not `else`) as
its branch test, so it re-pointed the false-jump past the else body. Any
program that took the else branch fell through it. No existing test had ever
taken an else branch, so it had gone unnoticed across every dialect. The fix
keeps an explicit `sawElse` flag and only patches the false-jump when no else
was parsed.

### The sample game (`game/hfArena.h` + `script/HfScript_Game.h`)

`hfArena.h` is a header-only, fully deterministic top-down arena: a player
that always fires at the nearest enemy and either advances or retreats, waves
of enemies that chase, projectiles, contact damage, scoring and win/lose
conditions, all powered by an LCG seeded from the reset value. It allocates
nothing and has no dependency beyond `hfprecompiled.h`; identical seeds give
identical runs down to the tick count.

`script/HfScript_Game.h` bridges it to the script with ten host functions —
`arena_reset`, `arena_running`, `arena_hp`, `arena_score`, `arena_wave`,
`arena_ticks`, `arena_nearest`, `arena_attack`, `arena_retreat`, `arena_step`
— registered on the global host table exactly like the built-ins. The state
lives in C++; the script only expresses the per-tick *policy*. That is the
whole point of the exercise: the gameplay decision is English, the
simulation stays typed and deterministic.

`base/script/arena.eng` is the game:

```eng
define main
    arena_reset(20260610)
    while arena_running() is 1
        if arena_hp() is less than 40
            arena_retreat()
        otherwise
            arena_attack()
        arena_step()
    if arena_wave() is at least 5
        show "arena cleared"
    otherwise
        show "arena lost"
    show arena_score()
    show arena_ticks()
main()
```

### Validation

`Test_Game` registers the arena hosts, reads `base/script/arena.eng`, and
asserts that it compiles, that it terminates on its own (`arena_running`
becomes 0) within `TICK_LIMIT`, and that it finishes with a positive score and
at least one wave cleared. It then runs the same script a second time and
asserts the score, tick count and wave are bit-identical — the determinism
guarantee that the whole engine is built around. A second, inline `.eng`
program exercises `define … with … and …`, `set … to`, `plus`, `is greater
than`, `otherwise`, `repeat N times`, `increase … by` and `give back`, so the
dialect is pinned independently of the game. The harness prints `phase 7 game:
ok` and `ALL PASSED`, plain and `--headless`.

### Known gaps

- **The rewrite is token-level and word-order fixed.** It understands `set x
  to E` but not `x becomes E`; `repeat E times` but not `repeat until cond`.
  Anything beyond the table passes through as identifiers, so a typo is a
  parse error rather than a helpful message.
- **Reserved words are global to the file.** There are no scoped dialects and
  no way to disable a keyword for one identifier. `times`, `show`, `stop` are
  simply unusable as names in a `.eng` source.
- **No `and`/`or`/`not` words.** Boolean operators stay symbolic; only the
  comparison and arithmetic words are lowered.
- **The arena is a toy.** Two entity kinds, straight-line projectiles, no
  spatial index; the max counts are small constants and the AI is one rule
  ("shoot the nearest"). It exists to prove the scripting loop, not to be a
  game engine's gameplay sample.
- **The game is not exposed to the running engine's frame loop.** The bridge
  can be registered and driven from a script, but nothing wires it into the
  `main` tick or the presentation layer yet; it lives in the harness.

---

## 22. Phase 13 — Game console porting layer (PS5 / Xbox / Switch style)

Sections 15–21 built the engine subsystems and the author-facing scripting
layer. This section is about the *platform* half: making the engine portable
onto a game console — the family of targets that have their own sign-in,
achievement, statistics, voice and per-user storage services, each behind a
vendor SDK.

### The problem with console ports

Every console platform exposes the same five capabilities behind a different
SDK, a different naming convention and a different lifetime model:

| capability | console semantics |
|---|---|
| sign-in | a set of signed-in accounts, each bound to a controller slot |
| achievements | a fixed set of bits, unlocked per account, silently idempotent |
| statistics | named int/float values, owned by an account |
| voice chat | per-account talk state and a local mute |
| storage | named save slots, segregated per account |

The engine already models all five (`hfSignInManagerBase`, `hfLocalUser`,
`hfAchievementSystem`, `hfVoiceChatMgr`, and the profile/save layer), and it
already has a platform backend per target — `sys/win32`, `sys/linux`. What it
did *not* have was a seam that lets a console port reuse the engine's
cross-platform code while its SDK calls stay in one place.

### The seam (`hflib/console/hfConsoleServices.h`)

`hfconsole::hfConsoleServices` is a platform-neutral interface over exactly
those five capabilities, and it is deliberately *not* an engine interface: it
lives in `hflib`, is header-only, has no engine or framework dependency, and
never touches wall-clock time. A real console port implements it against the
vendor SDK and installs it at boot:

```cpp
class MyPs5ConsoleServices : public hfconsole::hfConsoleServices {
    // sign in / achievements / stats / voice / storage
};

hfconsole::HfConsole_InstallServices( new MyPs5ConsoleServices() );
```

`HfConsole_GetServices()` returns the installed implementation, or — when
nothing is installed — a deterministic offline stub. That fallback is the point:
the desktop build keeps working unchanged, and the harness can pin the
contract headlessly with no console hardware and no SDK.

The stub (`hfconsole::hfConsoleServicesStub`) is a pure function of the
operations applied to it: one signed-in user, a fixed gamer tag, every
achievement locked until unlocked, stats and achievements keyed *per account*,
and a bounded table of per-account save slots. Two stubs driven with the same
sequence of calls are in identical states, which is what makes the harness
assertions meaningful.

### The engine adapters (`sys/consoles/hfconsole_services.{h,cpp}`)

The engine side binds that seam to the services the session already owns:

- `hfLocalUserConsole` — a local user whose profile/online state comes from
  the console backend rather than from a desktop profile.
- `hfSignInManagerConsole` — mirrors `hfSignInManagerLinux` (Pump, register,
  remove, unique handle) but takes the gamer tag from the console backend.
- `hfAchievementSystemConsole` — unlocks through the console backend *and*
  mirrors the bit into the player profile, exactly like the desktop path, so
  the profile keeps the achievement bits across a save.
- `hfVoiceChatMgrConsole` — the voice manager, wired to the console voice state.

They contain no SDK calls at all: that is the point of the seam. A console
target supplies the SDK-backed `hfConsoleServices` implementation and reuses
all of this unchanged.

### Build wiring

`sys/consoles/*.cpp` is compiled into the engine on every platform, so the
console code path is build-verified everywhere instead of rotting between
ports. `HF_CONSOLE_SUPPORT=ON` additionally defines `HF_CONSOLE`, the switch a
real console target uses to select its SDK-backed implementation.

### Verification

`Test_Console` in the harness pins the contract: sign-in state, gamer tag,
per-account achievement locking and idempotent unlock, int/float statistics
round-trip, voice mute, per-account save-slot write/read/size/delete,
rejection of an oversized blob, the bounded slot table, the install/restore of
a platform implementation, and determinism (a second stub driven identically
ends in the same state; a fresh stub starts clean). The harness prints `phase
8 console: ok` and `ALL PASSED`, plain and `--headless`, with and without
Jolt.

### Known gaps

- **No SDK-backed implementation ships here.** The layer is the seam and the
  offline stub; the PS5/Xbox/Switch implementations need the vendor SDKs and
  their certification requirements, which are not in this repository.
- **No console `Sys_*` OS layer.** The engine's OS layer stays the Linux one;
  a real port reuses it and adds only the SDK-backed service implementations.
  A full console OS layer is a much larger change than a porting seam.
- **No console session subclass.** `hfSession` is a large pure-virtual
  interface; a real target instantiates the console service classes in its own
  session constructor, mirroring `hfSessionLocalLinux`, rather than
  subclassing the platform-neutral session here.
- **Savegames still use the desktop save layer.** The console seam only
  provides the per-account segregation (`WriteSlot`/`ReadSlot`/`SlotSize`/
  `DeleteSlot`); the actual file format stays the shared one.
- **The stub is not a model of a real console.** One user, no sign-in flow, no
  network, fixed gamer tag, no error paths. It models the *contract*, not the
  platform.

## 23. Phase 14–15 — A modern GUI: control nodes, events, and input

Phase 9 built the UI *substrate*: a `hfUiCanvasComponent` per control, anchors,
row/column/grid flow layouts, clipping, z-ordering, hit-testing and a CPU
bitmap-font rasterizer (`Test_Ui`). Phase 14–15 build the part a game actually
programs against — the equivalent of Godot's `Control` node family with
signals, or Unity's `Canvas` + `EventSystem`.

### Design: widgets are ECS components

Following the same choice as Phase 9, widgets are plain components attached to
entities, not a class hierarchy. A "button" is an entity with a
`hfUiCanvasComponent` (geometry, hierarchy, interactivity) plus a
`hfUiButtonComponent` (visual state machine). This keeps the whole UI
serializable, deterministic and headless-testable, and it means the layout
solver never needs to know what kind of control it is placing.

`neo/hflib/ui/hfUiWidget.h` defines the widget data:

| Component | Contents |
|---|---|
| `hfUiPanelComponent` | background RGBA, border colour/width, corner radius |
| `hfUiButtonComponent` | `NORMAL/HOVER/PRESSED/DISABLED` state, per-state colours, `toggle` + `active`, optional sound name |
| `hfUiSliderComponent` | `minValue`/`maxValue`/`value`, `step`, axis, integer snap, handle size, bar/handle colours |
| `hfUiToggleComponent` | checked/disabled, box size, box/check colours |
| `hfUiTextInputComponent` | text, placeholder, cursor + selection, max length, password/read-only, focus, caret blink |

`hfUiLabelComponent` (Phase 9) supplies text; `hfUiButtonComponent::labelEntity`
lets a button own a label child, which is how `CreateButton` builds it.

### Events: named channels on the ECS event bus

Input is turned into *intents*, not raw key codes, and published through the
existing `HF_Ecs::EventBus` (`neo/hflib/ui/hfUiEvents.h`):

```
ui.hover   { entity, x, y }        ui.focus   { entity }
ui.unhover { entity, x, y }        ui.blur    { entity }
ui.press   { entity, button, x, y } ui.change { entity, valueF, valueI, text }
ui.release { entity, button, x, y } ui.submit { entity, text }
ui.click   { entity, button, x, y }
```

The channel is a string and the payload a POD struct, so a game subscribes
with a lambda and reads exactly the fields it needs — the moral equivalent of
Godot's `pressed`/`value_changed` signals, without a per-widget vtable. Because
dispatch is synchronous and ordered, replaying the same input sequence yields
the same event sequence, which is what the harness asserts.

### Input mapping: `hfUiInput`

`neo/hflib/ui/hfUiInput.h` owns the interaction state machine:

1. `SetMouse(x, y)` and `SetButton(button, down)` latch edge transitions
   (`m_buttonPressed`/`m_buttonReleased`) so a frame that both presses and
   releases is still observed.
2. `Update()` hit-tests once via `hfUiSystem::HitTest()`, emits
   `unhover`/`hover` on hover changes, and flips every button's visual state to
   match hover/press.
3. On press over an `interactive` control it records the press target and
   moves focus (`blur` on the old, `focus` on the new).
4. On release it emits `ui.release`; if the pointer is still over the press
   target it also emits `ui.click`. Toggle buttons flip `active` and emit
   `ui.change`. Releasing outside the control emits `release` only — the
   standard "drag off to cancel" behaviour.
5. `EndFrame()` clears the one-frame edge flags.

Deliberately headless-friendly: it consumes abstract state, not a window, so
the same code paths run in the harness and on a real platform.

### Builder helpers: `hfUiBuilder`

`neo/hflib/ui/hfUiBuilder.h` is the ergonomic front end — the constructor
functions that make a UI tree readable:

```cpp
HF_Ecs::Entity root  = hfUiBuilder::CreateRoot( reg, 1024, 768 );
HF_Ecs::Entity panel = hfUiBuilder::CreatePanel( reg, root, 50, 50, 300, 400 );
HF_Ecs::Entity box   = hfUiBuilder::CreateVerticalBox( reg, panel, 0, 0, 300, 400 );
HF_Ecs::Entity play  = hfUiBuilder::CreateButton( reg, box, 0, 0, 200, 40, "PLAY" );
HF_Ecs::Entity music = hfUiBuilder::CreateSlider( reg, box, 0, 0, 200, 30, 0, 100, 25 );
HF_Ecs::Entity sound = hfUiBuilder::CreateToggle( reg, box, 0, 0, 200, 24, "SOUND", true );
HF_Ecs::Entity name  = hfUiBuilder::CreateTextInput( reg, box, 0, 0, 200, 32, "NAME..." );
```

Each helper attaches the canvas component, sets the parent, marks
interactivity, and attaches the widget component. Buttons and toggles also
create their label child. The box helpers set `HF_UI_LAYOUT_COLUMN`/
`HF_UI_LAYOUT_ROW` plus padding and spacing.

One subtlety worth documenting: inside a flow container the solver *ignores*
child anchors and assigns rects from `minSize`. So the helpers set
`c.minSize` as well as the anchor. That is what makes `GetRect(play)` report
the requested 200×40 inside a column, and it is the bug the test now pins.

### Draw pass: `hfUiDrawPass`

`neo/renderer/hfUiDrawPass.{h,cpp}` runs after the 3D view. It switches the
projection to a pixel-space `glOrtho` with Y down, disables depth test, sets
premultiplied-style alpha blending (`SRC_ALPHA`/`ONE_MINUS_SRC_ALPHA`), and
walks `hfUiSystem`'s sorted draw list:

- panel background quads (visible/clipped rect),
- button quads tinted by state when no panel is present,
- toggle box + inner check,
- slider bar + handle positioned by `(value-min)/(max-min)`,
- labels rasterized through `hfBitmapFont` into an optional coverage buffer.

The GPU path is compile- and state-validated in this repository; the container
has no DISPLAY, so correctness is asserted headlessly through the draw list,
resolved rects, coverage output and the event log rather than a screenshot.

### Verification

`Test_UiWidgets` builds a panel → column → button/slider/toggle/text-input tree
with the helpers, then asserts:

- every helper attached the expected component,
- the layout solver resolved the button to exactly 200×40 inside the column
  and placed it inside the panel,
- a press/release over the resolved button rect produces at least one
  `ui.click` on the bus,
- the slider kept its initial value (25 of 0…100) and the text input its
  placeholder,
- toggle initial state survives construction.

Harness output: `phase 9 ui widgets: ok` → `ALL PASSED`, plain and
`--headless`, default and Jolt.

### Known gaps / next

- **No theme resource yet.** Colours live on each component; Phase 17 folds
  them into a `hfUiTheme` (nine-patch, font, state colours) so a whole UI can
  be reskinned.
- **Text input is data-only.** Caret movement, selection, key repeat and IME
  need the keyboard path; Phase 16 adds it with the frontend.
- **No layout invalidation yet.** `hfUiSystem::Update` re-solves every frame;
  a dirty flag is the obvious optimisation once UIs get large.
- **The draw pass is not yet wired into `R_RenderView`.** It is a standalone
  pass so the frontend (Phase 16) can call it at the right point in the frame.
- **No navigation.** Focus is mouse-driven; gamepad/keyboard focus order and
  `ui.submit` are Phase 16–17 work.

## 24. Phase 16 — The frontend: screens, HUD, and the renderer hook

Phase 14–15 gave the engine a widget kit. Phase 16 gives it somewhere to live:
a screen-flow state machine that starts at a title screen, owns one UI tree
per screen, mirrors the arena simulation into a HUD, and hands the tree to the
renderer once per frame.

### The state machine

`neo/hflib/ui/hfUiFrontend.h` defines:

```
BOOT -> TITLE -> MENU (options)
         TITLE -> PLAYING <-> PAUSED
         PLAYING -> RESULTS -> PLAYING | TITLE
         any -> LOADING -> PLAYING
```

Every screen is a fresh tree. `SetState()` calls `Registry::Clear()`, resets
the input latch, and rebuilds, so a transition cannot leave a widget, a hover
target or a dangling parent pointer behind. Because the frontend owns its
registry and UI system, the addresses handed to the renderer stay stable across
transitions.

Actions are an enum (`START`, `OPTIONS`, `BACK`, `PAUSE`, `RESUME`, `RESTART`,
`MAIN_MENU`, `QUIT`) rather than widgets calling each other directly. Buttons
are bound to actions at build time; a click is looked up and applied *after*
input dispatch, so a screen transition never destroys entities while the input
system is still iterating the registry.

Two things are observable from outside the class, which is what makes the
frontend testable and hostable:

- `ui.state` is published on the event bus with `{ state, previous }` on every
  transition.
- every applied action is appended to a bounded, deterministic action log
  (`GetActionLogCount` / `GetActionLog`).

### Screens

- **Title** — name, subtitle, PLAY / OPTIONS / QUIT.
- **Options** — music and SFX sliders (0…100), a fullscreen checkbox. The
  checkbox flips through the generic `hfUiToggleComponent` path added in
  `hfUiInput`: click, flip `checked`, publish `ui.change`. Values are stored on
  the frontend so the host can persist them (Phase 17).
- **Playing** — HUD labels for score / health / wave plus a PAUSE button.
- **Paused** — dimmed overlay, RESUME / RESTART / MAIN MENU / QUIT.
- **Results** — VICTORY or GAME OVER, final score, PLAY AGAIN / MAIN MENU /
  QUIT.
- **Loading** — a placeholder a host can show while a level loads; `BeginGame()`
  moves it to PLAYING.

### Input

`HandleSysEvent( const sysEvent_t & )` is the engine-facing adapter:

- `SE_MOUSE` accumulates relative deltas; `SE_MOUSE_ABSOLUTE` sets the cursor
  directly.
- `SE_KEY` in the `K_MOUSE1..K_MOUSE8` range becomes a UI mouse button.
- Enter/Space activates the focused control; Escape maps onto `Back()`, which
  is context sensitive (pause while playing, resume while paused, up a level in
  the menu, to the title from results).

The same behaviour is available headlessly through `SetMouse` / `SetButton` /
`Update`, which is exactly what the harness drives.

### Arena binding

`SetArena()` is optional. When attached, PLAYING advances the sim at a fixed
60 Hz (`1/60` accumulated, at most 8 steps per frame so a stall cannot spiral),
the HUD is rewritten from `score`/`hitPoints`/`wave` after each frame, and a
sim that reports game over transitions to RESULTS. PAUSED simply does not
step, which is what "the pause menu freezes the sim" means in practice.

### Renderer wiring

`hfUiDrawPass` gained an installed-source model:

```cpp
hfUiDrawPass::SetSource( frontend.RegistryPtr(), frontend.SystemPtr() );
```

and `RB_ExecuteBackEndCommands` calls `hfUiDrawPass::DrawInstalled()` once after
the frame's 3D views. That placement matters: `RB_DrawView` bails out early
when a view has no surfaces, so hanging the overlay off a view would make the
title screen invisible on exactly the frames where there is no world to draw.
`DrawInstalled()` resets the viewport and scissor to the full window, is a
no-op when no source is installed, and is a no-op under `--headless` (no GL
context exists).

Text now reaches the GPU as well: labels are rasterized at their real scale
into a window-sized 8-bit coverage buffer, uploaded as one `GL_INTENSITY`
texture, and blended over the widget quads. Centering/right alignment uses the
metrics already resolved by `hfUiSystem`, so a button label is centred in the
button rather than pinned to its corner. One texture upload per frame is
trivial at UI resolutions and avoids introducing a font atlas before the theme
resource exists.

### Verification

`Test_UiFrontend` drives the whole flow through real controls — it finds a
button by action, reads its resolved rect, moves the mouse to the centre and
presses/releases:

- TITLE → PLAYING via PLAY; the arena is reset and HP restored.
- The HUD mirrors `score`/`hitPoints`/`wave` (`SCORE 123`, `HP 42`, `WAVE 3/5`).
- PAUSE freezes the sim (tick count unchanged over two 0.25 s frames).
- RESUME advances it again; RESTART resets it.
- `gameOver` + `won` → RESULTS showing VICTORY.
- PLAY AGAIN → PLAYING; PAUSED → MAIN MENU → TITLE.
- OPTIONS → MENU; the fullscreen checkbox toggles and publishes `ui.change`.
- QUIT raises `QuitRequested()` and publishes `ui.quit` exactly once.
- LOADING → `BeginGame()` → PLAYING.
- Escape pauses/resumes; Enter activates the focused control.
- The action log is exactly the 14 actions performed and the state-event count
  is exactly 18, so a regression in transition ordering fails the test.

Harness output: `phase 10 frontend: ok` → `ALL PASSED`, plain and `--headless`,
default and Jolt.

### Known gaps

- **The engine's `main()` still boots the legacy menu.** The frontend is a
  complete, tested module with a renderer hook; mounting it in
  `hfCommonLocal::Init` instead of `StartMenu()` is the remaining wiring step,
  and it needs the desktop main loop that the legacy UI currently owns.
- **No gamepad navigation.** Focus is set by mouse or by a test; D-pad/stick
  focus traversal and a focus ring are Phase 17.
- **Sliders do not drag.** A click on a slider emits `ui.click`; press-drag
  value tracking needs pointer capture and is deferred.
- **Text is one colour.** The coverage texture is blended white; per-label
  colours need a glyph atlas or a per-label batch.
- **No fade/transition animation.** Screens swap instantly.
