# 13 — Building in WSL (Alpine or Debian/Ubuntu)

The CMake commands are the same on every Linux distro; only the package
install and the display setup differ. This chapter collects both WSL variants
in one place. The build commands below are identical to
[chapter 02](02-build.md); the Alpine notes are written against this repo's
includes (`GL/gl.h`, `GL/glx.h`, `X11/Xlib.h`, etc.) but have **not** been run
by the author of this guide — if you hit a musl-specific error, paste the
first error block and fix the specific cause rather than assuming the engine
is broken.

## 1. Common steps (both distros)

From the repository root:

```sh
cmake -S neo -B neo/build -DCMAKE_BUILD_TYPE=Release
cmake --build neo/build --target hellforge -j"$(nproc)"
```

Jolt backend:

```sh
cmake -S neo -B neo/build-jolt -DHF_ENABLE_JOLT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build neo/build-jolt --target hellforge -j"$(nproc)"
```

Optional real Python bridge (needs dev headers):

```sh
cmake -S neo -B neo/build-py -DCMAKE_BUILD_TYPE=Release -DHF_PYTHON=ON
cmake --build neo/build-py --target hellforge -j"$(nproc)"
```

If `$(nproc)` reports many cores but you run out of memory, use a smaller
value such as `-j4`. On the development host `-j7` is used because `-j4`
timed out on a cold rebuild after a widely-included header change.

`neo/build/`, `neo/build-jolt/` and any `build/` directory are gitignored —
never commit them.

## 2. Debian / Ubuntu WSL

```sh
sudo apt update
sudo apt install -y build-essential cmake git libgl1-mesa-dev libx11-dev
# optional, only for -DHF_PYTHON=ON:
sudo apt install -y python3-dev
```

`build-essential` supplies `g++`/`make`, `libgl1-mesa-dev` supplies
`GL/gl.h` + `GL/glx.h` + `libGL`, and `libx11-dev` supplies the `X11/*.h`
headers. This is the closest match to the verified glibc build environment.

## 3. Alpine WSL

Alpine uses **musl libc**, not glibc, and its default user is root, so no
`sudo`:

```sh
apk add --no-cache build-base cmake git mesa-dev libx11-dev
# optional, only for -DHF_PYTHON=ON:
apk add --no-cache python3-dev
```

`mesa-dev` provides the GL/GLX headers and library; `libx11-dev` provides the
X11 headers. `build-base` provides `g++`, `make` and the toolchain.

Alpine/musl notes:

- Use `g++` from `build-base`. If you switch to clang with libc++, the legacy
  `std::auto_ptr` headers can fail under C++17; stay on libstdc++ or enable
  the compatibility macro.
- musl provides stub `libdl`/`libpthread`, so the harness's `-ldl -lpthread`
  link flags should still resolve, but availability can vary by image —
  check your first link error if it does not.
- The large warning count (`format attribute only applies to function types`,
  deprecated `auto_ptr`, `operator delete` exception specifiers, string
  literals to `char*`) is pre-existing and is not a compile error.
- On aarch64 Alpine, CMake already skips the SSE2 flag and SIMD SSE sources;
  on x86_64 they are compiled normally.

## 4. Display / running

Compiling needs no display. **Running the full engine does.**

`--headless` is an argv flag, but `hfCommonLocal::Init()` still calls
`InitOpenGL()`. With no display you get `Unable to initialize OpenGL`.

WSL2 with **WSLg** usually sets `DISPLAY=:0`:

```sh
echo $DISPLAY
DISPLAY=:0 ./neo/build/hellforge
```

WSL1, or WSL2 without WSLg, needs a Windows X server (VcXsrv, Xming) and
forwarding. Common setup:

```sh
export DISPLAY="$(cat /etc/resolv.conf | awk '/nameserver/ {print $2}'):0"
export LIBGL_ALWAYS_INDIRECT=1     # often needed for GLX over forwarded X11
./neo/build/hellforge
```

For logic tests with no window at all, use the script harness with
`--headless` ([chapter 08](08-tests.md)); the harness scripts in this
workspace live in `/tmp/opencode/` and must be recreated on a fresh machine.

## 5. WSL troubleshooting

| Symptom | Fix |
|---------|-----|
| `GL/gl.h: No such file` | Debian: `libgl1-mesa-dev`; Alpine: `mesa-dev` |
| `X11/Xlib.h: No such file` | Debian: `libx11-dev`; Alpine: `libx11-dev` |
| `cmake: command not found` | Debian: `cmake`; Alpine: `cmake` |
| `Couldn't open X11 display. Is DISPLAY set?` | Start an X server / WSLg and set `DISPLAY` |
| `Unable to initialize OpenGL` | Same as above; `--headless` alone is not enough for the full engine |
| Link error with `auto_ptr` under clang/libc++ | Use `g++` from `build-base`/`build-essential` |
| Out of memory during build | Lower `-j` count |
| `hfsys_intrinsics.h`: `redefinition of '__m128'` / `__m128c` errors | Pull latest `main`: the Linux `__m128c` macro that shadowed the union was removed |
| New musl-specific compile error | Paste the **first** error block; fix that specific incompatibility |

## 6. After the build

- Both binaries should exist: `neo/build/hellforge` and
  `neo/build-jolt/hellforge`.
- Run the 4-way harness matrix before committing code changes
  ([chapter 08](08-tests.md)).
- Remember there are no `.map` assets or game DLL in this source release;
  START uses the engine-owned built-in arena. Gameplay input is wired to the
  sim ([chapter 09](09-first-game.md)).

## Where to go next

- [02 — Compiling](02-build.md) — full CMake option reference
- [03 — Running](03-run.md) — console, cvars and controls
- [08 — Tests](08-tests.md) — matrix details
