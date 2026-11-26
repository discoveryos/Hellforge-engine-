# 04 — Scripting

For a deeper, hfscript-only explanation of the language, VM, host functions and
compiled images, see [chapter 12](12-hfscript.md).

## The four source dialects

`HfScript_LoadFile()` (`neo/script/HfScript_Engine.cpp:383`) chooses a front
end by extension. All four are compiled by the **same lexer, parser and VM** —
they differ in keywords and error attribution, not in runtime.

| Extension | Dialect | Example keywords |
|-----------|---------|------------------|
| `.hfs` | Native hfscript | `def`, `if`, `elif`, `else`, `for i in range(n)`, `while`, `return`, `pass`, `break`, `continue` |
| `.py` | Python-flavoured tag | same compiler; Python diagnostics |
| `.rb` | Ruby-flavoured tag | `def … end`, `if/elsif/else/end`, `while … end`, `#` comments |
| `.eng` | English | `define`, `set x to`, `increase x by`, `show`, `repeat N times`, `give back`, `otherwise`, `is greater than / less than / equal to / at least / at most`, `plus/times/...` |

Blocks are indentation-delimited. Colons and `end` are optional where the
dialect allows them. Strings prefixed with `#str_` are localized at compile
time; `loc("key")` resolves at runtime.

Console commands: `hfs <script.hfs>` runs a script, `hfsReload` reloads it.

### `.hfobj` sidecars

`HfScript_LoadFile()` first looks for a precompiled sidecar (e.g.
`arena.hfobj` next to `arena.eng`) and loads it directly, bypassing lexing.
**A sidecar silently wins over the source file** — delete or regenerate it
when editing scripts.

API: `HfScript_SaveImage`, `HfScript_LoadImage`, `HfScript_SaveImageToFile`,
`HfScript_LoadImageFromFile`. Format magic is `HFS1`, version 1, canonical
little-endian with re-anchored string offsets, so images are portable between
x86_64 and aarch64. AOT images never contain raw pointers.

### JIT

`HfScript_JitCompileProgram` / `HfScript_JitRelease` can compile VM programs
through an external compiler. Environment knobs: `HFSJIT_CC`,
`HFSJIT_CFLAGS`, `HFSJIT_TMPDIR`. JIT code is never serialized into `.hfobj`.

## Built-in host functions

Registered in `HfScript_Init()`:

```
print, loc, lang, format, len, str, int, float, bool, abs, min, max,
sqrt, floor, ceil, round, random, getcvar, setcvar, exec, time
```

The harness adds its own (`getnum`, `getcvar` variants, etc.). Your game can
add more with `HfScript_RegisterHost`.

## The arena host functions

`neo/script/HfScript_Game.h` registers all ten existing `arena_*` hosts:

| Function | Meaning |
|----------|---------|
| `arena_reset(seed)` | Reset the sim with seed (`0` becomes `1`) and spawn wave 1 |
| `arena_running()` | `1` while not game over |
| `arena_hp()` | Player hit points |
| `arena_score()` | Score |
| `arena_wave()` | Current wave |
| `arena_ticks()` | Elapsed fixed ticks |
| `arena_nearest()` | Index of nearest enemy (`-1` if none) |
| `arena_attack()` | Set intent to +1 (steer toward and fire) |
| `arena_retreat()` | Set intent to -1 (move away) |
| `arena_step()` | Advance one fixed tick |

hfscript cannot read the structured arrays; that is deliberate. Planned but
**not implemented** hosts: `arena_set_move`, `arena_set_aim`, `arena_set_fire`,
`arena_set_dodge`, `arena_enemy_kind`, `arena_pickup_kind`.

## The sample game, line by line

`base/script/arena.eng`:

```
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

This is the whole "game": C++ owns the sim; the script decides attack vs.
retreat each tick. `Test_Game` runs this file and asserts deterministic
score/ticks/wave. A fixed seed plus a fixed decision sequence gives the same
result every run on the same binary.

## Native-style example (`demo.hfs`)

```
game = "Hellforge"

def greet(name):
    print(format("Hello, {} from {}!", name, game))
    return len(name)

def main():
    for i in range(5):
        print(format("i = {}", i))
    x = 3.5
    if x > 4:
        print("big")
    elif x > 3:
        print("medium")
    else:
        print("small")
    setcvar("test_scriptvar", "abc")
    print(getcvar("test_scriptvar"))

main()
```

## Real Python (optional)

`neo/script/HfScript_PyBridge.*` embeds CPython when built with
`-DHF_PYTHON=ON` (needs `python3-dev` and links `libpython`). It registers:

| Host | Purpose |
|------|---------|
| `python_eval("expr")` | Evaluate an expression, return its value |
| `python_exec("stmt")` | Run statements, return nil |
| `python_import("module")` | Make a module importable |
| `python_get("module", "attr")` | Read a module attribute |
| `python_call("module", "func", a...)` | Call a function with arguments |

Value conversion: `nil ↔ None`, bool, int, float and string round-trip;
anything else becomes `None`/`nil`. The GIL is held for each host call and the
interpreter uses the main thread. Call `HfScript_PyBridge_Shutdown()` at
engine shutdown. With `HF_PYTHON` undefined, all functions are no-op stubs.

Note this is a **bridge from hfscript into CPython**, not a replacement for the
VM: your `.hfs`/`.eng` files call `python_*` hosts to use the standard library.
It is OFF in both verified builds.

## Ruby, precisely

`HfScript_Ruby.h` states it is "purely a source tag: `.rb` files use the same
compiler/VM as `.hfs`". There is no embedded Ruby interpreter. Write
Ruby-flavoured hfscript, not actual Ruby gems or stdlib.

## Adding your own host function

1. Declare/register it with `HfScript_RegisterHost` (see
   `HfScript_RegisterArenaHosts()` in `neo/script/HfScript_Game.h`).
2. Keep the signature scalar-only: `HfScriptVM &vm`, arguments read from the
   VM, one scalar result pushed back.
3. Put structured state in C++ and expose scalar accessors, as the arena does.
4. Test through a script file so the host/VM contract is verified headlessly.
