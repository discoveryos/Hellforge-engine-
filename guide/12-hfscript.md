# 12 — hfscript, explained from scratch

This chapter is about **hfscript itself**: the language, its VM, host functions,
compiled images and how scripts are loaded. For the `.py` / `.rb` / `.eng`
syntax flavours and the optional real-Python bridge, see
[chapter 04](04-scripting.md).

## 1. What hfscript is

hfscript is the engine's own small scripting language. From
`neo/script/HfScript.h`:

> a small, stack-based, bytecode scripting system in the spirit of classic FPS
> engine script languages: lexer / parser / bytecode compiler, portable
> short-circuit expression evaluation, user functions + host (engine) function
> bindings, `.hfs` files loaded from the engine file system.

Its most important property is also its biggest limitation:

> **Values are scalars only.** There are no arrays, objects, structs or
> pointers in the language.

That is why the big state (player, enemies, bullets) lives in C++ in `HfArena`,
and scripts only pass numbers/strings/bools back and forth. A script is a
*decision policy*, not a data container.

Pipeline:

```
source .hfs ──► lexer ──► parser ──► HfScriptProgram (bytecode + constants)
                                          │
                            ┌─────────────┴──────────────┐
                            ▼                            ▼
                     interpreter VM                JIT native code
                            │
                       host functions (C++)
                            │
                        structured C++ state
```

## 2. Values and literals

`enum hfScriptType_t` (`HfScript.h:27`):

| Type | Literal | Notes |
|------|---------|-------|
| `HST_VOID` | — | no value (internal) |
| `HST_NIL` | `nil` | explicit "no value"; also what failed calls usually return |
| `HST_BOOL` | `true`, `false` | `and`/`or`/`not` work on truthiness |
| `HST_INT` | `42` | stored as `int64` |
| `HST_FLOAT` | `3.5` | 32-bit float |
| `HST_STRING` | `"text"` | pooled in the program's string pool |

Strings beginning with `#str_` are **localized at compile time**;
`loc("key")` looks up a string at runtime (it passes through the key if no
dictionary is loaded).

`HfScriptValue` provides `Nil()`, `Bool()`, `Int()`, `Float()`, `Str()`,
`IsNil()`, `IsNumeric()`, `AsBool()`, `AsInt()`, `AsFloat()`, `AsString()`
and `Describe()`. Conversions are explicit; don't assume a string silently
becomes a number.

## 3. Syntax

### Comments and blocks

```
# this is a comment
def greet(name):
    # indented body
    print(name)
```

Blocks are **indentation-delimited**. Use consistent spaces; a wrong indent
level is the most common syntax error.

### Operators

| Kind | Operators |
|------|-----------|
| Arithmetic | `+`, `-`, `*`, `/`, `%`, unary `-` |
| Comparison | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| Logic | `and` / `&&`, `or` / `||`, `not` / `!` |
| Assignment | `=`, plus augmented forms such as `+=` |
| Grouping | `( … )` |

`and`/`or` are implemented by `HOP_AND` / `HOP_OR` (eager), while `if`
conditions use the jump opcodes (`HOP_JUMP_TRUE` / `HOP_JUMP_FALSE`) with
truthy/falsy testing.

### Statements

| Statement | Meaning |
|-----------|---------|
| `def name(a, b):` | Define a function |
| `if cond:` / `elif cond:` / `else:` | Conditional (`elsif` in the Ruby tag) |
| `while cond:` | Loop while true |
| `for i in range(n):` | Count `i` from 0 to `n - 1` |
| `return value` | Return a value (bare `return` returns no value) |
| `break` / `continue` | Leave / skip a loop iteration |
| `pass` | Explicit no-op |
| `global x` | Declare that assignment targets a global |

Variables are **local by default**. Inside a function, assigning to a name
without `global` creates a local; the compiler tracks `totalLocals` per
function. Top-level statements live at `program->entryOp`, and a function named
`main` becomes `program->entryFunction` (index `-1` when absent).

### A complete native-style script

