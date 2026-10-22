/*
===========================================================================

Hellforge Power 1 Engine - hfscript virtual machine

The VM is a small stack machine executing HfScriptProgram bytecode.  It is
kept free of engine dependencies (apart from hflib memory/string helpers)
so it can be driven both from the game and from standalone tools.

===========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "HfScript.h"

/*
========================
HfScriptValue
========================
*/
HfScriptValue::HfScriptValue() : type( HST_NIL ) {
	b = false;
}

HfScriptValue HfScriptValue::Nil() {
	HfScriptValue v;
	v.type = HST_NIL;
	v.b = false;
	return v;
}

HfScriptValue HfScriptValue::Bool( bool value ) {
	HfScriptValue v;
	v.type = HST_BOOL;
	v.b = value;
	return v;
}

HfScriptValue HfScriptValue::Int( int64 value ) {
	HfScriptValue v;
	v.type = HST_INT;
	v.i = value;
	return v;
}

HfScriptValue HfScriptValue::Float( float value ) {
	HfScriptValue v;
	v.type = HST_FLOAT;
	v.f = value;
	return v;
}

HfScriptValue HfScriptValue::Str( const char * value ) {
	HfScriptValue v;
	v.type = HST_STRING;
	v.s = value;
	return v;
}

bool HfScriptValue::AsBool() const {
	switch ( type ) {
		case HST_BOOL:		return b;
		case HST_INT:		return i != 0;
		case HST_FLOAT:		return f != 0.0f;
		case HST_STRING:	return s != NULL && s[ 0 ] != 0;
		default:			return false;
	}
}

int64 HfScriptValue::AsInt() const {
	switch ( type ) {
		case HST_BOOL:		return b ? 1 : 0;
		case HST_INT:		return i;
		case HST_FLOAT:		return (int64)f;
		case HST_STRING: {
			if ( s == NULL || s[ 0 ] == 0 ) {
				return 0;
			}
			return (int64)atof( s );
		}
		default:			return 0;
	}
}

float HfScriptValue::AsFloat() const {
	switch ( type ) {
		case HST_BOOL:		return b ? 1.0f : 0.0f;
		case HST_INT:		return (float)i;
		case HST_FLOAT:		return f;
		case HST_STRING: {
			if ( s == NULL || s[ 0 ] == 0 ) {
				return 0.0f;
			}
			return (float)atof( s );
		}
		default:			return 0.0f;
	}
}

const char * HfScriptValue::AsString( hfStr & buffer ) const {
	switch ( type ) {
		case HST_BOOL:		buffer = b ? "true" : "false"; return buffer.c_str();
		case HST_INT: {
			char tmp[ 32 ];
			hfStr::snPrintf( tmp, sizeof( tmp ), "%lld", (long long)i );
			buffer = tmp;
			return buffer.c_str();
		}
		case HST_FLOAT: {
			buffer.Clear();
			buffer += f;
			return buffer.c_str();
		}
		case HST_STRING:	return s != NULL ? s : "";
		default:			return "";
	}
}

int HfScriptValue::Describe( char * out, int outSize ) const {
	hfStr buffer;
	const char * text = AsString( buffer );
	switch ( type ) {
		case HST_INT:		return hfStr::snPrintf( out, outSize, "int(%s)", text );
		case HST_FLOAT:		return hfStr::snPrintf( out, outSize, "float(%s)", text );
		case HST_STRING:	return hfStr::snPrintf( out, outSize, "string(\"%s\")", text );
		case HST_BOOL:		return hfStr::snPrintf( out, outSize, "bool(%s)", text );
		default:			return hfStr::snPrintf( out, outSize, "nil" );
	}
}

/*
========================
HfScriptProgram
========================
*/
HfScriptProgram::HfScriptProgram() {
	ops = NULL;				numOps = 0;				opAlloc = 0;
	constants = NULL;		numConstants = 0;		constAlloc = 0;
	functions = NULL;		numFunctions = 0;		funcAlloc = 0;
	globals = NULL;			numGlobals = 0;			globalAlloc = 0;
	globalValues = NULL;	globalValuesSize = 0;
	stringPool = NULL;		poolSize = 0;			poolUsed = 0;
	sourceName = NULL;
	valid = false;
	entryFunction = -1;
	entryOp = -1;
	jitHandle = NULL;
	jitEntry = NULL;
	errorLine = 0;
}

