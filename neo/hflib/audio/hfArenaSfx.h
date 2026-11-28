#ifndef __HF_ARENA_SFX_H__
#define __HF_ARENA_SFX_H__

/*
==============================================================================

	hfArenaSfx — pure, deterministic gameplay SFX cues for Hellforge Arena.

	No device, no miniaudio, no OS, no CVar.  The detector compares two
	snapshots and emits bitmask events; the generator fills small f32 PCM
	buffers that a runtime bridge can hand to miniaudio.  Both are safe in
	the harness and deterministic for the same inputs.

==============================================================================
*/

#include "../hfprecompiled.h"
#include "../../hflib/game/hfArena.h"
#include "hfAudioData.h"

namespace HF_Audio {

enum hfArenaSfxCue {
	HF_ARENA_SFX_FIRE = 0,
	HF_ARENA_SFX_HIT,
	HF_ARENA_SFX_PICKUP,
	HF_ARENA_SFX_DODGE,
	HF_ARENA_SFX_BOSS_WARNING,
	HF_ARENA_SFX_WAVE_CLEAR,
	HF_ARENA_SFX_DEATH,
	HF_ARENA_SFX_COUNT
};

struct hfArenaSfxEvents {
	unsigned int bits;

	HF_INLINE hfArenaSfxEvents() : bits( 0 ) {}
	HF_INLINE void Clear() { bits = 0; }
	HF_INLINE void Set( hfArenaSfxCue cue ) { bits |= 1u << ( unsigned int )cue; }
	HF_INLINE bool Has( hfArenaSfxCue cue ) const { return ( bits & ( 1u << ( unsigned int )cue ) ) != 0; }
	HF_INLINE bool Any() const { return bits != 0; }
};

// Compare previous/current arena state and the command that produced curr.
// commandMode=true for Step(cmd), false for legacy auto Step().
hfArenaSfxEvents HF_DetectArenaSfx( const HfArena &prev, const HfArena &curr,
									const HfArenaInput &cmd, bool commandMode );

// Generate deterministic mono f32 PCM for a cue.  outSamples must have room
// for maxFrames floats.  Returns false on bad args.
bool HF_GenerateArenaSfxPcm( hfArenaSfxCue cue, float *outSamples,
							 unsigned int maxFrames, hfAudioSampleInfo &outInfo,
							 unsigned int sampleRate = 48000 );

} // namespace HF_Audio

#endif /* !__HF_ARENA_SFX_H__ */
