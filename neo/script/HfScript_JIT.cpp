/*
===========================================================================

Hellforge Power 1 Engine - hfscript just-in-time native compilation

HfScript_JitCompileProgram lowers the bytecode stream (the SAME stream the
interpreter walks) into a single C++ function whose control flow is a
computed-goto thread over the ops.  That source is compiled to a shared
object with the system C compiler and dlopened, giving the interpreter's
semantics (identical operand stack / frame behaviour) at native speed.

Because the system compiler does the ISA-specific lowering, the same code
path produces x86_64 code on x86_64 and aarch64 code on aarch64.

	HFSJIT_CC       - compiler binary (default: "cc")
	HFSJIT_CFLAGS   - extra flags, e.g. -O2 (default: -O2)
	HFSJIT_TMPDIR   - scratch dir for generated files (default: /tmp)

The JIT module is transient: it is never serialized into .hfobj images and
is released with HfScript_JitRelease (also called by HfScriptProgram::Reset).

===========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "HfScript.h"

#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

/*
====================
Line builder
====================
*/
struct hfJitWriter {
	hfStr				text;
	char				tmp[ 4096 ];

	void Line( const char * fmt, ... ) {
		va_list ap;
		va_start( ap, fmt );
		hfStr::vsnPrintf( tmp, sizeof( tmp ), fmt, ap );
		va_end( ap );
		text += tmp;
		text += '\n';
	}
};

/*
====================
Max operand-stack height + branch targets
====================
*/
struct hfJitPlan {
	int					maxStack;
	bool *				isTarget;		// ops that are branch/call targets (budget checks)
	int					numOps;

	hfJitPlan() : maxStack( 0 ), isTarget( NULL ), numOps( 0 ) {}
	~hfJitPlan() { delete[] isTarget; }

	void Build( const HfScriptProgram * prog );
};

void hfJitPlan::Build( const HfScriptProgram * prog ) {
	const int N = prog->numOps;
	numOps = N;
	isTarget = new bool[ N > 0 ? N : 1 ];
	for ( int i = 0; i < ( N > 0 ? N : 1 ); i++ ) {
		isTarget[ i ] = false;
	}

	int * height = new int[ N > 0 ? N : 1 ];
	for ( int i = 0; i < ( N > 0 ? N : 1 ); i++ ) {
		height[ i ] = -1;
	}
	if ( prog->entryOp >= 0 && prog->entryOp < N ) {
		height[ prog->entryOp ] = 0;
		isTarget[ prog->entryOp ] = true;
	}

	bool changed = true;
	int passes = 0;
	while ( changed && passes++ < 4096 ) {
		changed = false;
		for ( int i = 0; i < N; i++ ) {
			if ( height[ i ] < 0 ) {
				continue;
			}
			const HfScriptOp & op = prog->ops[ i ];
			int h = height[ i ];
			int net = 0;
			int tg1 = -1, tg2 = -1;
			switch ( op.op ) {
				case HOP_NOP:
				case HOP_DEBUG_LINE:
					net = 0; tg1 = i + 1; break;
				case HOP_NIL:
				case HOP_TRUE:
				case HOP_FALSE:
				case HOP_CONST:
				case HOP_LOCAL:
				case HOP_GLOBAL:
					net = +1; tg1 = i + 1; break;
				case HOP_LOCAL_SET:
				case HOP_GLOBAL_SET:
				case HOP_NEG:
				case HOP_NOT:
					net = 0; tg1 = i + 1; break;
				case HOP_POP:
					net = -1; tg1 = i + 1; break;
				case HOP_ADD:
				case HOP_SUB:
				case HOP_MUL:
				case HOP_DIV:
				case HOP_MOD:
				case HOP_CMP:
				case HOP_AND:
				case HOP_OR:
					net = -1; tg1 = i + 1; break;
				case HOP_JUMP:
					net = 0; tg1 = op.a;
					if ( op.a >= 0 && op.a < N ) { isTarget[ op.a ] = true; }
					break;
				case HOP_JUMP_TRUE:
				case HOP_JUMP_FALSE:
					net = -1; tg1 = op.a; tg2 = i + 1;
					if ( op.a >= 0 && op.a < N ) { isTarget[ op.a ] = true; }
					break;
				case HOP_CALL:
					net = -op.b; tg1 = i + 1;	// result lands above the frame base
					if ( op.a >= 0 && op.a < prog->numFunctions ) {
						int st = 0;
						tg1 = prog->functions[ op.a ].opStart;
						tg2 = i + 1;
						if ( tg1 >= 0 && tg1 < N ) { isTarget[ tg1 ] = true; }
						(void)st;
					}
					break;
				case HOP_CALL_HOST:
					net = 1 - op.b; tg1 = i + 1; break;
				case HOP_RETURN:
				case HOP_END:
					break;	// sink
				default:
					net = 0; tg1 = i + 1; break;
			}
			const int ao = h + net;
			if ( tg1 >= 0 && tg1 < N && ao > height[ tg1 ] ) {
				height[ tg1 ] = ao;
				changed = true;
			}
			if ( tg2 >= 0 && tg2 < N && ao > height[ tg2 ] ) {
				height[ tg2 ] = ao;
				changed = true;
			}
		}
	}

	maxStack = 0;
	for ( int i = 0; i < N; i++ ) {
		if ( height[ i ] > maxStack ) {
			maxStack = height[ i ];
		}
	}
	maxStack += 128;
	delete[] height;
}

