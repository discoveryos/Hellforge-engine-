#ifndef __HF_ARENA_SAVE_H__
#define __HF_ARENA_SAVE_H__

/*
===============================================================================

	hfArenaSave — durable, versioned snapshots of HfArena.

	The UI owns slots; the host owns policy.  This struct is the payload
	policy in between: a compact text record with an explicit version, every
	mutable arena field, and no pointers or host globals.  It is deliberately
	header-only and allocation-free so the same code runs in the engine and in
	the headless harness.

	Floats are stored as their 32-bit bit patterns in hex.  That keeps the
	round trip exact on a given binary (including RNG-driven enemy placement)
	while staying readable and diffable as text.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "hfArena.h"
#include "../console/hfConsoleServices.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct hfArenaSave {
	enum {
		HF_ARENA_SAVE_VERSION = 1,
		HF_ARENA_SAVE_TEXT_MAX = 4096
	};

	float				playerX;
	float				playerY;
	int					hitPoints;
	int					score;
	int					wave;
	int					ticks;
	bool				gameOver;
	bool				won;

	int					enemyCount;
	int					bulletCount;
	int					fireCooldown;
	int					intent;

	HfArenaEnemy		enemies[ HfArena::MAX_ENEMIES ];
	HfArenaBullet		bullets[ HfArena::MAX_BULLETS ];

	unsigned int		rng;

	hfArenaSave() { Clear(); }

	void Clear() {
		playerX = 0.0f;
		playerY = 0.0f;
		hitPoints = 0;
		score = 0;
		wave = 0;
		ticks = 0;
		gameOver = false;
		won = false;
		enemyCount = 0;
		bulletCount = 0;
		fireCooldown = 0;
		intent = 0;
		memset( enemies, 0, sizeof( enemies ) );
		memset( bullets, 0, sizeof( bullets ) );
		rng = 0;
	}

	void Capture( const HfArena &arena ) {
		playerX = arena.playerX;
		playerY = arena.playerY;
		hitPoints = arena.hitPoints;
		score = arena.score;
		wave = arena.wave;
		ticks = arena.ticks;
		gameOver = arena.gameOver;
		won = arena.won;
		enemyCount = arena.enemyCount;
		bulletCount = arena.bulletCount;
		fireCooldown = arena.fireCooldown;
		intent = arena.intent;
		memcpy( enemies, arena.enemies, sizeof( enemies ) );
		memcpy( bullets, arena.bullets, sizeof( bullets ) );
		rng = arena.rng;
	}

	bool Restore( HfArena &arena ) const {
		if ( enemyCount < 0 || enemyCount > HfArena::MAX_ENEMIES
			|| bulletCount < 0 || bulletCount > HfArena::MAX_BULLETS
			|| hitPoints < 0 || score < 0 || wave < 0 || ticks < 0
			|| fireCooldown < 0 ) {
			return false;
		}
		arena.playerX = playerX;
		arena.playerY = playerY;
		arena.hitPoints = hitPoints;
		arena.score = score;
		arena.wave = wave;
		arena.ticks = ticks;
		arena.gameOver = gameOver;
		arena.won = won;
		arena.enemyCount = enemyCount;
		arena.bulletCount = bulletCount;
		arena.fireCooldown = fireCooldown;
		arena.intent = intent;
		memcpy( arena.enemies, enemies, sizeof( enemies ) );
		memcpy( arena.bullets, bullets, sizeof( bullets ) );
		arena.rng = rng;
		return true;
	}

	bool Equals( const hfArenaSave &other ) const {
		if ( playerX != other.playerX || playerY != other.playerY
			|| hitPoints != other.hitPoints || score != other.score
			|| wave != other.wave || ticks != other.ticks
			|| gameOver != other.gameOver || won != other.won
			|| enemyCount != other.enemyCount || bulletCount != other.bulletCount
			|| fireCooldown != other.fireCooldown || intent != other.intent
			|| rng != other.rng ) {
			return false;
		}
		// Field-wise: the structs may carry tail padding, which is not part
		// of the text payload and must not decide save equivalence.
		for ( int i = 0; i < HfArena::MAX_ENEMIES; i++ ) {
			if ( enemies[ i ].x != other.enemies[ i ].x || enemies[ i ].hp != other.enemies[ i ].hp ) {
				return false;
			}
		}
		for ( int i = 0; i < HfArena::MAX_BULLETS; i++ ) {
			if ( bullets[ i ].x != other.bullets[ i ].x || bullets[ i ].y != other.bullets[ i ].y
				|| bullets[ i ].vx != other.bullets[ i ].vx || bullets[ i ].vy != other.bullets[ i ].vy
				|| bullets[ i ].life != other.bullets[ i ].life ) {
				return false;
			}
		}
		return true;
	}

	// --- text format ------------------------------------------------------

	// Single source of truth for the on-disk schema.  A future field must be
	// added here as well as to Serialize/Parse/Restore/Equals; the harness
	// walks this list so an omission cannot silently ship.
	static const char * const * FieldKeys( int &count ) {
		static const char * const keys[] = {
			"version", "player_x", "player_y", "hit_points", "score",
			"wave", "ticks", "game_over", "won", "enemy_count",
			"bullet_count", "fire_cooldown", "intent", "rng",
			"enemies", "bullets"
		};
		count = ( int )( sizeof( keys ) / sizeof( keys[ 0 ] ) );
		return keys;
	}

	// Cheap self-check used by tests: every declared key must appear in the
	// serialized text, and the text must restore to an equal snapshot.
	bool SchemaRoundTrip() const {
		char text[ HF_ARENA_SAVE_TEXT_MAX ];
		if ( Serialize( text, sizeof( text ) ) <= 0 ) { return false; }
		int count = 0;
		const char * const * keys = FieldKeys( count );
		for ( int i = 0; i < count; i++ ) {
			if ( strstr( text, keys[ i ] ) == NULL ) { return false; }
		}
		hfArenaSave parsed;
		return parsed.Parse( text ) && parsed.Equals( *this );
	}

	int Serialize( char *out, int maxSize ) const {
		if ( out == NULL || maxSize <= 0 ) { return 0; }
		out[ 0 ] = '\0';
		AppendInt( out, maxSize, "version", HF_ARENA_SAVE_VERSION );
		AppendBits( out, maxSize, "player_x", playerX );
		AppendBits( out, maxSize, "player_y", playerY );
		AppendInt( out, maxSize, "hit_points", hitPoints );
		AppendInt( out, maxSize, "score", score );
		AppendInt( out, maxSize, "wave", wave );
		AppendInt( out, maxSize, "ticks", ticks );
		AppendInt( out, maxSize, "game_over", gameOver ? 1 : 0 );
		AppendInt( out, maxSize, "won", won ? 1 : 0 );
		AppendInt( out, maxSize, "enemy_count", enemyCount );
		AppendInt( out, maxSize, "bullet_count", bulletCount );
		AppendInt( out, maxSize, "fire_cooldown", fireCooldown );
		AppendInt( out, maxSize, "intent", intent );
		AppendUInt( out, maxSize, "rng", rng );

		const int used = ( int )strlen( out );
		if ( used < maxSize - 1 ) {
			snprintf( out + used, ( size_t )( maxSize - used ), "enemies = " );
			for ( int i = 0; i < HfArena::MAX_ENEMIES; i++ ) {
				snprintf( out + strlen( out ), ( size_t )( maxSize - ( int )strlen( out ) ),
						  "%s%08x,%d", i > 0 ? ";" : "", FloatBits( enemies[ i ].x ), enemies[ i ].hp );
			}
			snprintf( out + strlen( out ), ( size_t )( maxSize - ( int )strlen( out ) ), "\n" );
		}
		if ( ( int )strlen( out ) < maxSize - 1 ) {
			snprintf( out + strlen( out ), ( size_t )( maxSize - ( int )strlen( out ) ), "bullets = " );
			for ( int i = 0; i < HfArena::MAX_BULLETS; i++ ) {
				snprintf( out + strlen( out ), ( size_t )( maxSize - ( int )strlen( out ) ),
						  "%s%08x,%08x,%08x,%08x,%d", i > 0 ? ";" : "",
						  FloatBits( bullets[ i ].x ), FloatBits( bullets[ i ].y ),
						  FloatBits( bullets[ i ].vx ), FloatBits( bullets[ i ].vy ),
						  bullets[ i ].life );
			}
			snprintf( out + strlen( out ), ( size_t )( maxSize - ( int )strlen( out ) ), "\n" );
		}
		return ( int )strlen( out );
	}

	bool Parse( const char *text ) {
		if ( text == NULL ) { return false; }
		Clear();
		int recognized = 0;
		int version = 0;
		bool sawVersion = false;
		const char *p = text;
		while ( *p != '\0' ) {
			const char *lineEnd = p;
			while ( *lineEnd != '\0' && *lineEnd != '\n' ) { lineEnd++; }
			char line[ HF_ARENA_SAVE_TEXT_MAX ];
			int len = ( int )( lineEnd - p );
			if ( len > ( int )sizeof( line ) - 1 ) { len = ( int )sizeof( line ) - 1; }
			memcpy( line, p, len );
			line[ len ] = '\0';
			p = ( *lineEnd == '\n' ) ? lineEnd + 1 : lineEnd;

			char *comment = strchr( line, '#' );
			if ( comment != NULL ) { *comment = '\0'; }
			char *eq = strchr( line, '=' );
			if ( eq == NULL ) { continue; }
			*eq = '\0';
			char *key = Trim( line );
			char *value = Trim( eq + 1 );
			if ( *key == '\0' || *value == '\0' ) { continue; }

			if ( strcmp( key, "version" ) == 0 ) {
				version = atoi( value );
				sawVersion = true;
				recognized++;
			} else if ( strcmp( key, "player_x" ) == 0 ) {
				if ( !ParseBits( value, playerX ) ) { return false; }
				recognized++;
			} else if ( strcmp( key, "player_y" ) == 0 ) {
				if ( !ParseBits( value, playerY ) ) { return false; }
				recognized++;
			} else if ( strcmp( key, "hit_points" ) == 0 ) {
				hitPoints = atoi( value ); recognized++;
			} else if ( strcmp( key, "score" ) == 0 ) {
				score = atoi( value ); recognized++;
			} else if ( strcmp( key, "wave" ) == 0 ) {
				wave = atoi( value ); recognized++;
			} else if ( strcmp( key, "ticks" ) == 0 ) {
				ticks = atoi( value ); recognized++;
			} else if ( strcmp( key, "game_over" ) == 0 ) {
				gameOver = atoi( value ) != 0; recognized++;
			} else if ( strcmp( key, "won" ) == 0 ) {
				won = atoi( value ) != 0; recognized++;
			} else if ( strcmp( key, "enemy_count" ) == 0 ) {
				enemyCount = atoi( value ); recognized++;
			} else if ( strcmp( key, "bullet_count" ) == 0 ) {
				bulletCount = atoi( value ); recognized++;
			} else if ( strcmp( key, "fire_cooldown" ) == 0 ) {
				fireCooldown = atoi( value ); recognized++;
			} else if ( strcmp( key, "intent" ) == 0 ) {
				intent = atoi( value ); recognized++;
			} else if ( strcmp( key, "rng" ) == 0 ) {
				rng = ( unsigned int )strtoul( value, NULL, 10 ); recognized++;
			} else if ( strcmp( key, "enemies" ) == 0 ) {
				ParseEnemies( value );
				recognized++;
			} else if ( strcmp( key, "bullets" ) == 0 ) {
				ParseBullets( value );
				recognized++;
			}
		}
		if ( !sawVersion || version != HF_ARENA_SAVE_VERSION || recognized < 2 ) {
			return false;
		}
		return enemyCount >= 0 && enemyCount <= HfArena::MAX_ENEMIES
			&& bulletCount >= 0 && bulletCount <= HfArena::MAX_BULLETS;
	}

	// --- per-account console slots ----------------------------------------

	bool SaveToSlot( hfconsole::hfConsoleServices *services, hfconsole::hfConsoleUserId user,
					 const char *slot ) const {
		if ( services == NULL || slot == NULL ) { return false; }
		char text[ HF_ARENA_SAVE_TEXT_MAX ];
		const int length = Serialize( text, sizeof( text ) );
		return length > 0 && services->WriteSlot( user, slot, ( const byte * )text, length );
	}

	bool LoadFromSlot( hfconsole::hfConsoleServices *services, hfconsole::hfConsoleUserId user,
					   const char *slot ) {
		if ( services == NULL || slot == NULL ) { return false; }
		const int size = services->SlotSize( user, slot );
		if ( size <= 0 || size >= HF_ARENA_SAVE_TEXT_MAX ) { return false; }
		char text[ HF_ARENA_SAVE_TEXT_MAX ];
		const int got = services->ReadSlot( user, slot, ( byte * )text, size );
		if ( got <= 0 ) { return false; }
		text[ got ] = '\0';
		return Parse( text );
	}

private:
	static unsigned int FloatBits( float value ) {
		unsigned int bits = 0;
		memcpy( &bits, &value, sizeof( bits ) );
		return bits;
	}

	static void BitsToFloat( unsigned int bits, float &value ) {
		memcpy( &value, &bits, sizeof( value ) );
	}

	static bool ParseBits( const char *value, float &out ) {
		char *end = NULL;
		const unsigned long bits = strtoul( value, &end, 16 );
		if ( end == value ) { return false; }
		BitsToFloat( ( unsigned int )bits, out );
		return true;
	}

	static char *Trim( char *s ) {
		while ( *s == ' ' || *s == '\t' ) { s++; }
		int len = ( int )strlen( s );
		while ( len > 0 && ( s[ len - 1 ] == ' ' || s[ len - 1 ] == '\t' || s[ len - 1 ] == '\r' ) ) {
			s[ --len ] = '\0';
		}
		return s;
	}

	static void Append( char *out, int maxSize, const char *format, ... ) {
		const int used = ( int )strlen( out );
		if ( used >= maxSize - 1 ) { return; }
		va_list args;
		va_start( args, format );
		vsnprintf( out + used, ( size_t )( maxSize - used ), format, args );
		va_end( args );
	}

	static void AppendInt( char *out, int maxSize, const char *key, int value ) {
		Append( out, maxSize, "%s = %d\n", key, value );
	}

	static void AppendUInt( char *out, int maxSize, const char *key, unsigned int value ) {
		Append( out, maxSize, "%s = %u\n", key, value );
	}

	static void AppendBits( char *out, int maxSize, const char *key, float value ) {
		Append( out, maxSize, "%s = %08x\n", key, FloatBits( value ) );
	}

	void ParseEnemies( char *value ) {
		int index = 0;
		char *p = value;
		while ( p != NULL && *p != '\0' && index < HfArena::MAX_ENEMIES ) {
			char *sep = strchr( p, ';' );
			if ( sep != NULL ) { *sep = '\0'; }
			unsigned int bits = 0;
			if ( sscanf( p, "%x,%d", &bits, &enemies[ index ].hp ) == 2 ) {
				BitsToFloat( bits, enemies[ index ].x );
				index++;
			}
			p = ( sep != NULL ) ? sep + 1 : NULL;
		}
	}

	void ParseBullets( char *value ) {
		int index = 0;
		char *p = value;
		while ( p != NULL && *p != '\0' && index < HfArena::MAX_BULLETS ) {
			char *sep = strchr( p, ';' );
			if ( sep != NULL ) { *sep = '\0'; }
			unsigned int bx = 0, by = 0, bvx = 0, bvy = 0;
			if ( sscanf( p, "%x,%x,%x,%x,%d", &bx, &by, &bvx, &bvy, &bullets[ index ].life ) == 5 ) {
				BitsToFloat( bx, bullets[ index ].x );
				BitsToFloat( by, bullets[ index ].y );
				BitsToFloat( bvx, bullets[ index ].vx );
				BitsToFloat( bvy, bullets[ index ].vy );
				index++;
			}
			p = ( sep != NULL ) ? sep + 1 : NULL;
		}
	}
};

#endif /* !__HF_ARENA_SAVE_H__ */