```
# mybot.hfs — decide one action per arena tick
score = 0

def choose(hp):
    if hp < 40:
        return "retreat"
    else:
        return "attack"

def main():
    global score
    arena_reset(20260610)
    for i in range(3):
        arena_step()
    while arena_running() is 1:
        action = choose(arena_hp())
        if action == "retreat":
            arena_retreat()
        else:
            arena_attack()
        arena_step()
    score = arena_score()
    print(format("final score {}", score))

main()
```

The `is` spelling belongs to the English dialect; in native `.hfs` use `==`.
The harness is strict about scalar values: `arena_*` functions return ints or
floats, never objects.

## 4. Built-in host functions

Registered in `HfScript_Init()` (`neo/script/HfScript_Engine.cpp:281`):

| Category | Functions |
|----------|-----------|
| Output / text | `print`, `format`, `str`, `len`, `loc`, `lang` |
| Conversion | `int`, `float`, `bool` |
| Math | `abs`, `min`, `max`, `sqrt`, `floor`, `ceil`, `round`, `random` |
| Engine | `getcvar`, `setcvar`, `exec`, `time` |
| Game | the ten `arena_*` functions registered by `HfScript_RegisterArenaHosts()` |

`format("Hello, {}!", name)` uses numbered `{}` placeholders. `getcvar(name)`
reads a cvar as a string; `setcvar(name, value)` writes one. Avoid `random()`
and `time()` in gameplay logic that must be bit-deterministic — use a seeded
C++ sim instead.

### The arena hosts

`neo/script/HfScript_Game.h` registers:

```
arena_reset(seed)    arena_running()    arena_hp()       arena_score()
arena_wave()         arena_ticks()      arena_nearest()  arena_attack()
arena_retreat()      arena_step()
```

`arena_nearest()` returns the nearest enemy's **distance** (a float), not an
index. `arena_attack()` and `arena_retreat()` set the sim's `intent`; they do
not advance time. Only `arena_step()` advances the fixed tick.

## 5. Writing your own host function

A host function is any C++ function with this signature (`HfScript.h:251`):

```cpp
typedef void ( *HfScriptHostFn )( HfScriptVM * vm, int argc, HfScriptValue * outResult );
```

Read arguments with `vm->GetArg(index)` and `vm->GetArgc()`; write exactly one
result to `*outResult` (use `HfScriptValue::Nil()` for "no value"). Register by
name:

```cpp
static void Host_double( HfScriptVM * vm, int argc, HfScriptValue * out ) {
    *out = HfScriptValue::Int( argc > 0 ? vm->GetArg( 0 ).AsInt() * 2 : 0 );
}

HfScript_RegisterHost( "double", Host_double );
```

Then `print(double(21))` works in any script. Other useful VM helpers inside
hosts: `NewString(text)` (pool a string), `Push`/`Pop` (operand stack), and
`SetHostCallContext` (used by JIT-generated calls).

Rules of thumb:

- keep the signature scalar-only; structured state stays in C++;
- check `argc` before calling `GetArg`;
- make hosts pure where possible so replay stays deterministic;
- header-only hosts (like `HfScript_Game.h`) can be shared by the engine and
  the harness.

## 6. Bytecode and the VM

A compiled program is an `HfScriptProgram` containing:

- `ops` — array of `HfScriptOp { int32 op, a, b }`
- `constants` — `HfScriptValue` literals
- `functions` — `HfScriptFunction` (name, params, op range, locals, line)
- `globals` — `HfScriptGlobal` name/index pairs
- `globalValues` — runtime storage for globals
- `stringPool` — pooled literal text
- `entryFunction`, `entryOp` — where execution starts
- `jitHandle`, `jitEntry` — transient native code, never serialized
- `error`, `errorLine` — last compile/load error

Opcode families (`enum hfScriptOp_t`):

| Family | Opcodes |
|--------|---------|
| Constants / vars | `HOP_CONST`, `HOP_NIL`, `HOP_TRUE`, `HOP_FALSE`, `HOP_LOCAL`, `HOP_LOCAL_SET`, `HOP_GLOBAL`, `HOP_GLOBAL_SET` |
| Arithmetic / logic | `HOP_ADD`, `HOP_SUB`, `HOP_MUL`, `HOP_DIV`, `HOP_MOD`, `HOP_NEG`, `HOP_NOT`, `HOP_AND`, `HOP_OR`, `HOP_CMP` |
| Control flow | `HOP_JUMP`, `HOP_JUMP_TRUE`, `HOP_JUMP_FALSE`, `HOP_CALL`, `HOP_CALL_HOST`, `HOP_RETURN`, `HOP_END`, `HOP_POP`, `HOP_DEBUG_LINE` |