HfScriptProgram::~HfScriptProgram() {
	Reset();
}

void HfScriptProgram::Reset() {
	HfScript_JitRelease( this );
	if ( ops ) { Mem_Free( ops ); ops = NULL; }
	if ( constants ) { Mem_Free( constants ); constants = NULL; }
	if ( functions ) { Mem_Free( functions ); functions = NULL; }
	if ( globals ) { Mem_Free( globals ); globals = NULL; }
	if ( globalValues ) { Mem_Free( globalValues ); globalValues = NULL; }
	if ( stringPool ) { Mem_Free( stringPool ); stringPool = NULL; }
	numOps = 0; opAlloc = 0;
	numConstants = 0; constAlloc = 0;
	numFunctions = 0; funcAlloc = 0;
	numGlobals = 0; globalAlloc = 0;
	globalValuesSize = 0;
	poolSize = 0; poolUsed = 0;
	valid = false;
	entryFunction = -1;
	entryOp = -1;
	error = "";
	errorLine = 0;
}

HfScriptOp * HfScriptProgram::AllocOps( int count ) {
	if ( count <= opAlloc ) {
		return ops;
	}
	HfScriptOp * newOps = (HfScriptOp *)Mem_Alloc( sizeof( HfScriptOp ) * count, TAG_SCRIPT );
	if ( ops ) {
		memcpy( newOps, ops, sizeof( HfScriptOp ) * opAlloc );
		Mem_Free( ops );
	}
	ops = newOps;
	opAlloc = count;
	return ops;
}

int HfScriptProgram::AddConstant( const HfScriptValue & v ) {
	// numeric and nil constants are deduplicated
	for ( int i = 0; i < numConstants; i++ ) {
		const HfScriptValue & c = constants[ i ];
		if ( c.type == v.type ) {
			if ( c.type == HST_INT && c.i == v.i ) {
				return i;
			}
			if ( c.type == HST_FLOAT && c.f == v.f ) {
				return i;
			}
			if ( c.type == HST_STRING && v.s != NULL && c.s == v.s ) {
				return i;
			}
		}
	}
	if ( numConstants >= constAlloc ) {
		int newAlloc = constAlloc > 0 ? constAlloc * 2 : 64;
		HfScriptValue * newC = (HfScriptValue *)Mem_Alloc( sizeof( HfScriptValue ) * newAlloc, TAG_SCRIPT );
		if ( constants ) {
			memcpy( newC, constants, sizeof( HfScriptValue ) * numConstants );
			Mem_Free( constants );
		}
		constants = newC;
		constAlloc = newAlloc;
	}
	constants[ numConstants ] = v;
	return numConstants++;
}

int HfScriptProgram::AddFunction( const char * name, int numParams, int totalLocals ) {
	if ( numFunctions >= funcAlloc ) {
		int newAlloc = funcAlloc > 0 ? funcAlloc * 2 : 16;
		HfScriptFunction * newF = (HfScriptFunction *)Mem_ClearedAlloc( sizeof( HfScriptFunction ) * newAlloc, TAG_SCRIPT );
		if ( functions ) {
			for ( int i = 0; i < numFunctions; i++ ) {
				newF[ i ] = functions[ i ];		// deep-copies hfStr name
			}
			Mem_Free( functions );
		}
		functions = newF;
		funcAlloc = newAlloc;
	}
	HfScriptFunction & f = functions[ numFunctions ];
	f.name = name;
	f.numParams = numParams;
	f.opStart = 0;
	f.opEnd = 0;
	f.totalLocals = totalLocals;
	f.line = 0;
	return numFunctions++;
}

int HfScriptProgram::AddGlobal( const char * name ) {
	if ( numGlobals >= globalAlloc ) {
		int newAlloc = globalAlloc > 0 ? globalAlloc * 2 : 16;
		HfScriptGlobal * newG = (HfScriptGlobal *)Mem_ClearedAlloc( sizeof( HfScriptGlobal ) * newAlloc, TAG_SCRIPT );
		if ( globals ) {
			for ( int i = 0; i < numGlobals; i++ ) {
				newG[ i ] = globals[ i ];		// deep-copies hfStr name
			}
			Mem_Free( globals );
		}
		globals = newG;
		globalAlloc = newAlloc;
	}
	globals[ numGlobals ].name = name;
	globals[ numGlobals ].index = numGlobals;

	// grow the runtime storage for globals
	HfScriptValue * newGV = (HfScriptValue *)Mem_Alloc( sizeof( HfScriptValue ) * ( numGlobals + 1 ), TAG_SCRIPT );
	if ( globalValues ) {
		memcpy( newGV, globalValues, sizeof( HfScriptValue ) * numGlobals );
		Mem_Free( globalValues );
	}
	globalValues = newGV;
	globalValuesSize = numGlobals + 1;
	globalValues[ numGlobals ] = HfScriptValue::Nil();

	return numGlobals++;
}