/*
====================
Opcode -> C statement emission
All temporaries are function-scope (declared before any label) so computed
gotos never cross an initialization.
====================
*/
static void EmitOp( hfJitWriter & w, const HfScriptProgram * prog, const hfJitPlan & plan, int i ) {
	const HfScriptOp & op = prog->ops[ i ];

	w.Line( "L%d: ;", i );
	if ( plan.isTarget[ i ] ) {
		w.Line( "\tif ( --opbudget <= 0 ) { vm->ExecError( dline, \"script exceeded the op budget (possible infinite loop)\" ); HJEXIT; }" );
	}

	switch ( op.op ) {
		case HOP_NOP:
		case HOP_DEBUG_LINE:
			if ( op.op == HOP_DEBUG_LINE ) {
				w.Line( "\tdline = %d;", op.a );
			}
			break;

		case HOP_CONST:
			w.Line( "\tst[ sp++ ] = prog->constants[ %d ];", op.a );
			break;

		case HOP_NIL:
			w.Line( "\ttv.type = HST_NIL; tv.b = false; st[ sp++ ] = tv;" );
			break;
		case HOP_TRUE:
			w.Line( "\ttv.type = HST_BOOL; tv.b = true; st[ sp++ ] = tv;" );
			break;
		case HOP_FALSE:
			w.Line( "\ttv.type = HST_BOOL; tv.b = false; st[ sp++ ] = tv;" );
			break;

		case HOP_POP:
			w.Line( "\tsp--;" );
			break;

		case HOP_LOCAL:
			w.Line( "\tif ( %d < 0 || %d >= fr[ nf - 1 ].localCount ) { vm->ExecError( dline, \"local index out of range (%d)\" ); HJEXIT; }", op.a, op.a, op.a );
			w.Line( "\tst[ sp++ ] = fr[ nf - 1 ].locals[ %d ];", op.a );
			break;

		case HOP_LOCAL_SET:
			w.Line( "\tif ( %d < 0 || %d >= fr[ nf - 1 ].localCount ) { vm->ExecError( dline, \"local index out of range (%d)\" ); HJEXIT; }", op.a, op.a, op.a );
			w.Line( "\ttv = st[ --sp ]; fr[ nf - 1 ].locals[ %d ] = tv; st[ sp++ ] = tv;", op.a );
			break;

		case HOP_GLOBAL:
			w.Line( "\tif ( %d < 0 || %d >= prog->numGlobals ) { vm->ExecError( dline, \"global index out of range (%d)\" ); HJEXIT; }", op.a, op.a, op.a );
			w.Line( "\tst[ sp++ ] = prog->globalValues[ %d ];", op.a );
			break;

		case HOP_GLOBAL_SET:
			w.Line( "\tif ( %d < 0 || %d >= prog->numGlobals ) { vm->ExecError( dline, \"global index out of range (%d)\" ); HJEXIT; }", op.a, op.a, op.a );
			w.Line( "\ttv = st[ --sp ]; prog->globalValues[ %d ] = tv; st[ sp++ ] = tv;", op.a );
			break;

		case HOP_ADD:
		case HOP_SUB:
		case HOP_MUL:
		case HOP_DIV:
		case HOP_MOD: {
			if ( op.op == HOP_ADD ) {
				w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ];" );
				w.Line( "\tif ( tva.type == HST_STRING || tvb.type == HST_STRING ) {" );
				w.Line( "\t\tconst char * rs = vm->ExecConcatStrings( vm->ExecStringify( tva ), vm->ExecStringify( tvb ) );" );
				w.Line( "\t\ttv.type = HST_STRING; tv.s = rs; st[ sp++ ] = tv; }" );
				w.Line( "\telse if ( tva.IsNumeric() && tvb.IsNumeric() ) {" );
				w.Line( "\t\tif ( tva.type == HST_INT && tvb.type == HST_INT ) { tv.type = HST_INT; tv.i = tva.i + tvb.i; }" );
				w.Line( "\t\telse { tv.type = HST_FLOAT; tv.f = tva.AsFloat() + tvb.AsFloat(); } st[ sp++ ] = tv; }" );
				w.Line( "\telse { tv.type = HST_INT; tv.i = tva.AsInt() + tvb.AsInt(); st[ sp++ ] = tv; }" );
			} else if ( op.op == HOP_SUB || op.op == HOP_MUL ) {
				w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ];" );
				w.Line( "\tif ( tva.IsNumeric() && tvb.IsNumeric() ) {" );
				w.Line( "\t\tif ( tva.type == HST_INT && tvb.type == HST_INT ) { tv.type = HST_INT; tv.i = tva.i %s tvb.i; }", op.op == HOP_SUB ? "-" : "*" );
				w.Line( "\t\telse { tv.type = HST_FLOAT; tv.f = tva.AsFloat() %s tvb.AsFloat(); } st[ sp++ ] = tv; }", op.op == HOP_SUB ? "-" : "*" );
				w.Line( "\telse { tv.type = HST_INT; tv.i = tva.AsInt() %s tvb.AsInt(); st[ sp++ ] = tv; }", op.op == HOP_SUB ? "-" : "*" );
			} else if ( op.op == HOP_DIV ) {
				w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ];" );
				w.Line( "\tfdiv = tvb.AsFloat(); if ( fdiv == 0.0f ) { vm->ExecError( dline, \"divide by zero\" ); HJEXIT; }" );
				w.Line( "\ttv.type = HST_FLOAT; tv.f = tva.AsFloat() / fdiv; st[ sp++ ] = tv;" );
			} else {	// HOP_MOD
				w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ];" );
				w.Line( "\tif ( tva.type == HST_INT && tvb.type == HST_INT ) {" );
				w.Line( "\t\tif ( tvb.i == 0 ) { vm->ExecError( dline, \"modulo by zero\" ); HJEXIT; }" );
				w.Line( "\t\ttv.type = HST_INT; tv.i = tva.i %% tvb.i; st[ sp++ ] = tv; }" );
				w.Line( "\telse { fdiv = tvb.AsFloat(); if ( fdiv == 0.0f ) { vm->ExecError( dline, \"modulo by zero\" ); HJEXIT; }" );
				w.Line( "\t\ttv.type = HST_FLOAT; tv.f = (float)fmod( tva.AsFloat(), fdiv ); st[ sp++ ] = tv; }" );
			}
			break;
		}

		case HOP_NEG:
			w.Line( "\ttv = st[ --sp ]; if ( tv.type == HST_INT ) { tv.i = -tv.i; }" );
			w.Line( "\telse if ( tv.IsNumeric() ) { tv.type = HST_FLOAT; tv.f = -tv.f; }" );
			w.Line( "\telse { tv.type = HST_INT; tv.i = -tv.AsInt(); } st[ sp++ ] = tv;" );
			break;

		case HOP_NOT:
			w.Line( "\ttv = st[ --sp ]; tv.type = HST_BOOL; tv.b = !tv.AsBool(); st[ sp++ ] = tv;" );
			break;

		case HOP_AND:
			w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ]; tv.type = HST_BOOL;" );
			w.Line( "\ttv.b = tva.AsBool() && tvb.AsBool(); st[ sp++ ] = tv;" );
			break;

		case HOP_OR:
			w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ]; tv.type = HST_BOOL;" );
			w.Line( "\ttv.b = tva.AsBool() || tvb.AsBool(); st[ sp++ ] = tv;" );
			break;

		case HOP_CMP:
			w.Line( "\ttvb = st[ --sp ]; tva = st[ --sp ]; tv.type = HST_BOOL;" );
			w.Line( "\ttv.b = vm->ExecCompare( tva, tvb, (hfScriptCmp_t)%d ); st[ sp++ ] = tv;", op.b );
			break;

		case HOP_JUMP:
			w.Line( "\tgoto *hfjit_jt[ %d ];", op.a );
			break;

		case HOP_JUMP_TRUE:
			w.Line( "\tif ( st[ --sp ].AsBool() ) { goto *hfjit_jt[ %d ]; }", op.a );
			break;

		case HOP_JUMP_FALSE:
			w.Line( "\tif ( !st[ --sp ].AsBool() ) { goto *hfjit_jt[ %d ]; }", op.a );
			break;

		case HOP_CALL:
			w.Line( "\tcindex = %d; if ( cindex < 0 || cindex >= prog->numFunctions ) { vm->ExecError( dline, \"bad function index (%d)\" ); HJEXIT; }", op.a, op.a );
			w.Line( "\tcargc = %d; cfp = &prog->functions[ cindex ];", op.b );
			w.Line( "\tif ( cargc != cfp->numParams ) { vm->ExecError( dline, \"function \\\"%s\\\" expects %d arguments, got %d\" ); HJEXIT; }",
				prog->functions[ op.a ].name.c_str(), prog->functions[ op.a ].numParams, op.b );
			w.Line( "\tif ( nf >= 512 ) { vm->ExecError( dline, \"call stack overflow\" ); HJEXIT; }" );
			w.Line( "\tcnl = vm->ExecNewLocals( cfp->totalLocals );" );
			w.Line( "\tfor ( int qi = 0; qi < cargc; qi++ ) { cnl[ qi ] = st[ sp - cargc + qi ]; }" );
			w.Line( "\tfr[ nf ].locals = cnl; fr[ nf ].localCount = cfp->totalLocals; fr[ nf ].retOp = %d; fr[ nf ].stackBase = sp - cargc; nf++;", i + 1 );
			w.Line( "\tsp -= cargc;" );
			w.Line( "\tgoto *hfjit_jt[ cfp->opStart ];" );
			break;

		case HOP_CALL_HOST:
			w.Line( "\thres.type = HST_NIL; hres.b = false;" );
			w.Line( "\tvm->ExecCallHost( %d, %d, &st[ sp - %d ], &hres );", op.a, op.b, op.b );
			w.Line( "\tsp -= %d; st[ sp++ ] = hres;", op.b );
			break;

		case HOP_RETURN:
			w.Line( "\trv.type = HST_NIL; rv.b = false;" );
			if ( op.b != 0 ) {
				w.Line( "\trv = st[ --sp ];" );
			}
			w.Line( "\tif ( nf <= 0 ) { vm->ExecError( dline, \"return with no active frame\" ); HJEXIT; }" );
			w.Line( "\tcbase = fr[ nf - 1 ].stackBase; crop = fr[ nf - 1 ].retOp;" );
			w.Line( "\tvm->ExecFreeLocals( fr[ nf - 1 ].locals );" );
			w.Line( "\tnf--; sp = cbase;" );
			w.Line( "\tif ( crop == 0 ) { *out = rv; return; }" );
			w.Line( "\tst[ sp++ ] = rv; goto *hfjit_jt[ crop ];" );
			break;

		case HOP_END:
			w.Line( "\tout->type = HST_NIL; out->b = false; return;" );
			break;

		default:
			w.Line( "\tvm->ExecError( dline, \"unknown opcode %d in JIT\" ); HJEXIT;", op.op );
			break;
	}
}

