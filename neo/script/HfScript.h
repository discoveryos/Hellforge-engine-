/*
===========================================================================

Hellforge Power 1 Engine - hfscript

hfscript is the engine's scripting language.  Scripts are plain text files
with a .hfs extension.  hfscript is a small, stack-based, bytecode
scripting system in the spirit of classic FPS engine script languages:

	- lexer / parser / bytecode compiler
	- portable short-circuit expression evaluation
	- user functions + host (engine) function bindings
	- .hfs files loaded from the engine file system

===========================================================================
*/
#ifndef __HFSCRIPT_H__
#define __HFSCRIPT_H__

#include "../hflib/hfprecompiled.h"

/*
================================================
Types
================================================
*/
enum hfScriptType_t {
	HST_VOID,							// no value
	HST_NIL,							// explicit "no value" (null)
	HST_BOOL,
	HST_INT,
	HST_FLOAT,
	HST_STRING,
};

class HfScriptValue {
public:
					HfScriptValue();

	static HfScriptValue	Nil();
	static HfScriptValue	Bool( bool b );
	static HfScriptValue	Int( int64 i );
	static HfScriptValue	Float( float f );
	static HfScriptValue	Str( const char * s );

	bool			IsNil() const { return type == HST_NIL; }
	bool			IsNumeric() const { return type == HST_INT || type == HST_FLOAT; }
	bool			AsBool() const;
	int64			AsInt() const;
	float			AsFloat() const;
	const char *	AsString( hfStr & buffer ) const;

	int				Describe( char * out, int outSize ) const;

	union {
		bool		b;
		int64		i;
		float		f;
		const char * s;
	};
	int				type;
};

/*
================================================
Bytecode
================================================
*/
enum hfScriptOp_t {
	HOP_NOP,
	HOP_CONST,							// push constants[a]
	HOP_NIL,							// push nil
	HOP_TRUE,
	HOP_FALSE,
	HOP_LOCAL,							// push locals[a]
	HOP_LOCAL_SET,						// locals[a] = pop ; push back
	HOP_GLOBAL,							// push globals[a]
	HOP_GLOBAL_SET,						// globals[a] = pop ; push back
	HOP_ADD,
	HOP_SUB,
	HOP_MUL,
	HOP_DIV,
	HOP_MOD,
	HOP_NEG,
	HOP_NOT,
	HOP_AND,							// pop b,a ; push (a && b)  (eager)
	HOP_OR,								// pop b,a ; push (a || b)  (eager)
	HOP_CMP,							// pop b,a ; push (a <cmp[op]> b)
	HOP_JUMP,							// pc = a
	HOP_JUMP_TRUE,						// pop ; if truthy pc = a
	HOP_JUMP_FALSE,						// pop ; if !truthy pc = a
	HOP_CALL,							// a = function index
	HOP_CALL_HOST,						// a = host function index
	HOP_RETURN,							// b: 1 if a result sits on the stack
	HOP_END,							// end of program
	HOP_POP,							// drop top of stack
	HOP_DEBUG_LINE,						// a = source line of next op (diagnostics)
};

enum hfScriptCmp_t {
	HSC_EQ,
	HSC_NE,
	HSC_LT,
	HSC_LE,
	HSC_GT,
	HSC_GE,
};

struct HfScriptOp {
	int32			op;
	int32			a;
	int32			b;
};

class HfScriptVM;			// forward (defined below)

/*
================================================
Function / global / program
================================================
*/
struct HfScriptFunction {
	hfStr			name;
	int				numParams;
	int				opStart;
	int				opEnd;
	int				totalLocals;		// numParams + locals
	int				line;
};

struct HfScriptGlobal {
	hfStr			name;
	int				index;
};

class HfScriptProgram;			// forward (defined below)
typedef void		( *HfScriptJitEntry )( HfScriptVM * vm, HfScriptProgram * program, HfScriptValue * outResult );

class HfScriptProgram {
public:
					HfScriptProgram();
					~HfScriptProgram();

	void			Reset();