int HfScriptProgram::PoolString( const char * text ) {
	int len = text != NULL ? (int)strlen( text ) : 0;
	// The pool is allocated once and never moved: string references all over
	// the VM (stack values, constants) are raw pointers into it.  Growing it
	// in place is impossible with the block allocator, so reserve a generous
	// fixed budget up front.
	if ( poolSize == 0 ) {
		poolSize = 1 << 20;		// 1 MB
		stringPool = (char *)Mem_ClearedAlloc( poolSize, TAG_SCRIPT );
	}
	if ( len + poolUsed + 1 > poolSize ) {
		assert( false && "hfscript string pool exhausted" );
		poolUsed = 0;
	}
	int offset = poolUsed;
	memcpy( stringPool + poolUsed, text, len );
	poolUsed += len;
	stringPool[ poolUsed ] = 0;
	poolUsed++;
	return offset;
}

/*
========================
Host function registry
========================
*/
static hfList<HfScriptHostDef> hfscriptHosts;

void HfScript_RegisterHost( const char * name, HfScriptHostFn fn ) {
	for ( int i = 0; i < hfscriptHosts.Num(); i++ ) {
		if ( hfStr::Cmp( hfscriptHosts[ i ].name, name ) == 0 ) {
			hfscriptHosts[ i ].fn = fn;
			return;
		}
	}
	HfScriptHostDef def;
	def.name = name;
	def.fn = fn;
	hfscriptHosts.Append( def );
}

void HfScript_UnregisterAllHosts() {
	hfscriptHosts.Clear();
}

int HfScript_FindHostFunction( const char * name ) {
	for ( int i = 0; i < hfscriptHosts.Num(); i++ ) {
		if ( hfStr::Cmp( hfscriptHosts[ i ].name, name ) == 0 ) {
			return i;
		}
	}
	return -1;
}

/*
========================
HfScript_Host_default
Default placeholder host used by standalone tools.
========================
*/
void HfScript_Host_default( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	(void)vm;
	(void)argc;
	*out = HfScriptValue::Nil();
}

/*
========================
HfScriptVM
========================
*/
HfScriptVM::HfScriptVM() {
	stackSize = 64;
	stack = (HfScriptValue *)Mem_ClearedAlloc( sizeof( HfScriptValue ) * stackSize, TAG_SCRIPT );
	stackTop = 0;
	frameAlloc = 16;
	frames = (Frame *)Mem_ClearedAlloc( sizeof( Frame ) * frameAlloc, TAG_SCRIPT );
	numFrames = 0;
	arenaData = NULL;
	arenaUsed = 0;
	arenaSize = 0;
	program = NULL;
	callResult = HfScriptValue::Nil();
	hostArgc = 0;
	hostArgBase = 0;
	hostArgOverride = NULL;
	hostArgOverrideCount = 0;
	lastErrorLine = 0;
}

HfScriptVM::~HfScriptVM() {
	if ( stack ) { Mem_Free( stack ); stack = NULL; }
	if ( frames ) { Mem_Free( frames ); frames = NULL; }
	if ( arenaData ) { Mem_Free( arenaData ); arenaData = NULL; }
}

void HfScriptVM::GrowStack() {
	int newSize = stackSize * 2;
	HfScriptValue * newStack = (HfScriptValue *)Mem_Alloc( sizeof( HfScriptValue ) * newSize, TAG_SCRIPT );
	memcpy( newStack, stack, sizeof( HfScriptValue ) * stackSize );
	Mem_Free( stack );
	stack = newStack;
	stackSize = newSize;
}

