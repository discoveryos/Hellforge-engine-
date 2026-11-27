/*
===========================================================================

Hellforge Power 1 Engine - "Hellforge Arena" game simulation

A small, fully deterministic top-down arena.  The C++ side owns all structured
state (player, enemies, bullets, pickups, physics) and exposes it to scripts
and to game code through scalar host functions and the HfArenaGame facade.

Keeping game state behind a plain POD and scalar accessors is deliberate:
hfscript values are scalars only, so the engine - not the script - is the
natural home for the data.  A real game module can include this header (or
hfArenaGame.h) and drive the same simulation without rewriting engine code.

The simulation is pure: given the same seed and the same sequence of commands,
it produces bit-identical state on the same binary.

===========================================================================
*/
#ifndef __HF_ARENA_H__
#define __HF_ARENA_H__

#include <math.h>

// Enemy archetypes.  The integer is serialized in saves and exposed to
// hfscript through arena_enemy_kind(), so append new values only.
enum hfArenaEnemyKind {
	HF_ARENA_ENEMY_CHASER	= 0,
	HF_ARENA_ENEMY_SPITTER	= 1,
	HF_ARENA_ENEMY_BRUTE	= 2,
	HF_ARENA_ENEMY_BOSS		= 3,
	HF_ARENA_ENEMY_KIND_COUNT
};

// Pickup archetypes.  Also serialized and script-visible.
enum hfArenaPickupKind {
	HF_ARENA_PICKUP_AMMO	= 0,
	HF_ARENA_PICKUP_HEALTH	= 1,
	HF_ARENA_PICKUP_SCORE	= 2,
	HF_ARENA_PICKUP_KIND_COUNT
};

// Bullet owner.  Enemy bullets are introduced by spitter/brute/boss attacks
// in command-driven play; legacy auto-pilot only emits player bullets.
enum hfArenaBulletTeam {
	HF_ARENA_BULLET_PLAYER	= 0,
	HF_ARENA_BULLET_ENEMY	= 1
};

struct HfArenaEnemy {
	float	x;
	float	y;
	int		hp;
	int		kind;			// hfArenaEnemyKind
	int		fireCooldown;	// spitter/boss ranged attack timer
};

struct HfArenaBullet {
	float	x;
	float	y;
	float	vx;
	float	vy;
	int		life;
	int		team;			// hfArenaBulletTeam
};

struct HfArenaPickup {
	float	x;
	float	y;
	int		kind;			// hfArenaPickupKind
	int		life;
};

// One frame of intent from the host.  Kept as plain data so it can travel
// through the UI, hfscript or a deterministic replay without owning any
// platform input state.  The sim consumes a command; it never stores one.
struct HfArenaInput {
	float	moveX;		// -1..1
	float	moveY;		// -1..1
	float	aimX;		// direction to aim target (need not be normalized)
	float	aimY;
	bool	fire;
	bool	dodge;

	HF_INLINE void Clear() {
		moveX = moveY = 0.0f;
		aimX = 0.0f; aimY = -1.0f;
		fire = false;
		dodge = false;
	}

	HF_INLINE HfArenaInput() { Clear(); }
};

struct HfArena {
	enum {
		MAX_ENEMIES			= 32,
		MAX_BULLETS			= 64,
		MAX_PICKUPS			= 8,
		TOTAL_WAVES			= 5,
		BOSS_WAVE			= 5,
		TICK_LIMIT			= 4000,
		PLAYER_HITPOINTS	= 100,
		PLAYER_HIT_DAMAGE	= 10,
		MAX_AMMO			= 12,
		DODGE_COOLDOWN		= 45,
		DODGE_IFRAMES		= 6,
		COMBO_TIMEOUT		= 90,
		WAVE_DELAY			= 45
	};

	float	playerX;
	float	playerY;
	float	facingX;
	float	facingY;
	int		hitPoints;
	int		score;
	int		wave;
	int		ticks;
	bool	gameOver;
	bool	won;