	HfScriptOp *	AllocOps( int count );
	int				AddConstant( const HfScriptValue & v );
	int				AddFunction( const char * name, int numParams, int totalLocals );
	int				AddGlobal( const char * name );
	int				PoolString( const char * text );

	const char *	sourceName;			// for reloads / diagnostics
	bool			valid;

	HfScriptOp *	ops;				int numOps;			int opAlloc;
	HfScriptValue *	constants;			int numConstants;	int constAlloc;
	HfScriptFunction * functions;		int numFunctions;	int funcAlloc;
	HfScriptGlobal *	globals;		int numGlobals;		int globalAlloc;

	// runtime storage for global variables
	HfScriptValue *	globalValues;		int globalValuesSize;

	char *			stringPool;			int poolSize;		int poolUsed;

	int				entryFunction;		// index of "main" or -1
	int				entryOp;			// op index of top-level statements

	// JIT (transient - never serialized into images)
	void *			jitHandle;			// dlopen() handle of the native module, or NULL
	HfScriptJitEntry jitEntry;			// native entry, or NULL (use the interpreter)

	hfStr			error;				// last compile/load error
	int				errorLine;
};

/*
================================================
Compiler
================================================
*/
bool				HfScript_Compile( HfScriptProgram * program, const char * source, const char * sourceName );

/*
================================================
Ahead-of-time images (.hfobj)
================================================
The compiled program can be serialized to a portable, endian-stable image
that reloads instantly with no lexing/parsing.  Image format:

	"HFS1" | version u32 | flags u32 | op count u64 | constant count u64 |
	function count u64 | global count u64 | pool used u64 |
	entryOp s64 | entryFunction s64 | pool bytes | ops | constants |
	functions | globals

Constants are stored with a type tag + raw 64-bit union.  HST_STRING
constants store the *offset* of their text inside the string pool, so the
pool (which is reconstructed byte-for-byte and never moved) re-anchors the
pointers on load.  Images are canonical little-endian and load on both
aarch64 and x86_64.
*/
bool				HfScript_SaveImage( const HfScriptProgram * program, void * * outBuffer, int * outSize );	// caller frees with Mem_Free
bool				HfScript_LoadImage( HfScriptProgram * program, const void * buffer, int bufferSize );
bool				HfScript_SaveImageToFile( const HfScriptProgram * program, const char * path );
bool				HfScript_LoadImageFromFile( HfScriptProgram * program, const char * path );

/*
================================================
Just-in-time native compilation
================================================
The same bytecode stream feeds both the interpreter and a native path.
HfScript_JitCompileProgram emits a C translation of the whole program and
compiles it to a shared object with the system C compiler (overrideable with
HFSJIT_CC / HFSJIT_CFLAGS / HFSJIT_TMPDIR env vars), then dlopens it.  The
generated module preserves the interpreter's exact stack/frame semantics, so
interpreted and JIT runs are interchangeable; on x86_64 the system compiler
emits x86_64 code, on aarch64 it emits aarch64 code.  The JIT handle is
transient and is never serialized into .hfobj images.
*/
bool				HfScript_JitCompileProgram( HfScriptProgram * program );
void				HfScript_JitRelease( HfScriptProgram * program );

/*
================================================
Languages
================================================
A .py source runs through the same compiler/VM as .hfs - the dialect is a
strict, purely-functional-math Python subset (def/if/elif/else/while/
for-in-range/global/pass/break/continue, # comments).  The tag only affects
diagnostics and error attribution.

A .rb source uses the same compiler with a Ruby-flavored tag (def...end,
if...end, while...end, for-in-range...end, # comments).  Same bytecode,
same semantics, different error attribution.

At the engine layer (HfScript_LoadFile) the file extension selects the
front end: ".hfs", ".py", and ".rb" are compiled from source, and a
precompiled ".hfobj" sidecar ("foo.hfobj" next to "foo.hfs"/"foo.py"/"foo.rb")
is loaded and run directly when present, bypassing lexing/parsing.
*/
bool				HfScript_IsPythonSource( const char * sourceName );
bool				HfScript_IsRubySource( const char * sourceName );
bool				HfScript_IsEnglishSource( const char * sourceName );

