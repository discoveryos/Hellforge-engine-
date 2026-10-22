// Minimal platform shim for the standalone assettool binary. The tool links
// only hfStr.cpp + hfAssetStore.cpp (not the full hflib, which references the
// engine's cvarSystem/common/fileSystem). Those two files pull in the small
// memory/assert primitives below, so provide self-contained versions here.

#include "../hflib/hfprecompiled.h"
#include "../framework/hfCmdSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if 0
// declarations from hfsys_assert.h (not included by hfprecompiled.h)
#endif

bool AssertFailed( const char *file, int line, const char *expression ) {
	fprintf( stderr, "ASSERT FAILED: %s (%s:%d)\n", expression, file, line );
	return false;
}

void Mem_Free16( void *ptr ) {
	free( ptr );
}

void *Mem_Alloc16( const int size, const memTag_t tag ) {
	(void)tag;
	void *p = NULL;
	if ( posix_memalign( &p, 16, size ? size : 1 ) != 0 ) { return NULL; }
	return p;
}

void *Mem_ClearedAlloc( const int size, const memTag_t tag ) {
	void *p = Mem_Alloc16( size, tag );
	if ( p ) { memset( p, 0, size ); }
	return p;
}

// hfStr.cpp logs through hfLib::Printf and its global colour/command-link
// statics reference the hfCommandLink ctor; provide no-op stand-ins.
void hfLib::Printf( const char *fmt, ... ) {
	va_list ap;
	va_start( ap, fmt );
	vfprintf( stdout, fmt, ap );
	va_end( ap );
}

hfCommandLink::hfCommandLink( const char *cmdName, cmdFunction_t function,
		const char *description, argCompletion_t argCompletion )
	: next( NULL ), cmdName_( cmdName ), function_( function ), description_( description ),
	  argCompletion_( argCompletion ) {
}

// hfStr.cpp's testStrId_f command touches the localization dictionary; stub it.
void hfStrId::Set( const char *key ) { (void)key; }
const char *hfStrId::GetLocalizedString() const { return ""; }

// Static data members of the hfLib facade. The tool never drives the engine's
// systems, so they stay NULL here.
hfSys *hfLib::sys = NULL;
hfCommon *hfLib::common = NULL;
hfCVarSystem *hfLib::cvarSystem = NULL;
hfFileSystem *hfLib::fileSystem = NULL;
int hfLib::frameNumber = 0;
bool hfLib::mainThreadInitialized = false;
HF_TLS hfLib::isMainThread = NULL;

const float hfMath::INFINITY = 1e30f;
static const int HF_TOOL_SMALLEST_NON_DENORMAL = 1 << 23;
const float hfMath::FLT_SMALLEST_NON_DENORMAL =
	*reinterpret_cast< const float * >( &HF_TOOL_SMALLEST_NON_DENORMAL );