	int		enemyCount;
	int		bulletCount;
	int		pickupCount;
	int		fireCooldown;
	int		ammo;
	int		dodgeCooldown;
	int		dodgeIFrames;
	int		rapidTicks;
	int		combo;
	int		bestCombo;
	int		comboTimer;
	int		kills;
	int		waveDelay;
	int		intent;			// -1 retreat, 0 idle, +1 attack (auto-pilot only)

	HfArenaEnemy	enemies[ MAX_ENEMIES ];
	HfArenaBullet	bullets[ MAX_BULLETS ];
	HfArenaPickup	pickups[ MAX_PICKUPS ];

	unsigned int	rng;

	static float	Dist( float ax, float ay, float bx, float by ) {
		float dx = ax - bx;
		float dy = ay - by;
		return sqrtf( dx * dx + dy * dy );
	}

	static float	PlayerSpeed()	{ return 3.0f; }
	static float	DodgeSpeed()	{ return 7.0f; }
	static float	BulletSpeed()	{ return 12.0f; }
	static float	EnemyBulletSpeed() { return 5.0f; }
	static float	PlayerRadius()	{ return 2.5f; }

	static void	Normalize( float &x, float &y ) {
		const float len = sqrtf( x * x + y * y );
		if ( len < 0.001f ) {
			x = 0.0f;
			y = -1.0f;
			return;
		}
		x /= len;
		y /= len;
	}

	// Simple LCG: deterministic and stable across platforms (integer math).
	int		NextRandom() {
		rng = rng * 1664525u + 1013904223u;
		return (int)( ( rng >> 8 ) & 0x7fffff );
	}

	bool	Running() const { return !gameOver; }

	float	EnemySpeed( int kind ) const {
		switch ( kind ) {
			case HF_ARENA_ENEMY_SPITTER:	return 1.1f;
			case HF_ARENA_ENEMY_BRUTE:		return 0.8f;
			case HF_ARENA_ENEMY_BOSS:		return 0.65f;
			default:						return 1.5f;
		}
	}

	int		EnemyDamage( int kind ) const {
		switch ( kind ) {
			case HF_ARENA_ENEMY_BRUTE:	return 20;
			case HF_ARENA_ENEMY_BOSS:	return 25;
			case HF_ARENA_ENEMY_SPITTER:return 8;
			default:					return PLAYER_HIT_DAMAGE;
		}
	}

	int		EnemyScore( int kind ) const {
		switch ( kind ) {
			case HF_ARENA_ENEMY_SPITTER:return 25;
			case HF_ARENA_ENEMY_BRUTE:	return 50;
			case HF_ARENA_ENEMY_BOSS:	return 250;
			default:					return 10;
		}
	}

	float	EnemyRadius( int kind ) const {
		switch ( kind ) {
			case HF_ARENA_ENEMY_BRUTE:	return 4.5f;
			case HF_ARENA_ENEMY_BOSS:	return 7.0f;
			default:					return 2.5f;
		}
	}

	int		ComboMultiplier() const {
		int m = 1 + combo / 5;
		return m > 4 ? 4 : m;
	}

	void	Reset( int seed ) {
		playerX = 0.0f;
		playerY = 0.0f;
		facingX = 0.0f;
		facingY = -1.0f;
		hitPoints = PLAYER_HITPOINTS;
		score = 0;
		wave = 0;
		ticks = 0;
		gameOver = false;
		won = false;
		enemyCount = 0;
		bulletCount = 0;
		pickupCount = 0;
		fireCooldown = 0;
		ammo = MAX_AMMO;
		dodgeCooldown = 0;
		dodgeIFrames = 0;
		rapidTicks = 0;
		combo = 0;
		bestCombo = 0;
		comboTimer = 0;
		kills = 0;
		waveDelay = 0;
		intent = 0;
		memset( enemies, 0, sizeof( enemies ) );
		memset( bullets, 0, sizeof( bullets ) );
		memset( pickups, 0, sizeof( pickups ) );
		rng = (unsigned int)seed;
		if ( rng == 0 ) {
			rng = 1u;
		}
		SpawnWave();
	}

