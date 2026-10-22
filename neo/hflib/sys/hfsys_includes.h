#ifndef SYS_INCLUDES_H
#define SYS_INCLUDES_H

// Include the various platform specific header files (windows.h, etc)

/*
================================================================================================

	Windows

================================================================================================
*/


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion

#ifndef _D3SDK
#ifndef GAME_DLL

#if defined(_WIN32)

#define WINVER				0x501

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#define DIRECTINPUT_VERSION  0x0800			// was 0x0700 with the old mssdk
#define DIRECTSOUND_VERSION  0x0800

#include <dsound.h>
#include <dinput.h>

#endif /* _WIN32 */

#endif /* !GAME_DLL */
#endif /* !_D3SDK */

#if defined(_WIN32)
#include <intrin.h>			// needed for intrinsics like _mm_setzero_si28

#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4127)				// conditional expression is constant
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined
#pragma warning(disable : 4996)				// unsafe string operations

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for hfqgl.h
#undef FindText								// fix namespace pollution
#elif defined(HF_LINUX)
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>

#if defined(__SSE__) || defined(__SSE2__)
#include <emmintrin.h>
#include <xmmintrin.h>
#define __m128c __m128
#endif

#endif /* _WIN32 */

/*
================================================================================================

	Common Include Files

================================================================================================
*/

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <memory>

//-----------------------------------------------------

// Hacked stuff we may want to consider implementing later
class hfScopedGlobalHeap {
};

#endif // SYS_INCLUDES_H

