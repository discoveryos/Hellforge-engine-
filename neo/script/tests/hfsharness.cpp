/*
===========================================================================

Standalone hfscript test harness.

Links only the core language (Lexer + Parser + VM) against libhflib, so it
runs on systems without a display.  Exercises control flow, functions,
recursion, variable scoping, operator precedence, string handling, the
host function callback path, the AOT image loader, and the native-code JIT
(the JIT dlopens a subsystem-compiled module, so link with --export-dynamic
to export this executable's HfScriptVM/HfScriptValue symbols for it; note
HfScript_JIT falls back to "cc" unless HFSJIT_CC is set, and the generated
module finds HfScript.h via FindHeaderDir()).

Build (from the build/ dir):
	/usr/bin/c++ -O0 -g -std=gnu++17 -Wl,--export-dynamic \
		-I../neo -I../neo/hflib \
		../neo/script/tests/hfsharness.cpp \
		../neo/script/HfScript_Lexer.cpp ../neo/script/HfScript_Parser.cpp \
		../neo/script/HfScript_VM.cpp ../neo/script/HfScript_AOT.cpp \
		../neo/script/HfScript_JIT.cpp ../neo/script/HfScript_Ruby.cpp \
		libhflib.a -lz -ldl -lpthread -lm -o /tmp/opencode/hfstest

===========================================================================
*/
#include "../../hflib/hfprecompiled.h"
#include "../HfScript.h"
#include "../HfScript_Ruby.h"
#include "../HfScript_PyBridge.h"

// Engine globals referenced from libhflib that are unused by the harness;
// provide stubs so the static library links.
#include "../../framework/hfCVarSystem.h"
#include "../../framework/hfCommon.h"
#include "../../framework/hfFileSystem.h"
#include "../../sys/hfsys_public.h"
#include <sys/time.h>

hfCVar *			hfCVar::staticVars = NULL;
hfFileSystem *		fileSystem = NULL;
hfCommon *			common = NULL;
hfCVarSystem *			cvarSystem = NULL;

static int g_failures = 0;

