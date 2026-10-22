/*
===========================================================================

Hellforge Power 1 Engine - hfscript engine integration

Engine dependency layer for hfscript:

	- lifecycle (HfScript_Init / HfScript_Shutdown)
	- .hfs loading from the engine file system (+ hfs / hfsReload commands)
	- host function bindings the scripts call (print, loc, format, cvar, ...)

The core language (lexer/compiler/VM) lives in HfScript_Lexer.cpp,
HfScript_Parser.cpp and HfScript_VM.cpp and has no engine dependencies.

===========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "../hflib/hfLangDict.h"
#include "HfScript.h"
#include "HfScript_Game.h"
#include "HfScript_PyBridge.h"

#include "../framework/hfCmdSystem.h"
#include "../framework/hfCVarSystem.h"
#include "../framework/hfFileSystem.h"
#include "../sys/hfsys_public.h"

static HfScriptProgram *	hfscriptProgram = NULL;
static HfScriptVM *			hfscriptVM = NULL;
static hfStr				hfscriptLastPath;

static void Cmd_HfScript_f( const hfCmdArgs & args );
static void Cmd_HfScriptReload_f( const hfCmdArgs & args );

/*
========================
Host: print
========================
*/
static void HfScript_Host_print( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	for ( int i = 0; i < argc; i++ ) {
		if ( i > 0 ) {
			hfLib::Printf( " " );
		}
		hfLib::Printf( "%s", vm->GetArg( i ).AsString( buffer ) );
	}
	hfLib::Printf( "\n" );
	*out = HfScriptValue::Nil();
}

/*
========================
Host: loc - localized string
========================
*/
static void HfScript_Host_loc( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	const char * key = vm->GetArg( 0 ).AsString( buffer );
	*out = HfScriptValue::Str( vm->NewString( hfLocalization::GetString( key ) ) );
	(void)argc;
}

/*
========================
Host: lang - current language
========================
*/
static void HfScript_Host_lang( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Str( vm->NewString( cvarSystem->GetCVarString( "sys_lang" ) ) );
	(void)argc;
}

/*
========================
Host: printf / format - "{}" placeholder substitution
========================
*/
static void HfScript_Host_format( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	const char * fmt = vm->GetArg( 0 ).AsString( buffer );
	hfStr result;
	int argIndex = 1;
	hfStr argText;
	for ( int i = 0; fmt[ i ] != 0; i++ ) {
		if ( fmt[ i ] == '{' ) {
			if ( fmt[ i + 1 ] == '{' ) {
				result += '{';
				i++;
				continue;
			}
			if ( fmt[ i + 1 ] == '}' ) {
				if ( argIndex < argc ) {
					result += vm->GetArg( argIndex ).AsString( argText );
				}
				argIndex++;
				i++;
				continue;
			}
		}
		if ( fmt[ i ] == '}' && fmt[ i + 1 ] == '}' ) {
			result += '}';
			i++;
			continue;
		}
		result += fmt[ i ];
	}
	*out = HfScriptValue::Str( vm->NewString( result.c_str() ) );
}

/*
========================
Host: len
========================
*/
static void HfScript_Host_len( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	const char * text = vm->GetArg( 0 ).AsString( buffer );
	*out = HfScriptValue::Int( strlen( text ) );
	(void)argc;
}

/*
========================
Host: str / int / float / bool conversions
========================
*/
static void HfScript_Host_str( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	*out = HfScriptValue::Str( vm->NewString( vm->GetArg( 0 ).AsString( buffer ) ) );
	(void)argc;
}

static void HfScript_Host_int( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( vm->GetArg( 0 ).AsInt() );
	(void)argc;
}

static void HfScript_Host_float( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Float( vm->GetArg( 0 ).AsFloat() );
	(void)argc;
}

static void HfScript_Host_bool( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Bool( vm->GetArg( 0 ).AsBool() );
	(void)argc;
}

/*
========================
Host: math helpers
========================
*/
static void HfScript_Host_abs( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	HfScriptValue a = vm->GetArg( 0 );
	if ( a.type == HST_INT ) {
		*out = HfScriptValue::Int( a.i < 0 ? -a.i : a.i );
	} else {
		float f = a.AsFloat();
		*out = HfScriptValue::Float( f < 0.0f ? -f : f );
	}
	(void)argc;
}

static void HfScript_Host_min( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = vm->GetArg( 0 );
	for ( int i = 1; i < argc; i++ ) {
		if ( vm->GetArg( i ).AsFloat() < out->AsFloat() ) {
			*out = vm->GetArg( i );
		}
	}
}

static void HfScript_Host_max( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = vm->GetArg( 0 );
	for ( int i = 1; i < argc; i++ ) {
		if ( vm->GetArg( i ).AsFloat() > out->AsFloat() ) {
			*out = vm->GetArg( i );
		}
	}
}