/*
================================================
Host function bindings (engine functions callable from scripts)
================================================
*/
class HfScriptVM;

typedef void ( *HfScriptHostFn )( HfScriptVM * vm, int argc, HfScriptValue * outResult );

struct HfScriptHostDef {
	const char *	name;				// name exposed to scripts
	HfScriptHostFn	fn;
};

void				HfScript_RegisterHost( const char * name, HfScriptHostFn fn );
void				HfScript_UnregisterAllHosts();
int					HfScript_FindHostFunction( const char * name );	// -1 if not registered

/*
================================================
Virtual machine
================================================
*/
class HfScriptVM {
public:
					HfScriptVM();
					~HfScriptVM();

	// Compile + run a program from source text.  Returns false on failure.
	bool			RunText( HfScriptProgram * program, const char * source, const char * sourceName );
	// Run a previously compiled program.  Returns false on runtime error.
	bool			RunProgram( HfScriptProgram * program );

	// Helpers for use inside host functions
	HfScriptValue	GetArg( int index ) const;			// arg from current call
	int				GetArgc() const;
	const char *	NewString( const char * text );		// string pooled in the program

	// JIT execution bridge - used by native code generated from bytecode.
	// The generated module mirrors the interpreter's operand stack/frames and
	// calls back into these for allocations, host calls, strings and errors.
	void			ExecReset();
	HfScriptValue * ExecStackBase();
	void			ExecEnsureStack( int needed );
	int				ExecStackCapacity() const;
	HfScriptValue * ExecNewLocals( int count );
	void			ExecFreeLocals( HfScriptValue * locals );
	void			ExecCallHost( int hostIndex, int argc, const HfScriptValue * args, HfScriptValue * out );
	void			ExecError( int line, const char * msg );
	const char *	ExecStringify( const HfScriptValue & v );
	const char *	ExecConcatStrings( const char * a, const char * b );
	bool			ExecCompare( const HfScriptValue & a, const HfScriptValue & b, hfScriptCmp_t cmp ) const {
		return CompareValues( a, b, cmp );
	}

private:
	void			Error( const char * fmt, ... );		// sets lastError, aborts execution
	bool			ExecProgram();
	bool			CompareValues( const HfScriptValue & a, const HfScriptValue & b, hfScriptCmp_t cmp ) const;

	HfScriptProgram * program;

	// operand stack
	HfScriptValue *	stack;		int stackTop;		int stackSize;

	// call frames
	struct Frame {
		HfScriptValue *	locals;
		int			localCount;
		int			retOp;
		int			stackBase;
	};
	Frame *			frames;		int numFrames;		int frameAlloc;

	// per-run arena for call-frame locals: functions come and go in strict
	// LIFO order, so a bump allocator avoids malloc/free on every call.
	HfScriptValue *	arenaData;	int arenaUsed;		int arenaSize;

	HfScriptValue	callResult;

	// for GetArg within host functions
	int				hostArgc;
	int				hostArgBase;

	// host-argument override used by JIT-generated host calls (args live in
	// native stack storage, not on the operand stack)
	HfScriptValue *	hostArgOverride;
	int				hostArgOverrideCount;

public:
	hfStr			lastError;
	int				lastErrorLine;

	HF_INLINE void Push( const HfScriptValue & v ) {
		if ( stackTop >= stackSize ) {
			GrowStack();
		}
		stack[stackTop++] = v;
	}
	HF_INLINE HfScriptValue Pop() {
		if ( stackTop <= 0 ) {
			return HfScriptValue::Nil();
		}
		return stack[--stackTop];
	}
	void GrowStack();

	void			SetHostCallContext( int argc, int argBase ) { hostArgc = argc; hostArgBase = argBase; }

	friend void		HfScript_Host_default( HfScriptVM * vm, int argc, HfScriptValue * out );
};

/*
================================================
Engine integration
================================================
*/
void				HfScript_Init();
void				HfScript_Shutdown();
bool				HfScript_LoadFile( const char * path );	// compile+run a .hfs / .py (or load the .hfobj sidecar) from the file system
bool				HfScript_Reload();							// re-load the last loaded script

#endif /* !__HFSCRIPT_H__ */