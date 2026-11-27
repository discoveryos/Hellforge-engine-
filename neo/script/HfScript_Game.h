/*
==========================================================================

Hellforge Power 1 Engine - hfscript host bindings for "Hellforge Arena"

Bridges the C++ arena simulation (neo/hflib/game/hfArena.h) to hfscript so
that a script can drive the game one tick at a time.  Structured state stays
in C++; scripts only issue and read scalars.

	arena_reset(seed)   -> reset the sim (deterministic for a given seed)
	arena_running()     -> 1 while the game is still playing, else 0
	arena_hp()          -> player hit points
	arena_score()       -> current score
	arena_wave()        -> current wave number
	arena_ticks()       -> ticks elapsed
	arena_nearest()     -> distance to the nearest enemy
	arena_attack()      -> steer toward the nearest enemy (auto-pilot)
	arena_retreat()     -> steer away from the nearest enemy (auto-pilot)
	arena_step()        -> advance the auto-pilot one tick
	arena_set_move/aim/fire/dodge(...) -> build a gameplay command
	arena_step_cmd()    -> advance one tick using the command buffer
	arena_*_kind / ammo / combo / kills / ... -> read gameplay state

Header-only and engine-free (depends only on HfScript.h and hfArenaGame.h),
so both the engine and the standalone test harness register the same hosts.

==========================================================================
*/
#ifndef __HFSCRIPT_GAME_H__
#define __HFSCRIPT_GAME_H__

#include "../hflib/hfprecompiled.h"
#include "../hflib/game/hfArenaGame.h"
#include "HfScript.h"

namespace hfgame {

// Backwards-compatible alias.  The canonical instance lives in the reusable
// game facade so engine code and game code share one arena.
inline HfArena & Arena() {
	return HfArenaGame::Active();
}

inline HfArenaInput & Command() {
	return HfArenaGame::Command();
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

static void Host_setMove( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Command().moveX = vm->GetArg( 0 ).AsFloat();
	Command().moveY = vm->GetArg( 1 ).AsFloat();
	*out = HfScriptValue::Nil();
	(void)argc;
}

static void Host_setAim( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Command().aimX = vm->GetArg( 0 ).AsFloat();
	Command().aimY = vm->GetArg( 1 ).AsFloat();
	*out = HfScriptValue::Nil();
	(void)argc;
}

static void Host_setFire( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Command().fire = vm->GetArg( 0 ).AsInt() != 0;
	*out = HfScriptValue::Nil();
	(void)argc;
}

static void Host_setDodge( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Command().dodge = vm->GetArg( 0 ).AsInt() != 0;
	*out = HfScriptValue::Nil();
	(void)argc;
}

static void Host_clearCommand( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	HfArenaGame::ResetCommand();
	*out = HfScriptValue::Nil();
	(void)vm; (void)argc;
}

static void Host_stepCmd( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	Arena().Step( Command() );
	// Dodge is a one-shot intent; a held script flag must re-arm it.
	Command().dodge = false;
	*out = HfScriptValue::Nil();
	(void)vm; (void)argc;
}

static void Host_ammo( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().ammo );
	(void)vm; (void)argc;
}

static void Host_combo( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().combo );
	(void)vm; (void)argc;
}

static void Host_bestCombo( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().bestCombo );
	(void)vm; (void)argc;
}

static void Host_kills( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().kills );
	(void)vm; (void)argc;
}

static void Host_dodgeCooldown( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().dodgeCooldown );
	(void)vm; (void)argc;
}

static void Host_enemyCount( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().enemyCount );
	(void)vm; (void)argc;
}

static void Host_enemyKind( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	const int idx = (int)vm->GetArg( 0 ).AsInt();
	*out = HfScriptValue::Int( idx >= 0 && idx < Arena().enemyCount ? Arena().enemies[ idx ].kind : -1 );
	(void)argc;
}

static void Host_pickupCount( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Int( Arena().pickupCount );
	(void)vm; (void)argc;
}

static void Host_pickupKind( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	const int idx = (int)vm->GetArg( 0 ).AsInt();
	*out = HfScriptValue::Int( idx >= 0 && idx < Arena().pickupCount ? Arena().pickups[ idx ].kind : -1 );
	(void)argc;
}

static void Host_facingX( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Float( Arena().facingX );
	(void)vm; (void)argc;
}

static void Host_facingY( HfScriptVM * vm, int argc, HfScriptValue * out ) {
	*out = HfScriptValue::Float( Arena().facingY );
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

	HfScript_RegisterHost( "arena_set_move",  hfgame::Host_setMove );
	HfScript_RegisterHost( "arena_set_aim",   hfgame::Host_setAim );
	HfScript_RegisterHost( "arena_set_fire",  hfgame::Host_setFire );
	HfScript_RegisterHost( "arena_set_dodge", hfgame::Host_setDodge );
	HfScript_RegisterHost( "arena_clear_command", hfgame::Host_clearCommand );
	HfScript_RegisterHost( "arena_step_cmd",  hfgame::Host_stepCmd );

	HfScript_RegisterHost( "arena_ammo",           hfgame::Host_ammo );
	HfScript_RegisterHost( "arena_combo",          hfgame::Host_combo );
	HfScript_RegisterHost( "arena_best_combo",     hfgame::Host_bestCombo );
	HfScript_RegisterHost( "arena_kills",          hfgame::Host_kills );
	HfScript_RegisterHost( "arena_dodge_cooldown", hfgame::Host_dodgeCooldown );
	HfScript_RegisterHost( "arena_enemy_count",    hfgame::Host_enemyCount );
	HfScript_RegisterHost( "arena_enemy_kind",     hfgame::Host_enemyKind );
	HfScript_RegisterHost( "arena_pickup_count",   hfgame::Host_pickupCount );
	HfScript_RegisterHost( "arena_pickup_kind",    hfgame::Host_pickupKind );
	HfScript_RegisterHost( "arena_facing_x",       hfgame::Host_facingX );
	HfScript_RegisterHost( "arena_facing_y",       hfgame::Host_facingY );
}

#endif /* !__HFSCRIPT_GAME_H__ */
