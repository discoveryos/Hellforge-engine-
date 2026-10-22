/*
===========================================================================

Hellforge Power 1 Engine - "Hellforge Arena" sample game simulation

A small, fully deterministic top-down arena used to demonstrate driving a
game from hfscript.  The C++ side owns all structured state (players,
enemies, bullets, physics) and exposes it to scripts through scalar host
functions; the script owns the per-tick strategy.

Keeping every piece of game state behind scalar accessors is deliberate:
hfscript values are scalars only (no arrays/objects), so the engine - not
the script - is the natural home for the data.

The simulation is pure: given the same seed, the same sequence of script
decisions produces bit-identical score/ticks on the same binary.

===========================================================================
*/
#ifndef __HF_ARENA_H__
#define __HF_ARENA_H__

#include <math.h>

struct HfArenaEnemy {
	float	x;
	float	y;
	int		hp;
};

struct HfArenaBullet {
	float	x;
	float	y;
	float	vx;
	float	vy;
	int		life;
};

struct HfArena {
	enum {
		MAX_ENEMIES		= 32,
		MAX_BULLETS		= 64,
		TOTAL_WAVES		= 5,
		TICK_LIMIT		= 4000,
		PLAYER_HITPOINTS= 100,
		PLAYER_HIT_DAMAGE	= 10,
	};

	float	playerX;
	float	playerY;
	int		hitPoints;
	int		score;
	int		wave;
	int		ticks;
	bool	gameOver;
	bool	won;

	int		enemyCount;
	int		bulletCount;
	int		fireCooldown;
	int		intent;			// -1 retreat, 0 idle, +1 attack

	HfArenaEnemy	enemies[ MAX_ENEMIES ];
	HfArenaBullet	bullets[ MAX_BULLETS ];

	unsigned int	rng;

	static float	Dist( float ax, float ay, float bx, float by ) {
		float dx = ax - bx;
		float dy = ay - by;
		return sqrtf( dx * dx + dy * dy );
	}

	// Simple LCG: deterministic and stable across platforms (integer math).
	int		NextRandom() {
		rng = rng * 1664525u + 1013904223u;
		return (int)( ( rng >> 8 ) & 0x7fffff );
	}

	bool	Running() const { return !gameOver; }

	void	Reset( int seed ) {
		playerX = 0.0f;
		playerY = 0.0f;
		hitPoints = PLAYER_HITPOINTS;
		score = 0;
		wave = 0;
		ticks = 0;
		gameOver = false;
		won = false;
		enemyCount = 0;
		bulletCount = 0;
		fireCooldown = 0;
		intent = 0;
		rng = (unsigned int)seed;
		if ( rng == 0 ) {
			rng = 1u;
		}
		SpawnWave();
	}

	void	SpawnWave() {
		wave++;
		int count = 2 + wave;
		if ( count > MAX_ENEMIES ) {
			count = MAX_ENEMIES;
		}
		for ( int i = 0; i < count; i++ ) {
			float angle = 6.28318531f * ( (float)i + 0.5f * (float)wave ) / (float)count;
			float radius = 60.0f + (float)( NextRandom() % 40 );
			enemies[ i ].x = cosf( angle ) * radius;
			enemies[ i ].y = sinf( angle ) * radius;
			enemies[ i ].hp = 1 + wave / 2;
		}
		enemyCount = count;
	}

	int		NearestEnemy() const {
		int best = -1;
		float bestDist = 1.0e30f;
		for ( int i = 0; i < enemyCount; i++ ) {
			float d = Dist( playerX, playerY, enemies[ i ].x, enemies[ i ].y );
			if ( d < bestDist ) {
				bestDist = d;
				best = i;
			}
		}
		return best;
	}

	float	NearestDistance() const {
		int idx = NearestEnemy();
		if ( idx < 0 ) {
			return 0.0f;
		}
		return Dist( playerX, playerY, enemies[ idx ].x, enemies[ idx ].y );
	}

	// Strategy hooks called from scripts.  Firing happens automatically every
	// tick in Step(); "intent" only steers the player.
	void	Attack()  { intent = 1; }
	void	Retreat() { intent = -1; }