/*
====================
HfScript_JitGenerateSource
====================
*/
static bool HfScript_JitGenerateSource( hfJitWriter & w, const HfScriptProgram * prog, hfJitPlan & plan ) {
	plan.Build( prog );
	if ( prog->entryOp < 0 || prog->entryOp >= prog->numOps || prog->numOps <= 0 ) {
		return false;
	}

	w.Line( "// generated by the hfscript JIT - not a hand-written file" );
	w.Line( "#include \"HfScript.h\"" );
	w.Line( "#include <math.h>" );
	w.Line( "extern \"C\" void hf_jit_entry( HfScriptVM * vm, HfScriptProgram * prog, HfScriptValue * out ) {" );
	w.Line( "\tvm->ExecEnsureStack( %d );", plan.maxStack );
	w.Line( "\tHfScriptValue * st = vm->ExecStackBase();" );
	w.Line( "\tint sp = 0;" );
	w.Line( "\tint nf = 0;" );
	w.Line( "\tlong long opbudget = 50000000LL;" );
	w.Line( "\tint dline = 0;" );
	w.Line( "\tfloat fdiv = 0.0f;" );
	w.Line( "\tHfScriptValue tv; HfScriptValue tva; HfScriptValue tvb; HfScriptValue rv; HfScriptValue hres;" );
	w.Line( "\tHfScriptFunction * cfp = NULL; int cindex = 0; int cargc = 0; HfScriptValue * cnl = NULL;" );
	w.Line( "\tint cbase = 0; int crop = 0;" );
	w.Line( "\tstruct HfJitFrame { HfScriptValue * locals; int localCount; int retOp; int stackBase; } fr[ 512 ];" );
	w.Line( "\t#define HJEXIT do { for ( int qi = 0; qi < nf; qi++ ) vm->ExecFreeLocals( fr[ qi ].locals ); out->type = HST_NIL; out->b = false; return; } while ( 0 )" );
	w.Line( "\tstatic const void * const hfjit_jt[ %d ] = {", prog->numOps );
	for ( int i = 0; i < prog->numOps; i++ ) {
		w.Line( "\t\t&&L%d%s", i, i + 1 < prog->numOps ? "," : "" );
	}
	w.Line( "\t};" );
	w.Line( "\tgoto *hfjit_jt[ prog->entryOp ];" );
	for ( int i = 0; i < prog->numOps; i++ ) {
		EmitOp( w, prog, plan, i );
	}
	w.Line( "}" );
	return true;
}

