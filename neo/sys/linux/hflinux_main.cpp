#include "../../hflib/hfprecompiled.h"
#include "../hfsys_local.h"
#include "hflinux_local.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

LinuxVars_t	linux;

static hfList< sysEvent_t >		eventQueue;
static bool						queuedEventsAreFake = true;

/*
================
Sys_Error
================
*/
void Sys_Error( const char *error, ... ) {
	va_list argptr;
	va_start( argptr, error );
	Sys_DebugVPrintf( error, argptr );
	va_end( argptr );

	Sys_ShutdownInput();
	abort();
}

/*
================
Sys_GetCmdLine
================
*/
const char * Sys_GetCmdLine() {
	return "";  // Linux exposes args via main(); no global needed here
}

/*
================
Sys_Launch
================
*/
void Sys_Launch( const char * path, const hfCmdArgs & args, void * data, unsigned int dataSize ) {
	pid_t pid = fork();
	if ( pid == 0 ) {
		execl( path, path, (char *)data, NULL );
		_exit( 127 );
	}
}

/*
================
Sys_ReLaunch
================
*/
void Sys_ReLaunch( void * data, const unsigned int dataSize ) {
	Sys_Launch( Sys_EXEPath(), hfCmdArgs(), data, dataSize );
}

/*
================= 
hfSysLocal::OpenURL
=================
*/
void hfSysLocal::OpenURL( const char *url, bool doexit ) {
	static bool doexit_spamguard = false;

	if ( doexit_spamguard ) {
		common->DPrintf( "OpenURL: already in an exit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf( "Open URL: %s\n", url );

	pid_t pid = fork();
	if ( pid == 0 ) {
		execl( "/usr/bin/xdg-open", "xdg-open", url, NULL );
		execl( "/usr/bin/open", "open", url, NULL );
		_exit( 127 );
	}

	if ( doexit ) {
		doexit_spamguard = true;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
=================
hfSysLocal::StartProcess
=================
*/
void hfSysLocal::StartProcess( const char *exePath, bool doexit ) {
	pid_t pid = fork();
	if ( pid == 0 ) {
		execl( exePath, exePath, NULL );
		_exit( 127 );
	}

	if ( doexit ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
================
Sys_Quit
================
*/
void Sys_Quit() {
	Sys_ShutdownInput();
	exit( 0 );
}

/*
================
Sys_Shutdown
================
*/
void Sys_Shutdown() {
	Sys_ShutdownInput();
	Sys_ShutdownNetworking();
	Sys_ShutdownSymbols();
}

/*
================
Sys_SetLanguageFromSystem
================
*/
extern hfCVar sys_lang;
void Sys_SetLanguageFromSystem() {
	sys_lang.SetString( Sys_DefaultLanguage() );
}

/*
================
Sys_Printf
================
*/
void Sys_Printf( const char *fmt, ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	vprintf( fmt, argptr );
	va_end( argptr );
}

/*
================
Sys_DebugPrintf
================
*/
void Sys_DebugPrintf( const char *fmt, ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

/*
================
Sys_DebugVPrintf
================
*/
void Sys_DebugVPrintf( const char *fmt, va_list arg ) {
	vfprintf( stderr, fmt, arg );
}

/*
================
Sys_Sleep
================
*/
void Sys_Sleep( int msec ) {
	if ( msec <= 0 ) {
		return;
	}
	usleep( msec * 1000 );
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData() {
	return NULL;
}

/*
================
Sys_SetClipboardData
================
*/
void Sys_SetClipboardData( const char *string ) {
}

/*
================
Sys_Exec
================
*/
bool Sys_Exec( const char * appPath, const char * workingPath, const char * args,
	execProcessWorkFunction_t workFn, execOutputFunction_t outputFn, const int waitMS,
	unsigned int & exitCode ) {
	pid_t pid = fork();
	if ( pid == 0 ) {
		if ( workingPath != NULL ) {
			chdir( workingPath );
		}
		execl( appPath, appPath, args, (char *)NULL );
		_exit( 127 );
	}
	if ( pid < 0 ) {
		return false;
	}
	int status = 0;
	waitpid( pid, &status, 0 );
	exitCode = WIFEXITED( status ) ? WEXITSTATUS( status ) : 1;
	return true;
}

/*
================
Sys_DLL_Load
================
*/
int Sys_DLL_Load( const char *dllName ) {
	return (int)(intptr_t)dlopen( dllName, RTLD_NOW );
}

/*
================
Sys_DLL_GetProcAddress
================
*/
void *Sys_DLL_GetProcAddress( int dllHandle, const char *procName ) {
	return dlsym( (void *)(intptr_t)dllHandle, procName );
}

/*
================
Sys_DLL_Unload
================
*/
void Sys_DLL_Unload( int dllHandle ) {
	dlclose( (void *)(intptr_t)dllHandle );
}

/*
================
Sys_QueEvent
================
*/
void Sys_QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr, int inputDeviceNum ) {
	sysEvent_t ev;
	ev.evType = type;
	ev.evValue = value;
	ev.evValue2 = value2;
	ev.evPtrLength = ptrLength;
	ev.evPtr = ptr;
	ev.inputDevice = inputDeviceNum;
	eventQueue.Append( ev );
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents() {
	for ( int i = 0; i < eventQueue.Num(); i++ ) {
		if ( eventQueue[i].evPtr != NULL ) {
			Mem_Free( eventQueue[i].evPtr );
		}
	}
	eventQueue.Clear();
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent() {
	if ( eventQueue.Num() == 0 ) {
		sysEvent_t ev;
		memset( &ev, 0, sizeof( ev ) );
		ev.evType = SE_NONE;
		return ev;
	}
	sysEvent_t ev = eventQueue[0];
	eventQueue.RemoveIndex( 0 );
	return ev;
}

/*
================
Sys_AlreadyRunning
================
*/
bool Sys_AlreadyRunning() {
	// simple check using a lock file
	const char * lockPath = va( "%s/.hellforge.lock", Sys_DefaultSavePath() );
	int fd = open( lockPath, O_CREAT | O_EXCL, 0644 );
	if ( fd == -1 && errno == EEXIST ) {
		return true;
	}
	if ( fd != -1 ) {
		close( fd );
	}
	return false;
}

/*
================
Sys_Init
================
*/
void Sys_Init() {
	int i;
	for ( i = 0; i < MAX_CRITICAL_SECTIONS; i++ ) {
		Sys_MutexCreate( linux.criticalSections[i] );
	}
}

/*
================
Sys_InitNetworking
================
*/
void Sys_InitNetworking() {
}

/*
================
Sys_ShutdownNetworking
================
*/
void Sys_ShutdownNetworking() {
}

/*
================
Sys_SetFatalError
================
*/
void Sys_SetFatalError( const char *error ) {
}

/*
================
Sys_ShowConsole
================
*/
void Sys_ShowConsole( int visLevel, bool quitOnClose ) {
}

/*
================
main
================
*/
static char sys_cmdline[MAX_STRING_CHARS];

int main( int argc, char *argv[] ) {
	Sys_SetPhysicalWorkMemory( 192 << 20, 1024 << 20 );

	for ( int i = 0; i < MAX_CRITICAL_SECTIONS; i++ ) {
		Sys_MutexCreate( linux.criticalSections[i] );
	}

	// --headless means no window and no GPU: the display is the first thing a
	// windowed run needs and the first thing a headless run must do without,
	// so the switch is read before anything else. GLimp_Init and the input
	// layer both treat a NULL display as "there is no front end", which is
	// exactly this case.
	HF_Lib::ParseHeadlessCommand( argc, (const char * const *)argv );
	if ( !HF_Lib::IsHeadless() ) {
		// open the X11 display up front so GLimp_Init / Sys_GenerateEvents can use it
		x_display = XOpenDisplay( NULL );
		if ( x_display == NULL ) {
			fprintf( stderr, "Couldn't open X11 display. Is DISPLAY set?\n" );
			return 1;
		}
	} else {
		// common->Init() has not run yet, so this cannot go through the
		// console: it is the first thing anyone sees from a headless run
		fprintf( stdout, "headless mode: no X11 display, no window, no GPU\n" );
	}
	// DefaultScreen() dereferences the display, so it only exists when there
	// is one; headless leaves both at their inert defaults
	if ( x_display != NULL ) {
		x_screen = DefaultScreen( x_display );
	}
	x_window = 0;
	linux.visual = NULL;
	linux.glxContext = NULL;
	linux.wnd = 0;
	linux.dpy = x_display;
	linux.screen = x_screen;
	linux.activeApp = true;

	// build command line string
	hfStr::Copynz( sys_cmdline, "", sizeof( sys_cmdline ) );
	for ( int i = 1; i < argc; i++ ) {
		hfStr::Append( sys_cmdline, sizeof( sys_cmdline ), argv[i] );
		hfStr::Append( sys_cmdline, sizeof( sys_cmdline ), " " );
	}

	Sys_Milliseconds();

	Sys_FPU_SetPrecision( FPU_PRECISION_DOUBLE_EXTENDED );

	common->Init( argc, argv, sys_cmdline );

	common->Quit();

	if ( x_display != NULL ) {
		XCloseDisplay( x_display );
		x_display = NULL;
	}

	return 0;
}