The VM (`HfScriptVM`) has an operand stack, a call-frame array and a bump
arena for locals. Functions come and go in strict LIFO order, so local
allocation is a pointer bump rather than an allocation per call. `HOP_DEBUG_LINE`
keeps source lines for diagnostics.

API:

```cpp
HfScriptVM vm;
HfScriptProgram program;
bool ok = vm.RunText( &program, source, "mybot.hfs" );  // compile + run
// or: compile once with HfScript_Compile(), then vm.RunProgram( &program )
if ( !ok ) { printf( "%s at line %d\n", vm.lastError.c_str(), vm.lastErrorLine ); }
```

## 7. Loading scripts in the engine

`HfScript_LoadFile( path )`:

1. Derive a `.hfobj` sidecar path (replace the extension).
2. If the sidecar exists, load it directly and run — **no lexing or parsing**.
3. Otherwise read the source text, choose a front end by extension
   (`.hfs`, `.py`, `.rb`, `.eng`) and run it.

Console commands: `hfs <script.hfs>` and `hfsReload`. C++ equivalent of the
latter: `HfScript_Reload()`. Shut down with `HfScript_Shutdown()`.

**Gotcha:** if you edit a script but a same-named `.hfobj` exists, your edits
are ignored. Delete or regenerate the sidecar.

## 8. Ahead-of-time images (`.hfobj`)

`HfScript_SaveImage`, `HfScript_LoadImage`, `HfScript_SaveImageToFile`,
`HfScript_LoadImageFromFile`.

Format (from `HfScript.h:183`):

```
"HFS1" | version u32 | flags u32 | op count u64 | constant count u64 |
function count u64 | global count u64 | pool used u64 |
entryOp s64 | entryFunction s64 | pool bytes | ops | constants |
functions | globals
```

Constants carry a type tag and a raw 64-bit union. String constants store an
**offset** into the string pool, so loading re-anchors the pointers after the
pool is reconstructed. Images are canonical little-endian and load on both
x86_64 and aarch64; they contain no pointers and no JIT code.

## 9. JIT native compilation

`HfScript_JitCompileProgram( program )` emits a C translation of the whole
bytecode program, compiles it with the system C compiler into a shared object,
and `dlopen()`s it. `HfScript_JitRelease( program )` unloads it.

- Generated code mirrors the interpreter's exact stack/frame semantics, so
  interpreted and JIT runs are interchangeable.
- Environment overrides: `HFSJIT_CC`, `HFSJIT_CFLAGS`, `HFSJIT_TMPDIR`.
- Native code is emitted for the host CPU (x86_64 or aarch64).
- JIT state is transient and never goes into a `.hfobj`.

## 10. Debugging checklist

| Symptom | Likely cause |
|---------|--------------|
| Edits have no effect | A `.hfobj` sidecar is shadowing the source |
| "syntax error" near a line | Indentation change, missing colon, or English keyword in a `.hfs` file |
| Variable reads as `nil` | Missing `global` declaration, or wrong scope |
| Comparison always false | String vs. number mismatch; use `int()` / `float()` |
| Non-repeatable run | Used `random()` / `time()` instead of seeded sim state |
| Host not found | Forgot `HfScript_RegisterHost`, or registration happened after compile |
| Crash inside a host | `GetArg()` called without checking `argc` |

Diagnostics live in `program->error` / `program->errorLine` for compile/load
and `vm.lastError` / `vm.lastErrorLine` at runtime.

## Where to go next

- [04 — Scripting](04-scripting.md) — dialects and the real Python bridge
- [05 — Making a game in C++](05-making-a-game-cpp.md) — new `arena_*` hosts
- [08 — Tests](08-tests.md) — scripted tests and the 4-way matrix