/*
====================
File helpers
====================
*/
static void PathJoin( char * out, int outSize, const char * a, const char * b ) {
	hfStr::snPrintf( out, outSize, "%s/%s", a, b );
}

static bool WriteFile( const char * path, const char * text ) {
	FILE * f = fopen( path, "wb" );
	if ( f == NULL ) {
		return false;
	}
	size_t len = strlen( text );
	bool ok = fwrite( text, 1, len, f ) == len;
	fclose( f );
	return ok;
}

/*
====================
Header include dir from this file's own location
====================
*/
static bool FindHeaderDir( char * out, int outSize ) {
	const char * self = __FILE__;
	char selfDir[ 1024 ];
	hfStr::snPrintf( selfDir, sizeof( selfDir ), "%s", self );
	char * slash = selfDir + strlen( selfDir );
	while ( slash > selfDir && *slash != '/' ) {
		slash--;
	}
	if ( slash > selfDir ) {
		*slash = 0;
	} else {
		selfDir[ 0 ] = 0;
	}
	// Candidate include directories: the dir this file was compiled from,
	// then CWD-relative layouts for repo-root and build-dir runs.
	const char * candidates[ 5 ] = {
		selfDir[ 0 ] != 0 ? selfDir : NULL,
		"neo/script",
		"../neo/script",
		"../../neo/script",
		NULL
	};
	for ( int i = 0; i < 4; i++ ) {
		const char * c = candidates[ i ];
		if ( c == NULL || c[ 0 ] == 0 ) {
			continue;
		}
		char probe[ 1024 ];
		PathJoin( probe, sizeof( probe ), c, "HfScript.h" );
		if ( access( probe, R_OK ) == 0 ) {
			hfStr::snPrintf( out, outSize, "%s", c );
			return true;
		}
	}
	out[ 0 ] = 0;
	return false;
}

