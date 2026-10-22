#ifndef HFCONFIG_H
#define HFCONFIG_H 1

#define HF_VERSION 0x01000600
#define HF_VERSION_STRING "1.0.6"

#define HF_API
#define HF_CDECL
#define HF_NO_RETURN

// NOTE: HF_INLINE / HF_FORCE_INLINE are provided by sys/hfsys_defines.h,
// which is the canonical definition for this engine. They are only defined
// here as a fallback so that this header can be included standalone.
#ifndef HF_INLINE
#define HF_INLINE inline
#endif

#ifndef HF_FORCE_INLINE
#define HF_FORCE_INLINE inline
#endif

/*
===============================================================================
	ASSERT_ENUM_STRING( value, expected ) - compile time check that an enum
	value matches an expected constant, while expanding to a valid array
	element. Used to build name tables that must stay in sync with their enum.
===============================================================================
*/
#define ASSERT_ENUM_STRING( x, y ) ( ( ( x ) == ( y ) ) ? "" : __FILE__ ":" #x " != " #y, "" )

#endif
