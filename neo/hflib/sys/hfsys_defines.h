#ifndef SYS_DEFINES_H
#define SYS_DEFINES_H

#include <stdint.h>

/*
================================================================================================

	Platform Specific HF_ Defines

	The HF_ defines are the only platform defines we should be using.

================================================================================================
*/

#undef HF_PC
#undef HF_PC_WIN
#undef HF_PC_WIN64
#undef HF_CONSOLE
#undef HF_WIN32
#undef HF_LITTLE_ENDIAN
#undef HF_LINUX
#undef HF_UNIX

#if defined(_WIN32)
	// _WIN32 always defined
	// _WIN64 also defined for x64 target

	#define HF_PC
	#define HF_PC_WIN
	#define HF_WIN32
	#define HF_LITTLE_ENDIAN
#elif defined(__linux__) || defined(__linux)
	#define HF_PC
	#define HF_LINUX
	#define HF_UNIX
	#define HF_LITTLE_ENDIAN
	// enable SSE/SSE2 intrinsics on Linux x86
	#if defined(__i386__)
		#define HF_WIN_X86_SSE_INTRIN
		#define HF_WIN_X86_SSE2_INTRIN
	#elif defined(__x86_64__)
		#define HF_WIN_X86_SSE_INTRIN
		#define HF_WIN_X86_SSE2_INTRIN
		#define HF_WIN_X86_SSE3_INTRIN
	#endif
#else
#error Unknown Platform
#endif

#define HF_OPENGL

/*
================================================================================

	PC Windows

================================================================================
*/

#ifdef HF_PC_WIN

#define	CPUSTRING						"x86"

#define	BUILD_STRING					"win-" CPUSTRING
#define BUILD_OS_ID						0

#define ALIGN16( x )					__declspec(align(16)) x
#define ALIGNTYPE16						__declspec(align(16))
#define ALIGNTYPE128					__declspec(align(128))
#define FORMAT_PRINTF( x )

#define PATHSEPARATOR_STR				"\\"
#define PATHSEPARATOR_CHAR				'\\'
#define NEWLINE							"\r\n"

#define HF_INLINE						inline
#define HF_FORCE_INLINE					__forceinline

// lint complains that extern used with definition is a hazard, but it
// has the benefit (?) of making it illegal to take the address of the function
#ifdef _lint
#define HF_INLINE_EXTERN				inline
#define HF_FORCE_INLINE_EXTERN			__forceinline
#else
#define HF_INLINE_EXTERN				extern inline
#define HF_FORCE_INLINE_EXTERN			extern __forceinline
#endif

// we should never rely on this define in our code. this is here so dodgy external libraries don't get confused
#ifndef WIN32
	#define WIN32
#endif

#endif

/*
================================================================================

	PC Linux

================================================================================
*/

#ifdef HF_LINUX

#define	CPUSTRING						"x86"

#define	BUILD_STRING					"linux-" CPUSTRING
#define BUILD_OS_ID						0

#define ALIGN16( x )					__attribute__((aligned(16))) x
#define ALIGNTYPE16						__attribute__((aligned(16)))
#define ALIGNTYPE128					__attribute__((aligned(128)))
#define FORMAT_PRINTF( x )				__attribute__((format(printf, x, 0)))

#define PATHSEPARATOR_STR				"/"
#define PATHSEPARATOR_CHAR				'/'
#define NEWLINE							"\n"

#define HF_INLINE						inline
#define HF_FORCE_INLINE					inline __attribute__((always_inline))

#ifdef _lint
#define HF_INLINE_EXTERN				inline
#define HF_FORCE_INLINE_EXTERN			inline __attribute__((always_inline))
#else
#define HF_INLINE_EXTERN				inline
#define HF_FORCE_INLINE_EXTERN		inline __attribute__((always_inline))
#endif

#define __forceinline					__attribute__((always_inline))

#endif

/*
================================================================================================

Defines and macros usable in all code

================================================================================================
*/

#define ALIGN( x, a ) ( ( ( x ) + ((a)-1) ) & ~((a)-1) )