bool HfScriptVM::RunText( HfScriptProgram * prog, const char * source, const char * sourceName ) {
	if ( !HfScript_Compile( prog, source, sourceName ) ) {
		lastError = prog->error;
		lastErrorLine = prog->errorLine;
		return false;
	}
	return RunProgram( prog );
}

bool HfScriptVM::RunProgram( HfScriptProgram * prog ) {
	if ( prog == NULL || !prog->valid ) {
		lastError = "program is not valid";
		return false;
	}
	program = prog;
	stackTop = 0;
	numFrames = 0;
	arenaUsed = 0;
	callResult = HfScriptValue::Nil();
	hostArgOverride = NULL;
	hostArgOverrideCount = 0;
	lastError = "";
	lastErrorLine = 0;

	// Merged interpreter/JIT execution: a native module replaces the
	// interpreter for the whole program when one is attached to it.
	if ( program->jitEntry != NULL ) {
		program->jitEntry( this, program, &callResult );
		if ( lastError.Length() > 0 ) {
			return false;
		}
		return true;
	}
	return ExecProgram();
}

void HfScriptVM::Error( const char * fmt, ... ) {
	if ( lastError.Length() > 0 ) {
		return;
	}
	char buffer[ 1024 ];
	va_list argList;
	va_start( argList, fmt );
	hfStr::vsnPrintf( buffer, sizeof( buffer ), fmt, argList );
	va_end( argList );
	lastError = buffer;
}

HfScriptValue HfScriptVM::GetArg( int index ) const {
	if ( hostArgOverride != NULL ) {
		if ( index < 0 || index >= hostArgOverrideCount ) {
			return HfScriptValue::Nil();
		}
		return hostArgOverride[ index ];
	}
	if ( index < 0 || index >= hostArgc ) {
		return HfScriptValue::Nil();
	}
	return stack[ hostArgBase + index ];
}

int HfScriptVM::GetArgc() const {
	return hostArgc;
}

const char * HfScriptVM::NewString( const char * text ) {
	if ( program == NULL ) {
		return text;
	}
	return program->stringPool + program->PoolString( text );
}

/*
========================
JIT execution bridge
========================
Native code generated from bytecode mirrors the interpreter's operand
stack and frames.  It calls back here for anything that needs the VM's own
storage, the host registry, pooling, or error reporting, so the generated
code never touches private state directly.
*/
void HfScriptVM::ExecReset() {
	RunProgram( program );
}

HfScriptValue * HfScriptVM::ExecStackBase() {
	return stack;
}

int HfScriptVM::ExecStackCapacity() const {
	return stackSize;
}

void HfScriptVM::ExecEnsureStack( int needed ) {
	while ( stackSize < needed ) {
		GrowStack();
	}
}

HfScriptValue * HfScriptVM::ExecNewLocals( int count ) {
	int n = count > 0 ? count : 1;
	int newUsed = arenaUsed + n;
	if ( newUsed > arenaSize ) {
		int newSize = arenaSize > 0 ? arenaSize : 64;
		while ( newSize < newUsed ) {
			newSize *= 2;
		}
		HfScriptValue * newData = (HfScriptValue *)Mem_Alloc( sizeof( HfScriptValue ) * newSize, TAG_SCRIPT );
		if ( arenaData ) {
			memcpy( newData, arenaData, sizeof( HfScriptValue ) * arenaUsed );
			Mem_Free( arenaData );
		}
		arenaData = newData;
		arenaSize = newSize;
	}
	HfScriptValue * locals = arenaData + arenaUsed;
	arenaUsed = newUsed;
	for ( int i = 0; i < n; i++ ) {
		locals[ i ] = HfScriptValue::Nil();
	}
	return locals;
}

void HfScriptVM::ExecFreeLocals( HfScriptValue * locals ) {
	if ( locals == NULL ) {
		return;
	}
	// Frames close strictly LIFO, so release everything above this one.
	int used = (int)( locals - arenaData );
	if ( used >= 0 && used < arenaUsed ) {
		arenaUsed = used;
	}
}

