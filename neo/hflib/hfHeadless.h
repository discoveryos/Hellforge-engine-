#ifndef __HF_HEADLESS_H__
#define __HF_HEADLESS_H__

/*
===============================================================================

	Headless mode.

	Godot's `--headless` starts the engine with no window, no GPU and no audio
	device while still running the full simulation; this is the same flag.

	The state is deliberately process-wide and lives in hflib rather than in a
	component or a cvar, because it has to be answered before any subsystem
	exists: the Linux main() reads it before it opens the X11 display, and the
	simulation server reads it before it decides whether it needs anything from
	the front end at all.

	Recognised spellings, case insensitive, anywhere in argv:

		--headless   -headless   +headless

	Mirrors the engine's own command line conventions: `+` for console lines,
	`--` and `-` for switches.

===============================================================================
*/

#include "hfStr.h"

namespace HF_Lib {

	HF_INLINE bool IsHeadlessArgument( const char *arg ) {
		if ( !arg || !arg[0] ) { return false; }
		if ( arg[0] == '-' && arg[1] == '-' ) { arg++; }		// --headless
		if ( arg[0] != '-' && arg[0] != '+' ) { return false; }
		return hfStr::Icmp( arg + 1, "headless" ) == 0;
	}

	// Function local static on an inline function: one instance for the whole
	// program, initialized thread safely.
	HF_INLINE bool &HeadlessState() {
		static bool s_headless = false;
		return s_headless;
	}

	HF_INLINE void SetHeadless( bool on ) { HeadlessState() = on; }
	HF_INLINE bool IsHeadless() { return HeadlessState(); }

	// Parses argv, sets the flag if the switch is present and returns whether
	// this run is headless. Safe to call more than once; it only ever turns the
	// flag on.
	HF_INLINE bool ParseHeadlessCommand( int argc, const char * const *argv ) {
		for ( int i = 0; i < argc; i++ ) {
			if ( IsHeadlessArgument( argv[i] ) ) { HeadlessState() = true; return true; }
		}
		return HeadlessState();
	}

} // namespace HF_Lib

#endif