static void HfScript_Host_sqrt( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Float( (float)sqrt( vm->GetArg( 0 ).AsFloat() ) );
	(void)argc;
}

static void HfScript_Host_floor( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	float f = vm->GetArg( 0 ).AsFloat();
	*out = HfScriptValue::Float( (float)floor( f ) );
	(void)argc;
}

static void HfScript_Host_ceil( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	float f = vm->GetArg( 0 ).AsFloat();
	*out = HfScriptValue::Float( (float)ceil( f ) );
	(void)argc;
}

static void HfScript_Host_round( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	float f = vm->GetArg( 0 ).AsFloat();
	*out = HfScriptValue::Float( (float)( ( f >= 0.0f ) ? floor( f + 0.5f ) : ceil( f - 0.5f ) ) );
	(void)argc;
}

static void HfScript_Host_random( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	static int64 hfscriptRandomState = 88172645463325252LL;
	int a = 0;
	int b = 32768;
	if ( argc >= 1 ) {
		a = vm->GetArg( 0 ).AsInt();
	}
	if ( argc >= 2 ) {
		b = vm->GetArg( 1 ).AsInt();
	}
	if ( b < a ) {
		int swap = a;
		a = b;
		b = swap;
	}
	hfscriptRandomState = hfscriptRandomState * 2862933555777941757ULL + 3037000493ULL;
	double unit = (double)( (uint64)hfscriptRandomState >> 11 ) / (double)( 1LL << 53 );
	*out = HfScriptValue::Float( (float)( a + (int)( unit * ( b - a + 1 ) ) ) );
}

/*
========================
Host: getcvar / setcvar
========================
*/
static void HfScript_Host_getcvar( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	const char * name = vm->GetArg( 0 ).AsString( buffer );
	*out = HfScriptValue::Str( vm->NewString( cvarSystem->GetCVarString( name ) ) );
	(void)argc;
}

static void HfScript_Host_setcvar( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	hfStr nameText;
	const char * name = vm->GetArg( 0 ).AsString( nameText );
	const char * value = vm->GetArg( 1 ).AsString( buffer );
	cvarSystem->SetCVarString( name, value );
	*out = HfScriptValue::Nil();
	(void)argc;
}

/*
========================
Host: exec
========================
*/
static void HfScript_Host_exec( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	const char * command = vm->GetArg( 0 ).AsString( buffer );
	cmdSystem->ExecuteCommandText( command );
	*out = HfScriptValue::Nil();
	(void)argc;
}

/*
========================
Host: time
========================
*/
static void HfScript_Host_time( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Sys_Milliseconds() );
	(void)argc;
}

/*
========================
HfScript_Init
========================
*/
void HfScript_Init() {
	if ( hfscriptVM != NULL ) {
		return;
	}

	HfScript_RegisterHost( "print",		HfScript_Host_print );
	HfScript_RegisterHost( "loc",		HfScript_Host_loc );
	HfScript_RegisterHost( "lang",		HfScript_Host_lang );
	HfScript_RegisterHost( "format",	HfScript_Host_format );
	HfScript_RegisterHost( "len",		HfScript_Host_len );
	HfScript_RegisterHost( "str",		HfScript_Host_str );
	HfScript_RegisterHost( "int",		HfScript_Host_int );
	HfScript_RegisterHost( "float",		HfScript_Host_float );
	HfScript_RegisterHost( "bool",		HfScript_Host_bool );
	HfScript_RegisterHost( "abs",		HfScript_Host_abs );
	HfScript_RegisterHost( "min",		HfScript_Host_min );
	HfScript_RegisterHost( "max",		HfScript_Host_max );
	HfScript_RegisterHost( "sqrt",		HfScript_Host_sqrt );
	HfScript_RegisterHost( "floor",		HfScript_Host_floor );
	HfScript_RegisterHost( "ceil",		HfScript_Host_ceil );
	HfScript_RegisterHost( "round",		HfScript_Host_round );
	HfScript_RegisterHost( "random",	HfScript_Host_random );
	HfScript_RegisterHost( "getcvar",	HfScript_Host_getcvar );
	HfScript_RegisterHost( "setcvar",	HfScript_Host_setcvar );
	HfScript_RegisterHost( "exec",		HfScript_Host_exec );
	HfScript_RegisterHost( "time",		HfScript_Host_time );

	// sample game: "Hellforge Arena" script bridge (arena_* host functions)
	HfScript_RegisterArenaHosts();

#if defined( HF_PYTHON )
	// Optional CPython bridge (real Python eval/import/call from scripts).
	HfScript_PyBridge_Init();
	HfScript_PyBridge_RegisterHosts();
#endif

	hfscriptVM = new (TAG_SCRIPT) HfScriptVM();
	hfscriptProgram = new (TAG_SCRIPT) HfScriptProgram();

	cmdSystem->AddCommand( "hfs", Cmd_HfScript_f, CMD_FL_SYSTEM, "runs an hfscript file" );
	cmdSystem->AddCommand( "hfsReload", Cmd_HfScriptReload_f, CMD_FL_SYSTEM, "re-runs the last loaded hfscript file" );
}