/*
====================
HfScript_JitCompileProgram
====================
*/
bool HfScript_JitCompileProgram( HfScriptProgram * program ) {
	if ( program == NULL || !program->valid ) {
		return false;
	}
	HfScript_JitRelease( program );

	const char * envCC = getenv( "HFSJIT_CC" );
	const char * envCFlags = getenv( "HFSJIT_CFLAGS" );
	const char * envTmp = getenv( "HFSJIT_TMPDIR" );
	const char * cc = envCC != NULL && envCC[ 0 ] != 0 ? envCC : "cc";
	const char * cflags = envCFlags != NULL && envCFlags[ 0 ] != 0 ? envCFlags : "-O2";
	const char * tmpDir = envTmp != NULL && envTmp[ 0 ] != 0 ? envTmp : "/tmp";

	char includeDir[ 1024 ];
	includeDir[ 0 ] = 0;
	FindHeaderDir( includeDir, sizeof( includeDir ) );

	static int seq = 0;
	seq++;
	char ccFile[ 1024 ];
	char soFile[ 1024 ];
	char base[ 256 ];
	hfStr::snPrintf( base, sizeof( base ), "hfjit_%d_%d", (int)getpid(), seq );
	PathJoin( ccFile, sizeof( ccFile ), tmpDir, base );
	strncat( ccFile, ".cc", sizeof( ccFile ) - strlen( ccFile ) - 1 );
	PathJoin( soFile, sizeof( soFile ), tmpDir, base );
	strncat( soFile, ".so", sizeof( soFile ) - strlen( soFile ) - 1 );

	hfJitPlan plan;
	hfJitWriter writer;
	if ( !HfScript_JitGenerateSource( writer, program, plan ) ) {
		return false;
	}
	if ( !WriteFile( ccFile, writer.text.c_str() ) ) {
		return false;
	}

	char cmd[ 4096 ];
	if ( includeDir[ 0 ] != 0 ) {
		hfStr::snPrintf( cmd, sizeof( cmd ), "%s %s -fPIC -shared -I%s -o %s %s 2>&1", cc, cflags, includeDir, soFile, ccFile );
	} else {
		hfStr::snPrintf( cmd, sizeof( cmd ), "%s %s -fPIC -shared -o %s %s 2>&1", cc, cflags, soFile, ccFile );
	}
	int rc = system( cmd );
	rc = WIFEXITED( rc ) ? WEXITSTATUS( rc ) : -1;
	bool compiled = rc == 0 && access( soFile, R_OK ) == 0;
	remove( ccFile );
	if ( !compiled ) {
		remove( soFile );
		program->error = "JIT compile failed (set HFSJIT_CC/HFSJIT_CFLAGS to override)";
		return false;
	}

	void * handle = dlopen( soFile, RTLD_NOW | RTLD_LOCAL );
	if ( handle == NULL ) {
		remove( soFile );
		program->error = dlerror() != NULL ? dlerror() : "JIT dlopen failed";
		return false;
	}
	remove( soFile );

	HfScriptJitEntry entry = (HfScriptJitEntry)dlsym( handle, "hf_jit_entry" );
	if ( entry == NULL ) {
		fprintf( stderr, "JITDBG dlsym fail\n" );
		dlclose( handle );
		program->error = "JIT entry symbol missing";
		return false;
	}

	program->jitHandle = handle;
	program->jitEntry = entry;
	program->error = "";
	return true;
}

/*
====================
HfScript_JitRelease
====================
*/
void HfScript_JitRelease( HfScriptProgram * program ) {
	if ( program == NULL ) {
		return;
	}
	if ( program->jitHandle != NULL ) {
		dlclose( program->jitHandle );
		program->jitHandle = NULL;
	}
	program->jitEntry = NULL;
}