/*
=====================
HfTestCommon

hflib reports diagnostics through the global hfCommon, and plenty of it is
reachable from the tests: hfStr::snPrintf warns when a result does not fit, and
Format fatal-errors when it does. Leaving the global NULL turns those paths
into a segfault instead of a message, so the harness installs a sink that
prints them. Only the pure virtuals are implemented; nothing in the tests
drives the real subsystems.
=====================
*/
class HfTestCommon : public hfCommon {
public:
	virtual void				Init( int argc, const char * const * argv, const char *cmdline ) override {}
	virtual void				Shutdown() override {}
	virtual bool				IsShuttingDown() const override { return false; }
	virtual	void				CreateMainMenu() override {}
	virtual void				Quit() override {}
	virtual bool				IsInitialized() const override { return true; }
	virtual void				Frame() override {}
	virtual void				UpdateScreen( bool captureToImage ) override {}
	virtual void				UpdateLevelLoadPacifier() override {}
	virtual void				StartupVariable( const char *match ) override {}
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) override {}
	virtual void				EndRedirect() override {}
	virtual void				SetRefreshOnPrint( bool set ) override {}
	virtual void				Printf( VERIFY_FORMAT_STRING const char *fmt, ... ) override {
									va_list arg;
									va_start( arg, fmt );
									vfprintf( stdout, fmt, arg );
									va_end( arg );
									fflush( stdout );
								}
	virtual void				VPrintf( const char *fmt, va_list arg ) override { vfprintf( stdout, fmt, arg ); fflush( stdout ); }
	virtual void				DPrintf( VERIFY_FORMAT_STRING const char *fmt, ... ) override {}
	virtual void				Warning( VERIFY_FORMAT_STRING const char *fmt, ... ) override {
									va_list arg;
									va_start( arg, fmt );
									printf( "  [warn] " );
									vfprintf( stdout, fmt, arg );
									va_end( arg );
									fflush( stdout );
								}
	virtual void				DWarning( VERIFY_FORMAT_STRING const char *fmt, ... ) override {}
	virtual void				PrintWarnings() override {}
	virtual void				ClearWarnings( const char *reason ) override {}
	virtual void				Error( VERIFY_FORMAT_STRING const char *fmt, ... ) override {
									g_failures++;
									va_list arg;
									va_start( arg, fmt );
									printf( "  [error] " );
									vfprintf( stdout, fmt, arg );
									va_end( arg );
									fflush( stdout );
								}
	virtual void				FatalError( VERIFY_FORMAT_STRING const char *fmt, ... ) override {
									g_failures++;
									va_list arg;
									va_start( arg, fmt );
									printf( "  [fatal] " );
									vfprintf( stdout, fmt, arg );
									va_end( arg );
									fflush( stdout );
								}
	virtual const char *		KeysFromBinding( const char *bind ) override { return ""; }
	virtual const char *		BindingFromKey( const char *key ) override { return ""; }
	virtual int					ButtonState( int key ) override { return 0; }
	virtual int					KeyState( int key ) override { return 0; }
	virtual bool				IsMultiplayer() override { return false; }
	virtual bool				IsServer() override { return false; }
	virtual bool				IsClient() override { return false; }
	virtual bool				GetConsoleUsed() override { return false; }
	virtual int					GetSnapRate() override { return 0; }
	virtual void				NetReceiveReliable( int peer, int type, hfBitMsg & msg ) override {}
	virtual void				NetReceiveSnapshot( class hfSnapShot & ss ) override {}
	virtual void				NetReceiveUsercmds( int peer, hfBitMsg & msg ) override {}
	virtual	bool				ProcessEvent( const sysEvent_t * event ) override { return false; }
	virtual bool				LoadGame( const char * saveName ) override { return false; }
	virtual bool				SaveGame( const char * saveName ) override { return false; }
	virtual hfDemoFile *		ReadDemo() override { return NULL; }
	virtual hfDemoFile *		WriteDemo() override { return NULL; }
	virtual hfGame *			Game() override { return NULL; }
	virtual hfRenderWorld *		RW() override { return NULL; }
	virtual hfSoundWorld *		SW() override { return NULL; }
	virtual hfSoundWorld *		MenuSW() override { return NULL; }
	virtual hfSession *			Session() override { return NULL; }
	virtual hfCommonDialog &	Dialog() override { static hfCommonDialog dummy; return dummy; }
	virtual void				OnSaveCompleted( hfSaveLoadParms & parms ) override {}
	virtual void				OnLoadCompleted( hfSaveLoadParms & parms ) override {}
	virtual void				OnLoadFilesCompleted( hfSaveLoadParms & parms ) override {}
	virtual void				OnEnumerationCompleted( hfSaveLoadParms & parms ) override {}
	virtual void				OnDeleteCompleted( hfSaveLoadParms & parms ) override {}
	virtual void				TriggerScreenWipe( const char * _wipeMaterial, bool hold ) override {}
	virtual void				OnStartHosting( hfMatchParameters & parms ) override {}
	virtual int					GetGameFrame() override { return 0; }
	virtual void				LaunchExternalTitle( int titleIndex, int device, const lobbyConnectInfo_t * const connectInfo ) override {}
	virtual void				InitializeMPMapsModes() override {}
	virtual const hfStrList &	GetModeList() const override { static hfStrList empty; return empty; }
	virtual const hfStrList &	GetModeDisplayList() const override { static hfStrList empty; return empty; }
	virtual const hfList<mpMap_t> &	GetMapList() const override { static hfList<mpMap_t> empty; return empty; }
	virtual void				ResetPlayerInput( int playerIndex ) override {}
	virtual bool				JapaneseCensorship() const override { return false; }
	virtual void				QueueShowShell() override {}
	virtual currentGame_t		GetCurrentGame() const override { return currentGame_t( 0 ); }
	virtual void				SwitchToGame( currentGame_t newGame ) override {}
};

static HfTestCommon	hfTestCommon;

int Sys_Milliseconds() {
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (int)( tv.tv_sec * 1000 + tv.tv_usec / 1000 );
}

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static void Host_print( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	for ( int i = 0; i < argc; i++ ) {
		if ( i > 0 ) {
			printf( " " );
		}
		printf( "%s", vm->GetArg( i ).AsString( buffer ) );
	}
	printf( "\n" );
	*out = HfScriptValue::Nil();
}