	void	Step() {
		if ( gameOver ) {
			return;
		}
		ticks++;
		if ( ticks > TICK_LIMIT ) {
			gameOver = true;
			won = false;
			return;
		}
		if ( fireCooldown > 0 ) {
			fireCooldown--;
		}

		int idx = NearestEnemy();

		// steering
		if ( idx >= 0 && intent != 0 ) {
			float dx = enemies[ idx ].x - playerX;
			float dy = enemies[ idx ].y - playerY;
			float d = sqrtf( dx * dx + dy * dy );
			if ( d < 0.001f ) {
				d = 0.001f;
			}
			float s = ( intent > 0 ) ? 3.0f : -3.0f;
			playerX += dx / d * s;
			playerY += dy / d * s;
			if ( playerX >  120.0f ) playerX =  120.0f;
			if ( playerX < -120.0f ) playerX = -120.0f;
			if ( playerY >  120.0f ) playerY =  120.0f;
			if ( playerY < -120.0f ) playerY = -120.0f;
		}

		// automatic fire toward the nearest enemy
		if ( idx >= 0 && fireCooldown == 0 && bulletCount < MAX_BULLETS ) {
			float dx = enemies[ idx ].x - playerX;
			float dy = enemies[ idx ].y - playerY;
			float d = sqrtf( dx * dx + dy * dy );
			if ( d < 0.001f ) {
				d = 0.001f;
			}
			bullets[ bulletCount ].x = playerX;
			bullets[ bulletCount ].y = playerY;
			bullets[ bulletCount ].vx = dx / d * 12.0f;
			bullets[ bulletCount ].vy = dy / d * 12.0f;
			bullets[ bulletCount ].life = 60;
			bulletCount++;
			fireCooldown = 4;
		}

		// integrate bullets
		for ( int i = 0; i < bulletCount; i++ ) {
			bullets[ i ].x += bullets[ i ].vx;
			bullets[ i ].y += bullets[ i ].vy;
			bullets[ i ].life--;
		}

		// enemies chase; contact damages the player and destroys the enemy
		for ( int i = 0; i < enemyCount; i++ ) {
			float dx = playerX - enemies[ i ].x;
			float dy = playerY - enemies[ i ].y;
			float d = sqrtf( dx * dx + dy * dy );
			if ( d < 0.001f ) {
				d = 0.001f;
			}
			enemies[ i ].x += dx / d * 1.5f;
			enemies[ i ].y += dy / d * 1.5f;
			if ( Dist( playerX, playerY, enemies[ i ].x, enemies[ i ].y ) < 2.5f ) {
				hitPoints -= PLAYER_HIT_DAMAGE;
				enemies[ i ].hp = 0;
			}
		}

		// bullet/enemy collisions
		for ( int b = 0; b < bulletCount; b++ ) {
			for ( int e = 0; e < enemyCount; e++ ) {
				if ( enemies[ e ].hp <= 0 ) {
					continue;
				}
				if ( Dist( bullets[ b ].x, bullets[ b ].y, enemies[ e ].x, enemies[ e ].y ) < 3.0f ) {
					enemies[ e ].hp--;
					bullets[ b ].life = 0;
					if ( enemies[ e ].hp <= 0 ) {
						score += 10;
					}
					break;
				}
			}
		}

		// reap spent bullets (also drop any that leave the arena)
		{
			int w = 0;
			for ( int i = 0; i < bulletCount; i++ ) {
				if ( bullets[ i ].life > 0
					&& bullets[ i ].x > -200.0f && bullets[ i ].x < 200.0f
					&& bullets[ i ].y > -200.0f && bullets[ i ].y < 200.0f ) {
					bullets[ w++ ] = bullets[ i ];
				}
			}
			bulletCount = w;
		}

		// reap dead enemies
		{
			int w = 0;
			for ( int i = 0; i < enemyCount; i++ ) {
				if ( enemies[ i ].hp > 0 ) {
					enemies[ w++ ] = enemies[ i ];
				}
			}
			enemyCount = w;
		}

		if ( hitPoints <= 0 ) {
			hitPoints = 0;
			gameOver = true;
			won = false;
			return;
		}
		if ( enemyCount == 0 ) {
			if ( wave >= TOTAL_WAVES ) {
				gameOver = true;
				won = true;
			} else {
				SpawnWave();
			}
		}
	}
};

#endif /* !__HF_ARENA_H__ */