void HfScriptVM::ExecCallHost( int hostIndex, int argc, const HfScriptValue * args, HfScriptValue * out ) {
	if ( hostIndex < 0 || hostIndex >= hfscriptHosts.Num() ) {
		ExecError( 0, "bad host function index in JIT host call" );
		*out = HfScriptValue::Nil();
		return;
	}
	HfScriptValue * prevOverride = hostArgOverride;
	int prevOverrideCount = hostArgOverrideCount;
	hostArgOverride = (HfScriptValue *)args;
	hostArgOverrideCount = argc;
	hostArgc = argc;
	hostArgBase = 0;
	HfScriptValue result = HfScriptValue::Nil();
	hfscriptHosts[ hostIndex ].fn( this, argc, &result );
	hostArgOverride = prevOverride;
	hostArgOverrideCount = prevOverrideCount;
	*out = result;
}

void HfScriptVM::ExecError( int line, const char * msg ) {
	lastError = msg != NULL ? msg : "jit error";
	lastErrorLine = line;
}

const char * HfScriptVM::ExecConcatStrings( const char * a, const char * b ) {
	if ( program == NULL ) {
		return "";
	}
	hfStr both = a != NULL ? a : "";
	both += b != NULL ? b : "";
	return program->stringPool + program->PoolString( both.c_str() );
}

const char * HfScriptVM::ExecStringify( const HfScriptValue & v ) {
	if ( program == NULL ) {
		return "";
	}
	hfStr buffer;
	const char * text = v.AsString( buffer );
	return program->stringPool + program->PoolString( text != NULL ? text : "" );
}