static void Host_format( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr buffer;
	const char * fmt = vm->GetArg( 0 ).AsString( buffer );
	hfStr result;
	int argIndex = 1;
	hfStr argText;
	for ( int i = 0; fmt[ i ] != 0; i++ ) {
		if ( fmt[ i ] == '{' && fmt[ i + 1 ] == '{' ) {
			result += '{';
			i++;
			continue;
		}
		if ( fmt[ i ] == '{' && fmt[ i + 1 ] == '}' ) {
			if ( argIndex < argc ) {
				result += vm->GetArg( argIndex ).AsString( argText );
			}
			argIndex++;
			i++;
			continue;
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

static void Host_getnum( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( 42 );
	(void)argc;
	(void)vm;
}

static void Host_len( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr tmp;
	const char * text = argc > 0 ? vm->GetArg( 0 ).AsString( tmp ) : "";
	*out = HfScriptValue::Int( (int)strlen( text != NULL ? text : "" ) );
	(void)argc;
}

static void Host_int( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	(void)argc;
	*out = HfScriptValue::Int( vm->GetArg( 0 ).AsInt() );
}

static void Host_str( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr tmp;
	const char * text = argc > 0 ? vm->GetArg( 0 ).AsString( tmp ) : "";
	(void)argc;
	*out = HfScriptValue::Str( vm->NewString( text != NULL ? text : "" ) );
}

static void Host_sqrt( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	(void)argc;
	*out = HfScriptValue::Float( sqrt( fabs( vm->GetArg( 0 ).AsFloat() ) ) );
}

static void Host_loc( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	hfStr key;
	const char * text = argc > 0 ? vm->GetArg( 0 ).AsString( key ) : "";
	(void)argc;
	*out = HfScriptValue::Str( vm->NewString( text != NULL ? text : "" ) );
}

static void Host_getcvar( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	(void)argc;
	*out = HfScriptValue::Str( vm->NewString( "" ) );
}

static void Host_setcvar( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	(void)vm;
	(void)argc;
	*out = HfScriptValue::Nil();
}

static bool Run( HfScriptVM & vm, HfScriptProgram & prog, const char * name, const char * source ) {
	printf( "--- %s ---\n", name );
	if ( !HfScript_Compile( &prog, source, name ) ) {
		printf( "COMPILE FAIL: %s(%d): %s\n", name, prog.errorLine, prog.error.c_str() );
		g_failures++;
		return false;
	}
	printf( "  [compiled: %d ops, %d funcs]\n", prog.numOps, prog.numFunctions );
	if ( !vm.RunProgram( &prog ) ) {
		printf( "RUN FAIL: %s(%d): %s\n", name, vm.lastErrorLine, vm.lastError.c_str() );
		g_failures++;
		return false;
	}
	return true;
}

// CPU-only unit tests for the renderer's pure logic (uber feature masks and
// GL capability detection). Defined in hfstest_render.cpp.
int HfTest_RenderMain();
int HfTest_OptimizeMain();
int HfTest_FeaturesMain();

// --- test watchdog: a run that wedges prints where the main thread is ---
// The signal handler runs on the main thread, so backtrace() unwinds the
// frame that is actually stuck rather than the watchdog's own.
#include <pthread.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
static pthread_t	g_hfMainThread;
static void HfWatchdogHandler( int sig ) {
	(void)sig;
	void *bt[ 64 ];
	int n = backtrace( bt, 64 );
	const char *msg = "\n*** WATCHDOG: main thread backtrace follows ***\n";
	write( 2, msg, strlen( msg ) );
	backtrace_symbols_fd( bt, n, 2 );
}
static void *HfWatchdogThread( void * ) {
	// NOTE: sleep(), not usleep() - usleep() with a large value never
	// returned under this runtime, which silently disarmed the watchdog.
	// 600s is ~12x a normal run, so this only fires on a genuine wedge.
	sleep( 600 );
	pthread_kill( g_hfMainThread, SIGUSR1 );
	sleep( 10 );
	_exit( 142 );
	return NULL;
}
static void HfStartWatchdog() {
	g_hfMainThread = pthread_self();
	signal( SIGUSR1, HfWatchdogHandler );
	pthread_t t;
	pthread_attr_t a;
	pthread_attr_init( &a );
	pthread_attr_setdetachstate( &a, PTHREAD_CREATE_DETACHED );
	pthread_create( &t, &a, HfWatchdogThread, NULL );
	pthread_attr_destroy( &a );
}

/*
================
main
================
*/
int main( int argc, char ** argv ) {
	// --headless is a process-wide mode, not a message: parse it first so that
	// anything asking HF_Lib::IsHeadless() during startup already agrees.
	HF_Lib::ParseHeadlessCommand( argc, (const char * const *)argv );

	// hflib reports through the common global, so the sink has to be in place
	// before anything else can warn. hfLib keeps its own pointer to the same
	// interface and most of the library reports through that one.
	common = &hfTestCommon;
	hfLib::common = &hfTestCommon;

	setvbuf( stdout, NULL, _IONBF, 0 );

	HfStartWatchdog();

	hfLib::Init();
	printf( "[init done]\n" );

	HfScript_RegisterHost( "print", Host_print );
	HfScript_RegisterHost( "format", Host_format );
	HfScript_RegisterHost( "getnum", Host_getnum );
	HfScript_RegisterHost( "len", Host_len );
	HfScript_RegisterHost( "int", Host_int );
	HfScript_RegisterHost( "str", Host_str );
	HfScript_RegisterHost( "sqrt", Host_sqrt );
	HfScript_RegisterHost( "loc", Host_loc );
	HfScript_RegisterHost( "getcvar", Host_getcvar );
	HfScript_RegisterHost( "setcvar", Host_setcvar );
	printf( "[hosts registered]\n" );

	HfScriptVM vm;
	HfScriptProgram prog;
	printf( "[vm+program created]\n" );

	Run( vm, prog, "basics",
		"def main():\n"
		"    a = 1 + 2 * 3\n"
		"    print(format(\"a = {}\", a))\n"
		"    b = 10 % 3\n"
		"    print(format(\"b = {}\", b))\n"
		"    print(format(\"2 == 2: {}\", 2 == 2))\n"
		"    print(format(\"3 >= 2: {}\", 3 >= 2))\n"
		"    print(format(\"not false: {}\", not false))\n"
		"    c = 1.5 + 1\n"
		"    print(format(\"c = {}\", c))\n"
		"main()\n"
	);

	Run( vm, prog, "control",
		"def main():\n"
		"    x = 5\n"
		"    if x < 2:\n"
		"        print(\"small\")\n"
		"    elif x < 4:\n"
		"        print(\"medium\")\n"
		"    elif x < 6:\n"
		"        print(\"large\")\n"
		"    else:\n"
		"        print(\"huge\")\n"
		"    i = 0\n"
		"    total = 0\n"
		"    while i < 5:\n"
		"        i += 1\n"
		"        if i == 3:\n"
		"            continue\n"
		"        total += i\n"
		"    print(format(\"total = {}\", total))\n"
		"main()\n"
	);

	Run( vm, prog, "for",
		"def main():\n"
		"    s = 0\n"
		"    for i in range(10):\n"
		"        s += i\n"
		"        if s > 20:\n"
		"            break\n"
		"    print(format(\"s = {}\", s))\n"
		"    t = 0\n"
		"    for k in range(3, 6):\n"
		"        t += k\n"
		"    print(format(\"t = {}\", t))\n"
		"main()\n"
	);

	Run( vm, prog, "func",
		"def fib(n):\n"
		"    if n < 2:\n"
		"        return n\n"
		"    return fib(n - 1) + fib(n - 2)\n"
		"def add(a, b):\n"
		"    return a + b\n"
		"def main():\n"
		"    print(format(\"fib(8) = {}\", fib(8)))\n"
		"    print(format(\"add(2,3) = {}\", add(2, 3)))\n"
		"    print(format(\"host: {}\", getnum()))\n"
		"    print(format(\"str concat {}\", \"ab\" + \"cd\"))\n"
		"main()\n"
	);

	Run( vm, prog, "scopes",
		"counter = 1\n"
		"def bump():\n"
		"    global counter\n"
		"    counter += 1\n"
		"def main():\n"
		"    local = \"local only\"\n"
		"    bump()\n"
		"    bump()\n"
		"    print(format(\"counter = {}\", counter))\n"
		"    print(format(\"local = {}\", local))\n"
		"main()\n"
	);

	Run( vm, prog, "single-line-if",
		"def main():\n"
		"    if 1 < 2: print(\"yes\")\n"
		"    pass\n"
		"main()\n"
	);

	Run( vm, prog, "global-read",
		"game = \"Hellforge\"\n"
		"def greet(name):\n"
		"    print(format(\"hello {} from {}\", name, game))\n"
		"def main():\n"
		"    greet(\"world\")\n"
		"main()\n"
	);

	{
		// Python-flavored front end: same compiler, '#' comments, indentation.
		HfScriptProgram pyProg;
		HfScriptVM pyVM;
		printf( "--- python ---\n" );
		const char * pySrc =
			"# a python-flavored hfscript program\n"
			"def fib(n):\n"
			"    if n < 2:          # base case\n"
			"        return n\n"
			"    return fib(n - 1) + fib(n - 2)\n"
			"def main():\n"
			"    s = 0\n"
			"    for i in range(5):\n"
			"        s += i\n"
			"    print(format(\"py sum = {}\", s))\n"
			"    print(format(\"py fib(6) = {}\", fib(6)))\n"
			"main()\n";
		if ( !HfScript_Compile( &pyProg, pySrc, "test.py" ) ) {
			printf( "PY FAIL: compile: %s(%d): %s\n", pyProg.error.c_str(), pyProg.errorLine, pyProg.error.c_str() );
			g_failures++;
		} else if ( !HfScript_IsPythonSource( "test.py" ) ) {
			printf( "PY FAIL: .py not tagged\n" );
			g_failures++;
		} else {
			printf( "  [compiled (python tag): %d ops, %d funcs]\n", pyProg.numOps, pyProg.numFunctions );
			if ( !pyVM.RunProgram( &pyProg ) ) {
				printf( "PY FAIL: run: %s(%d)\n", pyVM.lastError.c_str(), pyVM.lastErrorLine );
				g_failures++;
			}
		}
	}

	{
		// Ruby-flavored front end: same compiler, '#' comments, 'end' blocks.
		HfScriptProgram rbProg;
		HfScriptVM rbVM;
		printf( "--- ruby ---\n" );
		const char * rbSrc =
			"# a ruby-flavored hfscript program\n"
			"def fib(n)\n"
			"    if n < 2\n"
			"        return n\n"
			"    end\n"
			"    return fib(n - 1) + fib(n - 2)\n"
			"end\n"
			"def main()\n"
			"    s = 0\n"
			"    for i in range(5)\n"
			"        s += i\n"
			"    end\n"
			"    print(format(\"rb sum = {}\", s))\n"
			"    print(format(\"rb fib(7) = {}\", fib(7)))\n"
			"    x = 5\n"
			"    if x < 2\n"
			"        print(\"small\")\n"
			"    elsif x < 6\n"
			"        print(\"medium\")\n"
			"    else\n"
			"        print(\"large\")\n"
			"    end\n"
			"end\n"
			"main()\n";
		if ( !HfScript_Compile( &rbProg, rbSrc, "test.rb" ) ) {
			printf( "RB FAIL: compile: %s(%d): %s\n", rbProg.error.c_str(), rbProg.errorLine, rbProg.error.c_str() );
			g_failures++;
		} else if ( !HfScript_IsRubySource( "test.rb" ) ) {
			printf( "RB FAIL: .rb not tagged\n" );
			g_failures++;
		} else {
			printf( "  [compiled (ruby tag): %d ops, %d funcs]\n", rbProg.numOps, rbProg.numFunctions );
			if ( !rbVM.RunProgram( &rbProg ) ) {
				printf( "RB FAIL: run: %s(%d)\n", rbVM.lastError.c_str(), rbVM.lastErrorLine );
				g_failures++;
			}
		}
	}

#if defined( HF_PYTHON )
	{
		// CPython bridge: real Python behind python_eval/exec/import/get/call.
		if ( !HfScript_PyBridge_Init() ) {
			printf( "PYBRIDGE FAIL: init\n" );
			g_failures++;
		} else {
			HfScript_PyBridge_RegisterHosts();
			HfScriptProgram pbProg;
			HfScriptVM pbVM;
			printf( "--- python bridge ---\n" );
			const char * pbSrc =
				"def main():\n"
				"    v = python_eval(\"10 + 5 * 2\")\n"
				"    print(format(\"eval = {}\", v))\n"
				"    python_exec(\"answer = 7 * 6\")\n"
				"    a = python_get(\"__main__\", \"answer\")\n"
				"    print(format(\"exec = {}\", a))\n"
				"    m = python_import(\"math\")\n"
				"    p = python_call(\"math\", \"sqrt\", 81)\n"
				"    print(format(\"sqrt = {}\", p))\n"
				"    s = python_eval('\\\"hello\\\".upper()')\n"
				"    print(format(\"upper = {}\", s))\n"
				"main()\n";
			if ( !HfScript_Compile( &pbProg, pbSrc, "bridge.py" ) ) {
				printf( "PYBRIDGE FAIL: compile: %s(%d)\n", pbProg.error.c_str(), pbProg.errorLine );
				g_failures++;
			} else if ( !pbVM.RunProgram( &pbProg ) ) {
				printf( "PYBRIDGE FAIL: run: %s(%d)\n", pbVM.lastError.c_str(), pbVM.lastErrorLine );
				g_failures++;
			}
		}
		HfScript_PyBridge_Shutdown();
	}
#endif /* HF_PYTHON */

	{
		// AOT image round-trip: compile -> save -> load -> run
		HfScriptProgram compiled;
		HfScriptProgram loaded;
		HfScriptVM jvm;
		const char * aotSrc =
			"game = \"Hellforge\"\n"
			"def greet(n):\n"
			"    return len(n) + 1\n"
			"def main():\n"
			"    print(format(\"aot {} from {}\", greet(\"xyz\"), game))\n"
			"main()\n";
		printf( "  [aot]\n" );
		if ( !HfScript_Compile( &compiled, aotSrc, "aot" ) ) {
			printf( "AOT FAIL: compile: %s(%d): %s\n", compiled.error.c_str(), compiled.errorLine, compiled.error.c_str() );
			g_failures++;
		} else {
			void * img = NULL;
			int imgSize = 0;
			if ( !HfScript_SaveImage( &compiled, &img, &imgSize ) ) {
				printf( "AOT FAIL: save\n" );
				g_failures++;
			} else {
				if ( !HfScript_LoadImage( &loaded, img, imgSize ) ) {
					printf( "AOT FAIL: load: %s\n", loaded.error.c_str() );
					g_failures++;
					Mem_Free( img );
				} else {
					Mem_Free( img );
					printf( "  [aot loaded: %d ops, %d funcs, %d bytes]\n", loaded.numOps, loaded.numFunctions, imgSize );
					if ( loaded.numOps != compiled.numOps || loaded.numFunctions != compiled.numFunctions ) {
						printf( "AOT FAIL: image mismatch (%d restored)\n", loaded.numOps );
						g_failures++;
					} else if ( !jvm.RunProgram( &loaded ) ) {
						printf( "AOT FAIL: run: %s(%d)\n", jvm.lastError.c_str(), jvm.lastErrorLine );
						g_failures++;
					}
				}
			}
		}
	}

	{
		// JIT: same bytecode lowered to a native module via the system compiler
		HfScriptProgram jprog;
		HfScriptVM ivm, jvm;
		const char * jitSrc =
			"def fib(n):\n"
			"    if n < 2:\n"
			"        return n\n"
			"    return fib(n - 1) + fib(n - 2)\n"
			"def main():\n"
			"    s = 0\n"
			"    for i in range(5):\n"
			"        s += i\n"
			"    print(format(\"jit sum = {}\", s))\n"
			"    print(format(\"jit fib(8) = {}\", fib(8)))\n"
			"    print(format(\"jit str = {}\", \"ab\" + \"cd\"))\n"
			"    n = 3\n"
			"    if n > 2: print(\"jit big\")\n"
			"    print(format(\"jit len = {}\", len(\"hello\")))\n"
			"main()\n";
		printf( "  [jit]\n" );
		if ( !HfScript_Compile( &jprog, jitSrc, "jit" ) ) {
			printf( "JIT FAIL: compile: %s(%d): %s\n", jprog.error.c_str(), jprog.errorLine, jprog.error.c_str() );
			g_failures++;
		} else if ( !ivm.RunProgram( &jprog ) ) {
			printf( "JIT FAIL: interpreter run: %s(%d)\n", ivm.lastError.c_str(), ivm.lastErrorLine );
			g_failures++;
		} else if ( getenv( "HF_NOBENCH" ) == NULL && !HfScript_JitCompileProgram( &jprog ) ) {
			printf( "JIT FAIL: native compile: %s\n", jprog.error.c_str() );
			g_failures++;
		} else {
			printf( "  [jit native module attached]\n" );
			if ( !jvm.RunProgram( &jprog ) ) {
				printf( "JIT FAIL: native run: %s(%d)\n", jvm.lastError.c_str(), jvm.lastErrorLine );
				g_failures++;
			}
		}
	}

	{
		// demo.hfs equivalence: interpreter vs AOT-load vs JIT native
		FILE * df = fopen( "base/script/demo.hfs", "rb" );
		if ( df == NULL ) {
			printf( "DEMO FAIL: cannot open demo\n" );
			g_failures++;
		} else {
			fseek( df, 0, SEEK_END );
			long dsize = ftell( df );
			fseek( df, 0, SEEK_SET );
			char * dsrc = (char *)Mem_ClearedAlloc( dsize + 1, TAG_SCRIPT );
			fread( dsrc, 1, dsize, df );
			fclose( df );
			dsrc[ dsize ] = '\0';

			HfScriptProgram dp;
			HfScriptVM divm;
			printf( "  [demo equivalence]\n" );
			if ( !HfScript_Compile( &dp, dsrc, "demo.hfs" ) ) {
				printf( "DEMO FAIL: compile: %s(%d)\n", dp.error.c_str(), dp.errorLine );
				g_failures++;
			} else if ( !divm.RunProgram( &dp ) ) {
				printf( "DEMO FAIL: interpreter run: %s(%d)\n", divm.lastError.c_str(), divm.lastErrorLine );
				g_failures++;
			} else {
				// AOT round trip
				void * img = NULL;
				int imgSize = 0;
				HfScriptProgram dl;
				HfScriptVM davm;
				if ( !HfScript_SaveImage( &dp, &img, &imgSize ) || !HfScript_LoadImage( &dl, img, imgSize ) ) {
					printf( "DEMO FAIL: aot\n" );
					g_failures++;
				} else if ( !davm.RunProgram( &dl ) ) {
					printf( "DEMO FAIL: aot run: %s(%d)\n", davm.lastError.c_str(), davm.lastErrorLine );
					g_failures++;
				} else if ( getenv( "HF_NOBENCH" ) == NULL && !HfScript_JitCompileProgram( &dp ) ) {
					printf( "DEMO FAIL: jit compile\n" );
					g_failures++;
				} else {
					HfScriptVM djvm;
					if ( !djvm.RunProgram( &dp ) ) {
						printf( "DEMO FAIL: jit run: %s(%d)\n", djvm.lastError.c_str(), djvm.lastErrorLine );
						g_failures++;
					}
				}
				if ( img != NULL ) {
					Mem_Free( img );
				}
				HfScript_JitRelease( &dp );
			}
			Mem_Free( dsrc );
		}
	}

	// HF_NOBENCH skips the JIT benchmark (its system-compiler invocation is
	// slow on constrained hosts); it does not affect what is verified.
	if ( getenv( "HF_NOBENCH" ) == NULL ) {
		// Speed benchmark: the same recursive workload through the
		// interpreter and the JIT native path, so per-run regressions are
		// visible and both execution modes are exercised under load.
		printf( "--- benchmark ---\n" );
		const char * benchSrc =
			"def fib(n):\n"
			"    if n < 2:\n"
			"        return n\n"
			"    return fib(n - 1) + fib(n - 2)\n"
			"def main():\n"
			"    print(format(\"fib(22) = {}\", fib(22)))\n"
			"main()\n";
		HfScriptProgram bp;
		HfScriptVM bvm;
		if ( !HfScript_Compile( &bp, benchSrc, "bench.hfs" ) ) {
			printf( "BENCH FAIL: compile: %s\n", bp.error.c_str() );
			g_failures++;
		} else {
			int t0 = Sys_Milliseconds();
			bool okInt = bvm.RunProgram( &bp );
			int tInt = Sys_Milliseconds() - t0;
			if ( !okInt ) {
				printf( "BENCH FAIL: interpreter: %s(%d)\n", bvm.lastError.c_str(), bvm.lastErrorLine );
				g_failures++;
			} else if ( !HfScript_JitCompileProgram( &bp ) ) {
				printf( "BENCH FAIL: jit compile\n" );
				g_failures++;
			} else {
				HfScriptVM bjm;
				int t1 = Sys_Milliseconds();
				bool okJit = bjm.RunProgram( &bp );
				int tJit = Sys_Milliseconds() - t1;
				if ( !okJit ) {
					printf( "BENCH FAIL: jit: %s(%d)\n", bjm.lastError.c_str(), bjm.lastErrorLine );
					g_failures++;
				} else {
					printf( "  interpreter: %d ms\n", tInt );
					printf( "  jit:         %d ms", tJit );
					if ( tJit > 0 && tInt > tJit ) {
						printf( "   (%dx faster)", tInt / tJit );
					}
					printf( "\n" );
				}
				HfScript_JitRelease( &bp );
			}
		}
	}

	// the first non-switch argument is a script to run; switches such as
	// --headless are modes and must not be mistaken for a file name
	int scriptArg = -1;
	for ( int i = 1; i < argc; i++ ) {
		if ( HF_Lib::IsHeadlessArgument( argv[ i ] ) ) { continue; }
		scriptArg = i;
		break;
	}
	if ( scriptArg >= 0 ) {
		FILE * f = fopen( argv[ scriptArg ], "rb" );
		if ( f == NULL ) {
			printf( "cannot open %s\n", argv[ scriptArg ] );
			g_failures++;
		} else {
			fseek( f, 0, SEEK_END );
			long size = ftell( f );
			fseek( f, 0, SEEK_SET );
			char * buffer = (char *)malloc( size + 1 );
			fread( buffer, 1, size, f );
			buffer[ size ] = 0;
			fclose( f );
			Run( vm, prog, argv[ scriptArg ], buffer );
			free( buffer );
		}
	}

	g_failures += HfTest_RenderMain();
	g_failures += HfTest_OptimizeMain();
	g_failures += HfTest_FeaturesMain();

	printf( "%s\n", g_failures == 0 ? "ALL PASSED" : "FAILURES PRESENT" );
	return g_failures == 0 ? 0 : 1;
}
// The headless harness links hflib, whose engine-global surface references the
// command system. The harness never drives commands, so provide a null global
// rather than pulling the full interactive command system into the build.
#include "../framework/hfCmdSystem.h"
hfCmdSystem *	cmdSystem = NULL;

// --- crash diagnostics: print a native backtrace on fatal signal ---
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
static void HfCrashHandler( int sig ) {
	void *bt[ 64 ];
	int n = backtrace( bt, 64 );
	const char *msg = "\n*** FATAL SIGNAL - backtrace follows ***\n";
	write( 2, msg, strlen( msg ) );
	backtrace_symbols_fd( bt, n, 2 );
	_exit( 139 );
}
struct HfCrashInstall {
	HfCrashInstall() { signal( SIGSEGV, HfCrashHandler ); signal( SIGBUS, HfCrashHandler ); signal( SIGABRT, HfCrashHandler ); }
};
static HfCrashInstall g_hfCrashInstall;
