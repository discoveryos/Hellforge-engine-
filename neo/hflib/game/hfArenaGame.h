#ifndef __HF_ARENA_GAME_H__
#define __HF_ARENA_GAME_H__

/*
==============================================================================

	hfArenaGame — game-facing facade for the twin-stick arena.

	A future game module should not need to know how the frontend, hfscript
	host table or save widget layer are wired.  This header gives game code a
	small, stable surface:

		HfArenaGame::Start( seed )
		HfArenaGame::Step( command )
		HfArenaGame::Active()
		HfArenaGame::Run( command, steps )

	Persistence uses hfArenaSave directly with Active() as the payload source.

	The facade owns the engine's default arena and command buffer.  A game that
	wants to own its state can still instantiate HfArena directly; every API
	below works with any HfArena reference.

==============================================================================
*/

#include "hfArena.h"

struct HfArenaGame {
	// One shared instance for the engine host, hfscript hosts and console
	// commands.  Inline function-local state avoids per-translation-unit
	// copies in a header-only engine.
	static HfArena & Active() {
		static HfArena arena;
		return arena;
	}

	static HfArenaInput & Command() {
		static HfArenaInput command;
		return command;
	}

	static void ResetCommand() {
		Command().Clear();
	}

	static void Start( int seed ) {
		Active().Reset( seed );
		ResetCommand();
	}

	static void Step( const HfArenaInput &command ) {
		Active().Step( command );
	}

	static void StepActive() {
		Active().Step( Command() );
	}

	static void StepAuto() {
		Active().Step();
	}

	// Scripted replay helper: applies the same command N times.  Dodge is a
	// one-shot intent after its cooldown starts, so clear it after the first
	// step to keep replays honest.
	static void Run( const HfArenaInput &command, int steps ) {
		HfArenaInput cmd = command;
		for ( int i = 0; i < steps && Active().Running(); i++ ) {
			Step( cmd );
			cmd.dodge = false;
		}
	}

	static bool Running()  { return Active().Running(); }
	static int	Score()    { return Active().score; }
	static int	Wave()     { return Active().wave; }
	static int	Kills()    { return Active().kills; }
	static int	Ammo()     { return Active().ammo; }
};

#endif /* !__HF_ARENA_GAME_H__ */
