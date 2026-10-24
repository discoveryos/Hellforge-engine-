# Hellforge — Phase 4 roadmap (Source-1-class, Godot 4 reference)

Target: bring Hellforge to the level of the *Source 1* generation (Havok-class
physics, real materials/shadows, maps, animation, multiplayer) using **Godot 4
as the architecture reference**. The Godot source tree lives at
`/home/kali/godot` (`/home/kali/godot/servers`, `/scene`, `/modules`,
`/editor`) and is **MIT-licensed** — read and adapt freely. The older
`/home/kali/SynapseEngine/.../Engine` reference is AGPL: reference the
architecture, never copy code.

## Phase conventions (all phases)

- Every phase ships: `hflib` green, a harness `Test_*` green and
  `ALL PASSED` (headless, deterministic — our differentiating feature), and a
  doc section appended to `neo/renderer/GLSL_uber_shaders.md`.
- Every new subsystem takes the **server split** pattern: a CPU/headless
  "server" (RID-style handle, methods testable without a GPU) + a GL/Vulkan
  front-end that only consumes server output.
- Keep hflib headers header-only `HF_INLINE` + relative intra-lib includes so
  the script JIT keeps compiling mid-air.

---

### Phase 4 — Rendering & materials (SpatialMaterial-class) ✅
- **Goal:** PBR metallic/roughness material, normal maps, shadow-casting
  point/spot/dir lights, fog, tone mapping, draw-list culling feeding a
  backend.
- **Do:** finish GLSL uber shader; add `render/hfMaterialSystem.h` resolving
  material params per draw entry (mirrors `renderer/GLSL_uber_shaders.md`);
  `render/hfRenderingServer.h` (RIDs, frame draw list) with a CPU reference
  rasterizer for the harness.
- **Godot ref:** `/home/kali/godot/servers/rendering`,
  `/home/kali/godot/servers/rendering/renderer_rd` (Environment/SpatialMaterial
  split; servers decouple logic from GPU).
- **Validate:** `Test_Materials` — resolve a material set, assert draw-list
  contents, light/shadow flags, tone-mapped LUT math.

### Phase 5 — Import & content pipeline (.import-class) ✅
- **Goal:** glTF/FBX → typed `PackedStore` assets; mesh-based maps assembled
  from entities in text scene files.