	void	SpawnWave() {
		wave++;
		waveDelay = 0;
		pickupCount = 0;

		const bool bossWave = ( wave % BOSS_WAVE ) == 0;
		int count = bossWave ? 5 : 2 + wave;
		if ( count > MAX_ENEMIES ) {
			count = MAX_ENEMIES;
		}
		for ( int i = 0; i < count; i++ ) {
			const float angle = 6.28318531f * ( (float)i + 0.5f * (float)wave ) / (float)count;
			const float radius = 60.0f + (float)( NextRandom() % 40 );
			int kind = HF_ARENA_ENEMY_CHASER;
			if ( bossWave ) {
				kind = ( i == 0 ) ? HF_ARENA_ENEMY_BOSS
					: ( i <= 2 ) ? HF_ARENA_ENEMY_BRUTE : HF_ARENA_ENEMY_CHASER;
			} else {
				const int roll = NextRandom() % 100;
				if ( wave >= 3 && roll < 20 ) {
					kind = HF_ARENA_ENEMY_BRUTE;
				} else if ( wave >= 2 && roll < 45 ) {
					kind = HF_ARENA_ENEMY_SPITTER;
				}
			}
			enemies[ i ].x = cosf( angle ) * radius;
			enemies[ i ].y = sinf( angle ) * radius;
			enemies[ i ].hp = ( kind == HF_ARENA_ENEMY_BOSS ) ? 18
				: ( kind == HF_ARENA_ENEMY_BRUTE ) ? 5
				: ( kind == HF_ARENA_ENEMY_SPITTER ) ? 2 + wave / 3
				: 1 + wave / 2;
			enemies[ i ].kind = kind;
			enemies[ i ].fireCooldown = 30 + ( i % 20 );
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

	// Strategy hooks called from scripts.  In auto-pilot, firing happens
	// automatically and "intent" only steers the player.
	void	Attack()  { intent = 1; }
	void	Retreat() { intent = -1; }

private:
	void	SetFacingFrom( float x, float y ) {
		if ( fabsf( x ) + fabsf( y ) < 0.001f ) {
			x = facingX;
			y = facingY;
		}
		Normalize( x, y );
		facingX = x;
		facingY = y;
	}

	void	ClampPlayer() {
		if ( playerX >  120.0f ) playerX =  120.0f;
		if ( playerX < -120.0f ) playerX = -120.0f;
		if ( playerY >  120.0f ) playerY =  120.0f;
		if ( playerY < -120.0f ) playerY = -120.0f;
	}

	void	Hurt( int amount ) {
		if ( dodgeIFrames > 0 ) {
			return;
		}
		hitPoints -= amount;
		combo = 0;
		comboTimer = 0;
	}

	void	DropPickup( int fromKind, float x, float y ) {
		if ( pickupCount >= MAX_PICKUPS ) {
			return;
		}
		const int roll = NextRandom() % 100;
		int kind;
		if ( fromKind == HF_ARENA_ENEMY_BOSS ) {
			kind = HF_ARENA_PICKUP_HEALTH;
		} else if ( roll < 50 ) {
			kind = HF_ARENA_PICKUP_AMMO;
		} else if ( roll < 75 ) {
			kind = HF_ARENA_PICKUP_HEALTH;
		} else {
			kind = HF_ARENA_PICKUP_SCORE;
		}
		HfArenaPickup &p = pickups[ pickupCount++ ];
		p.x = x;
		p.y = y;
		p.kind = kind;
		p.life = 300;
	}

	void	OnEnemyKilled( int kind, float x, float y ) {
		kills++;
		combo++;
		bestCombo = combo > bestCombo ? combo : bestCombo;
		comboTimer = COMBO_TIMEOUT;
		score += EnemyScore( kind ) * ComboMultiplier();
		DropPickup( kind, x, y );
	}

	void	IntegrateBullets() {
		for ( int i = 0; i < bulletCount; i++ ) {
			bullets[ i ].x += bullets[ i ].vx;
			bullets[ i ].y += bullets[ i ].vy;
			bullets[ i ].life--;
		}
	}

	void	ReapBullets() {
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

	void	ReapEnemies() {
		int w = 0;
		for ( int i = 0; i < enemyCount; i++ ) {
			if ( enemies[ i ].hp > 0 ) {
				enemies[ w++ ] = enemies[ i ];
			}
		}
		enemyCount = w;
	}

	void	ReapPickups() {
		int w = 0;
		for ( int i = 0; i < pickupCount; i++ ) {
			if ( pickups[ i ].life > 0 ) {
				pickups[ w++ ] = pickups[ i ];
			}
		}
		pickupCount = w;
	}

	void	ApplyAutoPickups() {
		if ( dodgeIFrames > 0 ) {
			dodgeIFrames--;
		}
		for ( int i = 0; i < pickupCount; i++ ) {
			HfArenaPickup &p = pickups[ i ];
			p.life--;
			if ( Dist( playerX, playerY, p.x, p.y ) > 6.0f ) {
				continue;
			}
			switch ( p.kind ) {
				case HF_ARENA_PICKUP_AMMO:
					ammo = ammo + 6 > MAX_AMMO ? MAX_AMMO : ammo + 6;
					break;
				case HF_ARENA_PICKUP_HEALTH:
					hitPoints = hitPoints + 25 > PLAYER_HITPOINTS ? PLAYER_HITPOINTS : hitPoints + 25;
					break;
				case HF_ARENA_PICKUP_SCORE:
					score += 50;
					break;
			}
			p.life = 0;
		}
	}

public:
	// Legacy zero-argument auto-pilot.  Deterministic and intentionally kept
	// separate from command-driven play so the shipped .eng demo and older
	// snapshots keep their behaviour.
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
		if ( idx >= 0 ) {
			SetFacingFrom( enemies[ idx ].x - playerX, enemies[ idx ].y - playerY );
		}

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
			ClampPlayer();
		}

		// automatic fire toward the nearest enemy
		if ( idx >= 0 && fireCooldown == 0 && bulletCount < MAX_BULLETS ) {
			bullets[ bulletCount ].x = playerX;
			bullets[ bulletCount ].y = playerY;
			bullets[ bulletCount ].vx = facingX * BulletSpeed();
			bullets[ bulletCount ].vy = facingY * BulletSpeed();
			bullets[ bulletCount ].life = 60;
			bullets[ bulletCount ].team = HF_ARENA_BULLET_PLAYER;
			bulletCount++;
			fireCooldown = 4;
		}

		IntegrateBullets();

		// enemies chase; contact damages the player and destroys light enemies
		for ( int i = 0; i < enemyCount; i++ ) {
			float dx = playerX - enemies[ i ].x;
			float dy = playerY - enemies[ i ].y;
			float d = sqrtf( dx * dx + dy * dy );
			if ( d < 0.001f ) {
				d = 0.001f;
			}
			enemies[ i ].x += dx / d * EnemySpeed( enemies[ i ].kind );
			enemies[ i ].y += dy / d * EnemySpeed( enemies[ i ].kind );
			if ( Dist( playerX, playerY, enemies[ i ].x, enemies[ i ].y )
				< EnemyRadius( enemies[ i ].kind ) ) {
				Hurt( EnemyDamage( enemies[ i ].kind ) );
				if ( enemies[ i ].kind != HF_ARENA_ENEMY_BRUTE
					&& enemies[ i ].kind != HF_ARENA_ENEMY_BOSS ) {
					enemies[ i ].hp = 0;
				}
			}
		}

		// bullet/enemy collisions
		for ( int b = 0; b < bulletCount; b++ ) {
			if ( bullets[ b ].team != HF_ARENA_BULLET_PLAYER || bullets[ b ].life <= 0 ) {
				continue;
			}
			for ( int e = 0; e < enemyCount; e++ ) {
				if ( enemies[ e ].hp <= 0 ) {
					continue;
				}
				if ( Dist( bullets[ b ].x, bullets[ b ].y, enemies[ e ].x, enemies[ e ].y )
					< EnemyRadius( enemies[ e ].kind ) + 0.5f ) {
					enemies[ e ].hp--;
					bullets[ b ].life = 0;
					if ( enemies[ e ].hp <= 0 ) {
						OnEnemyKilled( enemies[ e ].kind, enemies[ e ].x, enemies[ e ].y );
					}
					break;
				}
			}
		}

		ApplyAutoPickups();
		ReapBullets();
		ReapEnemies();
		ReapPickups();

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

	// Command-driven gameplay entry point.  This is what a game module, the
	// frontend or a deterministic replay calls.
	void	Step( const HfArenaInput &cmd ) {
		if ( gameOver ) {
			return;
		}
		ticks++;
		if ( ticks > TICK_LIMIT ) {
			gameOver = true;
			won = false;
			return;
		}
		if ( fireCooldown > 0 ) { fireCooldown--; }
		if ( dodgeCooldown > 0 ) { dodgeCooldown--; }
		if ( dodgeIFrames > 0 ) { dodgeIFrames--; }
		if ( rapidTicks > 0 ) { rapidTicks--; }
		if ( comboTimer > 0 ) {
			comboTimer--;
			if ( comboTimer == 0 ) { combo = 0; }
		}

		// Facing follows the aim vector; movement is the fallback when no aim
		// device has produced a direction yet.
		SetFacingFrom( cmd.aimX, cmd.aimY );
		if ( fabsf( cmd.aimX ) + fabsf( cmd.aimY ) < 0.001f
			&& fabsf( cmd.moveX ) + fabsf( cmd.moveY ) >= 0.001f ) {
			SetFacingFrom( cmd.moveX, cmd.moveY );
		}

		float mx = cmd.moveX;
		float my = cmd.moveY;
		if ( mx * mx + my * my > 1.0f ) {
			Normalize( mx, my );
		}
		const bool dodging = cmd.dodge && dodgeCooldown == 0;
		if ( dodging ) {
			dodgeCooldown = DODGE_COOLDOWN;
			dodgeIFrames = DODGE_IFRAMES;
			if ( mx == 0.0f && my == 0.0f ) {
				mx = facingX;
				my = facingY;
			}
		}
		const float speed = dodging || dodgeIFrames > 0 ? DodgeSpeed() : PlayerSpeed();
		playerX += mx * speed;
		playerY += my * speed;
		ClampPlayer();

		// Firing consumes ammo; pickups are the only way to replenish it.
		if ( cmd.fire && fireCooldown == 0 && ammo > 0 && bulletCount < MAX_BULLETS ) {
			bullets[ bulletCount ].x = playerX;
			bullets[ bulletCount ].y = playerY;
			bullets[ bulletCount ].vx = facingX * BulletSpeed();
			bullets[ bulletCount ].vy = facingY * BulletSpeed();
			bullets[ bulletCount ].life = 60;
			bullets[ bulletCount ].team = HF_ARENA_BULLET_PLAYER;
			bulletCount++;
			ammo--;
			fireCooldown = rapidTicks > 0 ? 2 : 4;
		}

		IntegrateBullets();

		for ( int i = 0; i < enemyCount; i++ ) {
			HfArenaEnemy &e = enemies[ i ];
			float dx = playerX - e.x;
			float dy = playerY - e.y;
			float d = sqrtf( dx * dx + dy * dy );
			if ( d < 0.001f ) { d = 0.001f; }
			if ( e.fireCooldown > 0 ) { e.fireCooldown--; }

			if ( e.kind == HF_ARENA_ENEMY_SPITTER ) {
				// Keep range and spit slow bullets at the player.
				if ( d > 75.0f ) {
					e.x += dx / d * EnemySpeed( e.kind );
					e.y += dy / d * EnemySpeed( e.kind );
				} else if ( d < 45.0f ) {
					e.x -= dx / d * EnemySpeed( e.kind );
					e.y -= dy / d * EnemySpeed( e.kind );
				}
			} else {
				e.x += dx / d * EnemySpeed( e.kind );
				e.y += dy / d * EnemySpeed( e.kind );
			}

			const bool ranged = e.kind == HF_ARENA_ENEMY_SPITTER
				|| e.kind == HF_ARENA_ENEMY_BOSS;
			if ( ranged && e.fireCooldown == 0 && d < 150.0f
				&& bulletCount < MAX_BULLETS ) {
				const int shots = ( e.kind == HF_ARENA_ENEMY_BOSS ) ? 3 : 1;
				for ( int s = 0; s < shots && bulletCount < MAX_BULLETS; s++ ) {
					float sx = dx / d;
					float sy = dy / d;
					if ( s == 1 ) {
						const float px = -sy;
						const float py = sx;
						sx = px; sy = py;
					} else if ( s == 2 ) {
						const float px = sy;
						const float py = -sx;
						sx = px; sy = py;
					}
					bullets[ bulletCount ].x = e.x;
					bullets[ bulletCount ].y = e.y;
					bullets[ bulletCount ].vx = sx * EnemyBulletSpeed();
					bullets[ bulletCount ].vy = sy * EnemyBulletSpeed();
					bullets[ bulletCount ].life = 90;
					bullets[ bulletCount ].team = HF_ARENA_BULLET_ENEMY;
					bulletCount++;
				}
				e.fireCooldown = ( e.kind == HF_ARENA_ENEMY_BOSS ) ? 55 : 75;
			}

			if ( Dist( playerX, playerY, e.x, e.y ) < EnemyRadius( e.kind ) ) {
				Hurt( EnemyDamage( e.kind ) );
				if ( e.kind != HF_ARENA_ENEMY_BRUTE && e.kind != HF_ARENA_ENEMY_BOSS ) {
					e.hp = 0;
				} else {
					e.x -= dx / d * 4.0f;
					e.y -= dy / d * 4.0f;
				}
			}
		}

		// Player bullets damage enemies.
		for ( int b = 0; b < bulletCount; b++ ) {
			if ( bullets[ b ].team != HF_ARENA_BULLET_PLAYER || bullets[ b ].life <= 0 ) {
				continue;
			}
			for ( int e = 0; e < enemyCount; e++ ) {
				if ( enemies[ e ].hp <= 0 ) { continue; }
				if ( Dist( bullets[ b ].x, bullets[ b ].y, enemies[ e ].x, enemies[ e ].y )
					< EnemyRadius( enemies[ e ].kind ) + 0.5f ) {
					enemies[ e ].hp--;
					bullets[ b ].life = 0;
					if ( enemies[ e ].hp <= 0 ) {
						OnEnemyKilled( enemies[ e ].kind, enemies[ e ].x, enemies[ e ].y );
					}
					break;
				}
			}
		}

		// Enemy bullets damage the player unless a dodge i-frame is active.
		for ( int b = 0; b < bulletCount; b++ ) {
			if ( bullets[ b ].team != HF_ARENA_BULLET_ENEMY || bullets[ b ].life <= 0 ) {
				continue;
			}
			if ( Dist( bullets[ b ].x, bullets[ b ].y, playerX, playerY ) < PlayerRadius() ) {
				Hurt( PLAYER_HIT_DAMAGE );
				bullets[ b ].life = 0;
			}
		}

		// Pickups expire and are collected on contact.
		for ( int i = 0; i < pickupCount; i++ ) {
			HfArenaPickup &p = pickups[ i ];
			p.life--;
			if ( Dist( playerX, playerY, p.x, p.y ) > 6.0f ) { continue; }
			switch ( p.kind ) {
				case HF_ARENA_PICKUP_AMMO:
					ammo = ammo + 6 > MAX_AMMO ? MAX_AMMO : ammo + 6;
					rapidTicks = 12;
					break;
				case HF_ARENA_PICKUP_HEALTH:
					hitPoints = hitPoints + 25 > PLAYER_HITPOINTS ? PLAYER_HITPOINTS : hitPoints + 25;
					break;
				case HF_ARENA_PICKUP_SCORE:
					score += 50 * ComboMultiplier();
					break;
			}
			p.life = 0;
		}

		ReapBullets();
		ReapEnemies();
		ReapPickups();

		if ( hitPoints <= 0 ) {
			hitPoints = 0;
			gameOver = true;
			won = false;
			return;
		}
		if ( enemyCount == 0 && waveDelay == 0 ) {
			score += 25 * wave;			// wave-clear bonus
			waveDelay = WAVE_DELAY;
		}
		if ( waveDelay > 0 ) {
			waveDelay--;
			if ( waveDelay == 0 ) {
				if ( wave >= TOTAL_WAVES ) {
					gameOver = true;
					won = true;
				} else {
					SpawnWave();
				}
			}
		}
	}
};

#endif /* !__HF_ARENA_H__ */
