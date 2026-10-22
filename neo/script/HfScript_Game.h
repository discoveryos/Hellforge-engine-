/*
===========================================================================

Hellforge Power 1 Engine - hfscript host bindings for "Hellforge Arena"

Bridges the C++ arena simulation (neo/hflib/game/hfArena.h) to hfscript so
that a script can drive the game one tick at a time:

	arena_reset(seed)   -> reset the sim (deterministic for a given seed)
	arena_running()     -> 1 while the game is still playing, else 0
	arena_hp()          -> player hit points
	arena_score()       -> current score
	arena_wave()        -> current wave number
	arena_ticks()       -> ticks elapsed
	arena_nearest()     -> distance to the nearest enemy
	arena_attack()      -> steer toward the nearest enemy
	arena_retreat()     -> steer away from the nearest enemy
	arena_step()        -> advance the simulation one tick

Header-only and engine-free (depends only on HfScript.h), so both the
engine and the standalone test harness can register the same bindings.

===========================================================================
*/
#ifndef __HFSCRIPT_GAME_H__
#define __HFSCRIPT_GAME_H__

#include "../hflib/hfprecompiled.h"
#include "../hflib/game/hfArena.h"
#include "HfScript.h"

namespace hfgame {

static HfArena & Arena() {
	static HfArena arena;
	return arena;
}

static void Host_reset( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Arena().Reset( (int)vm->GetArg( 0 ).AsInt() );
	*out = HfScriptValue::Nil();
	(void)argc;
}

static void Host_running( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().Running() ? 1 : 0 );
	(void)vm; (void)argc;
}

static void Host_hp( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().hitPoints );
	(void)vm; (void)argc;
}

static void Host_score( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().score );
	(void)vm; (void)argc;
}

static void Host_wave( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().wave );
	(void)vm; (void)argc;
}

static void Host_ticks( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().ticks );
	(void)vm; (void)argc;
}

static void Host_nearest( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Float( Arena().NearestDistance() );
	(void)vm; (void)argc;
}

static void Host_attack( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Arena().Attack();
	*out = HfScriptValue::Nil();
	(void)vm; (void)argc;
}

static void Host_retreat( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Arena().Retreat();
	*out = HfScriptValue::Nil();
	(void)vm; (void)argc;
}

static void Host_step( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Arena().Step();
	*out = HfScriptValue::Nil();
	(void)vm; (void)argc;
}

} // namespace hfgame

// Register every arena host function on the global hfscript host table.
static void HfScript_RegisterArenaHosts() {
	HfScript_RegisterHost( "arena_reset",   hfgame::Host_reset );
	HfScript_RegisterHost( "arena_running", hfgame::Host_running );
	HfScript_RegisterHost( "arena_hp",      hfgame::Host_hp );
	HfScript_RegisterHost( "arena_score",   hfgame::Host_score );
	HfScript_RegisterHost( "arena_wave",    hfgame::Host_wave );
	HfScript_RegisterHost( "arena_ticks",   hfgame::Host_ticks );
	HfScript_RegisterHost( "arena_nearest", hfgame::Host_nearest );
	HfScript_RegisterHost( "arena_attack",  hfgame::Host_attack );
	HfScript_RegisterHost( "arena_retreat", hfgame::Host_retreat );
	HfScript_RegisterHost( "arena_step",    hfgame::Host_step );
}

#endif /* !__HFSCRIPT_GAME_H__ */