- **Do:** extend assettool with an import step (glTF 2.0 node/skin/mesh+material
  → our resource manager); add CSG-ish brush primitives wrapped as scene
  entities (Godot's CSG/module approach, not Quake BSP).
- **Godot ref:** `/home/kali/godot/editor/import`, `/home/kali/godot/modules/gltf`,
  `/home/kali/godot/modules/csg` (CSGMerge, CSGBox…).
- **Validate:** `Test_Import` — round-trip a packed glTF, reject malformed,
  verify mesh/material ids wire into the resource manager.
- **Ship:** `@@asset/hfJson.h` (RFC-8259 JSON: single-alloc heap tree, strict
  trailing-data check, used by importer + editor tooling), `@@asset/hfGltf.h`
  (glTF 2.0 subset → mesh blob + `.mtr` items via `hfAssetStore`
  Pack/Validate/Extract + base64 data-URI buffer support), `@@scene/hfCsgComponent.h`
  + `@@scene/hfCsgSystem.h` (box/sphere/quad brushes as scene entities,
  12-tri box tessellation with local bounds). Harness `Test_Import` reports
  `Import ok` → `ALL PASSED`.

### Phase 6 — Animation (AnimationPlayer/AnimationTree-class) ✅
- **Goal:** skeleton component, GPU skinning, clips, blend trees + state
  machines, transform/rotation tracks, event tracks.
- **Do:** `anim/hfSkeletonComponent.h` (bone names+rest pose+world matrices),
  `anim/hfAnimationSystem.h` (clip playback, blend weights), reuse GPU
  skinning GLSL stubs already in the uber-shader doc.
- **Godot ref:** `/home/kali/godot/scene/resources/animation*`,
  `/home/kali/godot/scene/3d/skeleton_3d.*`,
  `/home/kali/godot/scene/animation/animation_tree*`.
- **Validate:** `Test_Animation` — playback advances pose, blend lerp correct,
  event track fires, skinning matrices match CPU reference.

### Phase 7 — Physics upgrade to Jolt (Godot 4's solver) ✅
- **Goal:** replace the Phase-3 impulse solver with Jolt behind the same
  `hfPhysicsSystem` interface; keep layers/masks/triggers parity.
- **Do:** add `external/jolt` (vendored, MIT), `physics/hfJoltBackend.h`
  implementing the existing Step narrowphase/solver contract; fall back to the
  built-in solver if Jolt is disabled at build time.
- **Godot ref:** `/home/kali/godot/modules/jolt` and `/home/kali/godot/thirdparty/jolt`
  show how Godot adapts the library into a PhysicsServer namespace.
- **Validate:** `Test_Physics` unchanged semantics still pass; add stack/rest
  stability and determinism checks (same input → same output).

### Phase 8 — Threaded servers + headless mode ✅
- **Goal:** physics/render/audio promoted to threaded servers with RID
  handles; `--headless` mode equivalent (no window, full sim) like Godot.
- **Do:** add `--headless`-style flag; move per-frame simulation into a worker
  thread publishing a snapshot the front-end consumes; keep ECS systems
  thread-agnostic.
- **Godot ref:** `/home/kali/godot/servers` threading model +
  `godot --headless`.
- **Validate:** harness runs under the flag; determinism assert across two
  runs of the same seed/input log.

### Phase 9 — UI (Control/Canvas-class) ✅
- **Goal:** menus, pause, HUD, console: 2D canvas, layout containers, text
  rendering, input events dispatched through the world.
- **Do:** `ui/hfCanvasComponent.h` + `ui/hfUiSystem.h` (anchors, containers),
  text via CPU rasterizer first (headless-testable).
- **Godot ref:** `/home/kali/godot/scene/gui/*` (Control, containers,
  Label/TextLine).
- **Validate:** `Test_Ui` — anchors/containers layout positions, clip/scroll,
  hit-testing order, text wrap metrics.

### Phase 10 — Networking (HighLevel/ENet-class) ✅
- **Goal:** server-authoritative replication over UDP on the deterministic
  sim (fixed 60 Hz snapshot = huge head start).
- **Do:** `net/hfNetComponent.h` + `net/hfReplicationSystem.h`: snapshot
  serialization of transform/physics state, input forwarding, lag
  compensation hooks; ENet-style transport behind an interface.
- **Godot ref:** `/home/kali/godot/modules/enet/`,
  `/home/kali/godot/scene/main/multiplayer_*`.
- **Validate:** `Test_Net` — N clients replay a logged input stream into the
  engine and converge to identical world states (determinism proof).

### Phase 11 — Editor (editor-first loop) ✅
- **Goal:** godot-like scene editor on our text scenes: gizmos, property
  panels, play-in-editor, import view.
- **Do:** editor tool that loads/edits/saves the serializer format; property
  reflection on components; scene tree + inspector.
- **Godot ref:** `/home/kali/godot/editor` (editor_node, inspector,
  gizmos).
- **Validate:** `Test_Editor` — open→edit→save→reload a scene keeps semantics;
  round-trip diff of the text scene file is empty.

### Phase 12 — English scripting dialect + sample game ✅
- **Goal:** make the hfscript language readable as plain English and ship a
  playable sample that uses it end to end.
- **Do:** `script/HfScript_English.h` — a token-level front end (selected by the
  `.eng` extension) that lowers natural-language keywords/operators onto the
  canonical Python/Ruby bytecode: `set x to`, `increase x by`, `show`,
  `repeat N times`, `define f with a and b`, `give back`, `otherwise`,
  `is greater than/less than/equal to/at least/at most`, `plus/times/…`,
  `stop/skip/do nothing`; blocks stay indentation-delimited and colons/.`end`
  are optional. Also fix a latent `ParseIf` bug where an `else` branch was
  re-patched and became unreachable. Add the deterministic
  `game/hfArena.h` simulation + `script/HfScript_Game.h` host bridge
  (`arena_*`) and the sample `base/script/arena.eng` that drives it.
- **Godot ref:** `/home/kali/godot/modules/gdscript` (GDScript as the engine's
  own readable scripting language).
- **Validate:** `Test_Game` — the `.eng` script compiles and drives the C++ sim
  to completion with a positive score, a fixed seed is bit-deterministic, and
  the inline English dialect exercises word operators/`define`/`repeat`/`give
  back`/`otherwise`. Harness reports `phase 7 game: ok` → `ALL PASSED`.
### Phase 13 — Game console porting layer (PS5/Xbox/Switch style) ✅
- **Goal:** make the engine portable onto game consoles — the family of
  targets that expose sign-in, achievements, statistics, voice chat and
  per-user storage behind vendor SDKs — without an SDK being present in the
  repository.
- **Do:** `hflib/console/hfConsoleServices.h` — a platform-neutral,
  header-only service interface over the five console capabilities plus a
  deterministic offline stub and an install/accessor pair
  (`HfConsole_InstallServices` / `HfConsole_GetServices`); a real port
  implements the interface against its vendor SDK and installs it at boot.
  `sys/consoles/hfconsole_services.{h,cpp}` — the engine adapters
  (`hfLocalUserConsole`, `hfSignInManagerConsole`,
  `hfAchievementSystemConsole`, `hfVoiceChatMgrConsole`) that bind the seam to
  the session's own services, with the sign-in flow mirroring
  `hfSignInManagerLinux` and achievements mirrored into the player profile.
  `sys/consoles/*.cpp` is compiled on every platform so the console path stays
  build-verified; `HF_CONSOLE_SUPPORT=ON` defines `HF_CONSOLE` for a real
  console target.
- **Godot ref:** `/home/kali/godot/platform/*` (the platform abstraction layer
  a console export sits on).
- **Validate:** `Test_Console` — the harness pins the contract: sign-in
  state, per-account achievement lock/unlock (idempotent), int/float stat
  round-trip, voice mute, per-account save-slot write/read/size/delete,
  oversized-blob rejection, bounded slot table, platform install/restore, and
  determinism (identical operation sequences produce identical state; a fresh
  stub starts clean). Harness reports `phase 8 console: ok` → `ALL PASSED`,
  plain and `--headless`, default and Jolt.

### Phase 14 — Modern GUI I: widgets, events, input, draw pass ✅
- **Goal:** turn the Phase 9 UI substrate into the control-node model other
  engines expose — Godot `Control` + signals, Unity `Canvas` + `EventSystem` —
  without abandoning the ECS.
- **Do:** `hflib/ui/hfUiEvents.h` — POD payloads and named channels
  (`ui.hover`, `ui.unhover`, `ui.press`, `ui.release`, `ui.click`, `ui.focus`,
  `ui.blur`, `ui.change`, `ui.submit`) published on `HF_Ecs::EventBus`.
  `hflib/ui/hfUiWidget.h` — widget components: panel, button (state machine,
  per-state colours, toggle mode), slider (min/max/step, axis, integer snap),
  toggle/checkbox, text input (text, cursor, selection, password, focus).
  `hflib/ui/hfUiInput.h` — pointer → intent mapping: hover transitions, press
  capture, click-on-release-inside, drag-off cancel, click-to-focus, toggle
  state flips. `renderer/hfUiDrawPass.{h,cpp}` — pixel-space ortho 2D pass
  drawing panels, buttons, toggles, sliders and bitmap-font labels from the
  sorted draw list.
- **Godot ref:** `/home/kali/godot/scene/gui/control.cpp` and
  `/home/kali/godot/scene/main/canvas_item.cpp` (control tree, input
  propagation, notification-driven redraw).
- **Validate:** UI tree resolves, hit-test drives the state machine, click
  events reach subscribers; both engine configs link.

### Phase 15 — Modern GUI II: builder helpers + harness coverage ✅
- **Goal:** make UI construction one line per control and pin the behaviour in
  the harness.
- **Do:** `hflib/ui/hfUiBuilder.h` — `CreateRoot/Panel/Label/Button/Slider/
  Toggle/TextInput/VerticalBox/HorizontalBox`, including auto-created label
  children and `minSize` so flow containers size children correctly.
  `Test_UiWidgets` builds a panel → column → button/slider/toggle/text-input
  tree and asserts component attachment, resolved 200×40 button rect inside the
  column, a real press/release producing `ui.click`, slider value, toggle
  state, and text-input placeholder.
- **Godot ref:** `/home/kali/godot/scene/gui/*` (ready-made controls) and
  `BoxContainer` (flow containers).
- **Validate:** `phase 9 ui widgets: ok` → `ALL PASSED`, plain and
  `--headless`, default and Jolt (4/4 matrix).

### Phase 16 — Frontend: title screen, menus, HUD, pause (planned)
- **Goal:** the game is launched into a real GUI rather than straight into a
  session loop.
- **Do:** an `hfUiFrontend` state machine (`BOOT → TITLE → MENU → LOADING →
  PLAYING ⇄ PAUSED → RESULTS`) owning a UI tree per state; wire
  `hfUiDrawPass` into `R_RenderView`'s GUI stage; route `Sys_*` mouse/key
  events into `hfUiInput`; keyboard/gamepad focus navigation + `ui.submit`;
  HUD built from the arena sim (score, health, wave) and a pause menu that
  freezes the sim.
- **Godot ref:** `/home/kali/godot/scene/main/scene_tree.cpp` (main loop and
  scene switching).
- **Validate:** headless frontend test drives synthetic input through every
  state transition and asserts a deterministic event log; the arena script
  runs from the title screen to a results panel.

### Phase 17 — Persistence: settings, keybinds, save/load, theming (planned)
- **Goal:** the GUI owns durable user state.
- **Do:** `hfUiTheme` resource (font, state colours, nine-patch borders) with
  a text/JSON definition loaded at boot; settings screen bound to cvars;
  keybind remapping with conflict detection; save-slot UI over the Phase 13
  per-account console storage; resolution/quality/vsync applied live.
- **Godot ref:** `/home/kali/godot/scene/resources/theme.cpp` and
  `/home/kali/godot/core/input/input_map.cpp`.
- **Validate:** settings round-trip through a save slot, keybind conflicts are
  rejected, a theme change reskins the tree without touching widget code.