/*
========================
Execution
========================
*/
bool HfScriptVM::ExecProgram() {
	if ( program == NULL || !program->valid ) {
		lastError = "program is not valid";
		return false;
	}

	int pc = 0;
	int debugLine = 0;
	bool running = true;
	int opBudget = 50000000;	// runaway-script guard

	// Run the top-level code from its first statement.  The compiler emits
	// top-level setup (global assignments, etc.) before the trailing call
	// to main(), so the prologue runs before the entry function.
	// Cache hot tables in locals: the program's arrays are immutable during
	// execution (no emit/realloc happens while running).
	const HfScriptOp * ops = program->ops;
	const HfScriptValue * consts = program->constants;
	HfScriptValue * gvals = program->globalValues;

	pc = program->entryOp;

	while ( running ) {
		if ( --opBudget <= 0 ) {
			pc = -1;
			Error( "script exceeded the op budget (possible infinite loop)" );
			break;
		}
		if ( pc < 0 || pc >= program->numOps ) {
			Error( "program counter out of range (%d)", pc );
			break;
		}
		const HfScriptOp & op = ops[ pc ];
		switch ( op.op ) {
			case HOP_NOP:
				pc++;
				break;
			case HOP_DEBUG_LINE:
				debugLine = op.a;
				pc++;
				break;
			case HOP_CONST:
				Push( consts[ op.a ] );
				pc++;
				break;
			case HOP_NIL:
				Push( HfScriptValue::Nil() );
				pc++;
				break;
			case HOP_TRUE:
				Push( HfScriptValue::Bool( true ) );
				pc++;
				break;
			case HOP_FALSE:
				Push( HfScriptValue::Bool( false ) );
				pc++;
				break;
			case HOP_POP:
				Pop();
				pc++;
				break;
			case HOP_LOCAL: {
				Frame & fr = frames[ numFrames - 1 ];
				if ( op.a < 0 || op.a >= fr.localCount ) {
					Error( "local index out of range (%d)", op.a );
					pc = -1;
					break;
				}
				Push( fr.locals[ op.a ] );
				pc++;
				break;
			}
			case HOP_LOCAL_SET: {
				Frame & fr = frames[ numFrames - 1 ];
				if ( op.a < 0 || op.a >= fr.localCount ) {
					Error( "local index out of range (%d)", op.a );
					pc = -1;
					break;
				}
				HfScriptValue v = Pop();
				fr.locals[ op.a ] = v;
				Push( v );
				pc++;
				break;
			}
			case HOP_GLOBAL:
				if ( op.a < 0 || op.a >= program->numGlobals ) {
					Error( "global index out of range (%d)", op.a );
					pc = -1;
					break;
				}
				Push( gvals[ op.a ] );
				pc++;
				break;
			case HOP_GLOBAL_SET: {
				if ( op.a < 0 || op.a >= program->numGlobals ) {
					Error( "global index out of range (%d)", op.a );
					pc = -1;
					break;
				}
				HfScriptValue v = Pop();
				gvals[ op.a ] = v;
				Push( v );
				pc++;
				break;
			}
			case HOP_ADD: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				if ( a.type == HST_STRING || b.type == HST_STRING ) {
					// string concatenation
					hfStr sa;
					hfStr sb;
					const char * ca = a.AsString( sa );
					const char * cb = b.AsString( sb );
					hfStr both;
					both = ca;
					both += cb;
					Push( HfScriptValue::Str( NewString( both.c_str() ) ) );
				} else if ( a.IsNumeric() && b.IsNumeric() ) {
					if ( a.type == HST_INT && b.type == HST_INT ) {
						Push( HfScriptValue::Int( a.i + b.i ) );
					} else {
						Push( HfScriptValue::Float( a.AsFloat() + b.AsFloat() ) );
					}
				} else {
					Push( HfScriptValue::Int( a.AsInt() + b.AsInt() ) );
				}
				pc++;
				break;
			}
			case HOP_SUB: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				if ( a.IsNumeric() && b.IsNumeric() ) {
					if ( a.type == HST_INT && b.type == HST_INT ) {
						Push( HfScriptValue::Int( a.i - b.i ) );
					} else {
						Push( HfScriptValue::Float( a.AsFloat() - b.AsFloat() ) );
					}
				} else {
					Push( HfScriptValue::Int( a.AsInt() - b.AsInt() ) );
				}
				pc++;
				break;
			}
			case HOP_MUL: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				if ( a.IsNumeric() && b.IsNumeric() ) {
					if ( a.type == HST_INT && b.type == HST_INT ) {
						Push( HfScriptValue::Int( a.i * b.i ) );
					} else {
						Push( HfScriptValue::Float( a.AsFloat() * b.AsFloat() ) );
					}
				} else {
					Push( HfScriptValue::Int( a.AsInt() * b.AsInt() ) );
				}
				pc++;
				break;
			}
			case HOP_DIV: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				float divisor = b.AsFloat();
				if ( divisor == 0.0f ) {
					Error( "divide by zero" );
					pc = -1;
					break;
				}
				Push( HfScriptValue::Float( a.AsFloat() / divisor ) );
				pc++;
				break;
			}
			case HOP_MOD: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				if ( a.type == HST_INT && b.type == HST_INT ) {
					if ( b.i == 0 ) {
						Error( "modulo by zero" );
						pc = -1;
						break;
					}
					Push( HfScriptValue::Int( a.i % b.i ) );
				} else {
					float divisor = b.AsFloat();
					if ( divisor == 0.0f ) {
						Error( "modulo by zero" );
						pc = -1;
						break;
					}
					Push( HfScriptValue::Float( (float)fmod( a.AsFloat(), divisor ) ) );
				}
				pc++;
				break;
			}
			case HOP_NEG: {
				HfScriptValue a = Pop();
				if ( a.type == HST_INT ) {
					Push( HfScriptValue::Int( -a.i ) );
				} else if ( a.IsNumeric() ) {
					Push( HfScriptValue::Float( -a.AsFloat() ) );
				} else {
					Push( HfScriptValue::Int( -a.AsInt() ) );
				}
				pc++;
				break;
			}
			case HOP_NOT:
				Push( HfScriptValue::Bool( !Pop().AsBool() ) );
				pc++;
				break;
			case HOP_AND: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				Push( HfScriptValue::Bool( a.AsBool() && b.AsBool() ) );
				pc++;
				break;
			}
			case HOP_OR: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				Push( HfScriptValue::Bool( a.AsBool() || b.AsBool() ) );
				pc++;
				break;
			}
			case HOP_CMP: {
				HfScriptValue b = Pop();
				HfScriptValue a = Pop();
				Push( HfScriptValue::Bool( CompareValues( a, b, (hfScriptCmp_t)op.b ) ) );
				pc++;
				break;
			}
			case HOP_JUMP:
				pc = op.a;
				break;
			case HOP_JUMP_TRUE: {
				if ( Pop().AsBool() ) {
					pc = op.a;
				} else {
					pc++;
				}
				break;
			}
			case HOP_JUMP_FALSE: {
				if ( !Pop().AsBool() ) {
					pc = op.a;
				} else {
					pc++;
				}
				break;
			}
			case HOP_CALL: {
				if ( op.a < 0 || op.a >= program->numFunctions ) {
					Error( "bad function index (%d)", op.a );
					pc = -1;
					break;
				}
				HfScriptFunction & fn = program->functions[ op.a ];
				int argc = op.b;
				if ( argc != fn.numParams ) {
					Error( "function \"%s\" expects %d arguments, got %d", fn.name.c_str(), fn.numParams, argc );
					pc = -1;
					break;
				}
				if ( numFrames >= frameAlloc ) {
					int newAlloc = frameAlloc * 2;
					Frame * newFrames = (Frame *)Mem_Alloc( sizeof( Frame ) * newAlloc, TAG_SCRIPT );
					memcpy( newFrames, frames, sizeof( Frame ) * numFrames );
					Mem_Free( frames );
					frames = newFrames;
					frameAlloc = newAlloc;
				}
				Frame frame;
				frame.stackBase = stackTop - argc;
				frame.retOp = pc + 1;
				frame.localCount = fn.totalLocals;
				frame.locals = ExecNewLocals( frame.localCount );		// nil-filled from the arena
				for ( int i = 0; i < argc; i++ ) {
					frame.locals[ i ] = stack[ frame.stackBase + i ];
				}
				frames[ numFrames++ ] = frame;
				pc = fn.opStart;
				break;
			}
			case HOP_CALL_HOST: {
				if ( op.a < 0 || op.a >= hfscriptHosts.Num() ) {
					Error( "bad host function index (%d)", op.a );
					pc = -1;
					break;
				}
				int argc = op.b;
				int argBase = stackTop - argc;
				hostArgc = argc;
				hostArgBase = argBase;
				callResult = HfScriptValue::Nil();
				hfscriptHosts[ op.a ].fn( this, argc, &callResult );
				// pop the arguments, push the result
				stackTop = argBase;
				Push( callResult );
				pc++;
				break;
			}
			case HOP_RETURN: {
				HfScriptValue result = HfScriptValue::Nil();
				if ( op.b != 0 ) {
					result = Pop();
				} else {
					result = HfScriptValue::Nil();
				}
				if ( numFrames <= 0 ) {
					Error( "return with no active frame" );
					pc = -1;
					break;
				}
				Frame frame = frames[ numFrames - 1 ];
				numFrames--;
				ExecFreeLocals( frame.locals );			// LIFO arena release
				stackTop = frame.stackBase;
				if ( frame.retOp == 0 ) {
					// returning from the entry call: done
					Push( result );
					running = false;
					break;
				}
				pc = frame.retOp;
				Push( result );
				break;
			}
			case HOP_END:
				running = false;
				break;

			default:
				Error( "unknown opcode %d", op.op );
				pc = -1;
				break;
		}
	}

	// unwind anything left on the call stack (error path)
	for ( int i = numFrames - 1; i >= 0; i-- ) {
		if ( frames[ i ].locals ) {
			Mem_Free( frames[ i ].locals );
		}
	}
	numFrames = 0;

	if ( lastError.Length() > 0 ) {
		lastErrorLine = debugLine;
		return false;
	}
	return true;
}

