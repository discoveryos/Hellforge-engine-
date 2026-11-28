#include "../../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfArenaSfx.h"
#include <math.h>

namespace HF_Audio {

static float ClampSample( float v ) {
	if ( v < -1.0f ) return -1.0f;
	if ( v > 1.0f ) return 1.0f;
	return v;
}

// Tiny deterministic LCG for procedural noise; never touches arena RNG.
static unsigned int NoiseStep( unsigned int &s ) {
	s = s * 1664525u + 1013904223u;
	return ( s >> 8 ) & 0x7fffffu;
}

static float NoiseSigned( unsigned int &s ) {
	return ( ( float )NoiseStep( s ) / 8388607.0f ) * 2.0f - 1.0f;
}

hfArenaSfxEvents HF_DetectArenaSfx( const HfArena &prev, const HfArena &curr,
									const HfArenaInput &cmd, bool commandMode ) {
	hfArenaSfxEvents ev;

	if ( !prev.gameOver && curr.gameOver && !curr.won ) {
		ev.Set( HF_ARENA_SFX_DEATH );
	}
	if ( !prev.gameOver && curr.gameOver && curr.won ) {
		ev.Set( HF_ARENA_SFX_WAVE_CLEAR );
	}

	if ( commandMode ) {
		const bool fired = cmd.fire && prev.fireCooldown == 0 && prev.ammo > 0
			&& prev.bulletCount < HfArena::MAX_BULLETS;
		if ( fired && curr.ammo < prev.ammo ) {
			ev.Set( HF_ARENA_SFX_FIRE );
		}
		const bool dodged = cmd.dodge && prev.dodgeCooldown == 0;
		if ( dodged && curr.dodgeCooldown == HfArena::DODGE_COOLDOWN ) {
			ev.Set( HF_ARENA_SFX_DODGE );
		}
	} else {
		if ( curr.fireCooldown > prev.fireCooldown && prev.bulletCount < curr.bulletCount + 0 ) {
			// legacy auto fire sets fireCooldown to 4 on fire
			if ( prev.fireCooldown == 0 && curr.bulletCount >= prev.bulletCount ) {
				ev.Set( HF_ARENA_SFX_FIRE );
			}
		}
	}

	if ( curr.hitPoints < prev.hitPoints ) {
		ev.Set( HF_ARENA_SFX_HIT );
	}
	if ( curr.kills > prev.kills ) {
		ev.Set( HF_ARENA_SFX_HIT );
	}
	if ( curr.pickupCount < prev.pickupCount && !curr.gameOver ) {
		// collected or expired; collection is the common case when score/ammo/hp changed
		if ( curr.score != prev.score || curr.ammo != prev.ammo || curr.hitPoints > prev.hitPoints || curr.rapidTicks > prev.rapidTicks ) {
			ev.Set( HF_ARENA_SFX_PICKUP );
		}
	}
	if ( curr.wave > prev.wave && ( curr.wave % HfArena::BOSS_WAVE ) == 0 ) {
		ev.Set( HF_ARENA_SFX_BOSS_WARNING );
	}
	if ( prev.enemyCount > 0 && curr.enemyCount == 0 && !curr.gameOver ) {
		ev.Set( HF_ARENA_SFX_WAVE_CLEAR );
	}
	if ( curr.waveDelay == HfArena::WAVE_DELAY && prev.waveDelay == 0 ) {
		ev.Set( HF_ARENA_SFX_WAVE_CLEAR );
	}

	return ev;
}

bool HF_GenerateArenaSfxPcm( hfArenaSfxCue cue, float *outSamples,
							 unsigned int maxFrames, hfAudioSampleInfo &outInfo,
							 unsigned int sampleRate ) {
	if ( outSamples == NULL || maxFrames == 0 || sampleRate == 0 ) {
		return false;
	}
	unsigned int frames = 0;
	unsigned int noise = 0xC0FFEEu + ( unsigned int )cue * 101u;
	const float sr = ( float )sampleRate;
	const float twoPi = 6.28318530718f;

	switch ( cue ) {
		case HF_ARENA_SFX_FIRE:      frames = sampleRate / 12; noise ^= 0x11u; break;
		case HF_ARENA_SFX_HIT:       frames = sampleRate / 8;  noise ^= 0x22u; break;
		case HF_ARENA_SFX_PICKUP:    frames = sampleRate / 5;  noise ^= 0x33u; break;
		case HF_ARENA_SFX_DODGE:     frames = sampleRate / 6;  noise ^= 0x44u; break;
		case HF_ARENA_SFX_BOSS_WARNING: frames = sampleRate / 2; noise ^= 0x55u; break;
		case HF_ARENA_SFX_WAVE_CLEAR:   frames = sampleRate * 2 / 3; noise ^= 0x66u; break;
		case HF_ARENA_SFX_DEATH:     frames = sampleRate * 3 / 4; noise ^= 0x77u; break;
		default: return false;
	}
	if ( frames > maxFrames ) frames = maxFrames;

	for ( unsigned int i = 0; i < frames; i++ ) {
		const float t = ( float )i / sr;
		const float p = ( float )i / ( float )frames;
		float v = 0.0f;
		switch ( cue ) {
			case HF_ARENA_SFX_FIRE: {
				float env = 1.0f - p;
				v = ( sinf( twoPi * 220.0f * t ) * 0.6f + NoiseSigned( noise ) * 0.4f ) * env * 0.7f;
				break;
			}
			case HF_ARENA_SFX_HIT: {
				float env = 1.0f - p;
				v = ( sinf( twoPi * 90.0f * t ) * 0.7f + NoiseSigned( noise ) * 0.3f ) * env * env * 0.8f;
				break;
			}
			case HF_ARENA_SFX_PICKUP: {
				float freq = 440.0f + p * 660.0f;
				float env = p < 0.8f ? 1.0f : ( 1.0f - p ) * 5.0f;
				v = sinf( twoPi * freq * t ) * env * 0.45f;
				break;
			}
			case HF_ARENA_SFX_DODGE: {
				float env = sinf( 3.14159265f * p );
				v = NoiseSigned( noise ) * env * 0.35f + sinf( twoPi * 180.0f * t ) * env * 0.15f;
				break;
			}
			case HF_ARENA_SFX_BOSS_WARNING: {
				float env = p < 0.1f ? p * 10.0f : ( p > 0.85f ? ( 1.0f - p ) / 0.15f : 1.0f );
				float tone = fmodf( t, 0.25f ) < 0.125f ? 220.0f : 165.0f;
				v = ( sinf( twoPi * tone * t ) * 0.7f + sinf( twoPi * tone * 2.0f * t ) * 0.2f ) * env * 0.5f;
				break;
			}
			case HF_ARENA_SFX_WAVE_CLEAR: {
				float env = p < 0.05f ? p * 20.0f : ( 1.0f - p );
				float note = fmodf( t, 0.12f );
				float freq = note < 0.04f ? 523.25f : ( note < 0.08f ? 659.25f : 783.99f );
				v = sinf( twoPi * freq * t ) * env * 0.45f;
				break;
			}
			case HF_ARENA_SFX_DEATH: {
				float env = 1.0f - p;
				float freq = 220.0f * ( 1.0f - p * 0.7f );
				v = ( sinf( twoPi * freq * t ) * 0.6f + NoiseSigned( noise ) * 0.25f ) * env * env * 0.7f;
				break;
			}
			default: break;
		}
		outSamples[ i ] = ClampSample( v );
	}

	outInfo.channels = 1;
	outInfo.sampleRate = sampleRate;
	outInfo.totalFrames = frames;
	outInfo.totalBytes = frames * sizeof( float );
	return true;
}

} // namespace HF_Audio
