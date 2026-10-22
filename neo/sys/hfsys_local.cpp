#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfsys_local.h"

const char * sysLanguageNames[] = {
	HF_LANG_ENGLISH, HF_LANG_FRENCH, HF_LANG_ITALIAN, HF_LANG_GERMAN, HF_LANG_SPANISH, HF_LANG_JAPANESE, NULL
};

const int numLanguages = sizeof( sysLanguageNames ) / sizeof sysLanguageNames[ 0 ] - 1;

hfCVar sys_lang( "sys_lang", HF_LANG_ENGLISH, CVAR_SYSTEM | CVAR_INIT, "", sysLanguageNames, hfCmdSystem::ArgCompletion_String<sysLanguageNames> );

hfSysLocal			sysLocal;
hfSys *				sys = &sysLocal;

void hfSysLocal::DebugPrintf( const char *fmt, ... ) {
	va_list argptr;

	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

void hfSysLocal::DebugVPrintf( const char *fmt, va_list arg ) {
	Sys_DebugVPrintf( fmt, arg );
}

double hfSysLocal::GetClockTicks() {
	return Sys_GetClockTicks();
}

double hfSysLocal::ClockTicksPerSecond() {
	return Sys_ClockTicksPerSecond();
}

cpuid_t hfSysLocal::GetProcessorId() {
	return Sys_GetProcessorId();
}

const char *hfSysLocal::GetProcessorString() {
	return Sys_GetProcessorString();
}

const char *hfSysLocal::FPU_GetState() {
	return Sys_FPU_GetState();
}

bool hfSysLocal::FPU_StackIsEmpty() {
	return Sys_FPU_StackIsEmpty();
}

void hfSysLocal::FPU_SetFTZ( bool enable ) {
	Sys_FPU_SetFTZ( enable );
}

void hfSysLocal::FPU_SetDAZ( bool enable ) {
	Sys_FPU_SetDAZ( enable );
}

bool hfSysLocal::LockMemory( void *ptr, int bytes ) {
	return Sys_LockMemory( ptr, bytes );
}

bool hfSysLocal::UnlockMemory( void *ptr, int bytes ) {
	return Sys_UnlockMemory( ptr, bytes );
}

void hfSysLocal::GetCallStack( address_t *callStack, const int callStackSize ) {
	Sys_GetCallStack( callStack, callStackSize );
}

const char * hfSysLocal::GetCallStackStr( const address_t *callStack, const int callStackSize ) {
	return Sys_GetCallStackStr( callStack, callStackSize );
}

const char * hfSysLocal::GetCallStackCurStr( int depth ) {
	return Sys_GetCallStackCurStr( depth );
}

void hfSysLocal::ShutdownSymbols() {
	Sys_ShutdownSymbols();
}

int hfSysLocal::DLL_Load( const char *dllName ) {
	return Sys_DLL_Load( dllName );
}

void *hfSysLocal::DLL_GetProcAddress( int dllHandle, const char *procName ) {
	return Sys_DLL_GetProcAddress( dllHandle, procName );
}

void hfSysLocal::DLL_Unload( int dllHandle ) {
	Sys_DLL_Unload( dllHandle );
}

void hfSysLocal::DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) {
#if defined(_WIN32)
	hfStr::snPrintf( dllName, maxLength, "%s" CPUSTRING ".dll", baseName );
#else
	hfStr::snPrintf( dllName, maxLength, "lib%s" CPUSTRING ".so", baseName );
#endif
}

sysEvent_t hfSysLocal::GenerateMouseButtonEvent( int button, bool down ) {
	sysEvent_t ev;
	ev.evType = SE_KEY;
	ev.evValue = K_MOUSE1 + button - 1;
	ev.evValue2 = down;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

sysEvent_t hfSysLocal::GenerateMouseMoveEvent( int deltax, int deltay ) {
	sysEvent_t ev;
	ev.evType = SE_MOUSE;
	ev.evValue = deltax;
	ev.evValue2 = deltay;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

void hfSysLocal::FPU_EnableExceptions( int exceptions ) {
	Sys_FPU_EnableExceptions( exceptions );
}

/*
=================
Sys_TimeStampToStr
=================
*/
const char *Sys_TimeStampToStr( HF_TIME_T timeStamp ) {
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	time_t ts = (time_t)timeStamp;
	tm*	time = localtime( &ts );
	if ( time == NULL ) {
		return "??/??/???? ??:??";
	}

	hfStr out;
	
	hfStr lang = cvarSystem->GetCVarString( "sys_lang" );
	if ( lang.Icmp( HF_LANG_ENGLISH ) == 0 ) {
		// english gets "month/day/year  hour:min" + "am" or "pm"
		out = va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += " ";	// changed to spaces since flash doesn't recognize \t
		if ( time->tm_hour > 12 ) {
			out += va( "%02d", time->tm_hour - 12 );
		} else if ( time->tm_hour == 0 ) {
				out += "12";
		} else {
			out += va( "%02d", time->tm_hour );
		}
		out += ":";
		out +=va( "%02d", time->tm_min );
		if ( time->tm_hour >= 12 ) {
			out += "pm";
		} else {
			out += "am";
		}
	} else {
		// europeans get "day/month/year  24hour:min"
		out = va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += " ";	// changed to spaces since flash doesn't recognize \t
		out += va( "%02d", time->tm_hour );
		out += ":";
		out += va( "%02d", time->tm_min );
	}
	hfStr::Copynz( timeString, out, sizeof( timeString ) );

	return timeString;
}

/*
========================
Sys_SecToStr
========================
*/
const char * Sys_SecToStr( int sec ) {
	static char timeString[MAX_STRING_CHARS];

	int weeks = sec / ( 3600 * 24 * 7 );
	sec -= weeks * ( 3600 * 24 * 7 );

	int days = sec / ( 3600 * 24 );
	sec -= days * ( 3600 * 24 );

	int hours = sec / 3600;
	sec -= hours * 3600;

	int min = sec / 60;
	sec -= min * 60;

	if ( weeks > 0 ) {
		sprintf( timeString, "%dw, %dd, %d:%02d:%02d", weeks, days, hours, min, sec );
	} else if ( days > 0 ) {
		sprintf( timeString, "%dd, %d:%02d:%02d", days, hours, min, sec );
	} else {
		sprintf( timeString, "%d:%02d:%02d", hours, min, sec );
	}

	return timeString;
}

// return number of supported languages
int Sys_NumLangs() {
	return numLanguages;
}

// get language name by index
const char * Sys_Lang( int idx ) {
	if ( idx >= 0 && idx < numLanguages ) {
		return sysLanguageNames[ idx ];
	}
	return "";
}

const char * Sys_DefaultLanguage() {
	// sku breakdowns are as follows
	//  EFIGS	Digital
	//  EF  S	North America
	//   FIGS	EU
	//  E		UK
	// JE    	Japan

	// If japanese exists, default to japanese
	// else if english exists, defaults to english
	// otherwise, french

	if ( !fileSystem->UsingResourceFiles() ) {
		return HF_LANG_ENGLISH;
	}

	return HF_LANG_ENGLISH;
}