/*
========================
CompareValues
========================
*/
bool HfScriptVM::CompareValues( const HfScriptValue & a, const HfScriptValue & b, hfScriptCmp_t cmp ) const {
	if ( a.IsNumeric() && b.IsNumeric() ) {
		float av = a.AsFloat();
		float bv = b.AsFloat();
		switch ( cmp ) {
			case HSC_EQ:	return av == bv;
			case HSC_NE:	return av != bv;
			case HSC_LT:	return av < bv;
			case HSC_LE:	return av <= bv;
			case HSC_GT:	return av > bv;
			case HSC_GE:	return av >= bv;
		}
		return false;
	}
	if ( a.type == HST_STRING && b.type == HST_STRING ) {
		int c = hfStr::Cmp( a.s != NULL ? a.s : "", b.s != NULL ? b.s : "" );
		switch ( cmp ) {
			case HSC_EQ:	return c == 0;
			case HSC_NE:	return c != 0;
			case HSC_LT:	return c < 0;
			case HSC_LE:	return c <= 0;
			case HSC_GT:	return c > 0;
			case HSC_GE:	return c >= 0;
		}
		return false;
	}
	// non-numeric, non-string values only support == and !=
	switch ( cmp ) {
		case HSC_EQ:	return a.type == b.type && a.i == b.i && a.s == b.s;
		case HSC_NE:	return !( a.type == b.type && a.i == b.i && a.s == b.s );
		default:		return false;
	}
}