/*
========================
HfScript_Shutdown
========================
*/
void HfScript_Shutdown() {
	if ( hfscriptProgram != NULL ) {
		delete hfscriptProgram;
		hfscriptProgram = NULL;
	}
	if ( hfscriptVM != NULL ) {
		delete hfscriptVM;
		hfscriptVM = NULL;
	}
#if defined( HF_PYTHON )
	HfScript_PyBridge_Shutdown();
#endif
	HfScript_UnregisterAllHosts();
}

/*
========================
HfScript_TryLoadObject
========================
Returns true if a precompiled .hfobj sidecar ("foo.hfobj" next to "foo.hfs"
or "foo.py") was found and run.  Falls back to source compilation otherwise.
*/
static bool HfScript_TryLoadObject( hfStr * outLastPath, const char * path ) {
	// Build the sidecar name: everything before the last '.' (if any) + ".hfobj".
	const char * dot = strrchr( path, '.' );
	if ( dot == NULL ) {
		return false;
	}
	hfStr objPath( path, 0, (int)( dot - path ) );
	objPath += ".hfobj";

	int sz = fileSystem->ReadFile( objPath.c_str(), NULL, NULL );
	if ( sz <= 0 ) {
		return false;	// no sidecar present
	}
	void * ob = NULL;
	if ( fileSystem->ReadFile( objPath.c_str(), &ob, NULL ) <= 0 || ob == NULL ) {
		return false;
	}
	bool ok = HfScript_LoadImage( hfscriptProgram, ob, sz );
	fileSystem->FreeFile( ob );
	if ( !ok ) {
		hfLib::Printf( "^3hfs^0: %s: %s\n", objPath.c_str(),
			hfscriptProgram->error.Length() > 0 ? hfscriptProgram->error.c_str() : "image load failed" );
		return false;
	}
	if ( !hfscriptVM->RunProgram( hfscriptProgram ) ) {
		hfLib::Printf( "^2hfs^0: %s(%d): %s\n", path, hfscriptProgram->errorLine, hfscriptProgram->error.c_str() );
		return false;
	}
	*outLastPath = path;
	return true;
}

/*
========================
HfScript_LoadFile
========================
*/
bool HfScript_LoadFile( const char * path ) {
	if ( hfscriptVM == NULL ) {
		HfScript_Init();
	}

	// Prefer a precompiled .hfobj sidecar when present.
	if ( HfScript_TryLoadObject( &hfscriptLastPath, path ) ) {
		return true;
	}

	void * buffer = NULL;
	if ( fileSystem->ReadFile( path, &buffer, NULL ) <= 0 || buffer == NULL ) {
		hfLib::Printf( "^3hfs^0: cannot read script ^1%s^0\n", path );
		return false;
	}

	static char sourceName[ 1024 ];
	hfStr::snPrintf( sourceName, sizeof( sourceName ), "%s", path );
	// A .py source runs through the same compiler as .hfs; the tag only affects
	// diagnostics and error attribution.
	if ( !hfscriptVM->RunText( hfscriptProgram, (const char *)buffer, sourceName ) ) {
		hfLib::Printf( "^2hfs^0: %s(%d): %s\n", path, hfscriptProgram->errorLine, hfscriptProgram->error.c_str() );
		fileSystem->FreeFile( buffer );
		return false;
	}

	fileSystem->FreeFile( buffer );
	hfscriptLastPath = path;
	return true;
}

/*
========================
HfScript_Reload
========================
*/
bool HfScript_Reload() {
	if ( hfscriptLastPath.Length() == 0 ) {
		hfLib::Printf( "^3hfs^0: nothing to reload\n" );
		return false;
	}
	return HfScript_LoadFile( hfscriptLastPath.c_str() );
}

/*
========================
Console commands
========================
*/
static void Cmd_HfScript_f( const hfCmdArgs & args ) {
	if ( args.Argc() >= 2 ) {
		HfScript_LoadFile( args.Argv( 1 ) );
		return;
	}
	hfLib::Printf( "usage: hfs <script.hfs>\n" );
}

static void Cmd_HfScriptReload_f( const hfCmdArgs & args ) {
	(void)args;
	HfScript_Reload();
}