#ifdef HF_LINUX
#include <alloca.h>
#define _alloca( x )		alloca( x )
#define UINT_PTR			uintptr_t
#define _alloca16( x )		((void *)ALIGN( (uintptr_t)alloca( ALIGN( x, 16 ) + 16 ), 16 ) )
#define _alloca128( x )		((void *)ALIGN( (uintptr_t)alloca( ALIGN( x, 128 ) + 128 ), 128 ) )
#else
#define _alloca16( x )					((void *)ALIGN( (UINT_PTR)_alloca( ALIGN( x, 16 ) + 16 ), 16 ) )
#define _alloca128( x )					((void *)ALIGN( (UINT_PTR)_alloca( ALIGN( x, 128 ) + 128 ), 128 ) )
#endif

#define likely( x )	( x )
#define unlikely( x )	( x )

// A macro to disallow the copy constructor and operator= functions
// NOTE: The macro contains "private:" so all members defined after it will be private until
// public: or protected: is specified.
#define DISALLOW_COPY_AND_ASSIGN(TypeName)	\
private:									\
  TypeName(const TypeName&);				\
  void operator=(const TypeName&);


/*
================================================================================================
Setup for /analyze code analysis, which we currently only have on the 360, but
we may get later for win32 if we buy the higher end vc++ licenses.

Even with VS2010 ultmate, /analyze only works for x86, not x64

Also note the __analysis_assume macro in hfsys_assert.h relates to code analysis.

This header should be included even by job code that doesn't reference the
bulk of the codebase, so it is the best place for analyze pragmas.
================================================================================================
*/

#if defined( HF_WIN32 )

// disable some /analyze warnings here
#pragma warning( disable: 6255 )	// warning C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)
#pragma warning( disable: 6262 )	// warning C6262: Function uses '36924' bytes of stack: exceeds /analyze:stacksize'32768'. Consider moving some data to heap
#pragma warning( disable: 6326 )	// warning C6326: Potential comparison of a constant with another constant

#pragma warning( disable: 6031 )	//  warning C6031: Return value ignored
// this warning fires whenever you have two calls to new in a function, but we assume new never fails, so it is not relevant for us
#pragma warning( disable: 6211 )	// warning C6211: Leaking memory 'staticModel' due to an exception. Consider using a local catch block to clean up memory

// we want to fix all these at some point...
#pragma warning( disable: 6246 )	// warning C6246: Local declaration of 'es' hides declaration of the same name in outer scope. For additional information, see previous declaration at line '969' of 'w:\tech5\game\ai\fsm\fsm_combat.cpp': Lines: 969
#pragma warning( disable: 6244 )	// warning C6244: Local declaration of 'viewList' hides previous declaration at line '67' of 'w:\tech5\engine\renderer\rendertools.cpp'

// win32 needs this, but 360 doesn't
#pragma warning( disable: 6540 )	// warning C6540: The use of attribute annotations on this function will invalidate all of its existing __declspec annotations [D:\tech5\engine\engine-10.vcxproj]


// checking format strings catches a LOT of errors
#include <CodeAnalysis\SourceAnnotations.h>
#define	VERIFY_FORMAT_STRING	[SA_FormatString(Style="printf")]


// We need to inform the compiler that Error() and FatalError() will
// never return, so any conditions that leeds to them being called are
// guaranteed to be false in the following code
#define NO_RETURN __declspec(noreturn)

#elif defined( HF_LINUX )

#define	VERIFY_FORMAT_STRING	__attribute__((format(printf, 1, 2)))
#define NO_RETURN __attribute__((noreturn))

#endif

// I don't want to disable "warning C6031: Return value ignored" from /analyze
// but there are several cases with sprintf where we pre-initialized the variables
// being scanned into, so we truly don't care if they weren't all scanned.
// Rather than littering #pragma statements around these cases, we can assign the
// return value to this, which means we have considered the issue and decided that
// it doesn't require action.
// The volatile qualifier is to prevent:PVS-Studio warnings like:
// False	2	4214	V519	The 'ignoredReturnValue' object is assigned values twice successively. Perhaps this is a mistake. Check lines: 545, 547.	Rage	collisionmodelmanager_debug.cpp	547	False
extern volatile int ignoredReturnValue;

#define MAX_TYPE( x )			( ( ( ( 1 << ( ( sizeof( x ) - 1 ) * 8 - 1 ) ) - 1 ) << 8 ) | 255 )
#define MIN_TYPE( x )			( - MAX_TYPE( x ) - 1 )
#define MAX_UNSIGNED_TYPE( x )	( ( ( ( 1U << ( ( sizeof( x ) - 1 ) * 8 ) ) - 1 ) << 8 ) | 255U )
#define MIN_UNSIGNED_TYPE( x )	0

#endif

